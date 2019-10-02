#include "tests.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitstream.h"
#include "dump.h"
#include "my_malloc.h"
#include "time_utils.h"
#include "tsc.h"

void test_tsc( void )
{
	my_malloc_init();

	int i;
	int num_pts = 5;
	int64_t t[] = {13423423, 13423483, 13423544, 13423607, 13423666};

	double y[] = {
		3.140000,
		2.140000,
		2.120000,
		2.111200,
		2.091110,
	};
	CU_ASSERT( sizeof( t ) == sizeof( int64_t ) * num_pts );
	CU_ASSERT( sizeof( y ) == sizeof( double ) * num_pts );

	struct tsc_header tsc;
	tsc_init( &tsc, 1024 );

	for( i = 0; i < num_pts; i++ ) {
		CU_ASSERT( tsc_add( &tsc, t[i], y[i] ) == 0 );
	}

	CU_ASSERT( TSC_START_TIME( tsc.data ) == t[0] );
	CU_ASSERT( TSC_NUM_PTS( tsc.data ) == num_pts );

	struct tsc_iter iter;
	tsc_iter_init( &iter );

	int64_t tt;
	double yy;
	i = 0;
	while( tsc_iter_next( tsc.data, &iter ) == 0 ) {

		yy = TSC_ITER_Y( &iter );
		tt = TSC_ITER_T( &iter );

		CU_ASSERT_FATAL( i < num_pts );
		ASSERT_LONG_EQUAL( tt, t[i] );
		ASSERT_DOUBLE_EQUAL( yy, y[i] );
		i++;
	}
	CU_ASSERT( i == num_pts );

	//// test trimming
	// int earliest_point = 2;
	// int64_t num_pts_remaining;
	// CU_ASSERT( tsc_trim( &tsc, t[earliest_point], &num_pts_remaining ) == 0 );
	// CU_ASSERT_EQUAL( num_pts_remaining, 3 );

	// i = earliest_point;
	// tsc_iter_init( &iter );
	// while( tsc_iter_next( &tsc, &iter, &tt, &yy ) == 0 ) {
	//	CU_ASSERT_FATAL( i < num_pts );
	//	ASSERT_LONG_EQUAL( tt, t[i] );
	//	ASSERT_DOUBLE_EQUAL( yy, y[i] );
	//	i++;
	//}
	// CU_ASSERT( i == num_pts );

	tsc_free( &tsc );

	my_malloc_assert_free();
	my_malloc_free();
}

void test_tsc_grow( void )
{
	// my_malloc_init();

	// int i;
	// int num_pts = 10000;
	// int64_t start_time = 13423666;
	// int64_t t;
	// double y;

	// struct tsc_header tsc;
	// tsc_init( &tsc, 128 );

	// for( i = 0; i < num_pts; i++ ) {
	//	t = start_time + i;
	//	y = (double)start_time / t;
	//	CU_ASSERT( tsc_add( &tsc, t, y ) == 0 );
	//}

	// struct tsc_iter iter;
	// tsc_iter_init( &iter );

	// int64_t tt;
	// double yy;
	// i = 0;
	// while( tsc_iter_next( &tsc, &iter, &tt, &yy ) == 0 ) {
	//	CU_ASSERT_FATAL( i < num_pts );
	//	// expected values
	//	t = start_time + i;
	//	y = (double)start_time / t;
	//	ASSERT_LONG_EQUAL( tt, t );
	//	ASSERT_DOUBLE_EQUAL( yy, y );
	//	i++;
	//}
	// CU_ASSERT( i == num_pts );

	// tsc_free( &tsc );

	// my_malloc_assert_free();
	// my_malloc_free();
}

