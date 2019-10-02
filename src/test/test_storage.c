#include "tests.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fast_rand.h"
#include "log.h"
#include "my_malloc.h"
#include "storage.h"
#include "test_storage_utils.h"
#include "time_utils.h"

// antoi is like atoi, but supports non-null strings
int antoi( const char* s, size_t n )
{
	int res = 0;
	int j = 0;
	int sign = 1;

	if( n <= 0 ) {
		return 0;
	}
	if( s[0] == '-' ) {
		if( n == 1 ) {
			return 0;
		}
		sign = -1;
		n--;
		s++;
	}

	for( int i = 0; i < n; i++ ) {
		if( '0' <= s[i] && s[i] <= '9' ) {
			j++;
		}
		else {
			break;
		}
	}
	int exp = 1;
	for( int i = j - 1; i >= 0; i-- ) {
		res += ( s[i] - '0' ) * exp;
		exp *= 10;
	}
	return sign * res;
}

void test_antoi( void )
{
	CU_ASSERT( antoi( "104999999", 3 ) == 104 );
	CU_ASSERT( antoi( "a04999999", 3 ) == 0 );
	CU_ASSERT( antoi( "004999999", 3 ) == 4 );
	CU_ASSERT( antoi( "004999999", 3 ) == 4 );
	CU_ASSERT( antoi( "-10", 3 ) == -10 );
	CU_ASSERT( antoi( "10", 0 ) == 0 );
}

void test_storage( void )
{
	int res;

	my_malloc_init();
	delete_test_storage();
	const char* storage_path = get_test_storage_path();

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

	const int user_id_index = NUM_SPECIAL_INDICES + 0;
	const int host_id_index = NUM_SPECIAL_INDICES + 1;

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

	res = storage_write( &my_storage,
						 "hello"
						 "\x00"
						 "user_id"
						 "\x00"
						 "123",
						 5 + 1 + 7 + 1 + 3,
						 t,
						 y,
						 1 );
	CU_ASSERT( res == 0 );

	res = storage_write( &my_storage,
						 "world"
						 "\x00"
						 "user_id"
						 "\x00"
						 "123",
						 5 + 1 + 7 + 1 + 3,
						 t,
						 y,
						 1 );
	CU_ASSERT( res == 0 );

	size_t num_pts;
	res = storage_get( &my_storage, "hello", 5, t, y, 1, &num_pts );

	CU_ASSERT( res == 0 );
	CU_ASSERT( num_pts == 1 );

	CU_ASSERT( t[0] == now );
	CU_ASSERT( y[0] == 3.14 );

	char results[1024];
	size_t results_n = 0;
	const char* expected = NULL;

	// test name lookup works
	res = storage_lookup(
		&my_storage, NAME_INDEX, "hello", NULL, 0, results, &results_n, 1024, 0, 100 );

	CU_ASSERT( res == 0 );
	CU_ASSERT( results_n == 26 );
	LOG_DEBUG( "results=*s n=d got results", results_n, results, results_n );
	expected = "\x05"
			   "\x00"
			   "hello"
			   "\x11"
			   "\x00"
			   "hello"
			   "\x00"
			   "user_id"
			   "\x00"
			   "123";
	CU_ASSERT( memcmp( expected, results, results_n ) == 0 );

	// test user_id lookup works
	res = storage_lookup(
		&my_storage, user_id_index, "123", NULL, 0, results, &results_n, 1024, 0, 100 );

	CU_ASSERT( res == 0 );
	CU_ASSERT( results_n == 38 );
	LOG_DEBUG( "results=*s n=d got results", results_n, results, results_n );
	expected = "\x11"
			   "\x00"
			   "hello"
			   "\x00"
			   "user_id"
			   "\x00"
			   "123"
			   "\x11"
			   "\x00"
			   "world"
			   "\x00"
			   "user_id"
			   "\x00"
			   "123";
	assert( results_n == 38 );
	CU_ASSERT( memcmp( expected, results, results_n ) == 0 );

	// test user_id lookup works with limit
	res = storage_lookup(
		&my_storage, user_id_index, "123", NULL, 0, results, &results_n, 1024, 0, 1 );

	CU_ASSERT( res == 0 );
	CU_ASSERT( results_n == 19 );
	LOG_DEBUG( "results=*s n=d got results", results_n, results, results_n );
	expected = "\x11"
			   "\x00"
			   "hello"
			   "\x00"
			   "user_id"
			   "\x00"
			   "123";
	CU_ASSERT( memcmp( expected, results, results_n ) == 0 );

	// test user_id lookup works with limit and offset
	res = storage_lookup(
		&my_storage, user_id_index, "123", NULL, 0, results, &results_n, 1024, 1, 1 );

	CU_ASSERT( res == 0 );
	CU_ASSERT( results_n == 19 );
	LOG_DEBUG( "results=*s n=d got results", results_n, results, results_n );
	expected = "\x11"
			   "\x00"
			   "world"
			   "\x00"
			   "user_id"
			   "\x00"
			   "123";
	CU_ASSERT( memcmp( expected, results, results_n ) == 0 );

	// test host_id lookup works
	res = storage_lookup( &my_storage,
						  host_id_index,
						  "nothing-exists-for-host-id",
						  NULL,
						  0,
						  results,
						  &results_n,
						  1024,
						  0,
						  100 );

	CU_ASSERT( res == 0 );
	CU_ASSERT( results_n == 0 );

	//////////////////////////////////////////
	// done with tests, cleanup

	storage_free( &my_storage );

	my_malloc_assert_free();
	my_malloc_free();
}

