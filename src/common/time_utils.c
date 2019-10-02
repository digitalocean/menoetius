#include "time_utils.h"

#include <stdbool.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

long long time_microseconds( void )
{
	struct timeval tv;
	gettimeofday( &tv, NULL );
	return 1000000L * tv.tv_sec + tv.tv_usec;
}

///////////////////////////////////////////////////
// Stubbed out time for unit testing
///////////////////////////////////////////////////

#ifdef DEBUG_BUILD

bool override_time = false;
time_t time_value = 0;

time_t my_time( time_t* tloc )
{
	if( override_time ) {
		return time_value;
	}
	return time( tloc );
}

void set_time( time_t t )
{
	override_time = true;
	time_value = t;
}
void add_time( time_t t )
{
	override_time = true;
	time_value += t;
}

#endif // DEBUG_BUILD
