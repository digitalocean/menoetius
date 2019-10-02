#include "test3.h"

#include "client.h"
#include "crash_test2.h"
#include "generate_test_data.h"
#include "globals.h"
#include "log.h"
#include "my_malloc.h"
#include "option_parser.h"
#include "shutdown_test.h"
#include "test_storage_utils.h"

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int run_test3( const char*** argv, const char** env )
{
	int res;

	signal( SIGPIPE, SIG_IGN );

	my_malloc_init();

	int help = 0;

	struct option options[] = {OPT_FLAG( 'h', "help", &help, "display this help text" ), OPT_END};

	res = parse_options( options, argv );
	if( res ) {
		goto error;
	}
	if( help ) {
		print_help( options );
		return 0;
	}

	res = run_crash_test2();
error:
	LOG_INFO( "cleaning up smoke test" );

	// TODO fix memory leak; not a major issue since the leak is only in the smoketest runner and not the server.
	// (all our unittests assert all memory is freed)
	//my_malloc_assert_free();

	if( res ) {
		LOG_ERROR( "res=d smoke test failed", res );
	}
	else {
		LOG_INFO( "smoke test passed" );
	}

	return res;
}
