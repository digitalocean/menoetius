#include "structured_stream.h"

#include "globals.h"
#include "log.h"
#include "my_malloc.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int structured_stream_dump( struct structured_stream* ss );

int structured_stream_dump( struct structured_stream* ss )
{
	// printf("ss->i = %d\n", ss->i);
	// printf("ss->n = %d\n", ss->n);
	// for( int i = 0; i < ss->buf_size; i++ ) {
	//	char c = ss->buf[i];
	//	if( c >= 'a' && c <= 'z' ) {
	//		printf("%c  ", c);
	//	} else {
	//		printf("%02x ", ss->buf[i] & 0xff);
	//	}
	//}
	// printf("\n");
	return 0;
}

int structured_stream_new( int fd,
						   size_t read_buffer_size,
						   size_t write_buffer_size,
						   struct structured_stream** p )
{
	assert( p );
	*p = my_malloc( sizeof( struct structured_stream ) );
	if( *p == NULL ) {
		return ERR_MEM_ALLOCATION;
	}
	memset( *p, 0, sizeof( struct structured_stream ) );
	if( read_buffer_size > 0 ) {
		( **p ).read_buf = my_malloc( read_buffer_size );
		assert( ( **p ).read_buf );
		( **p ).read_buf_size = read_buffer_size;
	}
	if( write_buffer_size > 0 ) {
		( **p ).write_buf = my_malloc( write_buffer_size );
		assert( ( **p ).write_buf );
		( **p ).write_buf_size = write_buffer_size;
	}
	structured_stream_init( *p, fd );
	return 0;
}

void structured_stream_init( struct structured_stream* ss, int fd )
{
	ss->read_i = 0;
	ss->read_n = 0;
	ss->write_i = 0;
	ss->fd = fd;
	ss->allowed_io = true;
	ss->allowed_discard = true;
}

void structured_stream_free( struct structured_stream* p )
{
	assert( p );
	if( p->read_buf ) {
		my_free( p->read_buf );
	}
	if( p->write_buf ) {
		my_free( p->write_buf );
	}
	my_free( p );
}

size_t structured_stream_read_avail( struct structured_stream* ss )
{
	assert( ss->read_n >= ss->read_i );
	size_t avail = ss->read_n - ss->read_i;
	return avail;
}

size_t structured_stream_write_avail( struct structured_stream* ss )
{
	assert( ss->write_buf_size >= ss->write_i );
	size_t avail = ss->write_buf_size - ss->write_i;
	return avail;
}

int structured_stream_ensure_read( struct structured_stream* ss, size_t n )
{
	assert( ss->read_n >= ss->read_i );
	ssize_t m;
	size_t avail = ss->read_n - ss->read_i;
	if( avail >= n ) {
		return 0;
	}
	if( !ss->allowed_io ) {
		LOG_ERROR( "n=d structured_stream_ensure_read failed in non-io mode", n );
		return ERR_STRUCTURED_STREAM_IO_REQUIRED;
	}
	if( ss->read_i > 0 && ss->allowed_discard ) {
		memmove( ss->read_buf, &ss->read_buf[ss->read_i], avail );
		ss->read_n -= ss->read_i;
		ss->read_i = 0;
	}
	size_t needed = n - ss->read_n;
	size_t max_space = ss->read_buf_size - ss->read_n;
	if( needed > max_space ) {
		return ERR_STRUCTURED_STREAM_BUF_TOO_SMALL;
	}

	for( ;; ) {
		assert( max_space > 0 );
		m = read( ss->fd, &ss->read_buf[ss->read_n], max_space );
		if( m < 0 ) {
			if( errno == EAGAIN ) {
				continue;
			}
			return ERR_STRUCTURED_STREAM_BUF_OTHER;
		}
		else if( m == 0 ) {
			// EOF was reached
			return ERR_STRUCTURED_STREAM_EOF;
		}
		else if( m < needed ) {
			LOG_WARN( "m=d needed=d didnt read as much as expected", m, needed );
			ss->read_n += (size_t)m;
			needed -= m;
			max_space -= m;
		}
		else {
			ss->read_n += (size_t)m;
			return 0;
		}
	}
}

