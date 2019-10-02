#include "tests.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fast_rand.h"
#include "globals.h"
#include "log.h"
#include "my_malloc.h"
#include "storage.h"
#include "test_storage_utils.h"
#include "time_utils.h"

void test_storage_reload( void )
{
	int res;

	my_malloc_init();
	delete_test_storage();
	const char* storage_path = get_test_storage_path();

	size_t num_pts;
	int64_t now = my_time( NULL );
	struct the_storage my_storage;
	int retention_seconds = 60;
	int target_cleaning_loop_seconds = 10;
	int initial_tsc_buf_size = 50;
	int num_indices = 2;
	const char* indices[] = {
		"user_id",
		"host_id",
	};
	assert( num_indices == sizeof( indices ) / sizeof( char* ) );

	//const int user_id_index = NUM_SPECIAL_INDICES + 0;
	//const int host_id_index = NUM_SPECIAL_INDICES + 1;

	// retention_seconds = 10;
	if( ( res = storage_init( &my_storage,
							  retention_seconds,
							  target_cleaning_loop_seconds,
							  initial_tsc_buf_size,
							  num_indices,
							  indices,
							  storage_path ) ) ) {
		CU_ASSERT( 0 );
	}

	int64_t t[] = {now};
	double y[] = {3.14};

	res = storage_write( &my_storage, "hello", 5, t, y, 1 );
	CU_ASSERT( res == 0 );

	// test read
	res = storage_get( &my_storage, "hello", 5, t, y, 1, &num_pts );
	CU_ASSERT( res == 0 );
	CU_ASSERT( num_pts == 1 );

	// This test assumes that writing a single point here will not cause the compressed timeseries to be written to disk
	// because:
	//  - a single point doesn't cause the tsc data to overflow the current block
	//  - the syncing/cleanup thread is not running

	// discard points without any sync to disk
	storage_free( &my_storage );

	// TODO even if this were synced, we could advance the clock to cause this data to have expired

	// reload from disk
	// the disk should only contain the key -> internal ID mapping, but no timeseries data
	if( ( res = storage_init( &my_storage,
							  retention_seconds,
							  target_cleaning_loop_seconds,
							  initial_tsc_buf_size,
							  num_indices,
							  indices,
							  storage_path ) ) ) {
		CU_ASSERT( 0 );
	}

	// test that the key was discarded since no timeseries data was found.

	res = storage_get( &my_storage, "hello", 5, t, y, 1, &num_pts );
	CU_ASSERT( res == ERR_LOOKUP_FAILED );

	storage_free( &my_storage );

	my_malloc_assert_free();
	my_malloc_free();
}

void test_storage_reload2( void )
{
	int res;

	my_malloc_init();
	delete_test_storage();
	const char* storage_path = get_test_storage_path();

	size_t num_pts;
	int64_t now = my_time( NULL );
	struct the_storage my_storage;
	int retention_seconds = 60;
	int target_cleaning_loop_seconds = 10;
	int initial_tsc_buf_size = 50;
	int num_indices = 2;
	const char* indices[] = {
		"user_id",
		"host_id",
	};
	assert( num_indices == sizeof( indices ) / sizeof( char* ) );

	//const int user_id_index = NUM_SPECIAL_INDICES + 0;
	//const int host_id_index = NUM_SPECIAL_INDICES + 1;

	// retention_seconds = 10;
	if( ( res = storage_init( &my_storage,
							  retention_seconds,
							  target_cleaning_loop_seconds,
							  initial_tsc_buf_size,
							  num_indices,
							  indices,
							  storage_path ) ) ) {
		CU_ASSERT( 0 );
	}

	int64_t t[] = {now, now + 1, now + 2};
	double y[] = {3.14, 8.12, 4.1};

	int64_t tt[3];
	double yy[3];

	for( int i = 0; i < 3; i++ ) {
		res = storage_write( &my_storage, "hello", 5, t + i, y + i, 1 );
		CU_ASSERT( res == 0 );
	}

	// test read
	res = storage_get( &my_storage, "hello", 5, tt, yy, 3, &num_pts );
	CU_ASSERT( res == 0 );
	CU_ASSERT( num_pts == 3 );
	for( int i = 0; i < 3; i++ ) {
		CU_ASSERT( tt[i] == t[i] );
		CU_ASSERT( yy[i] == y[i] );
	}

	// perform sync
	storage_sync( &my_storage, false );

	// close file
	storage_free( &my_storage );

	// reload from disk
	if( ( res = storage_init( &my_storage,
							  retention_seconds,
							  target_cleaning_loop_seconds,
							  initial_tsc_buf_size,
							  num_indices,
							  indices,
							  storage_path ) ) ) {
		CU_ASSERT( 0 );
	}
	//LOG_INFO("---------------");
	//// test that the key was correctly loaded
	//res = storage_get( &my_storage, "hello", 5, tt, yy, 3, &num_pts );
	//LOG_DEBUG("res=d got it", res);
	//CU_ASSERT( res == 0 );
	//CU_ASSERT( num_pts == 3 );
	//for( int i = 0; i < 3; i++ ) {
	//	CU_ASSERT( tt[i] == t[i] );
	//	CU_ASSERT( yy[i] == y[i] );
	//}

	storage_free( &my_storage );

	my_malloc_assert_free();
	my_malloc_free();
}
