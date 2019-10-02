#include "http_metrics.h"

#include "my_malloc.h"
#include "time_utils.h"

#include <assert.h>
#include <microhttpd.h>
#include <stdio.h>
#include <string.h>

#define MAX_BUF_SIZE 5 * 1024

#ifdef DEBUG_BUILD
#	define BUILD_TYPE "debug"
#else
#	define BUILD_TYPE "release"
#endif // DEBUG_BUILD

int http_metrics_start( struct http_metrics_server* server )
{
	server->start_time = my_time( NULL );
	server->daemon = MHD_start_daemon( MHD_USE_SELECT_INTERNALLY,
									   server->port,
									   NULL,
									   NULL,
									   &web_handler,
									   (void*)server,
									   MHD_OPTION_LISTENING_ADDRESS_REUSE,
									   1,
									   MHD_OPTION_END );
	if( NULL == server->daemon )
		return 1;
	return 0;
}

int http_metrics_stop_and_join( struct http_metrics_server* server )
{
	MHD_stop_daemon( server->daemon );
	return 0;
}

void http_metrics_register( struct http_metrics_server* server, const char* name, long long* val )
{
	struct registered_metric* p = my_malloc( sizeof( struct registered_metric ) );
	assert( strlen( name ) < MAX_METRIC_NAME );
	strcpy( p->name, name );
	p->data = val;
	p->next = server->metrics;
	server->metrics = p;
}

int metrics_handler( void* cls,
					 struct MHD_Connection* connection,
					 const char* url,
					 const char* method,
					 const char* version,
					 const char* upload_data,
					 size_t* upload_data_size,
					 void** con_cls )
{
	struct http_metrics_server* server = (struct http_metrics_server*)cls;

	char buf[MAX_BUF_SIZE];
	int i = 0;
	int res;
	long long val;

	for( struct registered_metric* m = server->metrics; m != NULL; m = m->next ) {
		val = __sync_fetch_and_add( m->data, 0 ); // TODO ensure this works with long long (int64)
		res = snprintf( &buf[i], MAX_BUF_SIZE - i, "%s %lld\n", m->name, val );
		if( res < 0 ) {
			break; // TODO handle error
		}
		i += res;
	}
	buf[MAX_BUF_SIZE - 1] = '\0';

	struct MHD_Response* response =
		MHD_create_response_from_buffer( strlen( buf ), (void*)buf, MHD_RESPMEM_MUST_COPY );
	if( response == NULL ) {
		return MHD_NO;
	}
	int ret = MHD_queue_response( connection, MHD_HTTP_OK, response );
	MHD_destroy_response( response );
	return ret;
}

int web_handler( void* cls,
				 struct MHD_Connection* connection,
				 const char* url,
				 const char* method,
				 const char* version,
				 const char* upload_data,
				 size_t* upload_data_size,
				 void** con_cls )
{
	struct http_metrics_server* server = (struct http_metrics_server*)cls;

	if( strcmp( method, "GET" ) == 0 && strcmp( url, "/metrics" ) == 0 ) {
		return metrics_handler(
			cls, connection, url, method, version, upload_data, upload_data_size, con_cls );
	}

	int uptime = my_time( NULL ) - server->start_time;

	if( strcmp( method, "GET" ) == 0 && strcmp( url, "/" ) == 0 ) {
		return status_handler(
			connection,
			MHD_HTTP_OK,
			"build type: %s\nbuild time: %s\nbuild version: %s\nuptime seconds: %d\n",
			BUILD_TYPE,
			BUILD_TIME,
			GIT_COMMIT,
			uptime );
	}

	return status_handler( connection, MHD_HTTP_BAD_REQUEST, "failed to dispatch: %s", url );
}

int status_handler( struct MHD_Connection* connection, int status_code, const char* fmt, ... )
{
	char buf[1024];
	va_list args;
	va_start( args, fmt );
	vsnprintf( buf, 1024, fmt, args );
	va_end( args );

	struct MHD_Response* response =
		MHD_create_response_from_buffer( strlen( buf ), (void*)buf, MHD_RESPMEM_MUST_COPY );
	if( response == NULL ) {
		return MHD_NO;
	}
	int ret = MHD_queue_response( connection, status_code, response );
	MHD_destroy_response( response );
	return ret;
}
