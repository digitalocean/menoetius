#include "option_parser.h"

#include "get.h"
#include "globals.h"
#include "help.h"
#include "put.h"

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
#define MAX_HOST 1024

//#define ARRAY_SIZE( x ) ( sizeof( x ) / sizeof( x[0] ) )

#define DEFAULT_PROCESS_NAME "menoetius-cli"

const char* process_name;
char host[MAX_HOST];
int port = MENOETIUS_PORT;

int is_little_endian()
{
	int i = 1;
	char* p = (char*)&i;
	return p[0] == 1;
}

void print_usage( struct cmd_struct* commands, const char* name )
{
	fprintf( stderr, "usage: %s <test-name> [<args>]\n", name );
	fprintf( stderr, "available tests:\n" );
	for( ; commands->name; commands++ ) {
		printf( " - %s\n", commands->name );
	}
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
		{NULL, NULL},
	};

	// TODO remove the requirement to use my_malloc from the cli and client lib
	// should be ifdef guarded
	my_malloc_init();

	if( !argv[0] ) {
		print_usage( commands, DEFAULT_PROCESS_NAME );
		return 2;
	}
	process_name = argv[0];
	argv++;

	if( !process_name[0] ) {
		process_name = DEFAULT_PROCESS_NAME;
	}

	res = parse_options( options, &argv );
	if( res || help ) {
		print_usage( commands, process_name );
		return res ? 1 : 0;
	}

	if( !argv[0] ) {
		fprintf( stderr,
				 "missing command name; run \"%s help\" to view available commands\n",
				 process_name );
		return 1;
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
