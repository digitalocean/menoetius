#define _GNU_SOURCE

#include "storage.h"

#include "crash_test.h"
#include "disk_storage.h"
#include "globals.h"
#include "http_metrics.h"
#include "log.h"
#include "lseek_utils.h"
#include "metrics.h"
#include "my_malloc.h"
#include "str_utils.h"
#include "time_utils.h"
#include "tsc.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define STORAGE_GET_BUF_N 1024 * 1024

void* storage_thread_run( void* ptr );

void storage_item_free( struct the_storage* store, StorageItem* item );

int storage_load( struct the_storage* store );

void storage_item_ensure_offset_room( StorageItem* item, int n );

StorageItem* storage_insert_item( struct the_storage* store,
								  const char* key,
								  uint16_t key_len,
								  uint64_t internal_id );

void storage_insert_data_block_during_initial_load( struct the_storage* store,
													uint64_t internal_id,
													uint64_t offset,
													int64_t start_time,
													StorageItem** item,
													bool* last_block );

bool storage_attach_key_to_item( struct the_storage* store,
								 const char* key,
								 uint16_t key_len,
								 uint64_t internal_id,
								 uint64_t key_storage_offset );

int read_or_create_cluster_config_from_disk( struct the_storage* store, const char* path );

///////////////////////////////////////// start
// SGLIB_DEFINE_RBTREE_FUNCTIONS( StorageItem, left, right, color_field, storage_item_cmp )
//
//
//
/////////////////////////////////////////// end

static inline int nullstrcmp( const char* a, const char* b )
{
	if( a == NULL ) {
		if( b == NULL ) {
			return 0;
		}
		return -1;
	}
	else if( b == NULL ) {
		return 1;
	}
	return strcmp( a, b );
}

int storage_item_cmp( const StorageItem* a, const StorageItem* b, int index )
{
#ifdef DEBUG_BUILD
	assert( a );
	assert( b );
#endif // DEBUG_BUILD
	switch( index ) {
	case INTERNAL_ID_INDEX:
		if( a->internal_id < b->internal_id ) {
			return -1;
		}
		if( a->internal_id > b->internal_id ) {
			return 1;
		}
		return 0;
	case LFM_INDEX: {
		LOG_TRACE( "index=d a=*s b=*s memcmp", index, a->key_len, a->key, b->key_len, b->key );
		int x = a->key_len - b->key_len;
		if( x != 0 ) {
			return x;
		}
		return memcmp( a->key, b->key, a->key_len );
	}
	case NAME_INDEX:
		return nullstrcmp( a->key, b->key );
	default:
		LOG_TRACE( "index=d a=s b=s strcmp",
				   index,
				   a->index_val[index - NUM_SPECIAL_INDICES],
				   b->index_val[index - NUM_SPECIAL_INDICES] );
		return nullstrcmp( a->index_val[index - NUM_SPECIAL_INDICES],
						   b->index_val[index - NUM_SPECIAL_INDICES] );
	}
}

int storage_init( struct the_storage* store,
				  int retention_seconds,
				  int target_cleaning_loop_seconds,
				  size_t initial_tsc_buf_size,
				  int num_indices,
				  const char** indices,
				  const char* disk_storage_path )
{
	int res;
	char hash[SHA_DIGEST_LENGTH_HEX];

	memset( store, 0, sizeof( struct the_storage ) );
	store->retention_seconds = retention_seconds;
	store->target_cleaning_loop_microseconds = target_cleaning_loop_seconds * 1000000;
	store->initial_tsc_buf_size = initial_tsc_buf_size;
	store->num_indices = num_indices;
	store->indices = indices;
	store->next_internal_id = 1;

	// we wont support really long paths
	if( strlen( disk_storage_path ) > 900 ) {
		return 1;
	}
	char tmp[1024];

	// create key store
	sprintf( tmp, "%s/%s", disk_storage_path, "key.store" );
	store->key_disk_store = my_malloc( sizeof( struct disk_storage ) );
	res = disk_storage_open( store->key_disk_store, tmp );
	if( res != 0 ) {
		return res;
	}

	// create data store
	sprintf( tmp, "%s/%s", disk_storage_path, "data.store" );
	store->data_disk_store = my_malloc( sizeof( struct disk_storage ) );
	res = disk_storage_open( store->data_disk_store, tmp );
	if( res != 0 ) {
		return res;
	}

	// index 0 is internal id, index 1 is lfm; index 2 is __name__; index 3+i is indices[i];
	size_t num_bytes = sizeof( StorageItem* ) * ( NUM_SPECIAL_INDICES + num_indices );
	store->root = my_malloc( num_bytes );
	memset( store->root, 0, num_bytes );

	assert( target_cleaning_loop_seconds > 0 );
	assert( pthread_mutex_init( &( store->lock ), NULL ) == 0 );

	// load cluster config from disk
	sprintf( tmp, "%s/%s", disk_storage_path, "cluster.config" );
	res = read_or_create_cluster_config_from_disk( store, tmp );
	if( res ) {
		return res;
	}
	encode_hash_to_str( hash, store->cluster_config_hash );
	LOG_INFO( "path=s hash=s loaded cluster config", store->cluster_config_path, hash );

	res = storage_load( store );
	if( res != 0 ) {
		return res;
	}

	__sync_synchronize();
	return 0;
}

