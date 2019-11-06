#include "tsc.h"

#include "bitstream.h"
#include "globals.h"
#include "log.h"
#include "metrics.h"
#include "my_malloc.h"
#include "my_utils.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

// max uint32 value / 8
#define MAX_NUM_VALID_BITS 536870912

// n is initial buffer size in bytes
int tsc_init( struct tsc_header* p, size_t n )
{
	assert( p );
	if( n * 8 > MAX_NUM_VALID_BITS ) {
		return ERR_TSC_UNKOWN; // this would overflow num_valid_bits;
	}
	memset( p, 0, sizeof( struct tsc_header ) );

	assert( 0 < n &&
			n < 0xFFFFFFFF ); // TODO this is due to the limit of using a uint32 for the size
	LOG_TRACE( "n=d new tsc", n );
	p->data = my_malloc(
		n ); // TODO now that we are syncing to disk, this no longer grows and can ultimately be packed into the *item malloc'ed memory (less fragmentation/wasted padding)
	p->data_len = n;
	TSC_NUM_PTS( p->data ) = 0;

	return ERR_OK;
}

void tsc_clear( struct tsc_header* p )
{
	p->last_timestamp = 0;
	p->last_timestamp_delta = 0;
	p->prev_value = 0.0;
	p->num_valid_bits = 0;

	p->last_leading = 0;
	p->last_sigbits = 0;

	TSC_NUM_PTS( p->data ) = 0;
}

bool tsc_is_clear( const struct tsc_header* p )
{
	// really I can just do this:
	//
	//     return p->num_valid_bits == 0;
	//
	// but for the time being, I want to be absolutely sure everything else is also clear

	if( p->num_valid_bits ) {
		return false;
	}
	assert( p->last_timestamp == 0 );
	assert( p->last_timestamp_delta == 0 );
	assert( TSC_NUM_PTS( p->data ) == 0 );
	return true;
}

void tsc_serialize_header( struct tsc_header* p, char* s )
{
#ifdef DEBUG_BUILD
	// dumb test to ensure these are equivalent
	assert( &p->last_timestamp == &( p->last_timestamp ) );
#endif // DEBUG_BUILD

	memcpy( s, &p->last_timestamp, sizeof( int64_t ) );
	s += sizeof( int64_t );

	memcpy( s, &p->last_timestamp_delta, sizeof( int64_t ) );
	s += sizeof( int64_t );

	memcpy( s, &p->prev_value, sizeof( double ) );
	s += sizeof( double );

	memcpy( s, &p->num_valid_bits, sizeof( uint32_t ) );
	s += sizeof( uint32_t );

	memcpy( s, &p->last_leading, sizeof( uint8_t ) );
	s += sizeof( uint8_t );

	memcpy( s, &p->last_sigbits, sizeof( uint8_t ) );
	s += sizeof( uint8_t );
}

// s is the serialized_tsc_header
// buf is the data bitstream
void tsc_deserialize_header( struct tsc_header* p, const char* s )
{
	assert( p );
	assert( s );

	// unpack header from the serialized tsc_header (stored in s)
	memcpy( &p->last_timestamp, s, sizeof( int64_t ) );
	s += sizeof( int64_t );

	memcpy( &p->last_timestamp_delta, s, sizeof( int64_t ) );
	s += sizeof( int64_t );

	memcpy( &p->prev_value, s, sizeof( double ) );
	s += sizeof( double );

	memcpy( &p->num_valid_bits, s, sizeof( uint32_t ) );
	s += sizeof( uint32_t );

	memcpy( &p->last_leading, s, sizeof( uint8_t ) );
	s += sizeof( uint8_t );

	memcpy( &p->last_sigbits, s, sizeof( uint8_t ) );
	//s += sizeof( uint8_t );
}
//
// s is the serialized_tsc_header
// buf is the data bitstream
void tsc_load_block( struct tsc_header* p, const char* s, const char* buf, size_t n )
{
	tsc_clear( p );
	assert( s );

	tsc_deserialize_header( p, s );

	// TODO FIXME we assume that the data_len will be fixed
	// but in reality, we will want this to be tunable, so ideally we need to
	// convert the data size if the value were to ever change.
	assert( n >= p->data_len );
	memcpy( p->data, buf, p->data_len );
}

