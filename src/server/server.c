#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <signal.h>

// common
#include "crash_test.h"
#include "env_parse.h"
#include "globals.h"
#include "http_metrics.h"
#include "log.h"
#include "lseek_utils.h"
#include "mkdir_p.h"
#include "my_malloc.h"
#include "my_utils.h"
#include "structured_stream.h"
#include "time_utils.h"

// server-specific
#include "conn_handler.h"
#include "metrics.h"
#include "storage.h"

bool shutdown_requested = false;
pthread_mutex_t shutdown_lock;
pthread_cond_t shutdown_cond;
pthread_t thread;

void sig_handler( int signo )
{
	if( signo == SIGINT || signo == SIGTERM ) {
		pthread_mutex_lock( &shutdown_lock );
		shutdown_requested = true;
		pthread_cond_signal( &shutdown_cond );
		pthread_mutex_unlock( &shutdown_lock );
	}
}

void* shutdown_thread( void* storage )
{
	int res;
	struct timespec timeout;
	timeout.tv_nsec = 0;

	pthread_mutex_lock( &shutdown_lock );
	while( !shutdown_requested ) {
		timeout.tv_sec = time( NULL ) + 1;
		res = pthread_cond_timedwait( &shutdown_cond, &shutdown_lock, &timeout );
		assert( res == 0 || res == ETIMEDOUT );
		pthread_mutex_unlock( &shutdown_lock );
	}

	LOG_INFO( "syncing data before shutting down" );
	res = storage_sync( (struct the_storage*)storage,
						true /* deadlock the server to prevent future connections */ );
	if( res ) {
		LOG_ERROR( "res=d failed to sync" );
	}

	LOG_INFO( "syncing data complete; shutting down" );
	exit( 0 );
}

void setup_shutdown_handler( struct the_storage* storage )
{
	assert( pthread_mutex_init( &shutdown_lock, NULL ) == 0 );
	assert( pthread_cond_init( &shutdown_cond, NULL ) == 0 );

	pthread_create( &thread, NULL, shutdown_thread, storage );

	if( signal( SIGINT, sig_handler ) == SIG_ERR ) {
		LOG_ERROR( "failed to setup SIGINT handler" );
		exit( 1 );
	}
	if( signal( SIGTERM, sig_handler ) == SIG_ERR ) {
		LOG_ERROR( "failed to setup SIGTERM handler" );
		exit( 1 );
	}
}

int make_socket( uint16_t port )
{
	int option;
	int sock;
	struct sockaddr_in name;

	/* Create the socket. */
	sock = socket( PF_INET, SOCK_STREAM, 0 );
	if( sock < 0 ) {
		perror( "socket" );
		exit( EXIT_FAILURE );
	}

	/* allow reuse of bounded ports to allow for quick server restarts */
	option = 1;
	setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof( option ) );

	/* Give the socket a name. */
	name.sin_family = AF_INET;
	name.sin_port = htons( port );
	name.sin_addr.s_addr = htonl( INADDR_ANY );
	if( bind( sock, (struct sockaddr*)&name, sizeof( name ) ) < 0 ) {
		perror( "bind" );
		exit( EXIT_FAILURE );
	}

	return sock;
}

static __thread char addr_buffer[64];
const char* format_addr( struct sockaddr_in* sin )
{
	snprintf( addr_buffer, 64, "%s:%d", inet_ntoa( sin->sin_addr ), sin->sin_port );
	addr_buffer[63] = '\0'; // ensure it is null terminated
	return addr_buffer;
}

void parse_label_indices( char* label_indices_config, char*** indices, int* num_indices )
{
	int n = 1;
	for( char* s = label_indices_config; *s; s++ ) {
		if( *s == ',' ) {
			n++;
		}
	}
	*indices = (char**)malloc( sizeof( char* ) * ( n ) );

	int i = 0;
	( *indices )[i++] = label_indices_config;
	for( char* s = label_indices_config; *s; s++ ) {
		if( *s == ',' ) {
			*s = '\x00';
			( *indices )[i++] = s + 1;
		}
	}
	assert( i == n );
	*num_indices = n;
}

