#include "conn_handler.h"

#include "escape_key.h"
#include "globals.h"
#include "metrics.h"
#include "my_malloc.h"
#include "storage.h"
#include "str_utils.h"
#include "structured_stream.h"

#include "http_metrics.h"
#include "log.h"
#include "time_utils.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LABEL_LEN 512
#define MAX_LFM_RESULTS 1024 * 1024

#define ENSURE_OK( r )                                                                             \
	do {                                                                                           \
		int ENSURE_OK_res = ( r );                                                                 \
		if( ENSURE_OK_res ) {                                                                      \
			LOG_ERROR( "ctx=ctx res=s unhandled error", data->log_ctx, err_str( ENSURE_OK_res ) ); \
			res = ENSURE_OK_res;                                                                   \
			goto error;                                                                            \
		}                                                                                          \
	} while( 0 );

void* conn_handler_thread_run( void* ptr )
{
	int res;
	struct conn_data* data = (struct conn_data*)ptr;

	struct timespec timeout;
	timeout.tv_nsec = 0;

	pthread_mutex_lock( &data->lock );
	for( ;; ) {
		timeout.tv_sec = time( NULL ) + 1;
		res = pthread_cond_timedwait( &data->cond, &data->lock, &timeout );
		assert( res == 0 || res == ETIMEDOUT );

		if( data->fd < 0 ) {
			goto error;
		}
		if( res == ETIMEDOUT ) {
			LOG_WARN( "lost a condition, recovered via timeout" );
		}
		structured_stream_init( data->ss, data->fd );

		res = handle_conn_rpc_entry( data );
		if( res ) {
			goto error;
		}

		while( res == 0 ) {
			LOG_TRACE( "handling RPC" );
			res = handle_conn_rpc_code( data );
		}

	error:
		if( data->fd >= 0 ) {
			LOG_TRACE( "fd=d disconnecting", data->fd );
			close( data->fd );
			data->fd = -1;

			conn_data_pool_release( data->pool, data );
		}
	}

	// technically this is never hit since this thread never exits
	// pthread_mutex_unlock( &data->lock );
	abort();
}

int conn_data_pool_init( struct conn_data_pool** pool,
						 size_t num_conns,
						 size_t buf_size_per_conn,
						 struct the_storage* storage )
{
	int res;
	struct conn_data* ptr;
	assert( pool );
	*pool = my_malloc( sizeof( struct conn_data_pool ) );
	if( *pool == NULL ) {
		return 1;
	}

	assert( pthread_mutex_init( &( ( **pool ).lock ), NULL ) == 0 );

	( **pool ).max_conn = num_conns;
	( **pool ).pool = my_malloc( sizeof( struct conn_data ) * num_conns );
	assert( ( **pool ).pool );
	( **pool ).free = my_malloc( sizeof( struct conn_data* ) * num_conns );
	assert( ( **pool ).free );
	( **pool ).free_i = num_conns - 1;
	for( int i = 0; i < num_conns; i++ ) {
		ptr = &( ( **pool ).pool[i] );
		( **pool ).free[i] = ptr;
		res = structured_stream_new( -1, buf_size_per_conn, buf_size_per_conn, &( ptr->ss ) );
		if( res != 0 ) {
			return res; // memory leak in the case where previous mallocs are not freed, but if this
				// code path happens, the process should die due to OOM
		}
		ptr->log_ctx = NULL;
		ptr->fd = -1;
		ptr->storage = storage;
		ptr->pool = *pool;

		assert( pthread_mutex_init( &ptr->lock, NULL ) == 0 );
		assert( pthread_cond_init( &ptr->cond, NULL ) == 0 );

		pthread_create( &( ptr->thread ), NULL, conn_handler_thread_run, (void*)ptr );
	}
	return 0;
}

int conn_data_pool_handle_conn( struct conn_data_pool* pool, int fd )
{
	int res;
	struct conn_data* p;

	res = conn_data_pool_get( pool, &p );
	if( res ) {
		return res;
	}

	pthread_mutex_lock( &p->lock );
	p->fd = fd;
	p->has_valid_config = false;
	LOG_TRACE( "assign" );
	pthread_cond_signal( &p->cond );
	pthread_mutex_unlock( &p->lock );

	LOG_TRACE( "done" );
	return 0;
}

