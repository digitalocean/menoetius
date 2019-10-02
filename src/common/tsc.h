#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// p is a compressed tsc data stream (the buffer that is stored in tsc_header->data)
#define TSC_NUM_PTS( p ) ( ( (uint32_t*)( p ) )[0] )
#define TSC_START_TIME( p ) ( ( (int64_t*)( p + sizeof( uint32_t ) ) )[0] )

#define TSC_ITER_T( iter_p ) ( ( iter_p )->t )

struct tsc_iter
{
	int64_t y;
	int64_t t;

	// internals
	uint32_t i;
	size_t r;
	int64_t dt;

	uint8_t last_leading;
	uint8_t last_sigbits;
};

// I had to move this macro into an inline function to avoid
// error:dereferencing type-punned pointer will break strict-aliasing rules
//#define TSC_ITER_Y( iter_p ) ( ( (double*)( &( ( iter_p )->y ) ) )[0] )
static inline double TSC_ITER_Y( struct tsc_iter* p )
{
	double y;
	memcpy( &y, &p->y, 8 );
	return y;
}
#define TSC_HEADER_SERIALIZATION_SIZE 30

struct tsc_header
{
	// comments indicate how these are serialized (weird formatting for clang-format reasons)
	// this data is only needed for when we sync an in-progress data stream to disk
	// once we write out a full data stream which we'll no longer add to, we won't need these.
	int64_t last_timestamp; //       serialization bytes: 8
	int64_t last_timestamp_delta; // serialization bytes: 8
	double prev_value; //            serialization bytes: 8

	// this does NOT include the initial uint32_t num_pts or int64_t start_time values
	uint32_t num_valid_bits; //      serialization bytes: 4

	uint8_t last_leading; //         serialization bytes: 1
	uint8_t last_sigbits; //         serialization bytes: 1

	char* data; // data[0:8] store num_pts as a uint32_t; beyond that point it's a bitstream.
	uint32_t data_len; // max free space, stored as num of bytes
};

int tsc_init( struct tsc_header* p, size_t n );

void tsc_clear( struct tsc_header* p );
bool tsc_is_clear( const struct tsc_header* p );

void tsc_load_block( struct tsc_header* p,
					 const char* serialized_tsc_header,
					 const char* buf,
					 size_t n );

// serializes the header into s[TSC_HEADER_SERIALIZATION_SIZE]
void tsc_serialize_header( struct tsc_header* p, char* s );
void tsc_deserialize_header( struct tsc_header* p, const char* s );

int tsc_free( struct tsc_header* p );

int tsc_add( struct tsc_header* p, int64_t t, double y );

int tsc_trim( struct tsc_header* p, int64_t start_time, int64_t* num_pts_remaining );

void tsc_iter_init( struct tsc_iter* iter );

// data is a pointer to raw data; it is not a string.
int tsc_iter_next( const char* data, struct tsc_iter* iter );

// only exposed here for testing purposes
int tsc_write_int64( char* buf, size_t n, size_t bit_index, int64_t x, size_t* new_bit_index );
int tsc_read_int64(
	const char* buf, size_t n, size_t bit_index, int64_t* value, size_t* num_bits_read );

int tsc_write_double( char* buf,
					  size_t n,
					  size_t bit_index,
					  uint64_t x,
					  size_t* new_bit_index,
					  uint8_t* last_leading,
					  uint8_t* last_trailing );
int tsc_read_double( const char* buf,
					 size_t n,
					 size_t bit_index,
					 uint64_t* value,
					 size_t* num_bits_read,
					 uint8_t* last_leading,
					 uint8_t* last_trailing );