int tsc_free( struct tsc_header* p )
{
	assert( p );
	if( p->data ) {
		my_free( p->data );
		p->data = NULL;
	}
	return ERR_OK;
}

// int tsc_ensure_room( struct tsc_header* p, size_t num_bytes )
//{
//	assert( p->data_len > 0 );
//
//	ssize_t avail_bytes = ( (ssize_t)p->data_len ) - ( (ssize_t)p->num_valid_bits / 8 + 1 );
//	if( avail_bytes > num_bytes ) {
//		return 0;
//	}
//
//	size_t n = p->data_len;
//	if( n < 1024 ) {
//		n *= 2;
//	}
//	else {
//		n += 1024;
//	}
//	LOG_TRACE( "from=d to=d resizing", p->data_len, n );
//	INCR_METRIC( num_resizes );
//
//	assert( 0 < n &&
//			n < 0xFFFFFFFF ); // TODO this is due to the limit of using a uint32 for the size
//
//	char* tmp = my_malloc( n );
//	assert( tmp );
//	memcpy( tmp, p->data, p->data_len );
//	my_free( p->data );
//	p->data = tmp;
//	p->data_len = n;
//
//	// TODO hacky case where doubling the buffer wasn't enough
//	return tsc_ensure_room( p, num_bytes );
//}

// will return ERR_TSC_OUT_OF_SPACE once the compressed block is full and needs to be written to
// disk
int tsc_add( struct tsc_header* p, int64_t t, double y )
{
#ifdef DEBUG_BUILD
	assert( p );
	assert( p->data_len > sizeof( uint32_t ) );
#endif // DEBUG_BUILD

	// printf("adding %ld %lf\n", t, y);
	// printf("curr points: %d\n", TSC_NUM_PTS( p->data ));

	int res;
	uint64_t y_xor;
	int64_t t_delta;

	char* buf = p->data + sizeof( uint32_t ); // first uint32_t is for number of valid bits
	size_t n = ( p->data_len - sizeof( uint32_t ) ) * 8;

	size_t num_valid_bits = p->num_valid_bits;
	size_t avail_bits = n - num_valid_bits;

	// worse case is 67 bits (9bytes) per 64bit value; multiply by two since we store two values
	// previously we did: tsc_ensure_room( p, 18 ); but now we're checking bits and not bytes
	if( avail_bits < 18 * 8 ) {
		return ERR_TSC_OUT_OF_SPACE; // signal we should write to disk.
	}

	// printf("writing point at %ld\n", num_valid_bits);

	int64_t y_as_uint64 = 0;
	memcpy( &y_as_uint64, &y, sizeof( double ) );

	int64_t prev_value_as_uint64 = 0;
	memcpy( &prev_value_as_uint64, &( p->prev_value ), sizeof( double ) );

	if( TSC_NUM_PTS( p->data ) == 0 ) {
		// save start time
		ENSURE_RES_OK( bitstream_write_int64(
			buf, n, num_valid_bits, 64, *(int64_t*)( &t ), &num_valid_bits ) );
		// save y value
		ENSURE_RES_OK( tsc_write_double( buf,
										 n,
										 num_valid_bits,
										 y_as_uint64,
										 &num_valid_bits,
										 &( p->last_leading ),
										 &( p->last_sigbits ) ) );

		assert( num_valid_bits <= MAX_NUM_VALID_BITS );
		p->num_valid_bits = num_valid_bits;
		p->last_timestamp = t;
		p->prev_value = y;
		TSC_NUM_PTS( p->data ) = 1;
		return ERR_OK;
	};

	if( t <= p->last_timestamp ) {
		INCR_METRIC( num_out_of_order );
		LOG_TRACE( "t=d last=d out of order timestamp", t, p->last_timestamp );
		res = ERR_OUT_OF_ORDER_TIMESTAMP;
		goto error;
	}

	t_delta = t - p->last_timestamp;
	y_xor = prev_value_as_uint64 ^ y_as_uint64;

	if( TSC_NUM_PTS( p->data ) == 1 ) {
		// write the delta of times
		ENSURE_RES_OK( tsc_write_int64( buf, n, num_valid_bits, t_delta, &num_valid_bits ) );
		// printf("write double second time: y=%lf prev=%lf xor=%ld\n", y, p->prev_value, y_xor);
		ENSURE_RES_OK( tsc_write_double( buf,
										 n,
										 num_valid_bits,
										 y_xor,
										 &num_valid_bits,
										 &( p->last_leading ),
										 &( p->last_sigbits ) ) );

		assert( num_valid_bits <= MAX_NUM_VALID_BITS );
		p->num_valid_bits = num_valid_bits;
		p->last_timestamp = t;
		p->last_timestamp_delta = t_delta;
		p->prev_value = y;
		TSC_NUM_PTS( p->data ) = 2;
		return ERR_OK;
	}

	int64_t t_delta_delta = t_delta - p->last_timestamp_delta;

	ENSURE_RES_OK( tsc_write_int64( buf, n, num_valid_bits, t_delta_delta, &num_valid_bits ) );
	// printf("write double: y=%lf prev=%lf xor=%ld\n", y, p->prev_value, y_xor);
	ENSURE_RES_OK( tsc_write_double( buf,
									 n,
									 num_valid_bits,
									 y_xor,
									 &num_valid_bits,
									 &( p->last_leading ),
									 &( p->last_sigbits ) ) );

	assert( num_valid_bits <= MAX_NUM_VALID_BITS );
	p->num_valid_bits = num_valid_bits;
	p->last_timestamp = t;
	p->last_timestamp_delta = t_delta;
	p->prev_value = y;
	TSC_NUM_PTS( p->data )++;

done:
	res = ERR_OK;
error:
	return res;
}