void test_nearest_pagination( void )
{
	int res;

	my_malloc_init();

	const char* storage_path = get_test_storage_path();

	int64_t now = my_time( NULL );
	struct the_storage my_storage;
	int retention_seconds = 60;
	int target_cleaning_loop_seconds = 10;
	int initial_tsc_buf_size = 50;
	int num_indices = 0;
	const char* indices[] = {};
	assert( num_indices == sizeof( indices ) / sizeof( char* ) );

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

	const int num_keys_to_send = 100;
	int n;
	char buf[1024];
	for( int i = 0; i < num_keys_to_send; i++ ) {
		sprintf( buf, "hello-%d", i );
		n = strlen( buf );
		res = storage_write( &my_storage, buf, n, t, y, 1 );
		CU_ASSERT( res == 0 );

		// ensure that IDs ___8 and ___9 never actually occur
		if( my_storage.next_internal_id % 10 == 8 ) {
			my_storage.next_internal_id += 2;
		}
	}

	const char* matchers = NULL;
	int num_matchers = 0;

	const int max_results_size = 1024;
	char results[max_results_size];
	size_t results_n;

	uint64_t last_internal_id = 0;
	uint64_t limit = 3;

	bool found[num_keys_to_send];
	memset( found, 0, sizeof( bool ) * num_keys_to_send );

	for( ;; ) {
		if( last_internal_id % 10 == 7 ) {
			// since ID ***8 and ***9 never exist, we're going to set
			// the last internal ID to one of those, to ensure we can correctly
			// jump to the next one (this is to simulate items being deleted while iterating)
			last_internal_id++;
			// now when the server gets last_internal_id, it won't point to a valid entry
		}
		res = storage_lookup_without_index( &my_storage,
											matchers,
											num_matchers,
											results,
											&results_n,
											max_results_size,
											&last_internal_id,
											limit );
		CU_ASSERT( res == 0 );
		LOG_DEBUG( "results=*s n=d last_internal_id=d got results",
				   results_n,
				   results,
				   results_n,
				   last_internal_id );
		if( results_n == 0 ) {
			break;
		}

		char* p = results;
		while( results_n > 0 ) {
			uint16_t lfm_len = *( (uint16_t*)p );
			p += sizeof( uint16_t );
			results_n -= sizeof( uint16_t );

			CU_ASSERT( lfm_len > 6 );
			CU_ASSERT( memcmp( p, "hello-", 6 ) == 0 );
			p += 6;
			results_n -= 6;
			lfm_len -= 6;
			int d = antoi( p, lfm_len );
			CU_ASSERT( 0 <= d && d <= num_keys_to_send );
			CU_ASSERT( found[d] == false );
			found[d] = true;

			p += lfm_len;
			results_n -= lfm_len;
		}
	}

	for( int i = 0; i < num_keys_to_send; i++ ) {
		CU_ASSERT( found[i] == true );
	}

	//////////////////////////////////////////
	// done with tests, cleanup

	storage_free( &my_storage );

	my_malloc_assert_free();
	my_malloc_free();
}