void test_tsc_write_read( void )
{
	my_malloc_init();

	char buf[40];
	size_t bit_index = 0;
	memset( buf, 0xFF, sizeof( buf ) );

	CU_ASSERT( tsc_write_int64( buf, 320, bit_index, 0, &bit_index ) == 0 );
	CU_ASSERT( bit_index == 1 );

	CU_ASSERT( tsc_write_int64( buf, 320, bit_index, 0, &bit_index ) == 0 );
	CU_ASSERT( bit_index == 2 );

	CU_ASSERT( tsc_write_int64( buf, 320, bit_index, 3, &bit_index ) == 0 );
	CU_ASSERT( bit_index == 11 );

	CU_ASSERT( tsc_write_int64( buf, 320, bit_index, -4, &bit_index ) == 0 );
	CU_ASSERT( bit_index == 20 );

	CU_ASSERT( tsc_write_int64( buf, 320, bit_index, -512, &bit_index ) == 0 );
	CU_ASSERT( bit_index == 36 );

	CU_ASSERT( tsc_write_int64( buf, 320, bit_index, 511, &bit_index ) == 0 );
	CU_ASSERT( bit_index == 52 );

	CU_ASSERT( tsc_write_int64( buf, 320, bit_index, -800, &bit_index ) == 0 );
	CU_ASSERT( bit_index == 68 );

	CU_ASSERT( tsc_write_int64( buf, 320, bit_index, 842341, &bit_index ) == 0 );
	CU_ASSERT( bit_index == 104 );

	int64_t val;
	size_t r = 0, num_bits_read;

	CU_ASSERT( tsc_read_int64( buf, 320, r, &val, &num_bits_read ) == 0 );
	CU_ASSERT( val == 0 );
	CU_ASSERT( num_bits_read == 1 );
	r += num_bits_read;

	CU_ASSERT( tsc_read_int64( buf, 320, r, &val, &num_bits_read ) == 0 );
	CU_ASSERT( val == 0 );
	CU_ASSERT( num_bits_read == 1 );
	r += num_bits_read;

	CU_ASSERT( tsc_read_int64( buf, 320, r, &val, &num_bits_read ) == 0 );
	CU_ASSERT( val == 3 );
	CU_ASSERT( num_bits_read == 9 );
	r += num_bits_read;

	CU_ASSERT( tsc_read_int64( buf, 320, r, &val, &num_bits_read ) == 0 );
	CU_ASSERT( val == -4 );
	CU_ASSERT( num_bits_read == 9 );
	r += num_bits_read;

	CU_ASSERT( tsc_read_int64( buf, 320, r, &val, &num_bits_read ) == 0 );
	CU_ASSERT( val == -512 );
	CU_ASSERT( num_bits_read == 16 );
	r += num_bits_read;

	CU_ASSERT( tsc_read_int64( buf, 320, r, &val, &num_bits_read ) == 0 );
	CU_ASSERT( val == 511 );
	CU_ASSERT( num_bits_read == 16 );
	r += num_bits_read;

	CU_ASSERT( tsc_read_int64( buf, 320, r, &val, &num_bits_read ) == 0 );
	CU_ASSERT( val == -800 );
	CU_ASSERT( num_bits_read == 16 );
	r += num_bits_read;

	CU_ASSERT( tsc_read_int64( buf, 320, r, &val, &num_bits_read ) == 0 );
	CU_ASSERT( val == 842341 );
	CU_ASSERT( num_bits_read == 36 );
	r += num_bits_read;

	my_malloc_assert_free();
	my_malloc_free();
}

