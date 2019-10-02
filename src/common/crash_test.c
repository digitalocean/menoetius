#include "crash_test.h"

#ifdef DEBUG_BUILD

#	include "log.h"
#	include <stdlib.h>
#	include <string.h>
#	include <assert.h>

int crash_test_location = CRASH_TEST_DISABLED;
int crash_test_num_before_crash = 0;

void crash_test_init( int location, int num_before_crash )
{
	crash_test_location = location;
	crash_test_num_before_crash = num_before_crash;
	if( location != CRASH_TEST_DISABLED ) {
		LOG_WARN( "location=d num=d server running under crash-mode", location, num_before_crash );
	}
}

void crash_test( int location, const char* caller_file, int caller_line )
{
	assert( location != CRASH_TEST_DISABLED );

	if( location != crash_test_location ) {
		LOG_DEBUG(
			"location=d crash_test_location=d caller_file=s caller_line=d ignoring crash_test",
			location,
			crash_test_location,
			caller_file,
			caller_line );
		return;
	}

	LOG_DEBUG( "sloc=d location=d num_before_crash=d caller_file=s caller_line=d crash_test",
			   crash_test_location,
			   location,
			   crash_test_num_before_crash,
			   caller_file,
			   caller_line );
	if( crash_test_num_before_crash == 0 ) {
		LOG_WARN(
			"caller_file=s caller_line=d intentially causing segfault", caller_file, caller_line );
		// Come on, you guys, there it is right there in front of you the whole time.
		// You're de-referencing a null pointer. Open your eyes.
		// https://www.youtube.com/watch?v=bLHL75H_VEM
		volatile int* p = NULL;
		*p = 0x1337D00D;
	}
	crash_test_num_before_crash--;
}

int crash_test_get_location_from_string( const char* location )
{
	if( !strcmp( location, "CRASH_TEST_DISABLED" ) ) {
		return CRASH_TEST_DISABLED;
	}
	if( !strcmp( location, "CRASH_TEST_LOCATION_1" ) ) {
		return CRASH_TEST_LOCATION_1;
	}
	if( !strcmp( location, "CRASH_TEST_LOCATION_2" ) ) {
		return CRASH_TEST_LOCATION_2;
	}
	if( !strcmp( location, "CRASH_TEST_WRITE" ) ) {
		return CRASH_TEST_WRITE;
	}
	if( !strcmp( location, "CRASH_TEST_AFTER_COMPLETE_BLOCK_WRITE" ) ) {
		return CRASH_TEST_AFTER_COMPLETE_BLOCK_WRITE;
	}
	LOG_ERROR( "location=s failed to parse crash location", location );
	abort();
}

ssize_t
maybe_write_( int fd, const void* buf, size_t count, const char* caller_file, int caller_line )
{
	if( crash_test_location != CRASH_TEST_WRITE ) {
		return write( fd, buf, count );
	}

	LOG_DEBUG( "num_before_crash=d caller_file=s caller_line=d crash_test",
			   crash_test_num_before_crash,
			   caller_file,
			   caller_line );

	if( count < crash_test_num_before_crash ) {
		crash_test_num_before_crash -= count;
		return write( fd, buf, count );
	}

	assert( crash_test_num_before_crash >= 0 );
	write( fd, buf, crash_test_num_before_crash );
	LOG_WARN( "caller_file=s caller_line=d intentially aborting due to CRASH_TEST_WRITE",
			  caller_file,
			  caller_line );
	abort();
}

#endif // DEBUG_BUILD
