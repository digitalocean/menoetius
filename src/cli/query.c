#define _GNU_SOURCE
#include "query.h"

#include "cli_globals.h"

#include "client.h"
#include "globals.h"
#include "my_malloc.h"
#include "option_parser.h"

#include "lfm.h"
#include "lfm_binary_parser.h"
#include "lfm_human_parser.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define MAX_PTS 1024

int run_query( const char*** argv, const char** env )
{
	int res;

	//option vars
	int help = 0;
	int full_scan = 0;

	struct option options[] = {
		OPT_FLAG(
			'f',
			"fullscan",
			&full_scan,
			"perform a full scan (by default if no indexes are available an error is returned)" ),
		OPT_FLAG( 'h', "help", &help, "display this help text" ),
		OPT_END};

	res = parse_options( options, argv );
	if( res ) {
		goto error;
	}
	if( help ) {
		printf( "Usage: %s [--host <host>] query [LFM]...\n", process_name );
		printf( "\n" );
		printf( "the LFM argument is a lexicographically flattened metric of the form:\n" );
		printf( "  <METRIC_NAME>{<KEY_LABEL>=\"<VALUE_LABEL>\", ...}\n" );
		printf( "\n" );
		printf( "Examples:\n" );
		printf( "  # search for all metrics with a given name\n" );
		printf( "  %s query num_instruments\n", process_name );
		printf( "  # search for all metrics with a specific label\n" );
		printf( "  %s query {instrument=\"guitar\"}\n", process_name );
		printf( "\n" );
		printf( "Output: a list of matched LFMs\n" );
		printf( "\n" );
		print_help( options );
		return 0;
	}

	struct menoetius_client client;
	menoetius_client_init( &client, host, port );
	client.read_buf_size = 1023 + 111; // pick weird values on purpose
	client.write_buf_size = 1023 + 103;

	struct LFM* lfm_input = NULL;
	struct LFM* lfm_result = NULL;

	size_t max_num_results = 1024;
	struct binary_lfm* results = my_malloc( sizeof( struct binary_lfm ) * max_num_results );
	size_t num_results;
	char* human_lfm = NULL;

	for( int i = 0; argv[0][i]; i++ ) {

		// TODO parse LFM here

		const char* raw_lfm = argv[0][i];

		if( ( res = parse_human_lfm( raw_lfm, &lfm_input ) ) ) {
			fprintf( stderr, "failed to parse %s\n", raw_lfm );
			goto error;
		}

		// when matching, we must set it as __name__
		if( lfm_input->name && strlen( lfm_input->name ) > 0 ) {
			lfm_add_label_unsorted( lfm_input, my_strdup( "__name__" ), lfm_input->name );
			lfm_sort_labels( lfm_input );
			lfm_input->name = NULL;
		}

		if( ( res = menoetius_client_query( &client,
											lfm_input->labels,
											lfm_input->num_labels,
											full_scan,
											results,
											max_num_results,
											&num_results ) ) ) {
			if( res == MENOETIUS_STATUS_MISSING_INDEX ) {
				fprintf( stderr,
						 "failed to query data: no suitible index found, use the --fullscan to "
						 "allow a full scan; WARNING: this can degrade the server performance)\n" );
			}
			else {
				fprintf( stderr, "failed to query data: server_err=%d\n", res );
			}
		}
		else {
			for( int i = 0; i < num_results; i++ ) {

				if( ( res = parse_binary_lfm( results[i].lfm, results[i].n, &lfm_result ) ) ) {
					fprintf( stderr, "failed to parse result\n" );
				}
				else {
					encode_human_lfm( lfm_result, &human_lfm );
					printf( "%s\n", human_lfm );
					my_free( human_lfm );
					my_free( lfm_result );
					lfm_result = NULL;
				}
			}
		}

		lfm_free( lfm_input );
		lfm_input = NULL;
	}

	res = 0;
error:
	return res;
}