int storage_load( struct the_storage* store )
{
	const size_t max_n = 1048576;
	char buf[max_n];
	const char* p;

	StorageItem* item;
	bool last_block;

	size_t n, offset;
	bool initial;
	int res;

	uint64_t internal_id;
	uint16_t key_len;

	const char* key;
	uint8_t block_status;

	LOG_INFO( "reading timeseries data" );
	initial = true;
	for( ;; ) {
		res = disk_storage_read_blocks_from_start(
			store->data_disk_store, initial, max_n, buf, &n, &offset, &block_status );
		if( res != 0 ) {
			LOG_ERROR( "res=d failed to read data", res );
			return res;
		}
		if( n == 0 ) {
			LOG_TRACE( "end of blocks" );
			// end of blocks
			break;
		}
		initial = false;
		ADD_METRIC( bytes_loaded, n );

		// must have at least <internal_id><num_pts><start_time>...
		if( n <= ( sizeof( uint64_t ) + sizeof( uint16_t ) + sizeof( uint64_t ) ) ) {
			LOG_ERROR( "num=d data block too small", n );
			return 1;
		}

		p = buf;

		memcpy( &internal_id, p, sizeof( uint64_t ) );
		p += sizeof( uint64_t );

		const char* serialized_tsc_header = NULL;
		const char* data_buf;
		switch( block_status ) {
		case DISK_STORAGE_BLOCK_STATUS_VALID:
			data_buf = p;
			break;
		case DISK_STORAGE_BLOCK_STATUS_ACTIVE:
			serialized_tsc_header = p;
			p += TSC_HEADER_SERIALIZATION_SIZE;
			data_buf = p;
			break;
		default:
			assert( 0 );
		}

		int64_t start_time = TSC_START_TIME( data_buf );
		if( start_time == 0 ) {
			LOG_ERROR( "offset=d n=d status=d bad time", offset, n, block_status );
			assert( 0 );
		}

		LOG_TRACE( "ID=d t=d read timeseries data", internal_id, start_time );
		storage_insert_data_block_during_initial_load(
			store, internal_id, offset, start_time, &item, &last_block );
		assert( item );

		if( block_status == DISK_STORAGE_BLOCK_STATUS_ACTIVE ) {
			assert( last_block );

			LOG_TRACE( "id=d path=s offset=d loading data",
					   internal_id,
					   store->data_disk_store->path,
					   offset );

			int data_buf_len = n - sizeof( uint64_t ) - TSC_HEADER_SERIALIZATION_SIZE;
			tsc_load_block( item->data, serialized_tsc_header, data_buf, data_buf_len );
		}
	}

	LOG_INFO( "reading keys" );
	initial = true;
	for( ;; ) {
		errno = 0;
		res = disk_storage_read_blocks_from_start(
			store->key_disk_store, initial, max_n, buf, &n, &offset, &block_status );
		if( res != 0 ) {
			LOG_ERROR(
				"res=d err=s failed to read key data from disk storage", res, strerror( errno ) );
			return res;
		}
		if( n == 0 ) {
			LOG_TRACE( "end of blocks" );
			// end of blocks
			break;
		}
		initial = false;
		ADD_METRIC( bytes_loaded, n );
		assert( n > sizeof( uint16_t ) );
		memcpy( &key_len, buf, sizeof( uint16_t ) );
		key = buf + sizeof( uint16_t );

		memcpy( &internal_id, key + key_len, sizeof( uint64_t ) );

		assert( key_len && key[0] );

		if( internal_id >= store->next_internal_id ) {
			store->next_internal_id = internal_id + 1;
		}

		LOG_TRACE( "n=d key=*s ID=d read key", n, key_len, key, internal_id );
		if( !storage_attach_key_to_item( store, key, key_len, internal_id, offset ) ) {
			// key doesn't have any data, delete it.
			off_t off_restore = must_lseek( store->key_disk_store->fd, 0, SEEK_CUR );
			res = disk_storage_release_block( store->key_disk_store, offset );
			if( res ) {
				LOG_ERROR( "res=d failed to release block", res );
				assert( 0 );
			}
			must_lseek( store->key_disk_store->fd, off_restore, SEEK_SET );
		}
	}
	LOG_INFO( "next_id=d setting next internal id", store->next_internal_id );

	// check that all items have valid keys (i.e. that we were able to attach a key to it)
	struct sglib_StorageItem_iterator itr;
	for( item = sglib_StorageItem_it_init_inorder(
			 &itr, store->root[INTERNAL_ID_INDEX], INTERNAL_ID_INDEX );
		 item != NULL;
		 item = sglib_StorageItem_it_next( &itr, INTERNAL_ID_INDEX ) ) {
		if( item->key_len == INVALID_KEY ) {
			LOG_ERROR( "id=d key_len=d corrupt data found (datapoints with unknown key)",
					   item->internal_id,
					   item->key_len );
			assert( 0 );
		}
	}

	// check that all items' last data block is active
	for( item = sglib_StorageItem_it_init_inorder(
			 &itr, store->root[INTERNAL_ID_INDEX], INTERNAL_ID_INDEX );
		 item != NULL;
		 item = sglib_StorageItem_it_next( &itr, INTERNAL_ID_INDEX ) ) {

		assert( item->num_data_offsets > 0 );
		int i = item->num_data_offsets - 1;

		assert( item->data_offsets[i].offset );
		if( tsc_is_clear( item->data ) ) {
			// this means that the item only has complete blocks in it, and no ACTIVE blocks
			// we must reserve space for a new-active block
			item->num_data_offsets++;
			storage_item_ensure_offset_room( item, item->num_data_offsets );
			item->data_offsets[item->num_data_offsets - 1].offset = 0;
			item->data_offsets[item->num_data_offsets - 1].start_time = 0;
		}
	}

	return 0;
}

void storage_free( struct the_storage* store )
{
	StorageItem* p;
	StorageItem* item = store->list_root;

	while( item ) {
		p = item;
		item = item->next;
		storage_item_free( store, p );
	}

	disk_storage_close( store->key_disk_store );
	my_free( store->key_disk_store );

	disk_storage_close( store->data_disk_store );
	my_free( store->data_disk_store );

	my_free( store->root );
	my_free( store->cluster_config );
	my_free( store->cluster_config_path );
}

int storage_start_thread( struct the_storage* store )
{
	pthread_create(
		&( store->trimmer_thread ), NULL, storage_thread_run, (struct the_storage*)store );
	return 0;
}

int storage_join_thread( struct the_storage* store )
{
	pthread_join( store->trimmer_thread, NULL );
	return 0;
}

