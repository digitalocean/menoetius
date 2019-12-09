#define _GNU_SOURCE
#include "get.h"

#include "cli_globals.h"

#include "client.h"
#include "my_malloc.h"
#include "option_parser.h"

#include "lfm.h"
#include "lfm_binary_parser.h"
#include "lfm_human_parser.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define MAX_CLUSTER_CONFIG 1024 * 1024

int run_get_config( const char*** argv, const char** env )
{
	int res;

	//option vars
	int help = 0;

	struct option options[] = {OPT_FLAG( 'h', "help", &help, "display this help text" ), OPT_END};

	res = parse_options( options, argv );
	if( res ) {
		goto error;
	}
	if( help ) {
		printf( "Usage: %s [--host <host>] get-config\n", process_name );
		printf( "\n" );
		printf( "Output: the cluster configuration;\n" );
		printf( "\n" );
		print_help( options );
		return 0;
	}

	struct menoetius_client client;
	menoetius_client_init( &client, host, port );

	char cluster_config[MAX_CLUSTER_CONFIG];

	res = menoetius_client_get_config( &client, cluster_config, MAX_CLUSTER_CONFIG );
	if( res ) {
		fprintf( stderr, "failed to get cluster-config\n" );
		goto error;
	}
	printf( "%s", cluster_config );
	res = 0;
error:
	return res;
}
