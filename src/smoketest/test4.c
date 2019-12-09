#include "test4.h"

#include "client.h"
#include "crash_test2.h"
#include "generate_test_data.h"
#include "globals.h"
#include "log.h"
#include "my_malloc.h"
#include "option_parser.h"
#include "shutdown_test.h"
#include "structured_stream.h"
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
#include <time.h>
#include <unistd.h>

// hidden from client.h, not meant for public consumption
int menoetius_client_ensure_connected( struct menoetius_client* client );

int foo()
{
	int res;
	pid_t pid = -1;

	const char* storage_path = get_test_storage_path();
	const char* crash_location = "CRASH_TEST_AFTER_COMPLETE_BLOCK_WRITE";
	const int num_crashes = 0;
	const int clock_time = time( NULL );

	LOG_INFO( "path=s starting integration test", storage_path );
	pid = run_server( storage_path, crash_location, num_crashes, clock_time );
	assert( pid > 0 );

	struct menoetius_client client;
	if( ( res = menoetius_client_init( &client, "localhost", MENOETIUS_PORT ) ) ) {
		LOG_ERROR( "failed to init client" );
		goto error;
	}

	res = wait_for_server_to_respond( &client, pid, 100 /* timeout ms */ );
	if( res ) {
		LOG_ERROR( "res=d failed to start server", res );
		goto error;
	}

	if( ( res = menoetius_client_ensure_connected( &client ) ) ) {
		LOG_ERROR( "failed to connect" );
		goto error;
	}

	char buf[1024];

	int n = 0;
	char* s = buf;

	uint8_t x = MENOETIUS_RPC_PUT_DATA;
	memcpy( s, &x, sizeof( uint8_t ) );
	s += sizeof( uint8_t );
	n += sizeof( uint8_t );

	int key_len = 10;
	memcpy( s, &key_len, sizeof( uint16_t ) );
	s += sizeof( uint16_t );
	n += sizeof( uint16_t );

	// intentially send less
	char key[3] = {'a', 'b', 'c'};
	memcpy( s, key, 3 );
	s += 3;
	n += 3;

	if( ( res = structured_stream_write_bytes( client.ss, buf, n ) ) ) {
		LOG_ERROR( "res=s write failed", err_str( res ) );
		goto error;
	}

	if( ( res = structured_stream_flush( client.ss ) ) ) {
		LOG_ERROR( "res=s flush failed", err_str( res ) );
		goto error;
	}

	usleep( 100 );

	// test server can still respond to another request

	struct menoetius_client client2;
	if( ( res = menoetius_client_init( &client2, "localhost", MENOETIUS_PORT ) ) ) {
		LOG_ERROR( "failed to init client" );
		goto error;
	}

	res = wait_for_server_to_respond( &client2, pid, 10 /* timeout ms */ );
	if( res ) {
		LOG_ERROR( "res=d server failed to respond", res );
		goto error;
	}

	res = 0;
error:

	if( pid > 0 ) {
		kill_server( pid );
		pid = -1;
	}

	return res;
}

int run_test4( const char*** argv, const char** env )
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

	res = foo();
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