void test_get_key_value( void )
{
	const char* lfm = "world"
					  "\x00"
					  "user_id"
					  "\x00"
					  "123";
	size_t lfm_len = 5 + 1 + 7 + 1 + 3;
	const char* key = "user_id";
	const char* val = get_key_value( lfm, lfm_len, key );
	CU_ASSERT( strcmp( val, "123" ) == 0 );
}

void test_ROUND_UP( void )
{
	CU_ASSERT( ROUND_UP( 0 ) == 0 );
	CU_ASSERT( ROUND_UP( 1 ) == 8 );
	CU_ASSERT( ROUND_UP( 2 ) == 8 );
	CU_ASSERT( ROUND_UP( 3 ) == 8 );
	CU_ASSERT( ROUND_UP( 4 ) == 8 );
	CU_ASSERT( ROUND_UP( 5 ) == 8 );
	CU_ASSERT( ROUND_UP( 6 ) == 8 );
	CU_ASSERT( ROUND_UP( 7 ) == 8 );
	CU_ASSERT( ROUND_UP( 8 ) == 8 );
	CU_ASSERT( ROUND_UP( 9 ) == 16 );
	CU_ASSERT( ROUND_UP( 10 ) == 16 );
	CU_ASSERT( ROUND_UP( 11 ) == 16 );
	CU_ASSERT( ROUND_UP( 12 ) == 16 );
	CU_ASSERT( ROUND_UP( 13 ) == 16 );
	CU_ASSERT( ROUND_UP( 14 ) == 16 );
	CU_ASSERT( ROUND_UP( 15 ) == 16 );
	CU_ASSERT( ROUND_UP( 16 ) == 16 );
	CU_ASSERT( ROUND_UP( 17 ) == 24 );
	CU_ASSERT( ROUND_UP( 18 ) == 24 );
}

void test_storage_with_writes( void )
{
	int res;

	my_malloc_init();
	const char* storage_path = get_test_storage_path();

	int num_pts = 100;
	int64_t now = my_time( NULL );
	struct the_storage my_storage;
	int retention_seconds = 60 * 60 * 24 * 365;
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

	const char* lfm = "hello"
					  "\x00"
					  "user_id"
					  "\x00"
					  "123";
	const int lfm_len = 5 + 1 + 7 + 1 + 3;

	double y = 0.0;
	int64_t t;
	for( int i = 0; i < num_pts; i++ ) {
		if( i % 2 ) {
			y += (double)fast_rand();
		}
		else {
			y -= (double)fast_rand();
		}
		t = now + i;

		LOG_DEBUG( "here" );
		res = storage_write( &my_storage, lfm, lfm_len, &t, &y, 1 );
		CU_ASSERT( res == 0 );
	}

	// TODO test read

	int64_t* tt = (int64_t*)my_malloc( sizeof( int64_t ) * num_pts );
	double* yy = (double*)my_malloc( sizeof( double ) * num_pts );

	size_t num_pts_read;
	res = storage_get( &my_storage, lfm, lfm_len, tt, yy, num_pts, &num_pts_read );
	CU_ASSERT( num_pts_read == num_pts );

	//////////////////////////////////////////
	// done with tests, cleanup

	storage_free( &my_storage );

	my_free( tt );
	my_free( yy );

	my_malloc_assert_free();
	my_malloc_free();
}
