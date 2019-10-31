#define _GNU_SOURCE

#include "time_parser.h"

#include <string.h>

int parse_time( const char* s, time_t* t )
{
	struct tm tm;
	memset( &tm, 0, sizeof( struct tm ) );
	if( !strptime( s, "%Y-%m-%dT%H:%M:%S", &tm ) ) {
		return 1;
	}
	*t = timegm( &tm );
	return 0;
}
