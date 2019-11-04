#include "help.h"

#include "cli_globals.h"

#include "globals.h"
#include "option_parser.h"

#include <stdio.h>

int run_help( const char*** argv, const char** env )
{
	printf( "usage: %s [OPTIONS] <command> [CMD-OPTIONS]\n", process_name );
	printf( "OPTIONS\n" );
	printf( "  -H, --host  menoetius host (default: localhost:%d)\n", MENOETIUS_PORT );
	printf( "  -h, --help  display this help text\n" );
	printf( "Basic data storage/retrival commands\n" );
	printf( "  put         Send data to the server\n" );
	printf( "  get         Get data from the server (given a fully-qualified metric key)\n" );
	printf( "  query       Discovery fully-qualified metric keys given a subset of label to search "
			"for\n" );

	printf( "Admin commands\n" );
	printf( "  get-config  Display the current cluster config\n" );
	printf( "  set-config  Change the current cluster config\n" );

	printf( "Misc commands\n" );
	printf( "  benchmark   perform a benchmark test\n" );
	//printf( "  help        Display this help\n" );

	printf( "CMD-OPTIONS: run `%s <command> --help` for more details\n", process_name );

	return 0;
}
