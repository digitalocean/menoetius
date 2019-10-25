#define _GNU_SOURCE
#include "help.h"

#include "option_parser.h"
#include "client.h"

#include <stdio.h>
#include <time.h>
#include <string.h>

extern char host[];
extern int port;

int parse_time( const char* s, time_t *t )
{
	struct tm tm;
	memset(&tm, 0, sizeof(struct tm));
	if( ! strptime(s, "%Y-%m-%dT%H:%M:%S", &tm) ) {
		return 1;
	}
	*t = timegm(&tm);
	return 0;
}

int run_put( const char*** argv, const char** env )
{
	int res;

	//option vars
	const char *time_str = NULL;
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
		print_help( options );
		return 0;
	}

	time_t t;
	if( time_str ) {
		if( (res = parse_time(time_str, &t)) ) {
			goto error;
		}
	} else {
		t = time(NULL);
	}

	struct menoetius_client client;
	menoetius_client_init( &client, host, port );
	client.read_buf_size = 1023 + 111; // pick weird values on purpose
	client.write_buf_size = 1023 + 103;

	double y = 3.14;

	for(int i = 0; argv[0][i]; i++) {

		// TODO parse LFM and value here

		const char *raw_lfm = argv[0][i];
		int raw_lfm_len = strlen(raw_lfm);
		printf("sdf\n");
		if( ( res = menoetius_client_send( &client, raw_lfm, raw_lfm_len, 1, &t, &y ) ) ) {
			fprintf(stderr, "failed to send data\n");
		}
		printf("done\n");
	}

	res = 0;
error:
	return res;
}

