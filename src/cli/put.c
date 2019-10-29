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

int parse_time( const char* s, time_t* t )
{
	struct tm tm;
	memset( &tm, 0, sizeof( struct tm ) );
	if( !strptime( s, "%Y-%m-%dT%H:%M:%S", &tm ) ) {
		return 1;
	}
	*t = timegm( &tm );
	return 0;
}

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

	time_t t;
	if( time_str ) {
		if( ( res = parse_time( time_str, &t ) ) ) {
			goto error;
		}
	}
	else {
		t = time( NULL );
	}

	struct menoetius_client client;
	menoetius_client_init( &client, host, port );
	client.read_buf_size = 1023 + 111; // pick weird values on purpose
	client.write_buf_size = 1023 + 103;

	double y = 3.14;

	const char* metric_name;
	struct KeyValue metric_labels[MAX_KEY_VALUE_PAIRS];
	int num_labels = 0;

	for( int i = 0; argv[0][i]; i++ ) {

		// TODO parse LFM and value here

		const char* raw_lfm = argv[0][i];

		if( ( res = parse_lfm(
				  raw_lfm, &metric_name, metric_labels, &num_labels, MAX_KEY_VALUE_PAIRS ) ) ) {
			fprintf( stderr, "failed to parse %s\n", raw_lfm );
			goto error;
		}

		printf( "metric name is %s\n", metric_name );
		for( int i = 0; i < num_labels; i++ ) {
			printf( "%s = %s\n", metric_labels[i].key, metric_labels[i].value );
		}

		int raw_lfm_len = strlen( raw_lfm );
		printf( "sdf\n" );
		if( ( res = menoetius_client_send( &client, raw_lfm, raw_lfm_len, 1, &t, &y ) ) ) {
			fprintf( stderr, "failed to send data\n" );
		}
		printf( "done\n" );
	}

	res = 0;
error:
	return res;
}