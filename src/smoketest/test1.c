#include "test1.h"

#include "client.h"
#include "crash_test.h"
#include "generate_test_data.h"
#include "globals.h"
#include "log.h"
#include "my_malloc.h"
#include "option_parser.h"
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

int run_test1( const char*** argv, const char** env )
{

	int res;
	int num_keys = 3;
	int num_pts = 1000;
	int start_time = 1562270341;
	int restart_freq = 10;

	char* crash_location = "CRASH_TEST_LOCATION_1";

	int initial_num_before_crash = 3;
	int incr_num_before_crash = 1;

	signal( SIGPIPE, SIG_IGN );

	my_malloc_init();

	int help = 0;

	const char* crash_test_choices[] = {"CRASH_TEST_DISABLED",
										"CRASH_TEST_LOCATION_1",
										"CRASH_TEST_LOCATION_2",
										"CRASH_TEST_WRITE",
										"CRASH_TEST_AFTER_COMPLETE_BLOCK_WRITE"};

	struct option options[] = {
		OPT_INT( 'k', "keys", &num_keys, "number of keys" ),
		OPT_INT( 'p', "pts", &num_pts, "number of points per key" ),
		OPT_INT( 't', "time", &start_time, "the starting time (int epoch timestamp)" ),
		OPT_INT( 'r', "restart", &restart_freq, "restart frequency" ),
		OPT_STR_CHOICES( 'l',
						 "location",
						 &crash_location,
						 "force the server to crash in this code location",
						 5,
						 crash_test_choices ),
		OPT_INT( 'c',
				 "crash",
				 &initial_num_before_crash,
				 "force the server to crash after the crash location is hit this many times" ),
		OPT_INT( 'i',
				 "incr",
				 &incr_num_before_crash,
				 "after crashing increase the crash number by this value" ),
		OPT_FLAG( 'h', "help", &help, "display this help text" ),
		OPT_END,
	};

	res = parse_options( options, argv );
	if( res ) {
		return res;
	}
	if( help ) {
		print_help( options );
		return 0;
	}

	res = run_crash_test( num_keys,
						  num_pts,
						  start_time,
						  crash_location,
						  initial_num_before_crash,
						  incr_num_before_crash );
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