int conn_data_pool_get( struct conn_data_pool* pool, struct conn_data** ptr )
{
	int res;
	pthread_mutex_lock( &pool->lock );

	if( pool->free_i < 0 ) {
		res = 1;
		goto error;
	}

	*ptr = pool->free[pool->free_i];
	pool->free_i--;

	LOG_TRACE( "ptr=p get pool instance", *ptr );
	INCR_METRIC( num_connections );

	res = 0;
error:
	pthread_mutex_unlock( &pool->lock );
	return res;
}

int conn_data_pool_release( struct conn_data_pool* pool, struct conn_data* ptr )
{
	int res;
	pthread_mutex_lock( &pool->lock );

	pool->free_i++;
	pool->free[pool->free_i] = ptr;

	LOG_TRACE( "ptr=p releasing pool instance", ptr );
	DECR_METRIC( num_connections );

	res = 0;
error:
	pthread_mutex_unlock( &pool->lock );
	return res;
}

int handle_conn_put_data( struct conn_data* data )
{
	int res;
	uint32_t num_pts;
	int64_t t;
	double y;

	char key[MAX_LFM_SIZE];
	size_t key_len;

	uint8_t error_code = 0;
	uint32_t total_num_pts = 0;

	int64_t earliest_time = (int64_t)my_time( NULL ) - data->storage->retention_seconds;

	LOG_TRACE( "handle_conn_put_data" );

	for( ;; ) {
		ENSURE_OK(
			structured_stream_read_uint16_prefixed_bytes( data->ss, key, &key_len, MAX_LFM_SIZE ) );

		if( key_len == 0 ) {
			break;
		}

		ENSURE_OK( structured_stream_read_uint32( data->ss, &num_pts ) );

		while( num_pts > 0 ) {
			ENSURE_OK(
				structured_stream_ensure_read( data->ss, sizeof( int64_t ) + sizeof( double ) ) );
			ENSURE_OK( structured_stream_read_int64( data->ss, &t ) );
			ENSURE_OK( structured_stream_read_double( data->ss, &y ) );

			LOG_TRACE( "ctx=ctx lfm=*s t=d y=f decode", data->log_ctx, key_len, key, t, y );

			if( t < earliest_time ) {
				LOG_TRACE( "ctx=ctx lfm=*s t=d earliest=d rejecting point",
						   data->log_ctx,
						   key_len,
						   key,
						   t,
						   earliest_time );
				INCR_METRIC( num_points_rejected )
			}
			else {
				LOG_TRACE( "ctx=ctx lfm=*s t=d writing point", data->log_ctx, key_len, key, t );
				if( ( res = storage_write( data->storage, key, key_len, &t, &y, 1 ) ) ) {
					INCR_METRIC( num_points_rejected );

					// keep as debug to avoid spamming logs (which happens when we get
					// ERR_OUT_OF_ORDER_TIMESTAMP due to docc restarts)
					LOG_TRACE( "ctx=ctx, key=*s t=d res=s failed to write to storage",
							   data->log_ctx,
							   key_len,
							   key,
							   t,
							   err_str( res ) );

					// WARNING: don't even let the client know this failed, because it will just
					// keep on sending the same point over and over; which will cause it to get
					// stuck. previously we let the client know an error happened by setting:
					//
					// data->error_code = MENOETIUS_STATUS_UNKNOWN_ERROR;
					//
					// Even if we do return an error here, we need to keep processing all other
					// messages from the client; otherwise, there's no way to gracefully let the
					// client know an error happened
				}
				else {
					INCR_METRIC( num_points_written );
				}
			}

			num_pts--;
			total_num_pts++;
		}
	}

	ENSURE_OK( structured_stream_ensure_write( data->ss, sizeof( uint8_t ) + sizeof( uint32_t ) ) );

	if( !data->has_valid_config ) {
		error_code |= MENOETIUS_CLUSTER_CONFIG_OUT_OF_DATE;
	}
	ENSURE_OK( structured_stream_write_uint8( data->ss, error_code ) );

	ENSURE_OK( structured_stream_flush( data->ss ) );

	res = 0;
error:
	return res;
}

