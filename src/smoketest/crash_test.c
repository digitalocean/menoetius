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
#include <unistd.h>

#define TEST_STATE_SEND_POINT 1
#define TEST_STATE_FORCE_SYNC 2
#define TEST_STATE_GET_POINTS 3

int run_crash_test( int num_keys,
					int num_pts,
					int64_t start_time,
					const char* crash_location,
					int initial_num_before_crash,
					int incr_num_before_crash )
{
	int res;
	size_t num_returned_pts;

	int64_t* tt = NULL;
	double* yy = NULL;

	signal( SIGPIPE, SIG_IGN );

	pid_t pid = -1;

	const char* host = "localhost";
	int port = MENOETIUS_PORT;
	int server_timeout = 3;

	// each time we restart, this will grow
	int num_success_before_crash = initial_num_before_crash;

	const char* storage_path = get_test_storage_path();
	LOG_INFO( "path=s starting integration test", storage_path );

	struct menoetius_client client;
	menoetius_client_init( &client, host, port );
	client.read_buf_size = 1023 + 111; // pick weird values on purpose
	client.write_buf_size = 1023 + 103;

	char lfm[TEST_LFM_MAX_SIZE];
	int lfm_len;
	double y;
	int64_t t;

	tt = (int64_t*)my_malloc( sizeof( int64_t ) * num_pts );
	yy = (double*)my_malloc( sizeof( double ) * num_pts );

	// TODO pass timeout? otherwise this will never connect
	res = wait_for_server_to_respond( &client, 0, 0 );
	if( res == 0 ) {
		LOG_ERROR( "server is already running; unable to run smoke test" );
		res = 1;
		goto error;
	}

	int num_send_crashes = 0;
	int num_sync_crashes = 0;

	bool check_for_crashes = strcmp( crash_location, "CRASH_TEST_DISABLED" ) != 0;

	int test_state = TEST_STATE_SEND_POINT;
	for( int i = 0; i < num_keys; i++ ) {
		for( int user_id = 0; user_id < num_keys; user_id++ ) {
		retry:
			LOG_DEBUG( "num_success_before_crash=d trying", num_success_before_crash );
			if( start_server_if_needed(
					&pid, storage_path, start_time, crash_location, num_success_before_crash ) ) {
				// server was (re)started
				num_success_before_crash += incr_num_before_crash;
				res = wait_for_server_to_respond( &client, pid, server_timeout );
				if( res ) {
					goto retry;
				}
			}

			t = start_time + i;
			get_test_lfm( lfm, &lfm_len, user_id );

			// --------- SEND POINT ------------
			if( test_state == TEST_STATE_SEND_POINT ) {
				y = get_test_pt( ( user_id + 1 ) * ( i + 1 ) );
				LOG_DEBUG( "t=d i=d lfm=*s y=f writing points", t, i, lfm_len, lfm, y );
				if( ( res = menoetius_client_send_sync( &client, lfm, lfm_len, 1, &t, &y ) ) ) {
					LOG_ERROR( "res=d failed to send points" );
					num_send_crashes++;
					goto retry;
				}
				LOG_DEBUG( "send point completed" );
				test_state = TEST_STATE_FORCE_SYNC;
			}

			// --------- FORCE SYNC ------------
			if( test_state == TEST_STATE_FORCE_SYNC ) {
				LOG_DEBUG( "forcing sync" );
				if( ( res = menoetius_client_test_hook( &client, TEST_HOOK_SYNC_FLAG ) ) ) {
					LOG_ERROR( "res=d failed to sync" );
					num_sync_crashes++;

					test_state =
						TEST_STATE_SEND_POINT; // must resend last point since it wasn't saved
					goto retry;
				}
				LOG_DEBUG( "forcing sync completed" );
				test_state = TEST_STATE_GET_POINTS;
			}

			// --------- TEST POINTS ------------
			if( test_state == TEST_STATE_GET_POINTS ) {
				LOG_DEBUG( "lfm=*s requesting points", lfm_len, lfm );
				memset( tt, 0, num_pts * sizeof( int64_t ) );
				memset( yy, 0, num_pts * sizeof( double ) );
				if( ( res = menoetius_client_get(
						  &client, lfm, lfm_len, num_pts, &num_returned_pts, tt, yy ) ) ) {
					LOG_ERROR( "res=d failed to get points" );
					goto retry;
				}
				LOG_DEBUG( "get points completed" );

				if( validate_points( start_time,
									 user_id,
									 i + 1 /* expected num pts */,
									 num_returned_pts,
									 tt,
									 yy ) ) {
					res = 1;
					goto error;
				}
			}

			// --------- advance in loop to next point to send/test ------------
			test_state = TEST_STATE_SEND_POINT;
		}
	}

	if( check_for_crashes ) {
		if( num_send_crashes == 0 ) {
			LOG_ERROR( "test never caused a send crash" );
			res = 1;
			goto error;
		}
		if( num_sync_crashes == 0 ) {
			LOG_ERROR( "test never caused a sync crash" );
			res = 1;
			goto error;
		}
	}

	res = 0;
error:
	LOG_INFO( "cleaning up smoke test" );
	if( pid != -1 ) {
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