const char* get_key_value( const char* lfm, size_t lfm_len, const char* key )
{
	int i;
	const char* end = lfm + lfm_len;
	bool found = false;
	for( i = 0; lfm < end; i++ ) {
		if( found ) {
			return lfm;
		}
		if( i % 2 == 1 ) {
			if( strcmp( lfm, key ) == 0 ) {
				found = true;
			}
		}
		lfm += strlen( lfm ) + 1;
	}
#ifdef DEBUG_BUILD
	assert( !found );
#endif // DEBUG_BUILD
	return NULL;
}

void storage_item_ensure_offset_room( StorageItem* item, int n )
{
	LOG_TRACE( "n=d cap=d checking", n, item->cap_data_offsets );

	if( n <= item->cap_data_offsets ) {
		return;
	}

	int cap = item->cap_data_offsets;
	if( cap == 0 ) {
		cap = 4;
	}
	while( cap < n ) {
		cap *= 2;
	}

	LOG_TRACE( "num_elements=d growing data offset storage", cap );
	size_t nn = sizeof( struct offset_and_time ) * cap;
	struct offset_and_time* p = (struct offset_and_time*)my_malloc( nn );
	// important to init this; since we use offset=0 to mean non-used block
	memset( p, 0, sizeof( struct offset_and_time ) * cap );

	LOG_TRACE( "n=d new storage space", nn );

	if( item->data_offsets ) {
		memcpy( p, item->data_offsets, sizeof( struct offset_and_time ) * item->cap_data_offsets );
		my_free( item->data_offsets );
	}
	item->data_offsets = p;
	item->cap_data_offsets = cap;
}

// when key_len is INVALID_KEY, this item is a placeholder for timeseries data read from disk
// where we only have the internal ID and have yet to discover the key.
// once all timeseries data is read in, we then read in the keys and will re-create
// the item with the actual key and then correctly index it.
StorageItem* storage_item_new( uint16_t key_len,
							   const char* key,
							   size_t initial_tsc_buf_size,
							   int num_indices,
							   const char** indices,
							   uint64_t internal_id )
{
	// this functions packs data together, so that we only call my_malloc once per stored item
	// this is to reduce memory fragmentation, and to get a tighter packing of used memory vs
	// padding.
	size_t storage_item_size = ROUND_UP( sizeof( StorageItem ) );
	size_t n = storage_item_size;

	size_t color_field_size = ROUND_UP( sizeof( char ) * ( num_indices + NUM_SPECIAL_INDICES ) );
	n += color_field_size;

	size_t left_size =
		ROUND_UP( sizeof( struct storage_item* ) * ( num_indices + NUM_SPECIAL_INDICES ) );
	n += left_size;

	size_t right_size =
		ROUND_UP( sizeof( struct storage_item* ) * ( num_indices + NUM_SPECIAL_INDICES ) );
	n += right_size;

	size_t index_val_size = ROUND_UP( sizeof( const char* ) * num_indices );
	n += index_val_size;

	size_t tsc_header_size = ROUND_UP( sizeof( struct tsc_header ) );
	n += tsc_header_size;

	size_t key_size = 0;
	if( key_len != INVALID_KEY ) {
		key_size = ROUND_UP( key_len + 1 );
		n += key_size;
	}

	LOG_TRACE( "n=d key_len=d new item", n, key_len );
	char* q = my_malloc( n );
	assert( q );

	StorageItem* p = (StorageItem*)q;
	q += storage_item_size;
	memset( p, 0, sizeof( StorageItem ) );

	p->color_field = (char*)q;
	q += color_field_size;

	p->left = (StorageItem**)q;
	q += left_size;

	p->right = (StorageItem**)q;
	q += right_size;

	p->index_val = (const char**)q;
	q += index_val_size;

	p->key_len = key_len;
	p->key = (char*)q;
	q += key_size;
	if( key_len != INVALID_KEY ) {
		memcpy( p->key, key, key_len );
		p->key[key_len] = 0; // null terminate the last value
	}

	p->data = (struct tsc_header*)q;
	q += tsc_header_size;
	tsc_init( p->data, initial_tsc_buf_size ); // TODO config val for initial tsc size

	// pre-allocate some room
	storage_item_ensure_offset_room( p, 4 );

	if( key_len != INVALID_KEY ) {
		LOG_TRACE( "lfm=*s extracting", p->key_len, p->key );
		for( int i = 0; i < num_indices; i++ ) {
			p->index_val[i] = get_key_value( p->key, p->key_len, indices[i] );
			LOG_TRACE( "index=d val=s lab=s extract label", i, p->index_val[i], indices[i] );
		}
	}

	p->internal_id = internal_id;

	return p;
}

// careful: storage_attach_key_to_item() will steam refrences to the item if a new peice of
// memory is malloc()/free()ed here, ensure it's also accounted for in storage_attach_key_to_item() too.
void storage_item_free( struct the_storage* store, StorageItem* item )
{
	tsc_free( item->data );
	my_free( item->data_offsets );
	my_free( item );
}

#define MAX_BUF 10240

int storage_write_key_and_internal_id( struct the_storage* store, StorageItem* item )
{
	// <key length><key><offset>
	size_t n = sizeof( uint16_t ) + item->key_len + sizeof( uint64_t );

	char buf[MAX_BUF];
	if( n > MAX_BUF )
		return 1;

	char* p = buf;
	memcpy( p, &( item->key_len ), sizeof( uint16_t ) );
	p += sizeof( uint16_t );

	memcpy( p, item->key, item->key_len );
	p += item->key_len;

	memcpy( p, &( item->internal_id ), sizeof( uint64_t ) );

	LOG_TRACE( "key=*s ID=d offset=d writing internal ID and key to disk",
			   item->key_len,
			   item->key,
			   item->internal_id,
			   item->key_storage_offset );
	return disk_storage_reserve_and_write_block(
		store->key_disk_store, DISK_STORAGE_BLOCK_STATUS_VALID, buf, n, &item->key_storage_offset );
}

int storage_get_item(
	struct the_storage* store, const char* key, uint16_t key_len, StorageItem** item, bool insert )
{
	StorageItem item_to_find;
	item_to_find.key_len = key_len;
	item_to_find.key = (char*)key;

