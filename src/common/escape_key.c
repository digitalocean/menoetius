#include "escape_key.h"

#include <string.h>

int escape_key( const char* s, size_t n, char* buf, size_t buf_size )
{
	int j = 0;
	for( int i = 0; i < n; i++ ) {
		if( s[i] == '\0' ) {
			if( ( j + 4 ) >= buf_size ) {
				return 1;
			}
			memcpy( &buf[j], "\\x00", 4 );
			j += 4;
		}
		else {
			if( j >= buf_size ) {
				return 1;
			}
			buf[j] = s[i];
			j++;
		}
	}
	if( j >= buf_size ) {
		return 1;
	}
	buf[j] = '\0';
	return 0;
}