int handle_conn_get_data( struct conn_data* data )
{
	uint8_t error_code = 0;
	int64_t t[MAX_PTS_PER_SERIES];
	double y[MAX_PTS_PER_SERIES];

	int keys_buf_i = 0;
	char keys_buf[MAX_KEY_BUF];

	int res;
	size_t num_pts;

	size_t key_len;
	const char* key;
	uint16_t num_keys;
	ENSURE_OK( structured_stream_read_uint16( data->ss, &num_keys ) );

	LOG_TRACE( "numkeys=d to read", num_keys );

	for( uint16_t i = 0; i < num_keys; i++ ) {
		ENSURE_OK(
			structured_stream_read_uint16_prefixed_bytes_inplace( data->ss, &key, &key_len ) );
		size_t n = ( keys_buf_i + sizeof( uint64_t ) + key_len );
		if( n >= MAX_KEY_BUF ) {
			LOG_ERROR( "n=d max=d request too large", n, MAX_KEY_BUF );
			error_code = MENOETIUS_STATUS_TOO_MANY_METRICS;
			continue; // important, must keep on reading, but just ignore the data
		}

		memcpy( keys_buf + keys_buf_i, &key_len, sizeof( uint16_t ) );
		keys_buf_i += sizeof( uint16_t );

		memcpy( keys_buf + keys_buf_i, key, key_len );
		keys_buf_i += key_len;
	}

	// now we're done reading from the client; send data back!

	const char* p = keys_buf;
	for( uint16_t i = 0; i < num_keys; i++ ) {
		if( error_code ) {
			// if an error happened, just pretend no data exists
			ENSURE_OK( structured_stream_write_uint32( data->ss, 0 ) );
			continue;
		}

		size_t key_len = *(uint16_t*)( p );
		p += sizeof( uint16_t );

		key = p;
		p += key_len;

		LOG_TRACE( "lfm=*s getting pts", key_len, key );
		res = storage_get( data->storage, key, key_len, t, y, MAX_PTS_PER_SERIES, &num_pts );
		if( res == 0 ) {
			ENSURE_OK( structured_stream_write_uint32( data->ss, (uint32_t)num_pts ) );
			for( int i = 0; i < num_pts; i++ ) {
				ENSURE_OK( structured_stream_write_int64( data->ss, t[i] ) );
				ENSURE_OK( structured_stream_write_double( data->ss, y[i] ) );
			}
		}
		else {
			// leave as DEBUG, this will happen if a node is restarted or if a user tries to lookup
			// data that doesn't exist. When left as an ERROR it leads people down the wrong path
			// thinking it's an actionable error.
			LOG_TRACE( "lfm=*s err=s no points returned", key_len, key, err_str( res ) );

			// ignore lookup errors (which may happen if a key is deleted between
			// fetching the index and requesting the actual data
			// if we failed here, the rest of the batch would also fail (a bad thing).
			if( res != ERR_LOOKUP_FAILED ) {
				error_code = MENOETIUS_STATUS_UNKNOWN_ERROR;
			}

			// no points to return
			ENSURE_OK( structured_stream_write_uint32( data->ss, 0 ) );
		}
	}

	if( !data->has_valid_config ) {
		error_code |= MENOETIUS_CLUSTER_CONFIG_OUT_OF_DATE;
	}
	ENSURE_OK( structured_stream_write_uint8( data->ss, error_code ) );
	ENSURE_OK( structured_stream_flush( data->ss ) );

	res = 0;
error:
	return res;
}

int handle_conn_replication_data( struct conn_data* data )
{
	LOG_ERROR( "replication not handled" );
	return 1; // TODO
}