	*item = sglib_StorageItem_find_member( store->root[LFM_INDEX], &item_to_find, LFM_INDEX );
	if( *item != NULL ) {
		return ERR_OK;
	}
	if( !insert ) {
		return ERR_LOOKUP_FAILED;
	}

	assert( key_len ); // TODO remove this, we should technically be able to store "" as a key.

	*item = storage_insert_item( store, key, key_len, store->next_internal_id );
	store->next_internal_id++;
	storage_write_key_and_internal_id( store, *item );

	LOG_TRACE( "key write worked" );

	return ERR_OK;
}

// when key_len = INVALID_KEY, the item is only inserted with the internal ID and must
// be fixed before the server is ready to start up.
StorageItem* storage_insert_item( struct the_storage* store,
								  const char* key,
								  uint16_t key_len,
								  uint64_t internal_id )
{
	// insert the item
	StorageItem* p = storage_item_new( key_len,
									   key,
									   store->initial_tsc_buf_size,
									   store->num_indices,
									   store->indices,
									   internal_id );

	if( key_len == INVALID_KEY ) {
		// only store it by the interal ID (since we dont have any other info to index off)
		// this item will be deleted and re-created once the key is known (and will be
		// properly inserted at that time)
		sglib_StorageItem_add( &( store->root[INTERNAL_ID_INDEX] ), p, INTERNAL_ID_INDEX );
		return p;
	}

	// insert into indices
	for( int i = 0; i < ( store->num_indices + NUM_SPECIAL_INDICES ); i++ ) {
		sglib_StorageItem_add( &( store->root[i] ), p, i );
	}

	// also insert into a doubly-linked list (used for walking and cleaning)
	if( store->list_root == NULL ) {
		p->next = p->prev = NULL;
		store->list_root = store->list_root_last = p;
	}
	else {
		assert( store->list_root_last->next == NULL );
		store->list_root_last->next = p;
		p->prev = store->list_root_last;
		p->next = 0;
		store->list_root_last = p;
	}

	INCR_METRIC( num_timeseries );

	return p;
}

void storage_insert_data_block_during_initial_load( struct the_storage* store,
													uint64_t internal_id,
													uint64_t offset,
													int64_t start_time,
													StorageItem** item,
													bool* last_block )
{
	int i;
	StorageItem item_to_find;
	item_to_find.internal_id = internal_id;
	StorageItem* p;

	p = sglib_StorageItem_find_member(
		store->root[INTERNAL_ID_INDEX], &item_to_find, INTERNAL_ID_INDEX );
	if( p == NULL ) {
		// insert a place holder for this where we'll fill in the key at a later time
		p = storage_insert_item( store, NULL, INVALID_KEY, internal_id );
		if( !p ) {
			LOG_ERROR( "id=d failed to insert item with null key", internal_id );
			assert( 0 );
		}
	}

	storage_item_ensure_offset_room( p, p->num_data_offsets + 1 );

	*last_block = true;
	for( i = 0; i < p->num_data_offsets; i++ ) {
		if( start_time < p->data_offsets[i].start_time ) {
			// make room for the item to be inserted
			memmove( &( p->data_offsets[i + 1] ),
					 &( p->data_offsets[i] ),
					 ( p->num_data_offsets - i ) * sizeof( struct offset_and_time ) );
			*last_block = false;
			break;
		}
		// if we reach the end of the loop without breaking, then it gets inserted at the end.
	}
	assert( offset > 0 );
	p->data_offsets[i].offset = offset;
	p->data_offsets[i].start_time = start_time;
	p->num_data_offsets++;

	LOG_TRACE( "offset=d start_time=d i=d inserted data block reference", offset, start_time, i );

	*item = p;
}

bool storage_attach_key_to_item( struct the_storage* store,
								 const char* key,
								 uint16_t key_len,
								 uint64_t internal_id,
								 uint64_t key_storage_offset )
{
	StorageItem item_to_find;
	item_to_find.internal_id = internal_id;
	StorageItem *p, *tmp;

	p = sglib_StorageItem_find_member(
		store->root[INTERNAL_ID_INDEX], &item_to_find, INTERNAL_ID_INDEX );
	if( p == NULL ) {
		LOG_TRACE( "key=*s ID=d unable to find an item to attach to", key_len, key, internal_id );
		return false;
	}
	assert( internal_id == p->internal_id );

	if( p->key_len != INVALID_KEY ) {
		LOG_ERROR( "key=*s existingkey=*s existingkeylen=d ID=d item already has a key",
				   key_len,
				   key,
				   p->key_len,
				   p->key,
				   p->key_len,
				   internal_id );
		assert( 0 );
	}

	sglib_StorageItem_delete( &( store->root[INTERNAL_ID_INDEX] ), p, INTERNAL_ID_INDEX );

	tmp = storage_insert_item( store, key, key_len, internal_id );
	assert( tmp );
	tmp->key_storage_offset = key_storage_offset;

	// steal the tsc data
	my_free( tmp->data->data );
	memcpy( tmp->data, p->data, sizeof( struct tsc_header ) );

	// steal the data_offset reference
	my_free( tmp->data_offsets );
	tmp->data_offsets = p->data_offsets;
	tmp->num_data_offsets = p->num_data_offsets;
	tmp->cap_data_offsets = p->cap_data_offsets;

	// since we stole the references, we can't call storage_item_free()
	my_free( p );

	return true;
}

