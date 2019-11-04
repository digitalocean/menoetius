#include "option_parser.h"

#include "cli_globals.h"

#include "benchmark.h"
#include "get.h"
#include "globals.h"
#include "help.h"
#include "put.h"
#include "query.h"

#include "log.h"
#include "my_malloc.h"

#include <assert.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFSIZE 10240

//#define ARRAY_SIZE( x ) ( sizeof( x ) / sizeof( x[0] ) )

#define DEFAULT_PROCESS_NAME "menoetius-cli"

int is_little_endian()
{
	int i = 1;
	char* p = (char*)&i;
	return p[0] == 1;
}

int parse_host_and_port( const char* s, char host[MAX_HOST], int* port )
{
	for( int i = 0; s[i]; i++ ) {
		if( s[i] == ':' ) {
			memcpy( host, s, i );
			host[i] = '\0';
			i++;
			if( s[i] ) {
				*port = atoi( s + i );
				if( *port == 0 ) {
					return 1;
				}
			}
			else {
				return 1;
			}
			return 0;
		}
	}
	strcpy( host, s );
	return 0;
}

int main( int argc, const char** argv, const char** env )
{
	int res;

	const char* host_and_port = "localhost";

	int help = 0;
	struct option options[] = {OPT_STR( 'H',
										"host",
										&host_and_port,
										"host (and optional port) to connect in the form host, or "
										"host:port (default: localhost)" ),
							   OPT_FLAG( 'h', "help", &help, "display this help text" ),
							   OPT_END};

	struct cmd_struct commands[] = {
		{"help", run_help},
		{"put", run_put},
		{"get", run_get},
		{"query", run_query},
		{"benchmark", run_benchmark},
		{NULL, NULL},
	};

	// TODO remove the requirement to use my_malloc from the cli and client lib
	// should be ifdef guarded
	my_malloc_init();

	set_log_level_from_env_variables( env );

	if( argv[0] != NULL ) {
		process_name = argv[0];
		argv++;
		if( !process_name[0] ) {
			// argv[0] is "", odd but we'll accept it
			process_name = DEFAULT_PROCESS_NAME;
		}
	}
	else {
		process_name = DEFAULT_PROCESS_NAME;
		help = 1; // force help, argv is bad
	}

	res = parse_options( options, &argv );
	if( res || help || !argv[0] ) {
		run_help( NULL, env );
		return res ? 1 : 0;
	}

	res = parse_host_and_port( host_and_port, host, &port );
	if( res ) {
		fprintf( stderr, "failed to parse host \"%s\"\n", host_and_port );
		return 1;
	}

	struct cmd_struct* cmd = get_command( commands, argv[0] );
	if( !cmd ) {
		printf( "invalid command (%s); run \"%s help\" to view available commands\n",
				argv[0],
				process_name );
		return 1;
	}
	argv++;

	return cmd->fn( &argv, env );
}
