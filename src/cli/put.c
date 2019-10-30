#define _GNU_SOURCE
#include "help.h"

#include "client.h"
#include "lfm_parser.h"
#include "option_parser.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define MAX_KEY_VALUE_PAIRS 1024

extern char host[];
extern int port;
extern const char* process_name;

int run_put( const char*** argv, const char** env )
{
	int res;

	//option vars
	const char* time_str = NULL;
	int help = 0;

	struct option options[] = {
		OPT_STR( 't', "time", &time_str, "override time value (YYYY-MM-DDTHH:MM:SS UTC)" ),
		OPT_FLAG( 'h', "help", &help, "display this help text" ),
		OPT_END};

	res = parse_options( options, argv );
	if( res ) {
		goto error;
	}
	if( help ) {
		printf( "Usage: %s put [LFM=VALUE[@TIMESTAMP]] [...]\n", process_name );
		printf( "\n" );
		printf( "the LFM argument is a lexicographically flattened metric of the form:\n" );
		printf( "  <METRIC_NAME>{<KEY_LABEL>=\"<VALUE_LABEL>\", ...}\n" );
		printf( "\n" );
		printf( "VALUE is a floating-point value\n" );
		printf( "\n" );
		printf(
			"an optional TIMESTAMP can be used to override the current time and has the form:\n" );
		printf( "  YYYY-MM-DDTHH:MM:SS\n" );
		printf( "\n" );
		printf( "Examples:\n" );
		printf( "  %s put num_instruments{instrument=\"guitar\"}=5\n", process_name );
		printf( "  %s put num_instruments{instrument=\"flute\"}=2@2019-10-23T17:10:02\n",
				process_name );
		printf( "\n" );
		print_help( options );
		return 0;
	}

	struct menoetius_client client;
	menoetius_client_init( &client, host, port );
	client.read_buf_size = 1023 + 111; // pick weird values on purpose
	client.write_buf_size = 1023 + 103;

	double y = 3.14;

	struct LFM* lfm;
	char* lfm_binary = NULL;
	int lfm_binary_len = 0;

	bool has_t;
	time_t t;

	for( int i = 0; argv[0][i]; i++ ) {

		const char* raw_lfm = argv[0][i];

		printf("parsing %s\n", raw_lfm);
		if( ( res = parse_lfm_and_value( raw_lfm, &lfm, &y, &t, &has_t ) ) ) {
			fprintf( stderr, "failed to parse %s\n", raw_lfm );
			goto error;
		}
		if( !has_t ) {
			t = time( NULL ); // use current time
		}

		printf( "metric name is %s; y=%lf; t=%ld\n", lfm->name, y, t );
		for( int i = 0; i < lfm->num_labels; i++ ) {
			printf( "%s = %s\n", lfm->labels[i].key, lfm->labels[i].value );
		}

		encode_binary_lfm( lfm, &lfm_binary, &lfm_binary_len );
		free_lfm( lfm );

		if( ( res = menoetius_client_send( &client, lfm_binary, lfm_binary_len, 1, &t, &y ) ) ) {
			fprintf( stderr, "failed to send data\n" );
		}
		printf( "done\n" );
	}

	res = 0;
error:
	return res;
}