int handle_conn_set_clock( struct conn_data* data )
{
	int res;
	int64_t t;
	ENSURE_OK( structured_stream_read_int64( data->ss, &t ) );

#ifdef DEBUG_BUILD
	LOG_INFO( "ctx=ctx t=d changing clock time", data->log_ctx, t );
	set_time( t );
#else
	LOG_WARN( "ctx=ctx t=d ignorning request to change clock time", data->log_ctx, t );
	res = 1;
	goto error;
#endif

	res = 0;
error:
	return res;
}

int handle_conn_test_hook( struct conn_data* data )
{
	int res;
	uint8_t status;

	uint64_t flags;
	ENSURE_OK( structured_stream_read_uint64( data->ss, &flags ) );

#ifdef DEBUG_BUILD
	status = MENOETIUS_STATUS_OK;

	if( flags & TEST_HOOK_SYNC_FLAG ) {
		LOG_INFO( "ctx=ctx performing sync", data->log_ctx );
		ENSURE_OK( storage_sync( data->storage, false ) );
	}

	if( flags & TEST_HOOK_SHUTDOWN_FLAG ) {
		LOG_WARN( "ctx=ctx got shutdown request", data->log_ctx );
		assert( 0 ); // FIXME for multi-threaded version
	}

#else
	LOG_WARN( "ctx=ctx flags=d ignorning test hook request", data->log_ctx, flags );
	status = MENOETIUS_STATUS_UNAUTHORIZED;
#endif

	ENSURE_OK( structured_stream_write_uint8( data->ss, status ) );
	ENSURE_OK( structured_stream_flush( data->ss ) );

	res = 0;
error:
	return res;
}

#define MAX_LABEL_MATCHES MAX_LABEL_LEN * 10