int storage_write_disk( struct the_storage* store, StorageItem* item, bool complete_block )
{
	int res;
	int err = 0;
	size_t offset;
	uint8_t block_status;

	int buf_len;
	char* buf;
	char* s;

	LOG_TRACE( "n=d start_time=d number of points",
			   TSC_NUM_PTS( item->data->data ),
			   TSC_START_TIME( item->data->data ) );

	// make sure the data we're writing contains at least one point.
	assert( TSC_NUM_PTS( item->data->data ) > 0 );
	assert( TSC_START_TIME( item->data->data ) > 0 );

	if( complete_block ) {
		block_status = DISK_STORAGE_BLOCK_STATUS_VALID;

		buf_len = sizeof( uint64_t ) + item->data->data_len;
		buf = (char*)my_malloc( buf_len );

		memcpy( buf, &( item->internal_id ), sizeof( uint64_t ) );
		memcpy( buf + sizeof( uint64_t ), item->data->data, item->data->data_len );
	}
	else {
		block_status = DISK_STORAGE_BLOCK_STATUS_ACTIVE;

		buf_len = sizeof( uint64_t ) + TSC_HEADER_SERIALIZATION_SIZE + item->data->data_len;
		buf = (char*)my_malloc( buf_len );
		s = buf;

		memcpy( s, &( item->internal_id ), sizeof( uint64_t ) );
		s += sizeof( uint64_t );

		tsc_serialize_header( item->data, s );
		s += TSC_HEADER_SERIALIZATION_SIZE;

		memcpy( s, item->data->data, item->data->data_len );
	}

	if( item->num_data_offsets == 0 ) {
		assert( item->cap_data_offsets > 0 ); // should always have some pre-allocated
		item->data_offsets[0].offset = 0;
		item->num_data_offsets++;
	}
	int current = item->num_data_offsets - 1;
	assert( current >= 0 );

	LOG_TRACE( "lfm=*s block=d offset=d status=d writing out block",
			   item->key_len,
			   item->key,
			   current,
			   item->data_offsets[current].offset,
			   block_status );

	if( item->data_offsets[current].offset == 0 ) {
		res = disk_storage_reserve_and_write_block(
			store->data_disk_store, block_status, buf, buf_len, &offset );
		if( res != 0 ) {
			LOG_ERROR( "err=d failed to write block", res );
			err = res;
			goto done;
		}

		assert( offset > 0 );
		item->data_offsets[current].offset = offset;
		item->data_offsets[current].start_time = TSC_START_TIME( item->data->data );

		LOG_TRACE( "offset=d done", item->data_offsets[current].offset );
		if( complete_block ) {
			item->num_data_offsets++;
			storage_item_ensure_offset_room( item, item->num_data_offsets );
			tsc_clear( item->data );
			item->data_offsets[item->num_data_offsets - 1].offset = 0;
			item->data_offsets[item->num_data_offsets - 1].start_time = 0;
		}
		goto done;
	}

	// once the start time has been recorded, it shouldn't change
	// we have hit this assert in prod; we're going to add extra debugging info here for the next time it happens.
	//assert( item->data_offsets[current].start_time == TSC_START_TIME( item->data->data ) );
	if( item->data_offsets[current].start_time != TSC_START_TIME( item->data->data ) ) {
		LOG_ERROR( "lfm=*s block=d offset=d status=d start_time_lhs=d start_time_rhs=d complete=d "
				   "num_vaild_bits=d assert failed while writing out block",
				   item->key_len,
				   item->key,
				   current,
				   item->data_offsets[current].offset,
				   block_status,
				   item->data_offsets[current].start_time,
				   TSC_START_TIME( item->data->data ),
				   complete_block,
				   item->data->num_valid_bits );

		assert( 0 ); // fail
	}

	res = disk_storage_write_block(
		store->data_disk_store, item->data_offsets[current].offset, block_status, buf, buf_len );
	if( res != 0 ) {
		err = res;
		goto done;
	}
	if( complete_block ) {
		item->num_data_offsets++;
		storage_item_ensure_offset_room( item, item->num_data_offsets );
		tsc_clear( item->data );
		item->data_offsets[item->num_data_offsets - 1].offset = 0;
		item->data_offsets[item->num_data_offsets - 1].start_time = 0;
	}

done:
	if( buf ) {
		my_free( buf );
	}
	return err;
}

int storage_write( struct the_storage* store,
				   const char* key,
				   uint16_t key_len,
				   int64_t* t,
				   double* y,
				   size_t num_pts )
{
	StorageItem* item;
	int res;
	int err = 0;

	res = pthread_mutex_lock( &( store->lock ) );
	if( res ) {
		LOG_ERROR( "res=d failed to lock" );
		return 1;
	}

	if( ( res = storage_get_item( store, key, key_len, &item, true ) ) ) {
		LOG_ERROR( "res=s failed to get item", err_str( res ) );
		err = res;
		goto error;
	}

	LOG_TRACE( "n=d start_time=d number of points",
			   TSC_NUM_PTS( item->data->data ),
			   TSC_START_TIME( item->data->data ) );

	for( int i = 0; i < num_pts; i++ ) {
		res = tsc_add( item->data, t[i], y[i] );
		if( res == ERR_TSC_OUT_OF_SPACE ) {
			// flush to disk
			LOG_TRACE( "out of memory, writing block to disk" );
			res = storage_write_disk( store, item, true );
			if( res != 0 ) {
				err = res;
				goto error;
			}

			CRASH_TEST(, CRASH_TEST_AFTER_COMPLETE_BLOCK_WRITE )

			// write the point (since the first time failed)
			res = tsc_add( item->data, t[i], y[i] );
			if( res != 0 ) {
				assert( 0 ); // TODO remove this
				err = res;
				goto error;
			}
		}
		if( res != 0 ) {
			err = res;
			goto error;
		}
	}

error:
	res = pthread_mutex_unlock( &( store->lock ) );
	if( res ) {
		LOG_ERROR( "res=d failed to unlock" );
	}

	return err;
}

int storage_sync( struct the_storage* store, bool deadlock )
{
	StorageItem* item;
	int res;
	int err = 0;

	res = pthread_mutex_lock( &( store->lock ) );
	if( res ) {
		LOG_ERROR( "res=d failed to lock" );
		return 1;
	}

	for( item = store->list_root; item; item = item->next ) {
		res = storage_write_disk( store, item, false );
		if( res != 0 ) {
			LOG_ERROR( "res=d failed to write to disk", res );
			err = res;
			goto error;
		}
	}

error:
	if( !deadlock ) {
		res = pthread_mutex_unlock( &( store->lock ) );
		if( res ) {
			LOG_ERROR( "res=d failed to unlock" );
		}
	}

	return err;
}

