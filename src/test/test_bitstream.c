#include "tests.h"

#include <assert.h>
#include <my_malloc.h>
#include <stdio.h>
#include <string.h>

#include "bitstream.h"
#include "dump.h"
#include "globals.h"
#include "tsc.h"

void test_bitstream_signed( void )
{
	my_malloc_init();

	const int n = 200;
	const int n_bits = n * 8;
	char buf[n];

	memset( buf, 0, n );

	int64_t value;
	int64_t got;

	size_t bit_offset;
	size_t num_bits = 20;

	for( bit_offset = 0; bit_offset < 70; bit_offset++ ) {
		for( value = -524288; value < 524288; value += 110 ) {
			// value = -123;
			// bit_offset = 45;

			// printf("before : ");
			// dump_mem_bits_strange((char*)buf, 20);

			CU_ASSERT( bitstream_write_int64( buf, n_bits, bit_offset, num_bits, value, NULL ) ==
					   0 );

			// printf("after  : ");
			// dump_mem_bits_strange((char*)buf, 20);

			// printf(" ----------------------- \n" );

			got = 9999;
			CU_ASSERT( bitstream_read_int64( buf, n_bits, bit_offset, num_bits, &got ) == 0 );
			// printf("got %ld %ld %ld\n", bit_offset, value, got);
			CU_ASSERT_EQUAL_FATAL( got, value );

			// return;
		}
	}

	my_malloc_assert_free();
	my_malloc_free();
}

void test_bitstream_signed_bounds_checking( void )
{
	my_malloc_init();
	int expected_res;

	const int n = 200;
	const int n_bits = n * 8;
	char buf[n];

	memset( buf, 0, n );

	int64_t value;

	size_t bit_offset = 0;
	size_t num_bits = 4;

	for( value = -22; value < 22; value++ ) {
		if( -8 <= value && value <= 7 ) {
			// these values can fit in 4 bits
			expected_res = 0; // ok
		}
		else {
			// outside the range, more bits are required
			expected_res = ERR_BITSTREAM_BAD_VALUE;
		}
		CU_ASSERT( bitstream_write_int64( buf, n_bits, bit_offset, num_bits, value, NULL ) ==
				   expected_res );
	}

	// test real large values
	num_bits = 63;

	value = -4611686018427387904;
	CU_ASSERT( bitstream_write_int64( buf, n_bits, bit_offset, num_bits, value, NULL ) == 0 );

	value = -4611686018427387905;
	CU_ASSERT( bitstream_write_int64( buf, n_bits, bit_offset, num_bits, value, NULL ) ==
			   ERR_BITSTREAM_BAD_VALUE );

	value = 4611686018427387904;
	CU_ASSERT( bitstream_write_int64( buf, n_bits, bit_offset, num_bits, value, NULL ) ==
			   ERR_BITSTREAM_BAD_VALUE );

	value = 4611686018427387903;
	CU_ASSERT( bitstream_write_int64( buf, n_bits, bit_offset, num_bits, value, NULL ) == 0 );

	num_bits = 64;

	// test extremes of 64 bit numbers
	value = 1L << 63; // smallest
	CU_ASSERT( bitstream_write_int64( buf, n_bits, bit_offset, num_bits, value, NULL ) == 0 );

	value = ~( 1L << 63 ); // largest
	CU_ASSERT( bitstream_write_int64( buf, n_bits, bit_offset, num_bits, value, NULL ) == 0 );

	my_malloc_assert_free();
	my_malloc_free();
}

void test_bitstream_unsigned( void )
{
	my_malloc_init();

	const int n = 200;
	const int n_bits = n * 8;
	char buf[n];

	memset( buf, 0, n );

	uint64_t value;
	uint64_t got;

	size_t bit_offset;
	size_t num_bits = 20;

	for( bit_offset = 0; bit_offset < 70; bit_offset++ ) {
		for( value = 0; value < 1048543; value += 110 ) {
			// value = 264; bit_offset = 56;
			CU_ASSERT( bitstream_write_uint64( buf, n_bits, bit_offset, num_bits, value, NULL ) ==
					   0 );

			got = 9999;
			CU_ASSERT( bitstream_read_uint64( buf, n_bits, bit_offset, num_bits, &got ) == 0 );
			// printf("got=%ld; want=%ld (bit_offset=%ld)\n", got, value, bit_offset);
			CU_ASSERT_EQUAL_FATAL( got, value );
			// return;
		}
	}

	my_malloc_assert_free();
	my_malloc_free();
}

void test_bitstream_unsigned_bounds_checking( void )
{
	my_malloc_init();

	const int n = 200;
	const int n_bits = n * 8;
	char buf[n];

	memset( buf, 0, n );

	uint64_t value;

	size_t bit_offset = 0;
	size_t num_bits = 3;

	value = 0;
	CU_ASSERT( bitstream_write_uint64( buf, n_bits, bit_offset, num_bits, value, NULL ) == 0 );

	value = 7;
	CU_ASSERT( bitstream_write_uint64( buf, n_bits, bit_offset, num_bits, value, NULL ) == 0 );

	value = 8;
	CU_ASSERT( bitstream_write_uint64( buf, n_bits, bit_offset, num_bits, value, NULL ) ==
			   ERR_BITSTREAM_BAD_VALUE );

	my_malloc_assert_free();
	my_malloc_free();
}
