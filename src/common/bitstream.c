#include "bitstream.h"

#include "dump.h"
#include "globals.h"
#include "log.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static inline uint64_t bit_mask( int x )
{
	assert( x >= 0 );
	assert( x <= 64 );
	if( x == 64 )
		return (uint64_t)-1;
	return ( ( (uint64_t)1 ) << x ) - 1;
}

int bitstream_read_uint64(
	const char* buf, size_t buf_size_bits, size_t bit_index, size_t num_bits, uint64_t* value )
{
	if( ( bit_index + num_bits ) > buf_size_bits )
		return ERR_BITSTREAM_OUT_OF_SPACE;

	size_t i = ( bit_index ) / 64;
	size_t j = bit_index % 64;

	uint64_t* p = (uint64_t*)buf;
	p += i;

	uint64_t vmask = bit_mask( num_bits );

	if( j == 0 ) {
		*value = ( *p & vmask );
		return 0;
	}

	uint64_t low;
	uint64_t high = 0;
	uint64_t low_mask;
	uint64_t high_mask;

	low_mask = vmask << j;
	high_mask = ( vmask >> ( 64 - j ) );

	assert( low_mask );
	low = *p & low_mask;
	p++;
	if( high_mask ) {
		high = *p & high_mask;
	}

	*value = ( low >> j ) | ( high << ( 64 - j ) );

	return 0;
}

int bitstream_write_uint64( char* buf,
							size_t buf_size_bits,
							size_t bit_index,
							size_t num_bits,
							uint64_t value,
							size_t* num_bits_written )
{
	size_t required_buf_size = ( bit_index + num_bits );
	size_t extra_bits = required_buf_size % 64;
	if( extra_bits ) {
		required_buf_size += 64 - extra_bits;
	}
	if( required_buf_size > buf_size_bits ) {
		LOG_ERROR( "buf too small" );
		return ERR_BITSTREAM_OUT_OF_SPACE;
	}

	uint64_t vmask = bit_mask( num_bits );
	if( value & ~vmask ) {
		LOG_ERROR( "value too big" );
		return ERR_BITSTREAM_BAD_VALUE; // value won't fit
	}

	size_t i = ( bit_index ) / 64;
	size_t j = bit_index % 64;

	uint64_t* p = (uint64_t*)buf;
	p += i;

	if( j == 0 ) {
		*p = ( *p & ~vmask ) | ( value & vmask );
		if( num_bits_written ) {
			*num_bits_written = bit_index + num_bits;
		}
		return 0;
	}

	// assert( num_bits == 64 );

	uint64_t low;
	uint64_t high;
	uint64_t low_mask;
	uint64_t high_mask;

	low = value << j;
	low_mask = vmask << j;
	high = value >> ( 64 - j );
	high_mask = ( vmask >> ( 64 - j ) );

	// printf("vmask  : ");
	// dump_mem_bits_strange( (char*)&vmask, sizeof(uint64_t));
	// printf("vmask l: ");
	// dump_mem_bits_strange( (char*)&low_mask, sizeof(uint64_t));
	// printf("vmask h: ");
	// dump_mem_bits_strange( (char*)&high_mask, sizeof(uint64_t));

	// printf("value  : ");
	// dump_mem_bits_strange( (char*)&value, sizeof(uint64_t));
	// printf("low    : ");
	// dump_mem_bits_strange( (char*)&low, sizeof(uint64_t));
	// printf("high   : ");
	// dump_mem_bits_strange( (char*)&high, sizeof(uint64_t));

	assert( low_mask );
	*p = ( *p & ~low_mask ) | ( low & low_mask );
	p++;
	if( high_mask ) {
		*p = ( *p & ~high_mask ) | ( high & high_mask );
	}

	if( num_bits_written ) {
		*num_bits_written = bit_index + num_bits;
	}
	return 0;
}

