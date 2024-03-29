#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct KeyValue;

struct menoetius_client
{
	const char* server;
	int port;
	int read_buf_size;
	int write_buf_size;

	int fd;
	struct structured_stream* ss;

	// used to queue sent metrics before flushing them
	char* queued_metrics;
	int queued_metrics_max_size;
	int queued_metrics_len;

	int num_queued_metrics;
	int num_queued_metrics_flush;
};

#define MAX_BINARY_LFM_SIZE 1024
struct binary_lfm
{
	char lfm[MAX_BINARY_LFM_SIZE];
	size_t n;
};

int menoetius_client_init( struct menoetius_client* client, const char* server, int port );

int menoetius_client_send_sync( struct menoetius_client* client,
								const char* key,
								size_t key_len,
								size_t num_pts,
								int64_t* t,
								double* y );

int menoetius_client_send( struct menoetius_client* client,
						   const char* key,
						   size_t key_len,
						   size_t num_pts,
						   int64_t* t,
						   double* y );

int menoetius_client_flush( struct menoetius_client* client );

int menoetius_client_get( struct menoetius_client* client,
						  const char* key,
						  size_t key_len,
						  size_t max_num_pts,
						  size_t* num_pts,
						  int64_t* t,
						  double* y );

int menoetius_client_query( struct menoetius_client* client,
							const struct KeyValue* matchers,
							size_t num_matchers,
							bool allow_full_scan,
							struct binary_lfm* results,
							size_t max_results,
							size_t* num_results );

int menoetius_client_get_status( struct menoetius_client* client, int* status );

int menoetius_client_get_config( struct menoetius_client* client,
								 char* cluster_config,
								 size_t MAX_CONFIG_SIZE );

int menoetius_client_set_config( struct menoetius_client* client, const char* cluster_config );

int menoetius_client_test_hook( struct menoetius_client* client, uint64_t flags );