int structured_stream_flush( struct structured_stream* ss )
{
	ssize_t w;
	if( !ss->allowed_io ) {
		LOG_ERROR( "structured_stream_flush failed in non-io mode" );
		return ERR_STRUCTURED_STREAM_IO_REQUIRED;
	}
	while( ss->write_i > 0 ) {
		w = write( ss->fd, ss->write_buf, ss->write_i );
		if( w < 0 ) {
			if( errno == EAGAIN ) {
				continue;
			}
			LOG_ERROR( "w=d err=s write returned an error", w, strerror( errno ) );
			return ERR_STRUCTURED_STREAM_BUF_OTHER;
		}
		assert( w <= ss->write_i );
		ss->write_i -= w;
	}
	assert( ss->write_i == 0 );
	return 0;
}

int structured_stream_ensure_write( struct structured_stream* ss, size_t n )
{
	size_t avail = ss->write_buf_size - ss->write_i;
	// LOG_DEBUG( "avail=d i=d ensure write", avail, ss->write_i );
	if( avail >= n ) {
		return 0;
	}

	if( n > ss->write_buf_size ) {
		return ERR_STRUCTURED_STREAM_BUF_TOO_SMALL;
	}

	return structured_stream_flush( ss );
}

int structured_stream_read_uint8( struct structured_stream* ss, uint8_t* x )
{
	int res;

	res = structured_stream_ensure_read( ss, sizeof( uint8_t ) );
	if( res ) {
		return res;
	}

	*x = *( (uint8_t*)( &ss->read_buf[ss->read_i] ) );
	ss->read_i += sizeof( uint8_t );
	assert( ss->read_n >= ss->read_i );
	return 0;
}

int structured_stream_read_uint16( struct structured_stream* ss, uint16_t* x )
{
	int res;

	res = structured_stream_ensure_read( ss, sizeof( uint16_t ) );
	if( res ) {
		return res;
	}

	*x = *( (uint16_t*)( &ss->read_buf[ss->read_i] ) );
	ss->read_i += sizeof( uint16_t );
	assert( ss->read_n >= ss->read_i );
	return 0;
}

int structured_stream_read_uint32( struct structured_stream* ss, uint32_t* x )
{
	int res;

	res = structured_stream_ensure_read( ss, sizeof( uint32_t ) );
	if( res ) {
		return res;
	}

	*x = *( (uint32_t*)( &ss->read_buf[ss->read_i] ) );
	ss->read_i += sizeof( uint32_t );
	assert( ss->read_n >= ss->read_i );
	return 0;
}

int structured_stream_read_uint64( struct structured_stream* ss, uint64_t* x )
{
	int res;

	res = structured_stream_ensure_read( ss, sizeof( uint64_t ) );
	if( res ) {
		return res;
	}

	*x = *( (uint64_t*)( &ss->read_buf[ss->read_i] ) );
	ss->read_i += sizeof( uint64_t );
	assert( ss->read_n >= ss->read_i );
	return 0;
}

int structured_stream_read_int16( struct structured_stream* ss, int16_t* x )
{
	int res;

	res = structured_stream_ensure_read( ss, sizeof( int16_t ) );
	if( res ) {
		return res;
	}

	*x = *( (int16_t*)( &ss->read_buf[ss->read_i] ) );
	ss->read_i += sizeof( int16_t );
	assert( ss->read_n >= ss->read_i );
	return 0;
}

int structured_stream_read_int32( struct structured_stream* ss, int32_t* x )
{
	int res;

	res = structured_stream_ensure_read( ss, sizeof( int32_t ) );
	if( res ) {
		return res;
	}

	*x = *( (int32_t*)( &ss->read_buf[ss->read_i] ) );
	ss->read_i += sizeof( int32_t );
	assert( ss->read_n >= ss->read_i );
	return 0;
}

int structured_stream_read_int64( struct structured_stream* ss, int64_t* x )
{
	int res;

	res = structured_stream_ensure_read( ss, sizeof( int64_t ) );
	if( res ) {
		return res;
	}

	*x = *( (int64_t*)( &ss->read_buf[ss->read_i] ) );
	ss->read_i += sizeof( int64_t );
	assert( ss->read_n >= ss->read_i );
	return 0;
}

int structured_stream_read_float( struct structured_stream* ss, float* x )
{
	int res;

	res = structured_stream_ensure_read( ss, sizeof( float ) );
	if( res ) {
		return res;
	}

	*x = *( (float*)( &ss->read_buf[ss->read_i] ) );
	ss->read_i += sizeof( float );
	assert( ss->read_n >= ss->read_i );
	return 0;
}

