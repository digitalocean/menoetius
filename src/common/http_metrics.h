#pragma once

#include <microhttpd.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <stdio.h> // TODO dont save this

#define MAX_METRIC_NAME 128

struct registered_metric
{
	char name[MAX_METRIC_NAME];
	long long* data;
	struct registered_metric* next;
};

struct http_metrics_server
{
	int port;
	struct MHD_Daemon* daemon;

	struct registered_metric* metrics;
	unsigned long start_time;
};

int http_metrics_start( struct http_metrics_server* server );

int http_metrics_stop_and_join( struct http_metrics_server* server );

int status_handler( struct MHD_Connection* connection, int status_code, const char* fmt, ... );

int web_handler( void* cls,
				 struct MHD_Connection* connection,
				 const char* url,
				 const char* method,
				 const char* version,
				 const char* upload_data,
				 size_t* upload_data_size,
				 void** con_cls );

void http_metrics_register( struct http_metrics_server* server, const char* name, long long* val );

#define DECLARE_METRIC( NAME )                                                                     \
	extern const char* http_metric_##NAME##_str;                                                   \
	extern long long http_metric_##NAME##_val;

#define DEFINE_METRIC( NAME )                                                                      \
	const char* http_metric_##NAME##_str = #NAME;                                                  \
	long long http_metric_##NAME##_val = 0;

#define DEFINE_METRIC_CUSTOM_NAME( NAME, CUSTOM_NAME )                                             \
	const char* http_metric_##NAME##_str = CUSTOM_NAME;                                            \
	long long http_metric_##NAME##_val = 0;

#define INCR_METRIC( NAME ) __sync_fetch_and_add( &( http_metric_##NAME##_val ), 1 );

#define ADD_METRIC( NAME, VALUE ) __sync_fetch_and_add( &( http_metric_##NAME##_val ), VALUE );

#define DECR_METRIC( NAME ) __sync_fetch_and_add( &( http_metric_##NAME##_val ), -1 );

#define SET_METRIC( NAME, VALUE )                                                                  \
	http_metric_##NAME##_val = VALUE;                                                              \
	__sync_synchronize();

#define GET_METRIC( NAME ) __sync_fetch_and_add( &( http_metric_##NAME##_val ), 0 );

#define REGISTER_METRIC( SERVER, NAME )                                                            \
	http_metrics_register( SERVER, http_metric_##NAME##_str, &( http_metric_##NAME##_val ) );
