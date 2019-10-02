#pragma once

#include <time.h>

long long time_microseconds( void );

#ifdef DEBUG_BUILD
time_t my_time( time_t* tloc );
void set_time( time_t t );
void add_time( time_t t );
#else
#	define my_time time
#endif // DEBUG_BUILD
