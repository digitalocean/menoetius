#pragma once

#include <stddef.h>
#include <stdint.h>

struct menoetius_client
{
	const char* server;
	int port;
	int read_buf_size;
	int write_buf_size;

	int fd;
	struct structured_stream* ss;
};

int menoetius_client_init( struct menoetius_client* client, const char* server, int port );

int menoetius_client_send( struct menoetius_client* client,
						   const char* key,
						   size_t key_len,
						   size_t num_pts,
						   int64_t* t,
						   double* y );

int menoetius_client_get( struct menoetius_client* client,
						  const char* key,
						  size_t key_len,
						  size_t max_num_pts,
						  size_t* num_pts,
						  int64_t* t,
						  double* y );

int menoetius_client_get_status( struct menoetius_client* client, int* status );

int menoetius_client_test_hook( struct menoetius_client* client, uint64_t flags );