int tsc_trim( struct tsc_header* p, int64_t start_time, int64_t* num_pts_remaining )
{
	assert( 0 );
	return 0;
	//	int res = ERR_OK;
	//	struct tsc_iter iter;
	//	struct tsc_iter last_iter;
	//
	//	tsc_iter_init( &iter );
	//
	//	size_t num_bytes_to_discard;
	//	int64_t t;
	//	double y;
	//
	//	int num_trimmed = 0;
	//	last_iter = iter;
	//	while( tsc_iter_next( p, &iter, &t, &y ) == 0 ) {
	//		if( t < start_time ) {
	//			num_trimmed++;
	//		}
	//		else {
	//			break;
	//		}
	//		last_iter = iter;
	//	}
	//
	//	if( num_trimmed ) {
	//		p->use_trimmed = true;
	//
	//		p->trimmed_y = last_iter.y;
	//		p->trimmed_t = last_iter.t;
	//		p->trimmed_r = last_iter.r;
	//		p->trimmed_dt = last_iter.dt;
	//
	//		p->trimmed_last_leading = last_iter.last_leading;
	//		p->trimmed_last_sigbits = last_iter.last_sigbits;
	//		p->num_pts -= num_trimmed;
	//
	//		num_bytes_to_discard = last_iter.r / 8;
	//		if( num_bytes_to_discard > 128 ) {
	//			memmove( p->data, p->data + num_bytes_to_discard, p->data_len - num_bytes_to_discard
	//); 			p->num_valid_bits -= num_bytes_to_discard * 8; 			p->trimmed_r -=
	// num_bytes_to_discard * 8;
	//		}
	//	}
	//
	//	*num_pts_remaining = p->num_pts;
	//
	// error:
	//	return res;
}

void tsc_iter_init( struct tsc_iter* iter )
{
	assert( iter );
	memset( iter, 0, sizeof( struct tsc_iter ) );
}