int storage_get( struct the_storage* store,
				 const char* key,
				 uint16_t key_len,
				 int64_t* t,
				 double* y,
				 size_t max_num_pts,
				 size_t* num_pts )
{
	struct tsc_iter iter;
	StorageItem* item;
	int res;
	int err = 0;
	int pts_i = 0;

	uint64_t internal_id;

	res = pthread_mutex_lock( &( store->lock ) );
	if( res ) {
		LOG_ERROR( "res=d failed to lock" );
		return ERR_UNKNOWN;
	}

	if( ( res = storage_get_item( store, key, key_len, &item, false ) ) != 0 ) {
		err = res;
		goto error;
	}

	// This won't work now that we have multiple blocks
	// we'll need to iterate over all blocks and sum up these values
	// if( TSC_NUM_PTS( item->data->data ) > max_num_pts ) {
	//	LOG_ERROR( "required=d avail=d client requested too many points",
	//			   TSC_NUM_PTS( item->data->data ),
	//			   max_num_pts );
	//	err = ERR_TOO_MANY_POINTS;
	//	goto error;
	//}

	char buf[STORAGE_GET_BUF_N]; // TODO FIXME use a known block size value and stick to it.
	size_t n;
	const char* p;
	uint8_t block_status;

	for( int i = 0; i < item->num_data_offsets; i++ ) {
		size_t offset = item->data_offsets[i].offset;
		LOG_TRACE( "offset=d i=d reading block at offset", offset, i );
		if( offset ) {
			res = disk_storage_read_block(
				store->data_disk_store, offset, &block_status, buf, &n, STORAGE_GET_BUF_N );
			assert( res == 0 );

			//printf( "status %d\n", block_status );
			if( block_status == DISK_STORAGE_BLOCK_STATUS_ACTIVE ) {
				// should only happen on the last item
				assert( i == item->num_data_offsets - 1 );
				break;
			}

			memcpy( &internal_id, buf, sizeof( uint64_t ) );
			if( internal_id != item->internal_id ) {
				LOG_ERROR( "offset=d corrupt data", offset );
				assert( 0 );
			}
			p = buf + sizeof( uint64_t );

			LOG_TRACE( "num_pts=d num of points to read", TSC_NUM_PTS( p ) );

			// decode points from block
			tsc_iter_init( &iter );
			for( ;; ) {
				if( tsc_iter_next( p, &iter ) ) {
					break;
				}

				if( pts_i >= max_num_pts ) {
					err = ERR_TOO_MANY_POINTS;
					goto error;
				}

				t[pts_i] = TSC_ITER_T( &iter );
				y[pts_i] = TSC_ITER_Y( &iter );
				pts_i++;
			}
		}
	}

	// next read data from memory
	LOG_TRACE( "reading from memory" );
	tsc_iter_init( &iter );
	for( ;; ) {
		if( tsc_iter_next( item->data->data, &iter ) ) {
			break;
		}

		if( pts_i >= max_num_pts ) {
			err = ERR_TOO_MANY_POINTS;
			goto error;
		}

		t[pts_i] = TSC_ITER_T( &iter );
		y[pts_i] = TSC_ITER_Y( &iter );
		pts_i++;
	}

	*num_pts = pts_i;

error:
	res = pthread_mutex_unlock( &( store->lock ) );
	if( res ) {
		LOG_ERROR( "res=d failed to unlock" );
	}

	return err;
}

void storage_trim_and_write_to_disk( struct the_storage* store,
									 StorageItem* item,
									 int64_t t,
									 bool* should_delete,
									 int64_t* num_pts_remaining )
{
	int i;

	if( item->data->last_timestamp < t ) {
		// all data is old, delete the entire thing
		*should_delete = true;
		return;
	}
	*should_delete = false;

	int save_i = 0;
	for( i = 0; i < item->num_data_offsets; i++ ) {
		if( item->data_offsets[i].offset == 0 ) {
			// the only item that might be 0 must be the last item; an item with a zero signals the
			// previous item was written as a complete block.
			assert( i == ( item->num_data_offsets - 1 ) );
			break;
		}
		assert( item->data_offsets[i].offset > 0 );
		if( item->data_offsets[i].start_time < t ) {
			save_i = i; // anything before i should be deleted
		}
		else {
			break;
		}
	}

	if( save_i > 0 ) {
		assert( save_i < item->num_data_offsets );
		for( i = 0; i < save_i; i++ ) {
			off_t offset = item->data_offsets[i].offset;
			LOG_TRACE(
				"key=*s offset=d deleting old data from disk", item->key_len, item->key, offset );
			disk_storage_release_block( store->data_disk_store, offset );
		}
		item->num_data_offsets -= save_i;
		assert( item->num_data_offsets > 0 );
		memmove( item->data_offsets,
				 &item->data_offsets[save_i],
				 item->num_data_offsets * sizeof( struct offset_and_time ) );
	}
}

// delete the item (both from memory AND DISK), and return item->next
StorageItem* storage_delete_item( struct the_storage* store, StorageItem* item )
{
	int i;

	StorageItem* next_item = item->next;
	if( item->prev ) {
		item->prev->next = item->next;
	}
	if( item->next ) {
		item->next->prev = item->prev;
	}
	if( item == store->list_root ) {
		assert( item->prev == NULL );
		store->list_root = item->next;
	}
	if( item == store->list_root_last ) {
		assert( item->next == NULL );
		store->list_root_last = item->prev;
	}

	for( i = 0; i < ( store->num_indices + NUM_SPECIAL_INDICES ); i++ ) {
		sglib_StorageItem_delete( &( store->root[i] ), item, i );
	}

	// delete the timeseries data from disk
	for( i = 0; i < item->num_data_offsets; i++ ) {
		off_t offset = item->data_offsets[i].offset;
		if( offset == 0 ) {
			// this means we hit the last block which was never actually written to
			// this happens when the previous block was written to as a complete block.
			assert( i == item->num_data_offsets - 1 );
			break;
		}
		LOG_TRACE( "offset=d delete block", offset );
		disk_storage_release_block( store->data_disk_store, offset );
	}

	disk_storage_release_block( store->key_disk_store, item->key_storage_offset );

	storage_item_free( store, item );

	DECR_METRIC( num_timeseries );
	return next_item;
}