void acquire_storage_lock( const char* storage_path )
{
	int res;
	char* s = my_malloc( strlen( storage_path ) + 1000 );
	sprintf( s, "%s/lock", storage_path );

	char buf[1024];
	int fd = open( s, O_RDWR | O_CREAT, 0644 );
	int n = read( fd, buf, 1024 );
	if( n > 1024 ) {
		LOG_ERROR( "lockpath=s lock file larger than expected", s );
		exit( 1 );
	}
	if( n < 0 ) {
		LOG_ERROR( "lockpath=s failed to read lock file", s );
		exit( 1 );
	}
	buf[n] = '\0';
	if( n > 0 ) {
		// check if pid exists
		int pid = atoi( buf );
		if( pid == 0 ) {
			LOG_ERROR( "lockpath=s failed to read lock file", s );
			exit( 1 );
		}

		res = kill( pid, 0 );
		if( res == 0 ) {
			LOG_ERROR( "lockpath=s lockpid=d server is already running (or another process with "
					   "the same pid)",
					   s,
					   pid );
			exit( 1 );
		}
		if( errno != ESRCH ) {
			LOG_ERROR(
				"lockpath=s lockpid=d err=s unable to determine if server is already running",
				s,
				pid,
				strerror( errno ) );
			exit( 1 );
		}
		LOG_INFO( "lockpath=s lockpid=d server is not already running", s, pid );
	}

	must_lseek( fd, 0, SEEK_SET );
	res = ftruncate( fd, 0 );
	if( res ) {
		LOG_ERROR( "lockpath=s err=s failed to truncate existing lock file", s, strerror( errno ) );
		exit( 1 );
	}
	n = sprintf( buf, "%d", getpid() );
	MUST_WRITE( fd, buf, n );
	close( fd );

	LOG_INFO( "lockpath=s aquired lock file", s );
	my_free( s );
}