int handle_conn_query_lfms( struct conn_data* data )
{
	int res;
	int index_num = -1;
	uint8_t flags;
	const char* index_val = NULL;
	const char* label;
	uint8_t num_labels;
	uint8_t error_code = 0;

	char results[MAX_LFM_RESULTS];
	uint64_t last_internal_id = 0;

	char label_matches[MAX_LABEL_MATCHES];
	int label_matches_i = 0;

	ENSURE_OK( structured_stream_read_uint8( data->ss, &flags ) );
	bool allow_full_scan = flags & 0x01;
	bool force_full_scan = flags & 0x02;
	LOG_TRACE( "flags=d got flags", flags );

	ENSURE_OK( structured_stream_read_uint8( data->ss, &num_labels ) );

	uint64_t offset, limit;
	size_t n;

	for( int i = 0; i < num_labels; i++ ) {
		size_t label_len;
		ENSURE_OK(
			structured_stream_read_uint16_prefixed_bytes_inplace( data->ss, &label, &label_len ) );
		LOG_TRACE( "val=*s index name", label_len, label );

		// ignoring __name__ matchers for now
		if( index_num == -1 && label_len == 8 && strncmp( label, "__name__", label_len ) == 0 ) {
			index_num = NAME_INDEX;
		}
		else {
			int j = 0;
			if( index_num >= NUM_SPECIAL_INDICES ) {
				j = index_num - NUM_SPECIAL_INDICES + 1;
			}
			for( ; j < data->storage->num_indices; j++ ) {
				if( strlen( data->storage->indices[j] ) == label_len &&
					strncmp( label, data->storage->indices[j], label_len ) == 0 ) {
					index_num = j + NUM_SPECIAL_INDICES;
					index_val = NULL;
					break;
				}
			}
		}

		if( ( label_matches_i + label_len + 1 ) >= MAX_LABEL_MATCHES ) {
			LOG_ERROR( "too many label matchers" );
			res = 1;
			goto error;
		}
		memcpy( &label_matches[label_matches_i], label, label_len );
		label_matches_i += label_len;
		label_matches[label_matches_i] = '\0';
		label_matches_i++;

		// grab value
		ENSURE_OK(
			structured_stream_read_uint16_prefixed_bytes_inplace( data->ss, &label, &label_len ) );
		LOG_TRACE( "val=*s index value", label_len, label );

		if( ( label_matches_i + label_len + 1 ) >= MAX_LABEL_MATCHES ) {
			LOG_ERROR( "too many label matchers" );
			res = 1;
			goto error;
		}

		if( index_val == NULL ) {
			index_val = &label_matches[label_matches_i];
		}

		memcpy( &label_matches[label_matches_i], label, label_len );
		label_matches_i += label_len;
		label_matches[label_matches_i] = '\0';
		label_matches_i++;
	}

	ENSURE_OK( structured_stream_read_uint64( data->ss, &offset ) );
	ENSURE_OK( structured_stream_read_uint64( data->ss, &limit ) );

	LOG_TRACE( "index=d val=s matchers=*s offset=d limit=d querying for lfms",
			   index_num,
			   index_val,
			   label_matches_i,
			   label_matches,
			   offset,
			   limit );

	if( force_full_scan ) {
		index_num = -1;
		allow_full_scan = true;
		last_internal_id = offset;
	}
	else {
		if( index_num == -1 && offset > 0 ) {
			// this happens if the client sends allow_full_scan and an offset
			// the fix is to send a force_full_scan and the offset
			LOG_ERROR( "client error" );
			res = 1;
			goto error;
		}
	}

	if( index_num == -1 ) {
		// full scan
		if( allow_full_scan ) {
			INCR_METRIC( num_full_lookups );
			res = storage_lookup_without_index( data->storage,
												label_matches,
												num_labels,
												results,
												&n,
												MAX_LFM_RESULTS,
												&last_internal_id,
												limit );
		}
		else {
			res = ERR_BAD_INDEX;
		}
	}
	else {
		// index lookup
		INCR_METRIC( num_index_lookups );
		res = storage_lookup( data->storage,
							  index_num,
							  index_val,
							  label_matches,
							  num_labels,
							  results,
							  &n,
							  MAX_LFM_RESULTS,
							  offset,
							  limit );
	}
	if( res ) {
		LOG_ERROR( "index_num=d value=s err=s failed to query lfms",
				   index_num,
				   index_val,
				   err_str( res ) );
		switch( res ) {
		case ERR_BUF_TOO_SMALL:
			error_code = MENOETIUS_STATUS_TOO_MANY_METRICS;
			break;
		case ERR_BAD_INDEX:
			error_code = MENOETIUS_STATUS_MISSING_INDEX;
			break;
		default:
			error_code = MENOETIUS_STATUS_UNKNOWN_ERROR;
			break;
		}
	}
	else {
		LOG_TRACE( "bytes=d returning matches", n );
		ENSURE_OK( structured_stream_write_bytes( data->ss, results, n ) );
	}
	ENSURE_OK( structured_stream_write_uint16_prefixed_string( data->ss, "" ) );
	if( allow_full_scan ) {
		ENSURE_OK( structured_stream_write_uint64( data->ss, last_internal_id ) );
	}

	if( !data->has_valid_config ) {
		error_code |= MENOETIUS_CLUSTER_CONFIG_OUT_OF_DATE;
	}
	ENSURE_OK( structured_stream_write_uint8( data->ss, error_code ) );

	ENSURE_OK( structured_stream_flush( data->ss ) );

	res = 0;
error:
	return res;
}

int handle_conn_rpc_entry( struct conn_data* data )
{
	int res;
	uint64_t magic_header;
	LOG_TRACE( "reading header" );
	ENSURE_OK( structured_stream_read_uint64( data->ss, &magic_header ) );

	if( magic_header != MENOETIUS_RPC_MAGIC_HEADER ) {
		LOG_ERROR( "ctx=ctx got=d want=d magic header missmatch",
				   data->log_ctx,
				   magic_header,
				   (uint64_t)MENOETIUS_RPC_MAGIC_HEADER );
		res = 1;
		goto error;
	}
	LOG_TRACE( "ctx=ctx got=d magic header correct", data->log_ctx, magic_header );

	res = 0;
error:
	return res;
}

