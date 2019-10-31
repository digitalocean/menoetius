#define _GNU_SOURCE
#include "get.h"

#include "client.h"
#include "option_parser.h"
#include "my_malloc.h"

#include "lfm.h"
#include "lfm_human_parser.h"
#include "lfm_binary_parser.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define MAX_PTS 1024

extern char host[];
extern int port;
extern const char* process_name;

int run_get( const char*** argv, const char** env )
{
	int res;

	//option vars
	int help = 0;
	int human_time = 0;

	struct option options[] = {
		OPT_FLAG( 't', "time", &human_time, "display time as YYYY-MM-DDTHH:MM:SS UTC" ),
		OPT_FLAG( 'h', "help", &help, "display this help text" ),
		OPT_END};

	res = parse_options( options, argv );
	if( res ) {
		goto error;
	}
	if( help ) {
		printf( "Usage: %s get [LFM]...\n", process_name );
		printf( "\n" );
		printf( "the LFM argument is a lexicographically flattened metric of the form:\n" );
		printf( "  <METRIC_NAME>{<KEY_LABEL>=\"<VALUE_LABEL>\", ...}\n" );
		printf( "\n" );
		printf( "Examples:\n" );
		printf( "  %s get num_instruments{instrument=\"guitar\"}\n", process_name );
		printf( "  %s get -t num_instruments{instrument=\"guitar\"}\n", process_name );
		printf( "\n" );
		printf( "Output: a tuple of (time, floating point value) are printed to stdout;\n" );
		printf( "if the --time flag is given, the time will be formatted, otherwise an \n" );
		printf( "epoch int value is used. All time values are in UTC.\n" );
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

	double y[MAX_PTS];
	int64_t t[MAX_PTS];

	size_t num_returned_pts;

	for( int i = 0; argv[0][i]; i++ ) {

		// TODO parse LFM here

		const char* raw_lfm = argv[0][i];

		if( ( res = parse_human_lfm( raw_lfm, &lfm ) ) ) {
			fprintf( stderr, "failed to parse %s\n", raw_lfm );
			goto error;
		}

		encode_binary_lfm( lfm, &lfm_binary, &lfm_binary_len );
		lfm_free( lfm );

		if( ( res = menoetius_client_get(
				  &client, lfm_binary, lfm_binary_len, MAX_PTS, &num_returned_pts, t, y ) ) ) {
			fprintf( stderr, "failed to get data\n" );
		}
		else {
			for( int j = 0; j < num_returned_pts; j++ ) {
				if( human_time ) {
					char buf[1024];
					strftime( buf, 1024, "%Y-%m-%dT%H:%M:%S", gmtime( &( t[j] ) ) );
					printf( "%s %lf\n", buf, y[j] );
				}
				else {
					printf( "%ld %lf\n", t[j], y[j] );
				}
			}
		}

		if( lfm_binary ) {
			my_free( lfm_binary );
			lfm_binary = NULL;
		}
	}

	res = 0;
error:
	return res;
}
