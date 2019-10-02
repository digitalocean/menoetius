#include "test_utils.h"

#include "client.h"
#include "env_builder.h"
#include "generate_test_data.h"
#include "globals.h"
#include "log.h"
#include "my_malloc.h"
#include "option_parser.h"
#include "test1.h"
#include "test_storage_utils.h"

#include <assert.h>
#include <errno.h>
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

bool is_process_healthy( int pid )
{
	siginfo_t infop;
	memset( &infop, 0x00, sizeof( infop ) );
	int res = waitid( P_PID, pid, &infop, WEXITED | WNOHANG | WNOWAIT );
	if( res ) {
		LOG_ERROR( "res=d pid=d waitid failed", res, pid );
		perror( "waitid" );
		abort();
	}
	if( !infop.si_pid ) {
		return true;
	}
	return false;
}

pid_t run_server( const char* storage_path,
				  const char* crash_location,
				  int num_crashes,
				  int clock_time )
{
	int res;
	pid_t pid;

	struct env_builder eb;
	env_builder_init( &eb );

	env_builder_addf( &eb, "MENOETIUS_STORAGE_PATH=%s", storage_path );
	env_builder_addf( &eb, "MENOETIUS_CLOCK_OVERRIDE=%d", clock_time );
	if( crash_location ) {
		env_builder_addf( &eb, "MENOETIUS_CRASH_LOCATION=%s", crash_location );
		env_builder_addf( &eb, "MENOETIUS_NUM_BEFORE_CRASH=%d", num_crashes );
	}

	const char* log_level = getenv( "LOG_LEVEL" );
	if( log_level ) {
		env_builder_addf( &eb, "LOG_LEVEL=%s", log_level );
	}

	LOG_INFO(
		"crash_location=s num_before_crash=d starting server (and redirecting logs to server.log)",
		crash_location,
		num_crashes );

	pid = fork();
	if( pid < 0 ) {
		LOG_ERROR( "failed to fork" );
		assert( 0 );
	}
	if( pid == 0 ) {
		char* argv[] = {"server", NULL};

		// redirect stdout+stderr to a file
		int fd = open( "server.log", O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR );
		dup2( fd, STDOUT_FILENO );
		dup2( fd, STDERR_FILENO );
		close( fd );

		LOG_INFO( "starting server via execve" );

		for( char** s = eb.envs; *s; s++ ) {
			LOG_DEBUG( "env=s exported environment\n", *s );
		}

		res = execve( "server", argv, eb.envs );
		LOG_ERROR( "res=d execve returned", res );
		abort();
	}

	LOG_INFO( "server_pid=d started server", pid );
	return pid;
}

int kill_server( pid_t pid )
{
	LOG_DEBUG( "server=d killing server", pid );
	int res;
	res = kill( pid, 0 );
	if( res != 0 ) {
		LOG_ERROR( "server is not running" );
		assert( 0 );
	}

	res = kill( pid, SIGKILL );
	if( res != 0 ) {
		LOG_ERROR( "failed to kill server" );
		assert( 0 );
	}

	return 0;
}

int wait_for_server_to_respond( struct menoetius_client* client, int pid, int timeout_ms )
{
	int status;
	int res;

	int t = timeout_ms;

	while( t > 0 ) {
		if( pid > 0 && !is_process_healthy( pid ) ) {
			LOG_ERROR( "server_pid=d server has died", pid );
			return 1;
		}
		LOG_DEBUG( "checking server" );
		res = menoetius_client_get_status( client, &status );
		if( res == 0 && status == MENOETIUS_STATUS_OK ) {
			LOG_DEBUG( "server is ready" );
			return 0;
		}
		if( res ) {
			LOG_DEBUG( "res=d failed to get status from server; waiting", res );
		}
		else {
			LOG_DEBUG( "status=d server returned non-ready status; waiting", status );
		}
		usleep( 10 * 1000 );
		t -= 10;
	}
	if( timeout_ms > 0 ) {
		LOG_ERROR( "timeout=d timedout waiting for server", timeout_ms );
	}
	return 1;
}

bool start_server_if_needed( int* pid,
							 const char* storage_path,
							 int start_time,
							 const char* crash_location,
							 int num_success_before_crash )
{
	int res;
	if( *pid > 0 ) {

		siginfo_t infop;
		memset( &infop, 0x00, sizeof( infop ) );
		res = waitid( P_PID, *pid, &infop, WEXITED | WNOHANG );
		if( res ) {
			LOG_ERROR( "res=d pid=d waitid failed", res, *pid );
			perror( "waitid" );
			abort();
		}
		if( !infop.si_pid ) {
			LOG_DEBUG( "pid=d server is still running", *pid );
			return false;
		}
		LOG_ERROR( "status=d code=d server has exited", infop.si_status, infop.si_code );
	}

	*pid = run_server( storage_path, crash_location, num_success_before_crash, start_time );
	return true;
}

int shutdown_server_gracefully_and_wait( int pid )
{
	int res;
	LOG_INFO( "serverpid=d sending SIGTERM", pid );
	res = kill( pid, SIGINT );
	if( res != 0 ) {
		LOG_ERROR( "server_pid=d error=s failed to send shutdown signal", pid, strerror( errno ) );
		return 1;
	}
	return join_server_pid( pid );
}

int join_server_pid( int pid )
{
	int res;
	siginfo_t infop;
	memset( &infop, 0x00, sizeof( infop ) );
	res = waitid( P_PID, pid, &infop, WEXITED );
	if( res ) {
		LOG_ERROR( "res=d serverpid=d waitid failed", res, pid );
		perror( "waitid" );
		abort();
	}
	if( infop.si_code == CLD_EXITED ) {
		LOG_DEBUG( "status=d server exited", infop.si_status );
		return infop.si_status;
	}
	LOG_ERROR(
		"si_code=d si_status=d waitpid did not return CLD_EXITED", infop.si_code, infop.si_status );
	abort();
}

int validate_points( int64_t start_time,
					 int key_num,
					 int expected_num_returned_pts,
					 int actual_num_returned_pts,
					 int64_t* tt,
					 double* yy )
{
	int64_t t;
	double y;

	for( int k = 0; k < actual_num_returned_pts; k++ ) {
		LOG_DEBUG( "t=d y=f got value", tt[k], yy[k] );
	}

	// validate results (errors here are fatal since they are data corruption related)
	if( actual_num_returned_pts != expected_num_returned_pts ) {
		LOG_ERROR( "want=d got=d unexpected number of returned points",
				   expected_num_returned_pts,
				   actual_num_returned_pts );
		return 1;
	}

	for( int k = 0; k < actual_num_returned_pts; k++ ) {
		t = start_time + k;
		y = get_test_pt( ( k + 1 ) * ( key_num + 1 ) );
		if( t != tt[k] ) {
			LOG_ERROR( "want=d got=d unexpected t value", t, tt[k] );
			return 1;
		}
		if( y != yy[k] ) {
			LOG_ERROR( "want=d got=d unexpected y value", y, yy[k] );
			return 1;
		}
	}
	LOG_DEBUG( "num_pts=d validated points", actual_num_returned_pts );
	return 0;
}
