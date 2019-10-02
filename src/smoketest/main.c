#include "all_tests.h"
#include "test1.h"
#include "test2.h"
#include "test3.h"

#include "escape_key.h"
#include "log.h"
#include "option_parser.h"
#include "structured_stream.h"

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

int main( int argc, const char** argv, const char** env )
{
	int res;
	set_log_level_from_env_variables( env );

	signal( SIGPIPE, SIG_IGN );

	int help = 0;
	struct option options[] = {{'h', "--help", &help, OPTION_FLAG}, {'\0', "", NULL, OPTION_END}};

	struct cmd_struct commands[] = {
		{"test1", run_test1},
		{"test2", run_test2},
		{"test3", run_test3},
		{"all", run_all_tests},
		{NULL, NULL},
	};

	if( !argv[0] ) {
		print_usage( commands, "smoke" );
		return 2;
	}
	const char* pname = argv[0];
	argv++;

	res = parse_options( options, &argv );
	if( res || help ) {
		print_usage( commands, pname );
		return res ? 1 : 0;
	}

	if( !argv[0] ) {
		fprintf( stderr, "missing test name; run \"%s --help\" to view tests\n", pname );
		return 1;
	}

	struct cmd_struct* cmd = get_command( commands, argv[0] );
	if( !cmd ) {
		printf( "invalid test (%s); run \"%s --help\" to view tests\n", argv[0], pname );
		return 1;
	}
	argv++;

	return cmd->fn( &argv, env );
}
