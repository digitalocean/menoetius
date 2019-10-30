#define _GNU_SOURCE
#include "query.h"

#include "client.h"
#include "lfm_parser.h"
#include "option_parser.h"
#include "globals.h"

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#define MAX_PTS 1024

extern char host[];
extern int port;
extern const char* process_name;

int run_query( const char*** argv, const char** env )
{
	int res;

	//option vars
	int help = 0;
	int full_scan = 0;

	struct option options[] = {
		OPT_FLAG( 'f', "fullscan", &full_scan, "perform a full scan (by default if no indexes are available an error is returned)" ),
		OPT_FLAG( 'h', "help", &help, "display this help text" ),
		OPT_END};

	res = parse_options( options, argv );
	if( res ) {
		goto error;
	}
	if( help ) {
		printf( "Usage: %s query [LFM]...\n", process_name );
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

	struct LFM* lfm;
	char* lfm_binary = NULL;
	int lfm_binary_len = 0;

	size_t max_num_results = 1024;
	struct binary_lfm *results = malloc(sizeof(struct binary_lfm) *max_num_results);
	size_t num_results;

	for( int i = 0; argv[0][i]; i++ ) {

		// TODO parse LFM here

		const char* raw_lfm = argv[0][i];

		if( ( res = parse_lfm( raw_lfm, &lfm ) ) ) {
			fprintf( stderr, "failed to parse %s\n", raw_lfm );
			goto error;
		}


		// when matching, we must set it as __name__
		if( lfm->name ) {
			lfm_add_label_unsorted( lfm, strdup("__name__"), lfm->name );
			lfm_sort_labels( lfm );
			lfm->name = NULL;
		}

		encode_binary_lfm( lfm, &lfm_binary, &lfm_binary_len );
		int num_matchers = lfm->num_labels;
		free_lfm( lfm );

		const char *lfm_matchers = lfm_binary + 1; // must skip first byte which is NULL since we discard the name

		if( ( res = menoetius_client_query( &client, lfm_matchers, num_matchers, full_scan, results, max_num_results, &num_results ) ) ) {
			if( res == MENOETIUS_STATUS_MISSING_INDEX ) {
				fprintf( stderr, "failed to query data: no suitible index found, use the --fullscan to allow a full scan; WARNING: this can degrade the server performance)\n" );
			} else {
				fprintf( stderr, "failed to query data: server_err=%d\n", res );
			}
		}
		else {
			for( int i = 0; i < num_results; i++ ) {

				if( (res = parse_binary_lfm( results[i].lfm, results[i].n, &lfm )) ) {
					fprintf( stderr, "failed to parse result\n" );
				} else {
					char *human_lfm = NULL;
					printf("ready?\n");
					encode_human_lfm( lfm, &human_lfm );
					printf("done\n");
					printf("%s\n", human_lfm);
					free(human_lfm);
					free(lfm);
				}
			}
		}

		if( lfm_binary ) {
			free( lfm_binary );
			lfm_binary = NULL;
		}
	}

	res = 0;
error:
	return res;
}

