#include "tests.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "my_malloc.h"
#include "storage.h"
#include "test_storage_utils.h"
#include "time_utils.h"

#include "generate_test_data.h"

#ifdef DEBUG_BUILD

// has_completed_block returns true when the most recent write caused the data block to be marked as completed
bool has_active_block( struct the_storage* store, const char* key, int key_len )
{
	StorageItem item_to_find, *item;
	item_to_find.key_len = key_len;
	item_to_find.key = (char*)key;

	item = sglib_StorageItem_find_member( store->root[LFM_INDEX], &item_to_find, LFM_INDEX );

	if( item->num_data_offsets == 0 ) {
		printf( "no blocks\n" );
		return false;
	}

	int i = item->num_data_offsets - 1;
	if( item->data_offsets[i].offset == 0 ) {
		printf( "no active block\n" );
		return false;
	}

	printf( "active block\n" );
	return true;
}

void test_storage_stress_impl( bool always_clean )
{
	int res;

	set_time( 5000 );
	my_malloc_init();
	const char* storage_path = get_test_storage_path();

	// test controls
	int num_metrics = 3;
	int num_pts = 3983;
	int retention_seconds = 500;

	struct the_storage my_storage;
	int target_cleaning_loop_seconds = 10;
	int initial_tsc_buf_size = 50;
	int num_indices = 2;
	const char* indices[] = {
		"user_id",
		"host_id",
	};
	assert( num_indices == sizeof( indices ) / sizeof( char* ) );

	// const int user_id_index = NUM_SPECIAL_INDICES + 0;
	// const int host_id_index = NUM_SPECIAL_INDICES + 1;

	if( ( res = storage_init( &my_storage,
							  retention_seconds,
							  target_cleaning_loop_seconds,
							  initial_tsc_buf_size,
							  num_indices,
							  indices,
							  storage_path ) ) ) {
		CU_ASSERT( 0 );
	}

	char lfm[TEST_LFM_MAX_SIZE];
	int lfm_len;
	double y;
	int64_t t;

	for( int i = 0; i < num_pts; i++ ) {
		t = my_time( NULL );
		LOG_DEBUG( "t=d i=d writing points", t, i );
		for( int user_id = 0; user_id < num_metrics; user_id++ ) {
			get_test_lfm( lfm, &lfm_len, user_id );
			y = get_test_pt( user_id * i );
			res = storage_write( &my_storage, lfm, lfm_len, &t, &y, 1 );
			CU_ASSERT( res == 0 );
		}
		add_time( 1 );
		if( always_clean ) {
			clean_items( &my_storage );
		}
		else {
			if( has_active_block( &my_storage, lfm, lfm_len ) ) {
				clean_items( &my_storage );
			}
			else {
				storage_sync( &my_storage, false );
			}
		}
	}

	// TODO test read

	//int64_t* tt = (int64_t*)my_malloc( sizeof( int64_t ) * num_pts );
	//double* yy = (double*)my_malloc( sizeof( double ) * num_pts );

	//size_t num_pts_read;
	//res = storage_get( &my_storage, lfm, lfm_len, tt, yy, num_pts, &num_pts_read );
	//CU_ASSERT( num_pts_read == num_pts );

	////////////////////////////////////////////
	//// done with tests, cleanup

	//my_free( tt );
	//my_free( yy );

	storage_free( &my_storage );

	my_malloc_assert_free();
	my_malloc_free();
}

void test_storage_stress1( void )
{
	test_storage_stress_impl( false );
}

void test_storage_stress2( void )
{
	test_storage_stress_impl( true );
}

#else
void test_storage_stress1( void )
{
	LOG_WARN( "test_storage_stress1 only runs in debug mode" );
}

void test_storage_stress2( void )
{
	LOG_WARN( "test_storage_stress2 only runs in debug mode" );
}
#endif // DEBUG_BUILD
