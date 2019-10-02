#include "mkdir_p.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* recursive mkdir */
// from https://github.com/dark777/standard-cxx/blob/5ee9301639fd32d10e6b4a6365a9501fbd1c411a/linux_unix/diretorios/example1.c#L10
int mkdir_p( const char* dir, const mode_t mode )
{
	char tmp[10240];
	char* p = NULL;
	struct stat sb;
	size_t len;

	/* copy path */
	strncpy( tmp, dir, sizeof( tmp ) );
	len = strlen( dir );
	if( len >= sizeof( tmp ) ) {
		return -1;
	}

	/* remove trailing slash */
	if( tmp[len - 1] == '/' ) {
		tmp[len - 1] = 0;
	}

	/* recursive mkdir */
	for( p = tmp + 1; *p; p++ ) {
		if( *p == '/' ) {
			*p = 0;
			/* test path */
			if( stat( tmp, &sb ) != 0 ) {
				/* path does not exist - create directory */
				if( mkdir( tmp, mode ) < 0 ) {
					return -1;
				}
			}
			else if( !S_ISDIR( sb.st_mode ) ) {
				/* not a directory */
				return -1;
			}
			*p = '/';
		}
	}
	/* test path */
	if( stat( tmp, &sb ) != 0 ) {
		/* path does not exist - create directory */
		if( mkdir( tmp, mode ) < 0 ) {
			return -1;
		}
	}
	else if( !S_ISDIR( sb.st_mode ) ) {
		/* not a directory */
		return -1;
	}
	return 0;
}