// TODO refactor this; it only exists for unittesting
// and needs to be combined with storage_thread_run()
void clean_items( struct the_storage* store )
{
	int64_t num_pts_remaining;
	int64_t t = (int64_t)my_time( NULL ) - store->retention_seconds;

	int res = pthread_mutex_lock( &( store->lock ) );
	assert( res == 0 );

	StorageItem* item = store->list_root;
	while( item ) {
		bool should_delete = false;
		storage_trim_and_write_to_disk( store, item, t, &should_delete, &num_pts_remaining );
		if( should_delete ) {
			LOG_TRACE( "key=*s delete empty key", item->key_len, item->key );
			item = storage_delete_item( store, item );
		}
		else {
			item = item->next;
		}
	}

	res = pthread_mutex_unlock( &( store->lock ) );
	assert( res == 0 );
}

void* storage_thread_run( void* ptr )
{
	long long inner_loop_start_time, loop_duration;
	int64_t t;
	int res;
	StorageItem* item = NULL;
	struct the_storage* store = (struct the_storage*)ptr;

	int max_num_items = 50;
	int num_items = 0;
	int64_t total_num_items = 0;

	bool first_loop = true;
	int64_t num_pts = 0;
	int64_t num_pts_remaining;

	for( ;; ) {
		if( total_num_items == 0 ) {
			// don't waste CPU when there's nothing to do
			sleep( 1 );
		}

		t = (int64_t)my_time( NULL ) - store->retention_seconds;
		res = pthread_mutex_lock( &( store->lock ) );
		if( res ) {
			LOG_ERROR( "res=d failed to lock" );
			continue;
		}

		if( item == NULL ) {
			item = store->list_root;
			if( first_loop ) {
				first_loop = false;
			}
			else {
				INCR_METRIC( num_cleaning_cycles );
				SET_METRIC( num_points, num_pts ); // TODO this should be tracked based on when
					// points are added or removed
				num_pts = 0;
			}
			total_num_items = 0;
		}

		inner_loop_start_time = time_microseconds();
		num_items = 0;
		for( ; item != NULL && num_items < max_num_items; num_items++, total_num_items++ ) {
			// LOG_TRACE( "key=*s has key", item->key_len, item->key );

			bool should_delete = false;
			storage_trim_and_write_to_disk( store, item, t, &should_delete, &num_pts_remaining );
			if( should_delete ) {
				LOG_TRACE( "key=*s delete empty key", item->key_len, item->key );
				item = storage_delete_item( store, item );
			}
			else {
				item = item->next;
				num_pts += num_pts_remaining;
			}
		}

		res = pthread_mutex_unlock( &( store->lock ) );
		if( res ) {
			LOG_ERROR( "res=d failed to unlock" );
		}

		loop_duration = time_microseconds() - inner_loop_start_time;
		int sleep_time = loop_duration * store->cleaning_sleep_multiplier;
		if( loop_duration > store->max_duration ) {
			LOG_WARN( "num_items=d actual=d max_target=d loop took longer than expected",
					  num_items,
					  loop_duration,
					  store->max_duration );
			sleep_time = store->max_duration * store->cleaning_sleep_multiplier;
		}
		ADD_METRIC( cleaning_item, num_items );

		usleep( sleep_time );
		SET_METRIC( cleaning_loop_sleep, sleep_time );
	}
	return NULL;
}

bool extra_matchers_match( const char* key,
						   uint16_t key_len,
						   const char* extra_matchers,
						   int num_extra_matchers )
{
	int i;
	size_t n;
	const char* match_key;
	const char* match_val;
	const char* p = extra_matchers;
	for( i = 0; i < num_extra_matchers; i++ ) {
		match_key = p;

		match_val = match_key + strlen( match_key ) + 1;

		// skip to next set
		p = match_val + strlen( match_val ) + 1;

		// length of match_key+\0+match_val+\0
		n = p - match_key;

		LOG_TRACE( "match_key=s match_val=s SEARCHING", match_key, match_val );
		if( strcmp( match_key, "__name__" ) == 0 ) {
			// special case for name, which is the first string in the lfm
			LOG_TRACE( "lfm=*s search=s extra_matchers_match name?", key_len, key, match_val );
			if( strcmp( key, match_val ) != 0 ) {
				LOG_TRACE( "no name match" );
				return false;
			}
		}
		else {
			LOG_TRACE( "lfm=*s search=*s extra_matchers_match?", key_len, key, n, match_key );
			if( memmem( key,
						key_len + 1 /*the key always has an extra null terminator, to ensure the
									   last value is null terminated */
						,
						match_key,
						n ) == NULL ) {
				LOG_TRACE( "no match" );
				return false;
			}
		}
	}
	LOG_TRACE( "match" );
	return true;
}

int storage_lookup_without_index( struct the_storage* store,
								  const char* matchers,
								  int num_matchers,
								  char* results,
								  size_t* results_n,
								  size_t max_results_size,
								  uint64_t* last_internal_id,
								  uint64_t limit )
{
	int res = 0;
	*results_n = 0;

	assert( last_internal_id );

	assert( pthread_mutex_lock( &( store->lock ) ) == 0 );

	StorageItem* item = store->list_root;

	if( *last_internal_id ) {
		StorageItem item_to_find;
		item_to_find.internal_id = *last_internal_id;

		item = sglib_StorageItem_find_member_or_nearest_lesser(
			store->root[INTERNAL_ID_INDEX], &item_to_find, INTERNAL_ID_INDEX );
		if( item ) {
			item = item->next;
			if( item ) {
				LOG_TRACE( "lfm=*s resuming scan", item->key_len, item->key );
			}
		}
	}

	while( item ) {
		if( extra_matchers_match( item->key, item->key_len, matchers, num_matchers ) ) {
			if( limit == 0 ) {
				break;
			}
			limit--;

			// save LFM to results
			LOG_TRACE( "lfm=*s found", item->key_len, item->key );
			size_t n = item->key_len + sizeof( uint16_t );
			if( max_results_size < n ) {
				res = ERR_BUF_TOO_SMALL;
				goto error;
			}
			assert( item->key_len >
					0 ); // this would signal an end of results, which would mess up serialization
			uint16_t x = item->key_len;
			memcpy( results, &x, sizeof( uint16_t ) );
			results += sizeof( uint16_t );
			memcpy( results, item->key, item->key_len );
			results += item->key_len;
			max_results_size -= n;
			*results_n += n;
			*last_internal_id = item->internal_id;
		}
		item = item->next;
	}

error:
	assert( pthread_mutex_unlock( &( store->lock ) ) == 0 );
	return res;
}

