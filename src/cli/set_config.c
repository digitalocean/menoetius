#define _GNU_SOURCE
#include "get.h"

#include "cli_globals.h"

#include "client.h"
#include "my_malloc.h"
#include "option_parser.h"

#include "lfm.h"
#include "lfm_binary_parser.h"
#include "lfm_human_parser.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define MAX_CLUSTER_CONFIG 1024 * 1024

int run_set_config( const char*** argv, const char** env )
{
	int res;
	int fd = -1;

	//option vars
	int help = 0;

	struct option options[] = {OPT_FLAG( 'h', "help", &help, "display this help text" ), OPT_END};

	res = parse_options( options, argv );
	if( res ) {
		goto error;
	}
	if( help ) {
		printf( "Usage: %s [--host <host>] set-config <PATH>\n", process_name );
		printf( "\n" );
		printf( "<PATH> a path to a file which will be read and then sent to the server; if '-' "
				"data from stdin is used." );
		print_help( options );
		return 0;
	}

	if( **argv == NULL ) {
		fprintf( stderr, "missing <PATH>\n" );
		res = 1;
		goto error;
	}
	const char* path = **argv;
	( *argv )++;

	if( **argv ) {
		fprintf( stderr, "unexpected arg: %s\n", **argv );
		res = 1;
		goto error;
	}

	char cluster_config[MAX_CLUSTER_CONFIG];

	if( strcmp( path, "-" ) == 0 ) {
		fd = STDIN_FILENO;
	}
	else {
		fd = open( path, O_RDONLY );
		if( fd < 0 ) {
			fprintf( stderr, "failed to open %s: %s\n", path, strerror( errno ) );
			res = 1;
			goto error;
		}
	}
	int buf_avail = MAX_CLUSTER_CONFIG;
	char* p = cluster_config;
	for( ;; ) {
		size_t n = read( fd, cluster_config, buf_avail );
		if( n > 0 ) {
			buf_avail -= n;
			p += n;
			p[0] = 0;
			continue;
		}
		if( n == 0 ) {
			break;
		}
		if( errno == EAGAIN ) {
			continue;
		}
		fprintf( stderr, "failed to read %s: %s\n", path, strerror( errno ) );
		res = 1;
		goto error;
	}

	struct menoetius_client client;
	menoetius_client_init( &client, host, port );

	res = menoetius_client_set_config( &client, cluster_config );
	if( res ) {
		fprintf( stderr, "failed to get cluster-config\n" );
		goto error;
	}
	res = 0;
error:
	if( fd >= 0 ) {
		close( fd );
	}
	return res;
}