int tsc_iter_next( const char* data, struct tsc_iter* iter )
{
	assert( data );
	assert( iter );
	int res;
	int64_t t;
	uint64_t y;
	size_t num_bits_read;

	const char* buf = data + sizeof( uint32_t );
	size_t num_valid_bits = 9999999999999; // TODO remove this

	if( iter->i >= TSC_NUM_PTS( data ) ) {
		return ERR_TSC_ITER_DONE;
	}

	if( iter->i == 0 ) {
		ENSURE_RES_OK( bitstream_read_int64( buf, num_valid_bits, iter->r, 64, &t ) );
		iter->r += 64;

		ENSURE_RES_OK( tsc_read_double( buf,
										num_valid_bits,
										iter->r,
										(uint64_t*)&iter->y,
										&num_bits_read,
										&( iter->last_leading ),
										&( iter->last_sigbits ) ) );
		iter->r += num_bits_read;

		iter->dt = 0;
		iter->t = t;
	}
	else {
		ENSURE_RES_OK( tsc_read_int64( buf, num_valid_bits, iter->r, &t, &num_bits_read ) );
		iter->r += num_bits_read;

		ENSURE_RES_OK( tsc_read_double( buf,
										num_valid_bits,
										iter->r,
										&y,
										&num_bits_read,
										&( iter->last_leading ),
										&( iter->last_sigbits ) ) );
		iter->r += num_bits_read;

		iter->dt += t;
		iter->t += iter->dt;
		iter->y = iter->y ^ y;
	}

	iter->i++;

	res = ERR_OK;
error:
	if( res ) {
		printf( "BAD!!!\n" );
	}
	return res;
}

int tsc_write_int64( char* buf, size_t n, size_t bit_index, int64_t x, size_t* new_bit_index )
{
	assert( buf );
	assert( new_bit_index );
	int res = ERR_OK;
	if( x == 0 ) {
		// b0
		ENSURE_RES_OK( bitstream_write_uint64( buf, n, bit_index, 1, 0, &bit_index ) );
	}
	else if( -64 <= x && x <= 63 ) {
		// b10
		ENSURE_RES_OK( bitstream_write_uint64(
			buf, n, bit_index, 2, 1, &bit_index ) ); // b10 becomes b01 when bits reversed
		ENSURE_RES_OK( bitstream_write_int64( buf, n, bit_index, 7, x, &bit_index ) );
	}
	else if( -256 <= x && x <= 255 ) {
		// b110
		ENSURE_RES_OK( bitstream_write_uint64(
			buf, n, bit_index, 3, 3, &bit_index ) ); // b110 becomes b011 when bits reversed
		ENSURE_RES_OK( bitstream_write_int64( buf, n, bit_index, 9, x, &bit_index ) );
	}
	else if( -2048 <= x && x <= 2047 ) {
		// b1110
		ENSURE_RES_OK( bitstream_write_uint64(
			buf, n, bit_index, 4, 7, &bit_index ) ); // b1110 becomes b0111 when bits reversed
		ENSURE_RES_OK( bitstream_write_int64( buf, n, bit_index, 12, x, &bit_index ) );
	}
	else {
		// b1111
		ENSURE_RES_OK( bitstream_write_uint64(
			buf, n, bit_index, 4, 15, &bit_index ) ); // b1111 stays the same when bits reversed
		ENSURE_RES_OK( bitstream_write_int64(
			buf, n, bit_index, 32, x, &bit_index ) ); // TODO int64 support (but gotta compare with
		// int32 which is what go uses)
	}
done:
error:
	*new_bit_index = bit_index;
	return res;
}

