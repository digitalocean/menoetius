#include "escape_key.h"
#include "structured_stream.h"

#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFSIZE 10240

int is_little_endian()
{
	int i = 1;
	char* p = (char*)&i;
	return p[0] == 1;
}

int main( int argc, char** argv )
{
	if( argc != 3 ) {
		printf( "usage: %s <hostname> <port>\n", argv[0] );
		return 1;
	}
	const char* hostname = argv[1];
	int portno = atoi( argv[2] );
	int res;

	if( !is_little_endian() ) {
		return 1;
	}

	int sockfd = socket( AF_INET, SOCK_STREAM, 0 );
	if( sockfd < 0 ) {
		// error("ERROR opening socket");
		return 1;
	}

	struct hostent* server = gethostbyname( hostname );
	if( server == NULL ) {
		// fprintf(stderr,"ERROR, no such host as %s\n", hostname);
		return 1;
	}

	struct sockaddr_in serveraddr;
	bzero( (char*)&serveraddr, sizeof( serveraddr ) );
	serveraddr.sin_family = AF_INET;
	bcopy( (char*)server->h_addr, (char*)&serveraddr.sin_addr.s_addr, server->h_length );
	serveraddr.sin_port = htons( portno );

	/* connect: create a connection with the server */
	if( connect( sockfd, (const struct sockaddr*)&serveraddr, sizeof( serveraddr ) ) < 0 ) {
		return 1;
	}

	struct structured_stream ss = {.fd = sockfd,
								   .read_i = 0,
								   .read_n = 0,
								   .read_buf_size = BUFSIZE,
								   .write_buf_size = BUFSIZE,
								   .read_buf = (char*)malloc( BUFSIZE ),
								   .write_i = 0,
								   .write_buf = (char*)malloc( BUFSIZE )};

	uint64_t magic_header = 1547675033;
	if( ( res = structured_stream_write_uint64( &ss, magic_header ) ) ) {
		return res;
	}

	uint16_t hash = 0;
	while( 1 ) {
		uint8_t rpc = 7;
		if( ( res = structured_stream_write_uint8( &ss, rpc ) ) ) {
			return res;
		}

		if( ( res = structured_stream_write_uint16( &ss, hash ) ) ) {
			return res;
		}

		uint64_t max_keys = 1024;
		if( ( res = structured_stream_write_uint64( &ss, max_keys ) ) ) {
			return res;
		}

		uint8_t include_data = 1;
		if( ( res = structured_stream_write_uint8( &ss, include_data ) ) ) {
			return res;
		}

		uint8_t num_filters = 0;
		if( ( res = structured_stream_write_uint8( &ss, num_filters ) ) ) {
			return res;
		}

		if( ( res = structured_stream_flush( &ss ) ) ) {
			return res;
		}

		uint16_t next_hash;
		if( ( res = structured_stream_read_uint16( &ss, &next_hash ) ) ) {
			printf( "failed to read next hash\n" );
			return res;
		}

		int64_t t;
		double y;
		uint32_t num_pts;
		size_t nn;
		char buf[BUFSIZE];
		char formatted_buf[BUFSIZE];
		while( 1 ) {
			if( ( res = structured_stream_read_uint16_prefixed_bytes( &ss, buf, &nn, BUFSIZE ) ) ) {
				printf( "failed to read string\n" );
				return res;
			}
			if( nn == 0 ) {
				break;
			}

			if( ( res = escape_key( buf, nn, formatted_buf, BUFSIZE ) ) ) {
				printf( "failed to format buf (len=%ld)\n", nn );
				printf( "%.*s", (int)nn, buf );
				return res;
			}
			printf( "%s", formatted_buf );

			if( ( res = structured_stream_read_uint32( &ss, &num_pts ) ) ) {
				printf( "failed to read num pts\n" );
				return res;
			}

			for( int i = 0; i < num_pts; i++ ) {
				if( ( res = structured_stream_read_int64( &ss, &t ) ) ) {
					printf( "failed to read t\n" );
					return res;
				}
				if( ( res = structured_stream_read_double( &ss, &y ) ) ) {
					printf( "failed to read y\n" );
					return res;
				}
				printf( " (%ld, %lf)", t, y );
			}
			printf( "\n" );
		}
		if( next_hash <= hash ) {
			break;
		}
		hash = next_hash;
	}

	close( sockfd );
	return 0;
}