int structured_stream_read_double( struct structured_stream* ss, double* x )
{
	int res;

	res = structured_stream_ensure_read( ss, sizeof( double ) );
	if( res ) {
		return res;
	}

	*x = *( (double*)( &ss->read_buf[ss->read_i] ) );
	ss->read_i += sizeof( double );
	assert( ss->read_n >= ss->read_i );
	return 0;
}

int structured_stream_read_bytes( struct structured_stream* ss, char* buf, size_t n )
{
	int res;

	res = structured_stream_ensure_read( ss, n );
	if( res ) {
		return res;
	}

	memcpy( buf, &ss->read_buf[ss->read_i], n );
	ss->read_i += n;
	assert( ss->read_n >= ss->read_i );
	return 0;
}

int structured_stream_read_uint16_prefixed_bytes( struct structured_stream* ss,
												  char* buf,
												  size_t* n,
												  size_t buf_size )
{
	int res;
	const char* s;
	res = structured_stream_read_uint16_prefixed_bytes_inplace( ss, &s, n );
	if( res ) {
		return res;
	}

	if( *n > buf_size ) {
		return ERR_STRUCTURED_STREAM_BUF_TOO_SMALL;
	}

	memcpy( buf, s, *n );
	return 0;
}

int structured_stream_read_uint16_prefixed_string( struct structured_stream* ss,
												   char* buf,
												   size_t buf_size )
{
	int res;
	const char* s;
	size_t n;

	res = structured_stream_read_uint16_prefixed_bytes_inplace( ss, &s, &n );
	if( res ) {
		return res;
	}

	if( n > ( buf_size - 1 ) || buf_size == 0 ) {
		return ERR_STRUCTURED_STREAM_BUF_TOO_SMALL;
	}

	memcpy( buf, s, n );
	buf[n] = '\0';
	return 0;
}

int structured_stream_read_uint16_prefixed_bytes_inplace( struct structured_stream* ss,
														  const char** buf,
														  size_t* n )
{
	int res;

	uint16_t nn;
	res = structured_stream_read_uint16( ss, &nn );
	if( res ) {
		return res;
	}
	*n = nn;

	// rewind (to treat read as a peek)
	ss->read_i -= sizeof( uint16_t );

	res = structured_stream_ensure_read( ss, *n + sizeof( uint16_t ) );
	switch( res ) {
	case 0:
		break; // no error
	case ERR_STRUCTURED_STREAM_EOF:
		return ERR_STRUCTURED_STREAM_BUF_UNDERRUN;
	default:
		return res;
	}

	// by this point, we have read all the data in, so we can fast-forward to
	// the real payload
	ss->read_i += sizeof( uint16_t );

	*buf = &ss->read_buf[ss->read_i];

	// be careful here! the called expects to be able to read from buf
	// so we can't do anything that will move the underlying memory around;
	// once another structured_stream_* call is made, it can then be moved around
	ss->read_i += *n;
	assert( ss->read_n >= ss->read_i );

	return 0;
}

int structured_stream_read_bytes_inplace( struct structured_stream* ss, size_t n, const char** buf )
{
	int res;

	res = structured_stream_ensure_read( ss, n );
	switch( res ) {
	case 0:
		break; // no error
	case ERR_STRUCTURED_STREAM_EOF:
		return ERR_STRUCTURED_STREAM_BUF_UNDERRUN;
	default:
		return res;
	}

	*buf = &ss->read_buf[ss->read_i];

	// be careful here! the called expects to be able to read from buf
	// so we can't do anything that will move the underlying memory around;
	// once another structured_stream_* call is made, it can then be moved around
	ss->read_i += n;
	assert( ss->read_n >= ss->read_i );

	return 0;
}

int structured_stream_write_uint8( struct structured_stream* ss, uint8_t x )
{
	int res;
	if( ( res = structured_stream_ensure_write( ss, sizeof( uint8_t ) ) ) ) {
		return res;
	}
	memcpy( &( ss->write_buf[ss->write_i] ), &x, sizeof( uint8_t ) );
	ss->write_i += sizeof( uint8_t );
	return 0;
}