int handle_conn_rpc_code( struct conn_data* data )
{
	int res;
	uint8_t rpc_code;

	LOG_TRACE( "waiting" );
	res = structured_stream_read_uint8( data->ss, &rpc_code );
	LOG_TRACE( "waiting done" );
	switch( res ) {
	case ERR_STRUCTURED_STREAM_EOF:
		LOG_TRACE( "client disconnected due to EOF while reading rpc" );
		return 1;
	case 0:
		break;
	default:
		LOG_ERROR( "ctx=ctx res=s unhandled socket error", data->log_ctx, err_str( res ) );
		return 1;
	}

	switch( rpc_code ) {
	case MENOETIUS_RPC_PUT_DATA:
		INCR_METRIC( num_call_put_data );
		LOG_TRACE( "ctx=ctx dispatching to handle put data", data->log_ctx );
		return handle_conn_put_data( data );

	case MENOETIUS_RPC_GET_DATA:
		INCR_METRIC( num_call_get_data );
		LOG_TRACE( "ctx=ctx dispatching to handle get data", data->log_ctx );
		return handle_conn_get_data( data );

	case MENOETIUS_RPC_REPLICATION_DATA:
		INCR_METRIC( num_call_replication_data );
		LOG_TRACE( "dispatching to handle replication data" );
		return handle_conn_replication_data( data );

	case MENOETIUS_RPC_QUERY_LFMS:
		INCR_METRIC( num_call_query_lfms );
		LOG_TRACE( "dispatching to handle query lfms" );
		return handle_conn_query_lfms( data );

	case MENOETIUS_RPC_SET_CLOCK:
		INCR_METRIC( num_call_set_clock );
		return handle_conn_set_clock( data );

	case MENOETIUS_RPC_TEST_HOOK:
		INCR_METRIC( num_call_test_hook );
		return handle_conn_test_hook( data );

	case MENOETIUS_RPC_GET_STATUS:
		INCR_METRIC( num_call_get_status );
		return handle_conn_get_status( data );

	case MENOETIUS_RPC_GET_CLUSTER_CONFIG:
		INCR_METRIC( num_call_get_cluster_config );
		return handle_conn_get_cluster_config( data );

	case MENOETIUS_RPC_SET_CLUSTER_CONFIG:
		INCR_METRIC( num_call_set_cluster_config );
		return handle_conn_set_cluster_config( data );

	case MENOETIUS_RPC_GET_VERSION:
		INCR_METRIC( num_call_get_version );
		return handle_conn_get_version( data );

	case MENOETIUS_RPC_REGISTER_CLIENT_CLUSTER_CONFIG:
		INCR_METRIC( num_call_register_client_cluster_config );
		return handle_conn_register_client_cluster_config( data );

	default:
		INCR_METRIC( num_call_unhandled );
		LOG_ERROR( "ctx=ctx rpc=d unsupported rpc code", data->log_ctx, rpc_code );
		return -1;
	}
	return 0;
}

int handle_conn_get_status( struct conn_data* data )
{
	int res;
	ENSURE_OK( structured_stream_write_uint8( data->ss, MENOETIUS_STATUS_OK ) );
	ENSURE_OK( structured_stream_flush( data->ss ) );

	res = 0;
error:
	return res;
}

int handle_conn_get_cluster_config( struct conn_data* data )
{
	int res;
	char hash[SHA_DIGEST_LENGTH];
	char* s = NULL;

	res = pthread_mutex_lock( &( data->storage->lock ) );
	if( res ) {
		LOG_ERROR( "res=d failed to lock" );
		goto error;
	}

	memcpy( hash, data->storage->cluster_config_hash, SHA_DIGEST_LENGTH );
	if( data->storage->cluster_config ) {
		s = my_strdup( data->storage->cluster_config );
	}

	pthread_mutex_unlock( &( data->storage->lock ) );

	if( ( res = structured_stream_write_bytes( data->ss, hash, SHA_DIGEST_LENGTH ) ) ) {
		LOG_ERROR( "res=d failed to write to socket" );
		goto error;
	}

	if( s ) {
		if( ( res = structured_stream_write_uint16_prefixed_string( data->ss, s ) ) ) {
			LOG_ERROR( "res=d failed to write to socket" );
			goto error;
		}
	}
	else {
		if( ( res = structured_stream_write_uint16( data->ss, 0 ) ) ) {
			LOG_ERROR( "res=d failed to write to socket" );
			goto error;
		}
	}

	ENSURE_OK( structured_stream_write_uint8( data->ss, MENOETIUS_STATUS_OK ) );
	ENSURE_OK( structured_stream_flush( data->ss ) );

	res = 0;
error:

	if( s ) {
		my_free( s );
	}

	return res;
}

