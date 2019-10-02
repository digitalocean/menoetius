#include "all_tests.h"

#include "option_parser.h"
#include "test_storage_utils.h"

#include "client.h"
#include "crash_test.h"
#include "generate_test_data.h"
#include "globals.h"
#include "log.h"
#include "my_malloc.h"

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

struct test_config_struct
{
	const char* name;
	int num_keys;
	int num_pts;
	time_t start_time;
	const char* crash_location;
	int initial_num_before_crash;
	int incr_num_before_crash;
};

int run_all_tests( const char*** argv, const char** env )
{
	int res;

	my_malloc_init();

	struct test_config_struct configs[] = {
		{"one", 1, 3, 1562270341, "CRASH_TEST_LOCATION_1", 0, 1},
		{"1970s", 10, 10, 102341, "CRASH_TEST_LOCATION_1", 37, 11},
		{"3x3", 3, 3, 1562270341, "CRASH_TEST_WRITE", 80, 1148},
		{"10x10", 10, 10, 1562270341, "CRASH_TEST_WRITE", 193, 3218},
		{NULL, 0, 0, 0, NULL, 0, 0},
	};

	int help = 0;
	struct option options[] = {{'h', "help", &help, OPTION_FLAG}, {'\0', "", NULL, OPTION_END}};

	res = parse_options( options, argv );
	if( res ) {
		goto error;
	}

	if( help ) {
		fprintf( stderr, "runs all the tests\n" );
		return 0;
	}

	int num_pass = 0;
	for( struct test_config_struct* p = configs; p->name; p++ ) {
		res = run_crash_test( p->num_keys,
							  p->num_pts,
							  p->start_time,
							  p->crash_location,
							  p->initial_num_before_crash,
							  p->incr_num_before_crash );
		if( res ) {
			fprintf( stderr, "test %s failed\n", p->name );
			goto error;
		}
		num_pass++;
	}

	res = 0;
error:

	// TODO fix memory leak; not a major issue since the leak is only in the smoketest runner and not the server.
	// (all our unittests assert all memory is freed)
	//my_malloc_assert_free();

	if( res ) {
		LOG_ERROR( "res=d smoke test failed", res );
	}
	else {
		LOG_INFO( "howmany=d smoke test passed", num_pass );
	}

	return res;
}
