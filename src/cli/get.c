#define _GNU_SOURCE
#include "help.h"

#include "option_parser.h"
#include "client.h"

#include <stdio.h>
#include <time.h>
#include <string.h>

#define MAX_PTS 1024

extern char host[];
extern int port;

int run_get( const char*** argv, const char** env )
{
	int res;

	//option vars
	int help = 0;

	struct option options[] = {
		OPT_FLAG( 'h', "help", &help, "display this help text" ),
		OPT_END};

	res = parse_options( options, argv );
	if( res ) {
		goto error;
	}
	if( help ) {
		print_help( options );
		return 0;
	}

	struct menoetius_client client;
	menoetius_client_init( &client, host, port );
	client.read_buf_size = 1023 + 111; // pick weird values on purpose
	client.write_buf_size = 1023 + 103;

	double y[MAX_PTS];
	int64_t t[MAX_PTS];

	size_t num_returned_pts;

	for(int i = 0; argv[0][i]; i++) {

		// TODO parse LFM here

		const char *raw_lfm = argv[0][i];
		int raw_lfm_len = strlen(raw_lfm);
		printf("sdf\n");

		if( ( res = menoetius_client_get(
				  &client, raw_lfm, raw_lfm_len, MAX_PTS, &num_returned_pts, t, y ) ) ) {
			fprintf(stderr, "failed to get data\n");
		} else {
			for(int j = 0; j < num_returned_pts; j++ ) {
				printf("(%ld %lf) ", t[j], y[j]);
			}
		}
	}

	res = 0;
error:
	return res;
}


