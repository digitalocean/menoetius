#include "help.h"

#include "option_parser.h"

#include <stdio.h>

int run_help( const char*** argv, const char** env )
{
	printf("Basic data storage/retrival functions\n");
	printf("  put         Send data to the server\n");
	printf("  get         Get data from the server (given a fully-qualified metric key)\n");
	printf("  query       Discovery fully-qualified metric keys given a subset of label to search for\n");

	printf("Admin functions\n");
	printf("  get-config  Display the current cluster config\n");
	printf("  set-config  Change the current cluster config\n");

	return 0;
}

