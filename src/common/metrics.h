#pragma once

#include "http_metrics.h"

DECLARE_METRIC( num_connections );
DECLARE_METRIC( num_connections_accepted );
DECLARE_METRIC( num_connections_rejected );
DECLARE_METRIC( num_points_rejected );
DECLARE_METRIC( num_points_written );
DECLARE_METRIC( num_cleaning_cycles );
DECLARE_METRIC( num_timeseries );
DECLARE_METRIC( num_points );
DECLARE_METRIC( num_index_lookups );
DECLARE_METRIC( num_full_lookups );
DECLARE_METRIC( cleaning_item );
DECLARE_METRIC( cleaning_loop_duration );
DECLARE_METRIC( cleaning_loop_sleep );
DECLARE_METRIC( start_time );
DECLARE_METRIC( num_disk_reads );
DECLARE_METRIC( num_disk_writes );

DECLARE_METRIC( num_malloc_no_pool );
DECLARE_METRIC( num_free_no_pool );

DECLARE_METRIC( num_malloc_pool_0 );
DECLARE_METRIC( num_malloc_pool_1 );
DECLARE_METRIC( num_malloc_pool_2 );
DECLARE_METRIC( num_malloc_pool_3 );
DECLARE_METRIC( num_malloc_pool_4 );
DECLARE_METRIC( num_malloc_pool_5 );
DECLARE_METRIC( num_malloc_pool_6 );
DECLARE_METRIC( num_malloc_pool_7 );
DECLARE_METRIC( num_malloc_pool_8 );
DECLARE_METRIC( num_malloc_pool_9 );
DECLARE_METRIC( num_malloc_pool_10 );
DECLARE_METRIC( num_malloc_pool_11 );
DECLARE_METRIC( num_malloc_pool_12 );
DECLARE_METRIC( num_malloc_pool_13 );
DECLARE_METRIC( num_malloc_pool_14 );

DECLARE_METRIC( num_free_pool_0 );
DECLARE_METRIC( num_free_pool_1 );
DECLARE_METRIC( num_free_pool_2 );
DECLARE_METRIC( num_free_pool_3 );
DECLARE_METRIC( num_free_pool_4 );
DECLARE_METRIC( num_free_pool_5 );
DECLARE_METRIC( num_free_pool_6 );
DECLARE_METRIC( num_free_pool_7 );
DECLARE_METRIC( num_free_pool_8 );
DECLARE_METRIC( num_free_pool_9 );
DECLARE_METRIC( num_free_pool_10 );
DECLARE_METRIC( num_free_pool_11 );
DECLARE_METRIC( num_free_pool_12 );
DECLARE_METRIC( num_free_pool_13 );
DECLARE_METRIC( num_free_pool_14 );

DECLARE_METRIC( num_reuse_pool_0 );
DECLARE_METRIC( num_reuse_pool_1 );
DECLARE_METRIC( num_reuse_pool_2 );
DECLARE_METRIC( num_reuse_pool_3 );
DECLARE_METRIC( num_reuse_pool_4 );
DECLARE_METRIC( num_reuse_pool_5 );
DECLARE_METRIC( num_reuse_pool_6 );
DECLARE_METRIC( num_reuse_pool_7 );
DECLARE_METRIC( num_reuse_pool_8 );
DECLARE_METRIC( num_reuse_pool_9 );
DECLARE_METRIC( num_reuse_pool_10 );
DECLARE_METRIC( num_reuse_pool_11 );
DECLARE_METRIC( num_reuse_pool_12 );
DECLARE_METRIC( num_reuse_pool_13 );
DECLARE_METRIC( num_reuse_pool_14 );

DECLARE_METRIC( num_resizes );
DECLARE_METRIC( num_out_of_order );

// disk metrics
DECLARE_METRIC( num_disk_reserve_pool_0 );
DECLARE_METRIC( num_disk_reserve_pool_1 );
DECLARE_METRIC( num_disk_reserve_pool_2 );
DECLARE_METRIC( num_disk_reserve_pool_3 );
DECLARE_METRIC( num_disk_reserve_pool_4 );
DECLARE_METRIC( num_disk_reserve_pool_5 );
DECLARE_METRIC( num_disk_reserve_pool_6 );
DECLARE_METRIC( num_disk_reserve_pool_7 );
DECLARE_METRIC( num_disk_reserve_pool_8 );
DECLARE_METRIC( num_disk_reserve_pool_9 );
DECLARE_METRIC( num_disk_reserve_pool_10 );

DECLARE_METRIC( num_disk_release_pool_0 );
DECLARE_METRIC( num_disk_release_pool_1 );
DECLARE_METRIC( num_disk_release_pool_2 );
DECLARE_METRIC( num_disk_release_pool_3 );
DECLARE_METRIC( num_disk_release_pool_4 );
DECLARE_METRIC( num_disk_release_pool_5 );
DECLARE_METRIC( num_disk_release_pool_6 );
DECLARE_METRIC( num_disk_release_pool_7 );
DECLARE_METRIC( num_disk_release_pool_8 );
DECLARE_METRIC( num_disk_release_pool_9 );
DECLARE_METRIC( num_disk_release_pool_10 );

DECLARE_METRIC( num_disk_reuse_pool_0 );
DECLARE_METRIC( num_disk_reuse_pool_1 );
DECLARE_METRIC( num_disk_reuse_pool_2 );
DECLARE_METRIC( num_disk_reuse_pool_3 );
DECLARE_METRIC( num_disk_reuse_pool_4 );
DECLARE_METRIC( num_disk_reuse_pool_5 );
DECLARE_METRIC( num_disk_reuse_pool_6 );
DECLARE_METRIC( num_disk_reuse_pool_7 );
DECLARE_METRIC( num_disk_reuse_pool_8 );
DECLARE_METRIC( num_disk_reuse_pool_9 );
DECLARE_METRIC( num_disk_reuse_pool_10 );

DECLARE_METRIC( num_call_put_data );
DECLARE_METRIC( num_call_get_data );
DECLARE_METRIC( num_call_replication_data );
DECLARE_METRIC( num_call_query_lfms );
DECLARE_METRIC( num_call_set_clock );
DECLARE_METRIC( num_call_test_hook );
DECLARE_METRIC( num_call_get_status );
DECLARE_METRIC( num_call_unhandled );
DECLARE_METRIC( num_call_get_cluster_config );
DECLARE_METRIC( num_call_set_cluster_config );
DECLARE_METRIC( num_call_get_version );
DECLARE_METRIC( num_call_register_client_cluster_config );

DECLARE_METRIC( bytes_loaded );

void register_metrics();