int bitstream_write_int64( char* buf,
						   size_t buf_size_bits,
						   size_t bit_index,
						   size_t num_bits,
						   int64_t value,
						   size_t* num_bits_written )
{
	// printf("in bitstream_write_int64 with %ld\n", value);
	size_t required_buf_size = ( bit_index + num_bits );
	size_t extra_bits = required_buf_size % 64;
	if( extra_bits ) {
		required_buf_size += 64 - extra_bits;
	}
	// printf("need %ld avail %ld\n", required_buf_size, buf_size_bits);
	if( required_buf_size > buf_size_bits ) {
		return ERR_BITSTREAM_OUT_OF_SPACE;
	}

	assert( num_bits > 1 );

	uint64_t vmask = bit_mask( num_bits );

	if( num_bits < 64 ) {
		if( value < 0 ) {
			uint64_t min_val = 1L << ( num_bits - 1 );
			if( value < -min_val ) {
				LOG_ERROR( "value too small" );
				return ERR_BITSTREAM_BAD_VALUE; // value won't fit
			}
		}
		else {
			uint64_t vmask_without_signed_bit = bit_mask( num_bits - 1 );
			if( value & ~vmask_without_signed_bit ) {
				LOG_ERROR( "value too big" );
				return ERR_BITSTREAM_BAD_VALUE; // value won't fit
			}
		}
	}

	size_t i = ( bit_index ) / 64;
	size_t j = bit_index % 64;

	uint64_t* p = (uint64_t*)buf;
	p += i;

	if( j == 0 ) {
		*p = ( *p & ~vmask ) | ( value & vmask );
		if( num_bits_written ) {
			*num_bits_written = bit_index + num_bits;
		}
		return 0;
	}

	// assert( num_bits == 64 );

	uint64_t low;
	uint64_t high;
	uint64_t low_mask;
	uint64_t high_mask;

	low = value << j;
	low_mask = vmask << j;
	high = value >> ( 64 - j );
	high_mask = ( vmask >> ( 64 - j ) );

	// printf("vmask  : ");
	// dump_mem_bits_strange( (char*)&vmask, sizeof(uint64_t));
	// printf("vmask l: ");
	// dump_mem_bits_strange( (char*)&low_mask, sizeof(uint64_t));
	// printf("vmask h: ");
	// dump_mem_bits_strange( (char*)&high_mask, sizeof(uint64_t));

	// printf("value  : ");
	// dump_mem_bits_strange( (char*)&value, sizeof(uint64_t));
	// printf("low    : ");
	// dump_mem_bits_strange( (char*)&low, sizeof(uint64_t));
	// printf("high   : ");
	// dump_mem_bits_strange( (char*)&high, sizeof(uint64_t));

	assert( low_mask );
	*p = ( *p & ~low_mask ) | ( low & low_mask );
	p++;
	if( high_mask ) {
		*p = ( *p & ~high_mask ) | ( high & high_mask );
	}

	if( num_bits_written ) {
		*num_bits_written = bit_index + num_bits;
	}
	return 0;
}

int bitstream_read_int64(
	const char* buf, size_t buf_size_bits, size_t bit_index, size_t num_bits, int64_t* value )
{
	if( ( bit_index + num_bits ) > buf_size_bits )
		return ERR_BITSTREAM_OUT_OF_SPACE;

	size_t i = ( bit_index ) / 64;
	size_t j = bit_index % 64;

	uint64_t* p = (uint64_t*)buf;
	p += i;
	// printf("reading at byte %ld; extra offset %ld\n", i, j);

	uint64_t vmask = bit_mask( num_bits );
	int64_t msb_mask = 1 << ( num_bits - 1 );

	if( j == 0 ) {
		// printf("special case j = 0\n");
		*value = ( *p & vmask );
		if( *value & msb_mask ) {
			// value is negative; need to set most significant bits to 1
			*value = *value | ~vmask;
		}
		return 0;
	}

	uint64_t low;
	uint64_t high = 0;
	uint64_t low_mask;
	uint64_t high_mask;

	low_mask = vmask << j;
	high_mask = ( vmask >> ( 64 - j ) );

	// printf("vmask  : ");
	// dump_mem_bits_strange( (char*)&vmask, sizeof(uint64_t));
	// printf("vmask l: ");
	// dump_mem_bits_strange( (char*)&low_mask, sizeof(uint64_t));
	// printf("vmask h: ");
	// dump_mem_bits_strange( (char*)&high_mask, sizeof(uint64_t));

	assert( low_mask );
	low = *p & low_mask;
	p++;
	if( high_mask ) {
		high = *p & high_mask;
	}

	// dump_mem_bits_strange((char*)&low, sizeof(uint64_t));
	// dump_mem_bits_strange((char*)&high, sizeof(uint64_t));

	*value = ( low >> j ) | ( high << ( 64 - j ) );

	// dump_mem_bits_strange((char*)&value, sizeof(uint64_t));

	if( num_bits < 64 ) {
		if( *value & msb_mask ) {
			// printf("special case flip\n");
			// value is negative; need to set most significant bits to 1
			*value = *value | ~vmask;
		}
	}

	return 0;
}