void test_tsc_trim( void )
{
	// my_malloc_init();

	// int i, j;
	// int num_pts = 1024;
	// int64_t num_pts_remaining;

	// int64_t tt;
	// double yy;

	// int64_t* t = malloc( sizeof( int64_t ) * num_pts );
	// double* y = malloc( sizeof( double ) * num_pts );

	// t[0] = 13423666;
	// for( i = 1; i < num_pts; i++ ) {
	//	t[i] = t[i - 1] + ( i % 60 ) + 1;
	//	y[i] = (double)t[i] / 19.3;
	//}

	// struct tsc_header tsc;
	// tsc_init( &tsc, 1024 );

	// for( i = 0; i < num_pts; i++ ) {
	//	CU_ASSERT( tsc_add( &tsc, t[i], y[i] ) == 0 );
	//}

	//// one by one; trim the points and test the remaining points
	// for( i = 0; i < num_pts; i++ ) {
	//	int64_t earliest_time = t[i];
	//	CU_ASSERT( tsc_trim( &tsc, earliest_time, &num_pts_remaining ) == 0 );
	//	CU_ASSERT_EQUAL( num_pts_remaining, num_pts - i );

	//	struct tsc_iter iter;
	//	tsc_iter_init( &iter );

	//	j = i;
	//	while( tsc_iter_next( &tsc, &iter, &tt, &yy ) == 0 ) {
	//		CU_ASSERT_FATAL( j < num_pts );
	//		ASSERT_LONG_EQUAL( tt, t[j] );
	//		ASSERT_DOUBLE_EQUAL( yy, y[j] );
	//		if( tt != t[j] || yy != y[j] ) {
	//			printf( "pt %d (iter %d)\n", j, i );
	//			printf( "got=%ld want=%ld diff=%ld\n", tt, t[j], tt - t[j] );
	//			printf( "got=%lf want=%lf diff=%lf\n", yy, y[j], yy - y[j] );
	//			printf( "UGH\n" );
	//			return;
	//		}
	//		j++;
	//	}
	//	CU_ASSERT( j == num_pts );
	//}

	// tsc_free( &tsc );

	// my_malloc_assert_free();
	// my_malloc_free();
}

void test_tsc_trim_and_write( void )
{
	// my_malloc_init();

	// int i, j;
	// int num_pts = 1024;
	// int64_t num_pts_remaining;

	// int64_t tt;
	// double yy;

	// int64_t* t = malloc( sizeof( int64_t ) * num_pts );
	// double* y = malloc( sizeof( double ) * num_pts );

	// t[0] = 13423666;
	// for( i = 1; i < num_pts; i++ ) {
	//	t[i] = t[i - 1] + ( i % 60 ) + 1;
	//	y[i] = (double)t[i] / 19.3;
	//}

	// struct tsc_header tsc;
	// tsc_init( &tsc, 1024 );

	//// insert half the points
	// int first_batch = num_pts / 2;
	// for( i = 0; i < first_batch; i++ ) {
	//	CU_ASSERT( tsc_add( &tsc, t[i], y[i] ) == 0 );
	//}

	// int earliest_time_i = num_pts / 4;
	// int64_t earliest_time = t[earliest_time_i];
	// CU_ASSERT( tsc_trim( &tsc, earliest_time, &num_pts_remaining ) == 0 );
	// CU_ASSERT_EQUAL( num_pts_remaining, first_batch - earliest_time_i );

	// struct tsc_iter iter;

	// tsc_iter_init( &iter );
	// j = earliest_time_i;
	// while( tsc_iter_next( &tsc, &iter, &tt, &yy ) == 0 ) {
	//	CU_ASSERT_FATAL( j < first_batch );
	//	ASSERT_LONG_EQUAL( tt, t[j] );
	//	ASSERT_DOUBLE_EQUAL( yy, y[j] );
	//	if( tt != t[j] || yy != y[j] ) {
	//		return; // bail earlier; otherwise performance goes down as all the errors are appended
	//	}
	//	j++;
	//}
	// CU_ASSERT( j == first_batch );

	//// insert the second half of the points
	// for( i = first_batch; i < num_pts; i++ ) {
	//	CU_ASSERT( tsc_add( &tsc, t[i], y[i] ) == 0 );
	//}

	//// test all points (expect trimmed) are found
	// tsc_iter_init( &iter );
	// j = earliest_time_i;
	// while( tsc_iter_next( &tsc, &iter, &tt, &yy ) == 0 ) {
	//	CU_ASSERT_FATAL( j < num_pts );
	//	ASSERT_LONG_EQUAL( tt, t[j] );
	//	ASSERT_DOUBLE_EQUAL( yy, y[j] );
	//	if( tt != t[j] || yy != y[j] ) {
	//		return; // bail earlier; otherwise performance goes down as all the errors are appended
	//	}
	//	j++;
	//}
	// CU_ASSERT( j == num_pts );

	// tsc_free( &tsc );

	// my_malloc_assert_free();
	// my_malloc_free();
}