int storage_lookup( struct the_storage* store,
					int index_num,
					const char* label_value,
					const char* extra_matchers,
					int num_extra_matchers,
					char* results,
					size_t* results_n,
					size_t max_results_size,
					uint64_t offset,
					uint64_t limit )
{
	int res = 0;

	assert( pthread_mutex_lock( &( store->lock ) ) == 0 );

	assert( label_value );

	StorageItem lookup;
	memset( &lookup, 0, sizeof( lookup ) );

	if( index_num == NAME_INDEX ) {
		lookup.key = my_strdup( label_value );
	}
	else if( NUM_SPECIAL_INDICES <= index_num &&
			 index_num < ( store->num_indices + NUM_SPECIAL_INDICES ) ) {
		lookup.index_val = my_malloc( sizeof( struct storage_item* ) * store->num_indices );
		lookup.index_val[index_num - NUM_SPECIAL_INDICES] = label_value;
	}
	else {
		LOG_ERROR( "index=d unhandled lookup", index_num );
		res = ERR_BAD_INDEX;
		goto error;
	}

	struct sglib_StorageItem_iterator itr;
	StorageItem* elem;
	*results_n = 0;
	for( elem = sglib_StorageItem_it_init_on_equal(
			 &itr, store->root[index_num], NULL, &lookup, index_num );
		 elem != NULL;
		 elem = sglib_StorageItem_it_next( &itr, index_num ) ) {
		if( extra_matchers_match( elem->key, elem->key_len, extra_matchers, num_extra_matchers ) ) {
			if( offset > 0 ) {
				offset--;
				continue;
			}
			if( limit == 0 ) {
				break;
			}
			limit--;

			LOG_TRACE( "lfm=*s found", elem->key_len, elem->key );
			size_t n = elem->key_len + sizeof( uint16_t );
			if( max_results_size < n ) {
				res = ERR_BUF_TOO_SMALL;
				goto error;
			}
			assert( elem->key_len >
					0 ); // this would signal an end of results, which would mess up serialization
			uint16_t x = elem->key_len;
			memcpy( results, &x, sizeof( uint16_t ) );
			results += sizeof( uint16_t );
			memcpy( results, elem->key, elem->key_len );
			results += elem->key_len;
			max_results_size -= n;
			*results_n += n;
		}
	}

error:
	if( lookup.key ) {
		my_free( lookup.key );
		lookup.key = NULL;
	}
	if( lookup.index_val ) {
		my_free( lookup.index_val );
		lookup.index_val = NULL;
	}
	assert( pthread_mutex_unlock( &( store->lock ) ) == 0 );
	return res;
}

int read_or_create_cluster_config_from_disk( struct the_storage* store, const char* path )
{
	int res = 0;

	store->cluster_config_path = my_strdup( path );

	int fd = open( path, O_RDWR | O_CREAT | O_LARGEFILE, 0644 );
	if( fd < 0 ) {
		LOG_ERROR( "err=s path=s failed to open cluster config", strerror( errno ), path );
		res = 1;
		goto error;
	}

	off_t end_position = lseek( fd, 0, SEEK_END );
	store->cluster_config = my_malloc( end_position + 1 );
	if( lseek( fd, 0, SEEK_SET ) != 0 ) {
		LOG_ERROR( "err=s path=s failed to seek to start of file", strerror( errno ), path );
		res = 1;
		goto error;
	}

	ssize_t n = read( fd, store->cluster_config, end_position );
	if( n != end_position ) {
		LOG_ERROR( "err=s n=d path=s failed to read", strerror( errno ), n, path );
		return 1;
	}
	store->cluster_config[end_position] = '\0';

	SHA1( (unsigned char*)store->cluster_config,
		  strlen( store->cluster_config ),
		  (unsigned char*)store->cluster_config_hash );

	return 0;
error:
	if( store->cluster_config_path ) {
		my_free( store->cluster_config_path );
		store->cluster_config_path = NULL;
	}
	if( store->cluster_config ) {
		my_free( store->cluster_config );
		store->cluster_config = NULL;
	}

	return res;
}

// TODO fix this to write to a tmp file, then do an atomic move to prevent failures during write/truncate calls
int save_cluster_config_to_disk( struct the_storage* store )
{
	int res = 0;

	int fd = open( store->cluster_config_path, O_RDWR | O_CREAT | O_LARGEFILE, 0644 );
	if( fd < 0 ) {
		LOG_ERROR( "err=s path=s failed to open cluster config",
				   strerror( errno ),
				   store->cluster_config_path );
		res = 1;
		goto error;
	}

	if( lseek( fd, 0, SEEK_SET ) != 0 ) {
		LOG_ERROR( "err=s path=s failed to seek to start of file",
				   strerror( errno ),
				   store->cluster_config_path );
		res = 1;
		goto error;
	}

	ssize_t l = strlen( store->cluster_config );
	ssize_t n = write( fd, store->cluster_config, l );
	if( n != l ) {
		LOG_ERROR( "err=s path=s failed to write", strerror( errno ), store->cluster_config_path );
		res = 1;
		goto error;
	}

	res = ftruncate( fd, l );
	if( res ) {
		LOG_ERROR(
			"err=s path=s failed to truncate", strerror( errno ), store->cluster_config_path );
		res = 1;
		goto error;
	}

	SHA1( (unsigned char*)store->cluster_config,
		  strlen( store->cluster_config ),
		  (unsigned char*)store->cluster_config_hash );

	return 0;
error:
	return res;
}
