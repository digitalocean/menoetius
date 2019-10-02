#include "escape_key.h"
#include "globals.h"
#include "log.h"
#include "structured_stream.h"
#include "tsc.h"

#include <assert.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFSIZE 1024 * 1024

int is_little_endian()
{
	int i = 1;
	char* p = (char*)&i;
	return p[0] == 1;
}

int main( int argc, const char** argv, const char** env )
{
	int res;

	// HACK; need an opt-parser
	const char* comp_path = argv[1];

	set_log_level_from_env_variables( env );

	int fd = open( comp_path, O_RDONLY );
	if( fd < 0 ) {
		LOG_ERROR( "path=s failed to read", comp_path );
		return 1;
	}

	struct structured_stream* ss;

	res = structured_stream_new( fd, 1024 * 1024, 0, &ss );
	if( res != 0 ) {
		LOG_ERROR( "res=d failed to create structured stream", res );
		return 1;
	}

	char escapped_key[BUFSIZE];

	struct tsc_header tsc;

	const char* s;
	size_t n;
	uint16_t num_pts;
	double y;
	int64_t t;
	uint64_t total_bytes = 0;
	for( ;; ) {
		res = structured_stream_read_uint16_prefixed_bytes_inplace( ss, &s, &n );
		switch( res ) {
		case 0:
			break; // no error
		case ERR_STRUCTURED_STREAM_EOF:
			goto done;
		default:
			LOG_ERROR( "res=d failed to read", res );
			return 1;
		}
		assert( escape_key( s, n, escapped_key, BUFSIZE ) == 0 );
		// printf("got %s\n", buf);

		tsc_init( &tsc, 64 );

		res = structured_stream_read_uint16( ss, &num_pts );
		if( res != 0 ) {
			LOG_ERROR( "res=d failed to read", res );
			return 1;
		}

		for( uint16_t i = 0; i < num_pts; i++ ) {
			res = structured_stream_read_int64( ss, &t );
			if( res != 0 ) {
				LOG_ERROR( "res=d failed to read", res );
				return 1;
			}
			res = structured_stream_read_double( ss, &y );
			if( res != 0 ) {
				LOG_ERROR( "res=d failed to read", res );
				return 1;
			}
			// printf("got %ld %lf\n", t, y);
			assert( tsc_add( &tsc, t, y ) == 0 );
		}
		total_bytes += tsc.data_len + sizeof( tsc );
		printf( "%s used: %d + %ld\n", escapped_key, tsc.data_len, sizeof( tsc ) );
		tsc_free( &tsc );
	}
done:
	printf( "total bytes used: %ld\n", total_bytes );
	return 0;

	// for( i = 0; i < num_pts; i++ ) {
	//	CU_ASSERT( tsc_add( &tsc, t[i], y[i] ) == 0 );
	//}

	// struct tsc_iter iter;
	// tsc_iter_init( &iter );

	// uint64_t tt;
	// double yy;
	// i = 0;
	// while( tsc_iter_next( &tsc, &iter, &tt, &yy ) == 0 ) {
	//	CU_ASSERT_FATAL( i < num_pts );
	//	ASSERT_LONG_EQUAL( tt, t[i] );
	//	ASSERT_DOUBLE_EQUAL( yy, y[i] );
	//	i++;
	//}
	// CU_ASSERT( i == num_pts );

	//// test trimming
	// int earliest_point = 2;
	// CU_ASSERT( tsc_trim( &tsc, t[earliest_point] ) == 0 );

	// i = earliest_point;
	// tsc_iter_init( &iter );
	// while( tsc_iter_next( &tsc, &iter, &tt, &yy ) == 0 ) {
	//	CU_ASSERT_FATAL( i < num_pts );
	//	ASSERT_LONG_EQUAL( tt, t[i] );
	//	ASSERT_DOUBLE_EQUAL( yy, y[i] );
	//	i++;
	//}
	// CU_ASSERT( i == num_pts );
}