int main( int argc, const char** argv, const char** env )
{
	int res;
	int sfd, s;

	if( argc > 1 ) {
		// TODO improve env parsing so we can print them all out here with a help message
		fprintf(
			stderr,
			"command takes no arguments\nyou might want to override MENOETIUS_STORAGE_PATH\n" );
		return 1;
	}

	// ensure this is initialized
	errno = 0;

	my_malloc_init();
	set_log_level_from_env_variables( env );

#ifdef DEBUG_BUILD
	// used by integration testing to simulate recoverying from crashes without corrupting on-disk data
	int location = crash_test_get_location_from_string(
		env_parse_get_str( "MENOETIUS_CRASH_LOCATION", "CRASH_TEST_DISABLED" ) );
	int num_before_crash = env_parse_get_int( "MENOETIUS_NUM_BEFORE_CRASH", 0 );
	crash_test_init( location, num_before_crash );

	// used by integration testing to control which points are accepted/rejected/trimmed
	int clock_override = env_parse_get_int( "MENOETIUS_CLOCK_OVERRIDE", 0 );
	if( clock_override ) {
		LOG_INFO( "t=d changing clock time", clock_override );
		set_time( clock_override );
	}

	// used by integration testing to control when cleaning occurs (via the MENOETIUS_RPC_TEST_HOOK call)
	const int enable_cleaning_thread = env_parse_get_int( "MENOETIUS_ENABLE_CLEANING_THREAD", 1 );

#endif //DEBUG_BUILD

	// dynamic index
	int num_indices = 0;
	char** indices = NULL;

	struct http_metrics_server server;
	struct the_storage my_storage;

	const int menoetius_port = env_parse_get_int( "MENOETIUS_PORT", MENOETIUS_PORT );
	const int metrics_port = env_parse_get_int( "MENOETIUS_METRIC_PORT", MENOETIUS_METRIC_PORT );
	const int max_conn = env_parse_get_int( "MENOETIUS_MAX_CONN", 256 );
	const int buf_size_per_conn = env_parse_get_int( "MENOETIUS_CONN_BUF_SIZE", 128 * 1024 );
	const int retention_seconds = env_parse_get_int( "MENOETIUS_RETENTION_SECONDS", 60 * 60 * 4 );
	const int target_cleaning_loop_seconds =
		env_parse_get_int( "MENOETIUS_CLEANING_LOOP_SECONDS", 60 * 5 ); // in seconds
	const int cleaning_sleep_multiplier = env_parse_get_int(
		"MENOETIUS_SLEEP_MULTIPLIER",
		3 ); // after cleaning for X many seconds, sleep for X * MENOETIUS_SLEEP_MULTIPLIER seconds
	const int max_duration =
		env_parse_get_int( "MENOETIUS_MAX_DURATION", 10000 ); // in microseconds

	// the memory pool will add 8 bytes to this amount; each pool size is a multiple of 1024;
	// so to allocate a full 1k of memory sub 8 bytes.
	const int initial_tsc_buf_size =
		env_parse_get_int( "MENOETIUS_INITIAL_TSC_BUF_SIZE", 1024 - 8 );

	char* storage_path = env_parse_get_str( "MENOETIUS_STORAGE_PATH", "/var/lib/menoetius" );

	res = mkdir_p( storage_path, 0755 );
	if( res ) {
		LOG_ERROR( "err=s path=s failed to create storage path", strerror( errno ), storage_path );
	}

	// the index furthest to the right takes precedence
	char* label_indices_config =
		env_parse_get_str( "MENOETIUS_INDICES", "user_id,hypervisor_hostname,host_id,instance" );
	parse_label_indices( label_indices_config, &indices, &num_indices );

	acquire_storage_lock( storage_path );

	// start http metrics server
	memset( &server, 0, sizeof( struct http_metrics_server ) );
	server.port = metrics_port;
	register_metrics( &server );
	res = http_metrics_start( &server );
	if( res != 0 ) {
		LOG_ERROR( "res=d err=s failed to start http metrics webserver", res, strerror( errno ) );
		return 1;
	}
	LOG_INFO( "port=d started http metrics webserver", server.port );

	for( int i = 0; i < num_indices; i++ ) {
		LOG_INFO( "lab=s label index", indices[i] );
	}

	// retention_seconds = 10;
	if( ( res = storage_init( &my_storage,
							  retention_seconds,
							  target_cleaning_loop_seconds,
							  initial_tsc_buf_size,
							  num_indices,
							  (const char**)indices,
							  storage_path ) ) ) {
		LOG_ERROR( "res=d failed to init storage" );
		return 1;
	}
	my_storage.cleaning_sleep_multiplier = cleaning_sleep_multiplier;
	my_storage.max_duration = max_duration;

#ifdef DEBUG_BUILD
	if( enable_cleaning_thread ) {
#endif
		if( ( res = storage_start_thread( &my_storage ) ) ) {
			LOG_ERROR( "res=d failed to init storage" );
			return 1;
		}
#ifdef DEBUG_BUILD
	}
	else {
		LOG_WARN( "storage cleaning thread has been disabled" );
	}
#endif

	setup_shutdown_handler( &my_storage );

	struct conn_data_pool* pool;
	assert( conn_data_pool_init( &pool, max_conn, buf_size_per_conn, &my_storage ) == 0 );

	sfd = make_socket( menoetius_port );
	if( sfd == -1 )
		abort();

	s = listen( sfd, SOMAXCONN );
	if( s == -1 ) {
		perror( "listen" );
		abort();
	}
	LOG_INFO( "port=d listening for connections", menoetius_port );

	while( 1 ) {
		struct sockaddr in_addr;
		socklen_t in_len;
		int infd;

		in_len = sizeof in_addr;
		infd = accept( sfd, &in_addr, &in_len );
		if( infd == -1 ) {
			perror( "accept" );
			exit( 1 );
		}

		LOG_INFO( "fd=d client=s accepted connection",
				  infd,
				  format_addr( (struct sockaddr_in*)&in_addr ) );

		res = conn_data_pool_handle_conn( pool, infd );
		if( res ) {
			LOG_ERROR( "fd=d max connections reached; closing connection", infd );
			close( infd );
			INCR_METRIC( num_connections_rejected );
		}
		else {
			INCR_METRIC( num_connections_accepted );
		}
	}

	close( sfd );

	if( label_indices_config != NULL ) {
		free( label_indices_config );
		label_indices_config = NULL;
	}
	if( indices != NULL ) {
		free( indices );
		indices = NULL;
	}

	my_malloc_free();
	return EXIT_SUCCESS;
}
