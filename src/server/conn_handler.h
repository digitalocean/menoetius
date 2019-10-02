#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <pthread.h>

struct structured_stream;
struct conn_data;
struct conn_data_pool;
struct the_storage;
struct log_context;

typedef int ( *ConnHandler )( struct the_storage* storage, struct conn_data* data );

#define MAX_LFM_SIZE 1024
#define MAX_KEY_BUF ( 4 + 512 ) * 5000
#define MAX_CONN_FORMATTED_DATA_BUF_SIZE ( 32 * 1024 )

struct conn_data
{
	pthread_t thread;
	pthread_cond_t cond;
	pthread_mutex_t lock;

	bool has_valid_config;

	struct conn_data_pool* pool;
	struct the_storage* storage;

	struct structured_stream* ss;
	int fd;

	struct log_context* log_ctx;
};

struct conn_data_pool
{
	pthread_mutex_t lock;
	struct conn_data* pool;
	struct conn_data** free;
	int free_i;
	int max_conn;
};

int conn_data_pool_init( struct conn_data_pool** pool,
						 size_t num_conns,
						 size_t buf_size_per_conn,
						 struct the_storage* storage );
int conn_data_pool_get( struct conn_data_pool* pool, struct conn_data** ptr );
int conn_data_pool_release( struct conn_data_pool* pool, struct conn_data* ptr );
int conn_data_pool_handle_conn( struct conn_data_pool* pool, int fd );

int handle_conn_rpc_code( struct conn_data* data );
int handle_conn_rpc_entry( struct conn_data* data );

int handle_conn_get_data( struct conn_data* data );
int handle_conn_put_data( struct conn_data* data );
int handle_conn_replication_data( struct conn_data* data );

int handle_conn_get_status( struct conn_data* data );
int handle_conn_get_cluster_config( struct conn_data* data );
int handle_conn_set_cluster_config( struct conn_data* data );
int handle_conn_register_client_cluster_config( struct conn_data* data );
int handle_conn_get_version( struct conn_data* data );
