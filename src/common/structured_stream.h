#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct structured_stream
{
	int fd;
	int read_i;
	int read_n;
	char* read_buf;
	int read_buf_size;

	int write_i;
	char* write_buf;
	int write_buf_size;

	// when false, structured stream will NOT attempt network reads or flushes
	// and will maintain the underlying buffers
	bool allowed_io;

	// when false, memory will be discarded after being read.
	// this can be set to true when doing multiple inplace reads without corrupting data
	bool allowed_discard;
};

int structured_stream_new( int fd,
						   size_t read_buffer_size,
						   size_t write_buffer_size,
						   struct structured_stream** p );

void structured_stream_init( struct structured_stream* ss, int fd );

void structured_stream_free( struct structured_stream* ss );

size_t structured_stream_read_avail( struct structured_stream* ss );
size_t structured_stream_write_avail( struct structured_stream* ss );

int structured_stream_ensure_read( struct structured_stream* ss, size_t n );
int structured_stream_ensure_write( struct structured_stream* ss, size_t n );

int structured_stream_read_uint8( struct structured_stream* ss, uint8_t* x );
int structured_stream_read_uint16( struct structured_stream* ss, uint16_t* x );
int structured_stream_read_uint32( struct structured_stream* ss, uint32_t* x );
int structured_stream_read_uint64( struct structured_stream* ss, uint64_t* x );
int structured_stream_read_int8( struct structured_stream* ss, int8_t* x );
int structured_stream_read_int16( struct structured_stream* ss, int16_t* x );
int structured_stream_read_int32( struct structured_stream* ss, int32_t* x );
int structured_stream_read_int64( struct structured_stream* ss, int64_t* x );
int structured_stream_read_float( struct structured_stream* ss, float* x );
int structured_stream_read_double( struct structured_stream* ss, double* x );

int structured_stream_read_bytes( struct structured_stream* ss, char* buf, size_t n );

int structured_stream_read_uint16_prefixed_bytes( struct structured_stream* ss,
												  char* buf,
												  size_t* n,
												  size_t buf_size );

int structured_stream_read_uint16_prefixed_string( struct structured_stream* ss,
												   char* buf,
												   size_t buf_size );

int structured_stream_read_uint16_prefixed_bytes_inplace( struct structured_stream* ss,
														  const char** buf,
														  size_t* n );

int structured_stream_read_bytes_inplace( struct structured_stream* ss,
										  size_t n,
										  const char** buf );

int structured_stream_write_uint8( struct structured_stream* ss, uint8_t x );
int structured_stream_write_uint16( struct structured_stream* ss, uint16_t x );
int structured_stream_write_uint32( struct structured_stream* ss, uint32_t x );
int structured_stream_write_uint64( struct structured_stream* ss, uint64_t x );
int structured_stream_write_int8( struct structured_stream* ss, int8_t x );
int structured_stream_write_int16( struct structured_stream* ss, int16_t x );
int structured_stream_write_int32( struct structured_stream* ss, int32_t x );
int structured_stream_write_int64( struct structured_stream* ss, int64_t x );
int structured_stream_write_float( struct structured_stream* ss, float x );
int structured_stream_write_double( struct structured_stream* ss, double x );

int structured_stream_write_bytes( struct structured_stream* ss, const char* buf, size_t n );
int structured_stream_write_uint16_prefixed_bytes( struct structured_stream* ss,
												   const char* buf,
												   uint16_t n );
int structured_stream_write_uint16_prefixed_string( struct structured_stream* ss, const char* buf );

int structured_stream_flush( struct structured_stream* ss );
