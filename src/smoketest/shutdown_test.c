#include "shutdown_test.h"

#include "client.h"
#include "env_builder.h"
#include "generate_test_data.h"
#include "globals.h"
#include "log.h"
#include "my_malloc.h"
#include "option_parser.h"
#include "test1.h"
#include "test_storage_utils.h"
#include "test_utils.h"

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

int graceful_restart( int* pid,
					  struct menoetius_client* client,
					  const char* storage_path,
					  int64_t start_time,
					  int timeout_ms )
{
	int res;
	LOG_DEBUG( "shutting down" );
	res = shutdown_server_gracefully_and_wait( *pid );
	if( res ) {
		return 1;
	}
	LOG_DEBUG( "shutdown complete" );

	LOG_DEBUG( "starting server" );
	*pid = run_server( storage_path, NULL, 0, start_time );
	assert( *pid > 0 );
	LOG_DEBUG( "serverpid=d run server", *pid );

	res = wait_for_server_to_respond( client, *pid, timeout_ms );
	if( res ) {
		LOG_ERROR( "failed to wait for server" );
		return 1;
	}
	return 0;
}

int run_shutdown_test( int num_keys, int num_pts, int64_t start_time )
{
	int res;
	size_t num_returned_pts;

	int64_t* tt = NULL;
	double* yy = NULL;

	pid_t pid = -1;

	const char* host = "localhost";
	int port = MENOETIUS_PORT;

	const char* storage_path = get_test_storage_path();
	LOG_INFO( "path=s starting integration test", storage_path );

	struct menoetius_client client;
	menoetius_client_init( &client, host, port );
	client.read_buf_size = 1023 + 134;
	client.write_buf_size = 1023 + 173;

	char lfm[TEST_LFM_MAX_SIZE];
	int lfm_len;
	double y;
	int64_t t;
	int timeout_ms = 500;

	tt = (int64_t*)my_malloc( sizeof( int64_t ) * num_pts );
	yy = (double*)my_malloc( sizeof( double ) * num_pts );

	res = wait_for_server_to_respond( &client, 0, 0 );
	if( res == 0 ) {
		LOG_ERROR( "server is already running; unable to run smoke test" );
		res = 1;
		goto error;
	}

	LOG_DEBUG( "starting server" );
	pid = run_server( storage_path, NULL, 0, start_time );
	assert( pid > 0 );
	LOG_DEBUG( "serverpid=d run server", pid );

	res = wait_for_server_to_respond( &client, pid, timeout_ms );
	if( res ) {
		LOG_ERROR( "failed to wait for server" );
		res = 1;
		goto error;
	}

	for( int i = 0; i < num_pts; i++ ) {
		for( int key_num = 0; key_num < num_keys; key_num++ ) {
			t = start_time + i;
			y = get_test_pt( ( i + 1 ) * ( key_num + 1 ) );
			get_test_lfm( lfm, &lfm_len, key_num );

			// Send data
			LOG_DEBUG( "t=d i=d lfm=*s y=f writing points", t, i, lfm_len, lfm, y );
			if( ( res = menoetius_client_send_sync( &client, lfm, lfm_len, 1, &t, &y ) ) ) {
				LOG_ERROR( "res=d failed to send points" );
				res = 1;
				goto error;
			}

			// get data
			LOG_DEBUG( "lfm=*s requesting points", lfm_len, lfm );
			memset( tt, 0, num_pts * sizeof( int64_t ) );
			memset( yy, 0, num_pts * sizeof( double ) );
			if( ( res = menoetius_client_get(
					  &client, lfm, lfm_len, num_pts, &num_returned_pts, tt, yy ) ) ) {
				LOG_ERROR( "res=d failed to get points", res );
				goto error;
			}
			LOG_DEBUG( "lfm=*s num=d get points completed", lfm_len, lfm, num_returned_pts );

			// test data
			if( validate_points( start_time,
								 key_num,
								 i + 1 /* expected num pts */,
								 num_returned_pts,
								 tt,
								 yy ) ) {
				res = 1;
				goto error;
			}

			// shutdown and restart server
			res = graceful_restart( &pid, &client, storage_path, start_time, timeout_ms );
			if( res ) {
				goto error;
			}

			// get data
			LOG_DEBUG( "lfm=*s requesting points (after restart)", lfm_len, lfm );
			memset( tt, 0, num_pts * sizeof( int64_t ) );
			memset( yy, 0, num_pts * sizeof( double ) );
			if( ( res = menoetius_client_get(
					  &client, lfm, lfm_len, num_pts, &num_returned_pts, tt, yy ) ) ) {
				LOG_ERROR( "res=d failed to get points", res );
				goto error;
			}
			LOG_DEBUG( "num=d get points completed", num_returned_pts );

			// test data
			if( validate_points( start_time,
								 key_num,
								 i + 1 /* expected num pts */,
								 num_returned_pts,
								 tt,
								 yy ) ) {
				res = 1;
				goto error;
			}
		}
	}

	LOG_INFO( "final shutdown" );
	res = shutdown_server_gracefully_and_wait( pid );
	if( res ) {
		goto error;
	}
	pid = -1;

error:
	if( pid > 0 ) {
		LOG_INFO( "killing server during cleanup" );
		kill( pid, SIGKILL );
	}

	if( res ) {
		LOG_ERROR( "res=d smoke test failed", res );
	}
	else {
		LOG_INFO( "smoke test passed" );
	}

	return res;
}