int tsc_read_int64(
	const char* buf, size_t n, size_t bit_index, int64_t* value, size_t* num_bits_read )
{
	assert( buf );
	assert( value );
	assert( num_bits_read );
	int res = ERR_OK;
	uint64_t x;
	ENSURE_RES_OK( bitstream_read_uint64( buf, n, bit_index, 1, &x ) );
	bit_index++;
	if( x == 0 ) {
		// b0
		*value = 0;
		*num_bits_read = 1 + 0;
		return ERR_OK;
	}

	ENSURE_RES_OK( bitstream_read_uint64( buf, n, bit_index, 1, &x ) );
	bit_index++;
	if( x == 0 ) {
		// b10
		ENSURE_RES_OK( bitstream_read_int64( buf, n, bit_index, 7, value ) );
		*num_bits_read = 2 + 7;
		return ERR_OK;
	}

	ENSURE_RES_OK( bitstream_read_uint64( buf, n, bit_index, 1, &x ) );
	bit_index++;
	if( x == 0 ) {
		// b110
		ENSURE_RES_OK( bitstream_read_int64( buf, n, bit_index, 9, value ) );
		*num_bits_read = 3 + 9;
		return ERR_OK;
	}

	ENSURE_RES_OK( bitstream_read_uint64( buf, n, bit_index, 1, &x ) );
	bit_index++;
	if( x == 0 ) {
		// b1110
		ENSURE_RES_OK( bitstream_read_int64( buf, n, bit_index, 12, value ) );
		*num_bits_read = 4 + 12;
		return ERR_OK;
	}

	// b1111
	ENSURE_RES_OK( bitstream_read_int64( buf, n, bit_index, 32, value ) );
	*num_bits_read = 4 + 32;
	return ERR_OK;

error:
	return res;
}

int tsc_write_double( char* buf,
					  size_t n,
					  size_t bit_index,
					  uint64_t x,
					  size_t* new_bit_index,
					  uint8_t* last_leading,
					  uint8_t* last_sigbits )
{
	assert( buf );
	assert( new_bit_index );
	int res = ERR_OK;
	if( x == 0 ) {
		// write b0
		ENSURE_RES_OK( bitstream_write_uint64( buf, n, bit_index, 1, 0, &bit_index ) );
		// printf("write b0 val=0\n");
	}
	else {
		// write b1
		ENSURE_RES_OK( bitstream_write_uint64( buf, n, bit_index, 1, 1, &bit_index ) );

		int leading_zeros = __builtin_clzll( x );
		int trailing_zeros = __builtin_ctzll( x );

		// clamp leading zeros, since we only user 5 bits to store this value
		if( leading_zeros >= 32 ) {
			leading_zeros = 31;
		}

		int sig_bits = 64 - leading_zeros - trailing_zeros;
		int last_trailing = 64 - *last_leading - *last_sigbits;
		// printf("val %ld has %d leading zeros; %d trailing zeros; %d sigbits\n", x, leading_zeros,
		// trailing_zeros, sig_bits);

		int bits_if_reset = 5 + 6 + sig_bits;
		if( leading_zeros >= *last_leading && trailing_zeros >= last_trailing &&
			bits_if_reset >= *last_sigbits ) {
			// write b0
			ENSURE_RES_OK( bitstream_write_uint64( buf, n, bit_index, 1, 0, &bit_index ) );
			ENSURE_RES_OK( bitstream_write_uint64(
				buf, n, bit_index, *last_sigbits, x >> last_trailing, &bit_index ) );
			// printf("==write b10; val=%d (cached leading=%d sigbits=%d); shifted by %d\n",
			// x>>last_trailing, *last_leading, last_sigbits, last_trailing); printf("write b00;
			// val=%ld bits=%d\n", x>>(*last_trailing), last_sigbits);
		}
		else {
			// write b1
			ENSURE_RES_OK( bitstream_write_uint64( buf, n, bit_index, 1, 1, &bit_index ) );

			// printf("writing at %d\n", bit_index);
			assert( 0 <= leading_zeros && leading_zeros <= 31 );
			ENSURE_RES_OK(
				bitstream_write_uint64( buf, n, bit_index, 5, leading_zeros, &bit_index ) );

			// this case should never happen; because we handle the case where the value is 0
			// separately
			assert( 0 < sig_bits );

			// sig_bits will be 64 when leading == trailing == 0; this will require 7 bits, but we
			// only have 6 however, the case where sig_bits == 0 will never happen (because it's
			// covered above where we write b0 so we will encode 64 bits as 0 for this special case
			// (and will have to detect it while reading)
			assert( sig_bits <= 64 );
			if( sig_bits == 64 ) {
				ENSURE_RES_OK( bitstream_write_uint64(
					buf, n, bit_index, 6, 0, &bit_index ) ); // special case 0 represents 64 bits
				assert( leading_zeros == 0 );
				assert( trailing_zeros == 0 );
			}
			else {
				ENSURE_RES_OK(
					bitstream_write_uint64( buf, n, bit_index, 6, sig_bits, &bit_index ) );
			}
			ENSURE_RES_OK( bitstream_write_uint64(
				buf, n, bit_index, sig_bits, x >> trailing_zeros, &bit_index ) );
			// printf("write b01; leading=%d bits=%d val=%ld\n", leading_zeros, sig_bits,
			// x>>trailing_zeros); printf("==write b11; leading=%d sigbits=%d val=%d; shifted by
			// %d\n", leading_zeros, sig_bits, x>>trailing_zeros, trailing_zeros );

			*last_leading = leading_zeros;
			*last_sigbits = sig_bits;
		}
	}
error:
	*new_bit_index = bit_index;
	return res;
}

