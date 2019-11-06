#include "client.h"

#include "globals.h"
#include "lfm.h"
#include "log.h"
#include "my_malloc.h"
#include "structured_stream.h"
#include "time_utils.h"

#include <assert.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void menoetius_client_grow_queued_metrics( struct menoetius_client* client, int min_growth );

int menoetius_client_init( struct menoetius_client* client, const char* server, int port )
{
	client->server = server;
	client->port = port;
	client->read_buf_size = 1024;
	client->write_buf_size = 1024;

	client->fd = -1;

	client->queued_metrics_max_size = 16384;
	client->queued_metrics = my_malloc( client->queued_metrics_max_size );
	client->queued_metrics_len = 0;

	client->num_queued_metrics = 0;
	client->num_queued_metrics_flush = 1000;

	return 0;
}

void menoetius_client_shutdown( struct menoetius_client* client )
{
	if( client->fd >= 0 ) {
		close( client->fd );
		client->fd = -1;
	}
	if( client->ss ) {
		structured_stream_free( client->ss );
		client->ss = NULL;
	}
}

int menoetius_client_ensure_connected( struct menoetius_client* client )
{
	int res = 0;

	if( client->fd >= 0 ) {
		assert( client->ss );
		LOG_DEBUG( "reusing existing client connection" );
		return 0;
	}

	client->fd = socket( AF_INET, SOCK_STREAM, 0 );
	if( client->fd < 0 ) {
		LOG_ERROR( "error opening socket" );
		return 1;
	}

	struct hostent* server = gethostbyname( client->server );
	if( server == NULL ) {
		LOG_ERROR( "hostname=s error looking up hostname", client->server );
		close( client->fd );
		client->fd = -1;
		return 1;
	}

	struct sockaddr_in serveraddr;
	bzero( (char*)&serveraddr, sizeof( serveraddr ) );
	serveraddr.sin_family = AF_INET;
	bcopy( (char*)server->h_addr, (char*)&serveraddr.sin_addr.s_addr, server->h_length );
	serveraddr.sin_port = htons( client->port );

	/* connect: create a connection with the server */
	if( connect( client->fd, (const struct sockaddr*)&serveraddr, sizeof( serveraddr ) ) < 0 ) {
		LOG_ERROR( "hostname=s failed to connect", client->server );
		close( client->fd );
		client->fd = -1;
		return 1;
	}

	res = structured_stream_new(
		client->fd, client->read_buf_size, client->write_buf_size, &( client->ss ) );
	if( res ) {
		return res;
	}

	uint64_t magic_header = 1547675033;
	if( ( res = structured_stream_write_uint64( client->ss, magic_header ) ) ) {
		LOG_ERROR( "res=s write failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}
	LOG_DEBUG( "established new client connection" );
	return 0;
}

int menoetius_client_send_sync( struct menoetius_client* client,
								const char* key,
								size_t key_len,
								size_t num_pts,
								int64_t* t,
								double* y )
{
	int i;
	int res;

	if( ( res = menoetius_client_ensure_connected( client ) ) ) {
		LOG_ERROR( "res=s failed to connect", err_str( res ) );
		return res;
	}

	if( ( res = structured_stream_write_uint8( client->ss, MENOETIUS_RPC_PUT_DATA ) ) ) {
		LOG_ERROR( "res=s write failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	if( ( res = structured_stream_write_uint16_prefixed_bytes( client->ss, key, key_len ) ) ) {
		LOG_ERROR( "res=s write failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	if( ( res = structured_stream_write_uint32( client->ss, num_pts ) ) ) {
		LOG_ERROR( "res=s write failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	for( i = 0; i < num_pts; i++ ) {
		if( ( res = structured_stream_write_uint64( client->ss, t[i] ) ) ) {
			LOG_ERROR( "res=s write failed", err_str( res ) );
			return res;
		}

		if( ( res = structured_stream_write_double( client->ss, y[i] ) ) ) {
			LOG_ERROR( "res=s write failed", err_str( res ) );
			return res;
		}
	}

	// end of batch marker (empty string "")
	if( ( res = structured_stream_write_uint16_prefixed_bytes( client->ss, NULL, 0 ) ) ) {
		LOG_ERROR( "res=s write failed", err_str( res ) );
		return res;
	}

	if( ( res = structured_stream_flush( client->ss ) ) ) {
		LOG_ERROR( "res=s flush failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	// wait for response from server
	uint8_t server_response;
	if( ( res = structured_stream_read_uint8( client->ss, &server_response ) ) ) {
		LOG_ERROR( "res=s failed to read", err_str( res ) );
		return res;
	}

	// ignore out of date response for now
	server_response &= ~MENOETIUS_CLUSTER_CONFIG_OUT_OF_DATE;

	LOG_INFO( "num_pts=d resp=d sent points", num_pts, server_response );
	return server_response;
}

int menoetius_client_send( struct menoetius_client* client,
						   const char* key,
						   size_t key_len,
						   size_t num_pts,
						   int64_t* t,
						   double* y )
{
	int i;

	int n = sizeof( uint16_t ) + key_len + sizeof( uint32_t ) +
			( sizeof( uint64_t ) + sizeof( double ) ) * num_pts;
	menoetius_client_grow_queued_metrics( client, n );

	char* s = client->queued_metrics + client->queued_metrics_len;

	memcpy( s, &key_len, sizeof( uint16_t ) );
	s += sizeof( uint16_t );

	memcpy( s, key, key_len );
	s += key_len;

	memcpy( s, &num_pts, sizeof( uint32_t ) );
	s += sizeof( uint32_t );

	for( i = 0; i < num_pts; i++ ) {
		memcpy( s, &( t[i] ), sizeof( uint64_t ) );
		s += sizeof( uint64_t );

		memcpy( s, &( y[i] ), sizeof( double ) );
		s += sizeof( double );
	}

	assert( s - client->queued_metrics == n + client->queued_metrics_len );
	client->queued_metrics_len += n;

	client->num_queued_metrics++;
	if( client->num_queued_metrics >= client->num_queued_metrics_flush ) {
		return menoetius_client_flush( client );
	}

	return 0;
}

int menoetius_client_flush( struct menoetius_client* client )
{
	int res;

	if( ( res = menoetius_client_ensure_connected( client ) ) ) {
		LOG_ERROR( "res=s failed to connect", err_str( res ) );
		return res;
	}

	if( ( res = structured_stream_write_uint8( client->ss, MENOETIUS_RPC_PUT_DATA ) ) ) {
		LOG_ERROR( "res=s write failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	size_t chunk_size = 32768;
	size_t remaining = client->queued_metrics_len;
	size_t n;
	char* s = client->queued_metrics;
	while( remaining > 0 ) {
		n = remaining;
		if( n > chunk_size ) {
			n = chunk_size;
		}
		if( ( res = structured_stream_write_bytes( client->ss, s, n ) ) ) {
			LOG_ERROR( "res=s write failed", err_str( res ) );
			menoetius_client_shutdown( client );
			return res;
		}
		remaining -= n;
		s += n;
	}

	// end of batch marker (empty string "")
	if( ( res = structured_stream_write_uint16_prefixed_bytes( client->ss, NULL, 0 ) ) ) {
		LOG_ERROR( "res=s write failed", err_str( res ) );
		return res;
	}

	if( ( res = structured_stream_flush( client->ss ) ) ) {
		LOG_ERROR( "res=s flush failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	// wait for response from server
	uint8_t server_response;
	if( ( res = structured_stream_read_uint8( client->ss, &server_response ) ) ) {
		LOG_ERROR( "res=s failed to read", err_str( res ) );
		return res;
	}

	// ignore out of date response for now
	server_response &= ~MENOETIUS_CLUSTER_CONFIG_OUT_OF_DATE;

	LOG_INFO( "num_pts=d resp=d flushed points", client->num_queued_metrics, server_response );

	// TODO re-evaluate if this should be done here, or if batches should be done outside of the client?
	if( server_response == 0 ) {
		client->queued_metrics_len = 0;
		client->num_queued_metrics = 0;
	}

	return server_response;
}

int menoetius_client_get( struct menoetius_client* client,
						  const char* key,
						  size_t key_len,
						  size_t max_num_pts,
						  size_t* num_pts,
						  int64_t* t,
						  double* y )
{
	int res;

	if( ( res = menoetius_client_ensure_connected( client ) ) ) {
		LOG_ERROR( "res=s failed to connect", err_str( res ) );
		return res;
	}

	if( ( res = structured_stream_write_uint8( client->ss, MENOETIUS_RPC_GET_DATA ) ) ) {
		LOG_ERROR( "res=s write failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	// write num of keys
	if( ( res = structured_stream_write_uint16( client->ss, 1 ) ) ) {
		LOG_ERROR( "res=s write failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	// write keys
	if( ( res = structured_stream_write_uint16_prefixed_bytes( client->ss, key, key_len ) ) ) {
		LOG_ERROR( "res=s write failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	// flush
	if( ( res = structured_stream_flush( client->ss ) ) ) {
		LOG_ERROR( "res=s flush failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	// read num points
	uint32_t num_pts_server;
	if( ( res = structured_stream_read_uint32( client->ss, &num_pts_server ) ) ) {
		LOG_ERROR( "res=s read failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	if( num_pts_server > max_num_pts ) {
		LOG_ERROR( "num=d too many points received from server; increase client memory", num_pts );
		menoetius_client_shutdown( client );
		return 1;
	}

	for( uint32_t j = 0; j < num_pts_server; j++ ) {
		if( ( res = structured_stream_read_int64( client->ss, &t[j] ) ) ) {
			LOG_ERROR( "res=s read failed", err_str( res ) );
			menoetius_client_shutdown( client );
			return res;
		}
		if( ( res = structured_stream_read_double( client->ss, &y[j] ) ) ) {
			LOG_ERROR( "res=s read failed", err_str( res ) );
			menoetius_client_shutdown( client );
			return res;
		}
	}

	// read response code
	uint8_t server_response;
	if( ( res = structured_stream_read_uint8( client->ss, &server_response ) ) ) {
		LOG_ERROR( "res=s read failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	// ignore out of date response for now
	server_response &= ~MENOETIUS_CLUSTER_CONFIG_OUT_OF_DATE;

	*num_pts = num_pts_server;
	return server_response;
}

int menoetius_client_query( struct menoetius_client* client,
							const struct KeyValue* matchers,
							size_t num_matchers,
							bool allow_full_scan,
							struct binary_lfm* results,
							size_t max_results,
							size_t* num_results )
{
	int res;

	uint64_t offset = 0;
	uint64_t limit = 1000;

	if( ( res = menoetius_client_ensure_connected( client ) ) ) {
		LOG_ERROR( "res=s failed to connect", err_str( res ) );
		return res;
	}

	if( ( res = structured_stream_write_uint8( client->ss, MENOETIUS_RPC_QUERY_LFMS ) ) ) {
		LOG_ERROR( "res=s write failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	// write flags
	uint8_t flags = 0;
	if( allow_full_scan ) {
		flags |= 0x01;
	}
	if( ( res = structured_stream_write_uint8( client->ss, flags ) ) ) {
		LOG_ERROR( "res=s write failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	// write num matchers
	if( ( res = structured_stream_write_uint8( client->ss, num_matchers ) ) ) {
		LOG_ERROR( "res=s write failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	// write key and values (hence the *2)
	for( int i = 0; i < num_matchers; i++ ) {
		if( ( res = structured_stream_write_uint16_prefixed_string( client->ss,
																	matchers[i].key ) ) ) {
			LOG_ERROR( "res=s write failed", err_str( res ) );
			menoetius_client_shutdown( client );
			return res;
		}
		if( ( res = structured_stream_write_uint16_prefixed_string( client->ss,
																	matchers[i].value ) ) ) {
			LOG_ERROR( "res=s write failed", err_str( res ) );
			menoetius_client_shutdown( client );
			return res;
		}
	}

	//write offset
	if( ( res = structured_stream_write_uint64( client->ss, offset ) ) ) {
		LOG_ERROR( "res=s write failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	//write limit
	if( ( res = structured_stream_write_uint64( client->ss, limit ) ) ) {
		LOG_ERROR( "res=s write failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	// flush
	if( ( res = structured_stream_flush( client->ss ) ) ) {
		LOG_ERROR( "res=s flush failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	size_t i;
	for( i = 0;; i++ ) {
		const char* raw_lfm;
		size_t n;
		if( ( res = structured_stream_read_uint16_prefixed_bytes_inplace(
				  client->ss, &raw_lfm, &n ) ) ) {
			LOG_ERROR( "res=s read failed", err_str( res ) );
			menoetius_client_shutdown( client );
			return res;
		}
		if( n == 0 ) {
			break;
		}
		if( i >= max_results ) {
			return -1;
		}
		if( n >= MAX_BINARY_LFM_SIZE ) {
			return -1;
		}

		memcpy( results[i].lfm, raw_lfm, n );
		results[i].n = n;
	}
	*num_results = i;

	if( allow_full_scan ) {
		uint64_t last_internal_id;
		if( ( res = structured_stream_read_uint64( client->ss, &last_internal_id ) ) ) {
			LOG_ERROR( "res=s read failed", err_str( res ) );
			menoetius_client_shutdown( client );
			return res;
		}
	}

	// read response code
	uint8_t server_response;
	if( ( res = structured_stream_read_uint8( client->ss, &server_response ) ) ) {
		LOG_ERROR( "res=s read failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	// ignore out of date response for now
	server_response &= ~MENOETIUS_CLUSTER_CONFIG_OUT_OF_DATE;

	return server_response;
}

int menoetius_client_get_status( struct menoetius_client* client, int* status )
{
	int res;

	if( ( res = menoetius_client_ensure_connected( client ) ) ) {
		LOG_ERROR( "res=s failed to connect", err_str( res ) );
		return res;
	}

	if( ( res = structured_stream_write_uint8( client->ss, MENOETIUS_RPC_GET_STATUS ) ) ) {
		LOG_ERROR( "res=s write failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	if( ( res = structured_stream_flush( client->ss ) ) ) {
		LOG_ERROR( "res=s flush failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	uint8_t tmp;
	if( ( res = structured_stream_read_uint8( client->ss, &tmp ) ) ) {
		LOG_ERROR( "res=s read failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	*status = tmp;
	return 0;
}

int menoetius_client_get_cluster_config( struct menoetius_client* client )
{
	int res;

	if( ( res = menoetius_client_ensure_connected( client ) ) ) {
		LOG_ERROR( "res=s failed to connect", err_str( res ) );
		return res;
	}

	if( ( res = structured_stream_write_uint8( client->ss, MENOETIUS_RPC_GET_CLUSTER_CONFIG ) ) ) {
		LOG_ERROR( "res=s write failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	if( ( res = structured_stream_flush( client->ss ) ) ) {
		LOG_ERROR( "res=s flush failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	const char* hash;
	if( ( res = structured_stream_read_bytes_inplace( client->ss, HASH_LENGTH, &hash ) ) ) {
		LOG_ERROR( "res=s read failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	size_t n;
	const char* cluster_config;
	if( ( res = structured_stream_read_uint16_prefixed_bytes_inplace(
			  client->ss, &cluster_config, &n ) ) ) {
		LOG_ERROR( "res=s read failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}
	LOG_INFO( "n=d got the config", n );

	return 0;
}

int menoetius_client_test_hook( struct menoetius_client* client, uint64_t flags )
{
	int res;

	if( ( res = menoetius_client_ensure_connected( client ) ) ) {
		LOG_ERROR( "res=s failed to connect", err_str( res ) );
		return res;
	}

	if( ( res = structured_stream_write_uint8( client->ss, MENOETIUS_RPC_TEST_HOOK ) ) ) {
		LOG_ERROR( "res=s write failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	if( ( res = structured_stream_write_uint64( client->ss, flags ) ) ) {
		LOG_ERROR( "res=s write failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	if( ( res = structured_stream_flush( client->ss ) ) ) {
		LOG_ERROR( "res=s flush failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	uint8_t server_status;
	if( ( res = structured_stream_read_uint8( client->ss, &server_status ) ) ) {
		LOG_ERROR( "res=s read failed", err_str( res ) );
		menoetius_client_shutdown( client );
		return res;
	}

	return server_status;
}

void menoetius_client_grow_queued_metrics( struct menoetius_client* client, int min_growth )
{
	size_t new_size = client->queued_metrics_max_size;
	while( ( new_size - client->queued_metrics_len ) < min_growth ) {
		new_size *= 2;
	}

	client->queued_metrics_max_size = new_size;
	client->queued_metrics = my_realloc( client->queued_metrics, client->queued_metrics_max_size );
}
