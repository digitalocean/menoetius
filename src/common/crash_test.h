#pragma once

#define CRASH_TEST_DISABLED 0
#define CRASH_TEST_LOCATION_1 1
#define CRASH_TEST_LOCATION_2 2
#define CRASH_TEST_WRITE 3
#define CRASH_TEST_AFTER_COMPLETE_BLOCK_WRITE 4

#ifdef DEBUG_BUILD

#	include <unistd.h>

int crash_test_get_location_from_string( const char* location );

void crash_test_init( int location, int num_before_crash );

void crash_test_set_num_calls_before_crash( int num );

void crash_test( int location, const char* calling_file, int calling_line );

#	define CRASH_TEST( code, location )                                                           \
		do {                                                                                       \
			code;                                                                                  \
			crash_test( location, __FILE__, __LINE__ );                                            \
		} while( 0 );

ssize_t
maybe_write_( int fd, const void* buf, size_t count, const char* caller_file, int caller_line );
#	define maybe_write( fd, buf, count ) maybe_write_( fd, buf, count, __FILE__, __LINE__ )

#else

#	define CRASH_TEST( code, location ) code;
#	define maybe_write write

#endif // DEBUG_BUILD