int tsc_read_double( const char* buf,
					 size_t n,
					 size_t bit_index,
					 uint64_t* value,
					 size_t* num_bits_read,
					 uint8_t* last_leading,
					 uint8_t* last_sigbits )
{
	assert( buf );
	assert( value );
	assert( num_bits_read );
	int res = ERR_OK;
	uint64_t x;
	ENSURE_RES_OK( bitstream_read_uint64( buf, n, bit_index, 1, &x ) );
	bit_index++;
	if( x == 0 ) {
		// printf("==read b0 val=0\n");
		// b0
		*value = 0;
		*num_bits_read = 1 + 0;
		return ERR_OK;
	}

	ENSURE_RES_OK( bitstream_read_uint64( buf, n, bit_index, 1, &x ) );
	bit_index++;
	if( x == 0 ) {
		int last_trailing = 64 - *last_sigbits - *last_leading;
		// printf("(2)calc trailing = 64 - %d sigbits - %d leading = %d\n", *last_sigbits,
		// *last_leading, last_trailing);
		ENSURE_RES_OK( bitstream_read_uint64( buf, n, bit_index, *last_sigbits, &x ) );
		*value = x << last_trailing;
		bit_index += ( *last_sigbits );
		// printf("got the val: %ld\n", *value);

		// printf("==read b10; val=%d (cached leading=%d sigbits=%d); unshifted by %d\n", x,
		// *last_leading, *last_sigbits, last_trailing);
		*num_bits_read = 1 + 1 + ( *last_sigbits );
		return ERR_OK;
	}
	// printf("read b11\n");

	ENSURE_RES_OK( bitstream_read_uint64( buf, n, bit_index, 5, &x ) );
	*last_leading = x;
	bit_index += 5;

	ENSURE_RES_OK( bitstream_read_uint64( buf, n, bit_index, 6, &x ) );
	if( x == 0 ) {
		*last_sigbits = 64; // special case, see tsc_write_double method for details
		// printf("special case!!!\n");
	}
	else {
		*last_sigbits = x;
	}
	bit_index += 6;

	int trailing = 64 - *last_sigbits - *last_leading;
	// printf("(2)calc trailing = 64 - %d sigbits - %d leading = %d\n", *last_sigbits,
	// *last_leading, trailing);

	ENSURE_RES_OK( bitstream_read_uint64( buf, n, bit_index, *last_sigbits, &x ) );
	*value = x << trailing;
	bit_index += *last_sigbits;

	// printf("==read b11; leading=%d sigbits=%d val=%d; unshifted by %d\n", *last_leading,
	// *last_sigbits, x, trailing);
	*num_bits_read = 1 + 1 + 5 + 6 + ( *last_sigbits );
	return ERR_OK;

error:
	return res;
}