int structured_stream_write_uint16( struct structured_stream* ss, uint16_t x )
{
	int res;
	if( ( res = structured_stream_ensure_write( ss, sizeof( uint16_t ) ) ) ) {
		return res;
	}
	memcpy( &( ss->write_buf[ss->write_i] ), &x, sizeof( uint16_t ) );
	ss->write_i += sizeof( uint16_t );
	return 0;
}

int structured_stream_write_uint32( struct structured_stream* ss, uint32_t x )
{
	int res;
	if( ( res = structured_stream_ensure_write( ss, sizeof( uint32_t ) ) ) ) {
		return res;
	}
	memcpy( &( ss->write_buf[ss->write_i] ), &x, sizeof( uint32_t ) );
	ss->write_i += sizeof( uint32_t );
	return 0;
}

int structured_stream_write_uint64( struct structured_stream* ss, uint64_t x )
{
	int res;
	if( ( res = structured_stream_ensure_write( ss, sizeof( uint64_t ) ) ) ) {
		return res;
	}
	memcpy( &( ss->write_buf[ss->write_i] ), &x, sizeof( uint64_t ) );
	ss->write_i += sizeof( uint64_t );
	return 0;
}

int structured_stream_write_int8( struct structured_stream* ss, int8_t x )
{
	int res;
	if( ( res = structured_stream_ensure_write( ss, sizeof( int8_t ) ) ) ) {
		return res;
	}
	memcpy( &( ss->write_buf[ss->write_i] ), &x, sizeof( int8_t ) );
	ss->write_i += sizeof( int8_t );
	return 0;
}

int structured_stream_write_int16( struct structured_stream* ss, int16_t x )
{
	int res;
	if( ( res = structured_stream_ensure_write( ss, sizeof( int16_t ) ) ) ) {
		return res;
	}
	memcpy( &( ss->write_buf[ss->write_i] ), &x, sizeof( int16_t ) );
	ss->write_i += sizeof( int16_t );
	return 0;
}

int structured_stream_write_int32( struct structured_stream* ss, int32_t x )
{
	int res;
	if( ( res = structured_stream_ensure_write( ss, sizeof( int32_t ) ) ) ) {
		return res;
	}
	memcpy( &( ss->write_buf[ss->write_i] ), &x, sizeof( int32_t ) );
	ss->write_i += sizeof( int32_t );
	return 0;
}

int structured_stream_write_int64( struct structured_stream* ss, int64_t x )
{
	int res;
	if( ( res = structured_stream_ensure_write( ss, sizeof( int64_t ) ) ) ) {
		return res;
	}
	memcpy( &( ss->write_buf[ss->write_i] ), &x, sizeof( int64_t ) );
	ss->write_i += sizeof( int64_t );
	return 0;
}

int structured_stream_write_float( struct structured_stream* ss, float x )
{
	int res;
	if( ( res = structured_stream_ensure_write( ss, sizeof( float ) ) ) ) {
		return res;
	}
	memcpy( &( ss->write_buf[ss->write_i] ), &x, sizeof( float ) );
	ss->write_i += sizeof( float );
	return 0;
}

int structured_stream_write_double( struct structured_stream* ss, double x )
{
	int res;
	if( ( res = structured_stream_ensure_write( ss, sizeof( double ) ) ) ) {
		return res;
	}
	memcpy( &( ss->write_buf[ss->write_i] ), &x, sizeof( double ) );
	ss->write_i += sizeof( double );
	return 0;
}

int structured_stream_write_bytes( struct structured_stream* ss, const char* buf, size_t n )
{
	int res;
	if( ( res = structured_stream_ensure_write( ss, n ) ) ) {
		return res;
	}
	memcpy( &( ss->write_buf[ss->write_i] ), buf, n );
	ss->write_i += n;
	return 0;
}

int structured_stream_write_uint16_prefixed_bytes( struct structured_stream* ss,
												   const char* buf,
												   uint16_t n )
{
	int res;
	if( ( res = structured_stream_write_uint16( ss, n ) ) ) {
		return res;
	}
	if( n == 0 ) {
		return 0;
	}
	if( ( res = structured_stream_write_bytes( ss, buf, (size_t)n ) ) ) {
		return res;
	}
	return 0;
}

int structured_stream_write_uint16_prefixed_string( struct structured_stream* ss, const char* buf )
{
	return structured_stream_write_uint16_prefixed_bytes( ss, buf, strlen( buf ) );
}
