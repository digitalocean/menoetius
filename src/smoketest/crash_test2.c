#include "crash_test.h"

#include "option_parser.h"
#include "test1.h"
#include "test_utils.h"

#include "client.h"
#include "generate_test_data.h"
#include "globals.h"
#include "log.h"
#include "my_malloc.h"
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
#include <time.h>
#include <unistd.h>

#define TEST_STATE_SEND_POINT 1
#define TEST_STATE_FORCE_SYNC 2
#define TEST_STATE_GET_POINTS 3

int run_crash_test2()
{
	int res;
	pid_t pid = -1;
	double y;

	int64_t* tt = NULL;
	double* yy = NULL;
	size_t num_returned_pts;

	const char* storage_path = get_test_storage_path();
	const char* crash_location = "CRASH_TEST_AFTER_COMPLETE_BLOCK_WRITE";
	const int num_crashes = 0;
	const int clock_time = time( NULL );

	LOG_INFO( "path=s starting integration test", storage_path );
	pid = run_server( storage_path, crash_location, num_crashes, clock_time );
	assert( pid > 0 );

	const char* host = "localhost";
	int port = MENOETIUS_PORT;

	struct menoetius_client client;
	menoetius_client_init( &client, host, port );
	client.read_buf_size = 1023 + 179; // pick weird values on purpose
	client.write_buf_size = 1023 + 109;

	res = wait_for_server_to_respond( &client, pid, 100 /* timeout ms */ );
	if( res ) {
		LOG_ERROR( "res=d failed to start server", res );
		goto error;
	}

	const char* lfm = "hello";
	int lfm_len = 5;
	int64_t t = clock_time;

	int num_written = 0;
	for( ;; ) {
		y = get_test_pt( 1 );
		LOG_DEBUG( "t=d lfm=*s y=f writing points", t, lfm_len, lfm, y );
		if( ( res = menoetius_client_send( &client, lfm, lfm_len, 1, &t, &y ) ) ) {
			LOG_INFO( "res=d failed to send points", res );
			break;
		}
		t += 1;
		num_written++;
	}

	waitpid( pid, NULL, 0 );
	pid = -1;

	if( num_written < 50 ) {
		LOG_ERROR( "num_points=d not enough points written before server crashed; perhaps it "
				   "crashed due to something unrelated to CRASH_TEST_AFTER_COMPLETE_BLOCK_WRITE?",
				   num_written );
		res = 1;
		goto error;
	}

	// at this point the server has crashed (hopefuly due to CRASH_TEST_AFTER_COMPLETE_BLOCK_WRITE being hit)
	LOG_INFO( "num_points=d wrote out points before crashing", num_written );

	// start the server back up to test that it can recover the completed block data.
	crash_location = "CRASH_TEST_DISABLED";
	pid = run_server( storage_path, crash_location, num_crashes, clock_time );
	assert( pid > 0 );

	// reconnect (TODO should close/cleanup the previous client)
	menoetius_client_init( &client, host, port );

	res = wait_for_server_to_respond( &client, pid, 2000 /* timeout ms */ );
	if( res ) {
		LOG_ERROR( "res=d failed to start server", res );
		goto error;
	}
	LOG_INFO( "connected to restarted server" );

	// send enough points to cause a block to be written out
	int n = num_written * 10;
	for( int i = 0; i < n; i++ ) {
		y = get_test_pt( 1 );
		LOG_DEBUG( "t=d lfm=*s y=f writing points", t, lfm_len, lfm, y );
		if( ( res = menoetius_client_send( &client, lfm, lfm_len, 1, &t, &y ) ) ) {
			LOG_ERROR( "res=d failed to send points", res );
			goto error;
		}
		t += 1;
		num_written++;
	}

	LOG_INFO( "sent second loop of metrics" );
	assert( num_written > 200 );

	// get points and ensure num_written match

	tt = (int64_t*)my_malloc( sizeof( int64_t ) * num_written );
	yy = (double*)my_malloc( sizeof( double ) * num_written );

	memset( tt, 0, num_written * sizeof( int64_t ) );
	memset( yy, 0, num_written * sizeof( double ) );
	res = menoetius_client_get( &client, lfm, lfm_len, num_written, &num_returned_pts, tt, yy );
	if( res ) {
		LOG_ERROR( "res=d failed to get points", res );
		goto error;
	}
	if( num_returned_pts != num_written ) {
		LOG_ERROR( "got=d want=d num points missmatch", num_returned_pts, num_written );
		res = 1;
		goto error;
	}
	for( int i = 0; i < num_written; i++ ) {
		assert( tt[i] == clock_time + i );
		assert( yy[i] == y );
	}

	res = 0;
error:
	LOG_INFO( "cleaning up smoke test" );
	if( pid > 0 ) {
		kill_server( pid );
		pid = -1;
	}
	delete_test_storage();

	if( yy ) {
		my_free( yy );
		yy = NULL;
	}
	if( tt ) {
		my_free( tt );
		tt = NULL;
	}

	if( res ) {
		LOG_ERROR( "res=d smoke test failed", res );
	}
	else {
		LOG_INFO( "smoke test passed" );
	}

	return res;
}
