#define _GNU_SOURCE
#include "get.h"

#include "cli_globals.h"

#include "client.h"
#include "my_malloc.h"
#include "option_parser.h"
#include "time_utils.h"

#include "lfm.h"
#include "lfm_binary_parser.h"
#include "lfm_human_parser.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

int run_benchmark( const char*** argv, const char** env )
{
	int res;

	//option vars
	int num_metrics = 1000000;
	int num_points = 24 * 60 * 60;
	int batch_size = 1;
	int help = 0;

	struct option options[] = {
		OPT_INT( 'm', "metrics", &num_metrics, "number of metrics" ),
		OPT_INT( 'p', "points", &num_points, "number of points per metric" ),
		OPT_INT( 'b', "batch", &batch_size, "number of metrics/points per batch" ),
		OPT_FLAG( 'h', "help", &help, "display this help" ),
		OPT_END};

	res = parse_options( options, argv );
	if( res ) {
		goto error;
	}
	if( help ) {
		printf( "Usage: %s [--host <host>] benchmark [OPTIONS]...\n", process_name );
		printf( "\n" );
		print_help( options );
		return 0;
	}

	assert( batch_size == 1 ); //not yet supported

	struct menoetius_client client;
	menoetius_client_init( &client, host, port );
	client.read_buf_size = 1023 + 111; // pick weird values on purpose
	client.write_buf_size = 1023 + 103;

	int lfm_binary_len;
	char lfm_binary[1024];
	memcpy( lfm_binary, "benchmark\x00i\x00", 12 );
	char* i_val = lfm_binary + 12;

	time_t t = time( NULL );
	double y = 3.14;

	long long start = time_microseconds();

	int total_samples = num_points * num_metrics;
	int progress_report_interval = total_samples / 100;
	int progress_report_i = 0;

	for( int point_i = 0; point_i < num_points; point_i++ ) {
		for( int metric_i = 0; metric_i < num_metrics; metric_i++ ) {

			sprintf( i_val, "%d", point_i );
			lfm_binary_len = 12 + strlen( i_val );

			if( ( res =
					  menoetius_client_send( &client, lfm_binary, lfm_binary_len, 1, &t, &y ) ) ) {
				fprintf( stderr, "failed to send data\n" );
				goto error;
			}

			// print a '.' for each percentage
			if( ++progress_report_i == progress_report_interval ) {
				printf( "." );
				fflush( stdout );
				progress_report_i = 0;
			}
		}
	}
	printf( "\n" );

	double duration = ( time_microseconds() - start ) / 1000000.0;
	double ingestion_rate = total_samples / duration;
	printf( "sent %d total samples in %lf seconds; ingestion rate = %0.2lf metrics/second\n",
			total_samples,
			duration,
			ingestion_rate );

	res = 0;
error:
	return res;
}