int handle_conn_set_cluster_config( struct conn_data* data )
{
	int res;
	const char* s;
	size_t n;

	res = structured_stream_read_uint16_prefixed_bytes_inplace( data->ss, &s, &n );
	if( res ) {
		LOG_ERROR( "res=d failed to read" );
		goto error;
	}

	res = pthread_mutex_lock( &( data->storage->lock ) );
	if( res ) {
		LOG_ERROR( "res=d failed to lock" );
		goto error;
	}

	if( data->storage->cluster_config ) {
		my_free( data->storage->cluster_config );
	}
	data->storage->cluster_config = my_malloc( n + 1 );
	memcpy( data->storage->cluster_config, s, n );
	data->storage->cluster_config[n] = '\0';

	res = save_cluster_config_to_disk( data->storage );

	// now that the config has changed, we need to set has_valid_config = false for ALL clients
	for( int i = 0; i < data->pool->max_conn; i++ ) {
		data->pool->pool[i].has_valid_config = false;
	}

	pthread_mutex_unlock( &( data->storage->lock ) );

	char hash[SHA_DIGEST_LENGTH_HEX];
	encode_hash_to_str( hash, data->storage->cluster_config_hash );
	LOG_DEBUG( "hash=s new cluster config set", hash );

	ENSURE_OK( structured_stream_write_uint8( data->ss, MENOETIUS_STATUS_OK ) );
	ENSURE_OK( structured_stream_flush( data->ss ) );

	res = 0;
error:
	return res;
}

int handle_conn_register_client_cluster_config( struct conn_data* data )
{
	int res;
	int status = 0;
	const char* s;
	char client_hash[SHA_DIGEST_LENGTH_HEX] = "\0";
	char server_hash[SHA_DIGEST_LENGTH_HEX] = "\0";

	res = structured_stream_read_bytes_inplace( data->ss, SHA_DIGEST_LENGTH, &s );
	if( res ) {
		LOG_ERROR( "res=d failed to read" );
		goto error;
	}
	encode_hash_to_str( client_hash, s );

	res = pthread_mutex_lock( &( data->storage->lock ) );
	if( res ) {
		LOG_ERROR( "res=d failed to lock" );
		goto error;
	}

	if( memcmp( s, data->storage->cluster_config_hash, SHA_DIGEST_LENGTH ) ) {
		data->has_valid_config = false;
		status |= MENOETIUS_CLUSTER_CONFIG_OUT_OF_DATE;
	}
	else {
		data->has_valid_config = true;
	}
	encode_hash_to_str( server_hash, data->storage->cluster_config_hash );

	pthread_mutex_unlock( &( data->storage->lock ) );

	LOG_DEBUG( "client_hash=s server_hash=s valid=d client registered version",
			   client_hash,
			   server_hash,
			   data->has_valid_config );

	ENSURE_OK( structured_stream_write_uint8( data->ss, status ) );
	ENSURE_OK( structured_stream_flush( data->ss ) );

	res = 0;
error:
	return res;
}

int handle_conn_get_version( struct conn_data* data )
{
	int res;

	ENSURE_OK( structured_stream_write_uint16_prefixed_string( data->ss, GIT_COMMIT ) );
	ENSURE_OK( structured_stream_write_uint16_prefixed_string( data->ss, BUILD_TIME ) );
	ENSURE_OK( structured_stream_write_uint8( data->ss, MENOETIUS_STATUS_OK ) );
	ENSURE_OK( structured_stream_flush( data->ss ) );

	res = 0;
error:
	return res;
}
