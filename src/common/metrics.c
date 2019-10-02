#include "metrics.h"

#include "time_utils.h"

#include <string.h>

DEFINE_METRIC( cleaning_item );
DEFINE_METRIC( cleaning_loop_duration );
DEFINE_METRIC( cleaning_loop_sleep );
DEFINE_METRIC( num_cleaning_cycles );
DEFINE_METRIC( num_connections );
DEFINE_METRIC( num_connections_accepted );
DEFINE_METRIC( num_connections_rejected );
DEFINE_METRIC( num_full_lookups );
DEFINE_METRIC( num_index_lookups );
DEFINE_METRIC( num_points );
DEFINE_METRIC( num_points_rejected );
DEFINE_METRIC( num_points_written );
DEFINE_METRIC( num_timeseries );
DEFINE_METRIC( start_time );
DEFINE_METRIC( num_disk_reads );
DEFINE_METRIC( num_disk_writes );
DEFINE_METRIC( bytes_loaded );

DEFINE_METRIC( num_malloc_no_pool );
DEFINE_METRIC( num_free_no_pool );

DEFINE_METRIC_CUSTOM_NAME( num_malloc_pool_0, "num_malloc{pool=\"0\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_malloc_pool_1, "num_malloc{pool=\"1\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_malloc_pool_2, "num_malloc{pool=\"2\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_malloc_pool_3, "num_malloc{pool=\"3\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_malloc_pool_4, "num_malloc{pool=\"4\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_malloc_pool_5, "num_malloc{pool=\"5\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_malloc_pool_6, "num_malloc{pool=\"6\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_malloc_pool_7, "num_malloc{pool=\"7\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_malloc_pool_8, "num_malloc{pool=\"8\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_malloc_pool_9, "num_malloc{pool=\"9\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_malloc_pool_10, "num_malloc{pool=\"10\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_malloc_pool_11, "num_malloc{pool=\"11\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_malloc_pool_12, "num_malloc{pool=\"12\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_malloc_pool_13, "num_malloc{pool=\"13\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_malloc_pool_14, "num_malloc{pool=\"14\"}" );

DEFINE_METRIC_CUSTOM_NAME( num_free_pool_0, "num_free{pool=\"0\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_free_pool_1, "num_free{pool=\"1\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_free_pool_2, "num_free{pool=\"2\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_free_pool_3, "num_free{pool=\"3\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_free_pool_4, "num_free{pool=\"4\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_free_pool_5, "num_free{pool=\"5\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_free_pool_6, "num_free{pool=\"6\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_free_pool_7, "num_free{pool=\"7\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_free_pool_8, "num_free{pool=\"8\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_free_pool_9, "num_free{pool=\"9\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_free_pool_10, "num_free{pool=\"10\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_free_pool_11, "num_free{pool=\"11\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_free_pool_12, "num_free{pool=\"12\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_free_pool_13, "num_free{pool=\"13\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_free_pool_14, "num_free{pool=\"14\"}" );

DEFINE_METRIC_CUSTOM_NAME( num_reuse_pool_0, "num_reuse{pool=\"0\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_reuse_pool_1, "num_reuse{pool=\"1\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_reuse_pool_2, "num_reuse{pool=\"2\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_reuse_pool_3, "num_reuse{pool=\"3\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_reuse_pool_4, "num_reuse{pool=\"4\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_reuse_pool_5, "num_reuse{pool=\"5\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_reuse_pool_6, "num_reuse{pool=\"6\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_reuse_pool_7, "num_reuse{pool=\"7\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_reuse_pool_8, "num_reuse{pool=\"8\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_reuse_pool_9, "num_reuse{pool=\"9\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_reuse_pool_10, "num_reuse{pool=\"10\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_reuse_pool_11, "num_reuse{pool=\"11\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_reuse_pool_12, "num_reuse{pool=\"12\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_reuse_pool_13, "num_reuse{pool=\"13\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_reuse_pool_14, "num_reuse{pool=\"14\"}" );

DEFINE_METRIC( num_resizes );
DEFINE_METRIC( num_out_of_order );

DEFINE_METRIC_CUSTOM_NAME( num_disk_reserve_pool_0, "num_disk_reserve{pool=\"0\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_reserve_pool_1, "num_disk_reserve{pool=\"1\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_reserve_pool_2, "num_disk_reserve{pool=\"2\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_reserve_pool_3, "num_disk_reserve{pool=\"3\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_reserve_pool_4, "num_disk_reserve{pool=\"4\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_reserve_pool_5, "num_disk_reserve{pool=\"5\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_reserve_pool_6, "num_disk_reserve{pool=\"6\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_reserve_pool_7, "num_disk_reserve{pool=\"7\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_reserve_pool_8, "num_disk_reserve{pool=\"8\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_reserve_pool_9, "num_disk_reserve{pool=\"9\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_reserve_pool_10, "num_disk_reserve{pool=\"10\"}" );

DEFINE_METRIC_CUSTOM_NAME( num_disk_release_pool_0, "num_disk_release{pool=\"0\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_release_pool_1, "num_disk_release{pool=\"1\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_release_pool_2, "num_disk_release{pool=\"2\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_release_pool_3, "num_disk_release{pool=\"3\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_release_pool_4, "num_disk_release{pool=\"4\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_release_pool_5, "num_disk_release{pool=\"5\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_release_pool_6, "num_disk_release{pool=\"6\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_release_pool_7, "num_disk_release{pool=\"7\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_release_pool_8, "num_disk_release{pool=\"8\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_release_pool_9, "num_disk_release{pool=\"9\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_release_pool_10, "num_disk_release{pool=\"10\"}" );

DEFINE_METRIC_CUSTOM_NAME( num_disk_reuse_pool_0, "num_disk_reuse{pool=\"0\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_reuse_pool_1, "num_disk_reuse{pool=\"1\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_reuse_pool_2, "num_disk_reuse{pool=\"2\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_reuse_pool_3, "num_disk_reuse{pool=\"3\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_reuse_pool_4, "num_disk_reuse{pool=\"4\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_reuse_pool_5, "num_disk_reuse{pool=\"5\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_reuse_pool_6, "num_disk_reuse{pool=\"6\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_reuse_pool_7, "num_disk_reuse{pool=\"7\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_reuse_pool_8, "num_disk_reuse{pool=\"8\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_reuse_pool_9, "num_disk_reuse{pool=\"9\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_disk_reuse_pool_10, "num_disk_reuse{pool=\"10\"}" );

DEFINE_METRIC_CUSTOM_NAME( num_call_put_data, "num_call{rpc=\"put_data\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_call_get_data, "num_call{rpc=\"get_data\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_call_replication_data, "num_call{rpc=\"replication_data\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_call_query_lfms, "num_call{rpc=\"query_lfms\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_call_set_clock, "num_call{rpc=\"set_clock\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_call_test_hook, "num_call{rpc=\"test_hook\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_call_get_status, "num_call{rpc=\"get_status\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_call_get_cluster_config, "num_call{rpc=\"get_cluster_config\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_call_set_cluster_config, "num_call{rpc=\"set_cluster_config\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_call_register_client_cluster_config,
						   "num_call{rpc=\"register_cluster_config\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_call_get_version, "num_call{rpc=\"get_version\"}" );
DEFINE_METRIC_CUSTOM_NAME( num_call_unhandled, "num_call{rpc=\"unhandled\"}" );

void register_metrics( struct http_metrics_server* server )
{
	REGISTER_METRIC( server, cleaning_item );
	REGISTER_METRIC( server, cleaning_loop_duration );
	REGISTER_METRIC( server, cleaning_loop_sleep );
	REGISTER_METRIC( server, num_cleaning_cycles );
	REGISTER_METRIC( server, num_connections );
	REGISTER_METRIC( server, num_connections_accepted );
	REGISTER_METRIC( server, num_connections_rejected );
	REGISTER_METRIC( server, num_full_lookups );
	REGISTER_METRIC( server, num_index_lookups );
	REGISTER_METRIC( server, num_points );
	REGISTER_METRIC( server, num_points_rejected );
	REGISTER_METRIC( server, num_points_written );
	REGISTER_METRIC( server, num_timeseries );
	REGISTER_METRIC( server, start_time );
	REGISTER_METRIC( server, num_disk_reads );
	REGISTER_METRIC( server, num_disk_writes );
	REGISTER_METRIC( server, bytes_loaded );

	REGISTER_METRIC( server, num_malloc_no_pool );
	REGISTER_METRIC( server, num_free_no_pool );

	REGISTER_METRIC( server, num_malloc_pool_0 );
	REGISTER_METRIC( server, num_malloc_pool_1 );
	REGISTER_METRIC( server, num_malloc_pool_2 );
	REGISTER_METRIC( server, num_malloc_pool_3 );
	REGISTER_METRIC( server, num_malloc_pool_4 );
	REGISTER_METRIC( server, num_malloc_pool_5 );
	REGISTER_METRIC( server, num_malloc_pool_6 );
	REGISTER_METRIC( server, num_malloc_pool_7 );
	REGISTER_METRIC( server, num_malloc_pool_8 );
	REGISTER_METRIC( server, num_malloc_pool_9 );
	REGISTER_METRIC( server, num_malloc_pool_10 );
	REGISTER_METRIC( server, num_malloc_pool_11 );
	REGISTER_METRIC( server, num_malloc_pool_12 );
	REGISTER_METRIC( server, num_malloc_pool_13 );
	REGISTER_METRIC( server, num_malloc_pool_14 );

	REGISTER_METRIC( server, num_free_pool_0 );
	REGISTER_METRIC( server, num_free_pool_1 );
	REGISTER_METRIC( server, num_free_pool_2 );
	REGISTER_METRIC( server, num_free_pool_3 );
	REGISTER_METRIC( server, num_free_pool_4 );
	REGISTER_METRIC( server, num_free_pool_5 );
	REGISTER_METRIC( server, num_free_pool_6 );
	REGISTER_METRIC( server, num_free_pool_7 );
	REGISTER_METRIC( server, num_free_pool_8 );
	REGISTER_METRIC( server, num_free_pool_9 );
	REGISTER_METRIC( server, num_free_pool_10 );
	REGISTER_METRIC( server, num_free_pool_11 );
	REGISTER_METRIC( server, num_free_pool_12 );
	REGISTER_METRIC( server, num_free_pool_13 );
	REGISTER_METRIC( server, num_free_pool_14 );

	REGISTER_METRIC( server, num_reuse_pool_0 );
	REGISTER_METRIC( server, num_reuse_pool_1 );
	REGISTER_METRIC( server, num_reuse_pool_2 );
	REGISTER_METRIC( server, num_reuse_pool_3 );
	REGISTER_METRIC( server, num_reuse_pool_4 );
	REGISTER_METRIC( server, num_reuse_pool_5 );
	REGISTER_METRIC( server, num_reuse_pool_6 );
	REGISTER_METRIC( server, num_reuse_pool_7 );
	REGISTER_METRIC( server, num_reuse_pool_8 );
	REGISTER_METRIC( server, num_reuse_pool_9 );
	REGISTER_METRIC( server, num_reuse_pool_10 );
	REGISTER_METRIC( server, num_reuse_pool_11 );
	REGISTER_METRIC( server, num_reuse_pool_12 );
	REGISTER_METRIC( server, num_reuse_pool_13 );
	REGISTER_METRIC( server, num_reuse_pool_14 );

	REGISTER_METRIC( server, num_resizes );
	REGISTER_METRIC( server, num_out_of_order );

	REGISTER_METRIC( server, num_disk_reserve_pool_0 );
	REGISTER_METRIC( server, num_disk_reserve_pool_1 );
	REGISTER_METRIC( server, num_disk_reserve_pool_2 );
	REGISTER_METRIC( server, num_disk_reserve_pool_3 );
	REGISTER_METRIC( server, num_disk_reserve_pool_4 );
	REGISTER_METRIC( server, num_disk_reserve_pool_5 );
	REGISTER_METRIC( server, num_disk_reserve_pool_6 );
	REGISTER_METRIC( server, num_disk_reserve_pool_7 );
	REGISTER_METRIC( server, num_disk_reserve_pool_8 );
	REGISTER_METRIC( server, num_disk_reserve_pool_9 );
	REGISTER_METRIC( server, num_disk_reserve_pool_10 );

	REGISTER_METRIC( server, num_disk_release_pool_0 );
	REGISTER_METRIC( server, num_disk_release_pool_1 );
	REGISTER_METRIC( server, num_disk_release_pool_2 );
	REGISTER_METRIC( server, num_disk_release_pool_3 );
	REGISTER_METRIC( server, num_disk_release_pool_4 );
	REGISTER_METRIC( server, num_disk_release_pool_5 );
	REGISTER_METRIC( server, num_disk_release_pool_6 );
	REGISTER_METRIC( server, num_disk_release_pool_7 );
	REGISTER_METRIC( server, num_disk_release_pool_8 );
	REGISTER_METRIC( server, num_disk_release_pool_9 );
	REGISTER_METRIC( server, num_disk_release_pool_10 );

	REGISTER_METRIC( server, num_disk_reuse_pool_0 );
	REGISTER_METRIC( server, num_disk_reuse_pool_1 );
	REGISTER_METRIC( server, num_disk_reuse_pool_2 );
	REGISTER_METRIC( server, num_disk_reuse_pool_3 );
	REGISTER_METRIC( server, num_disk_reuse_pool_4 );
	REGISTER_METRIC( server, num_disk_reuse_pool_5 );
	REGISTER_METRIC( server, num_disk_reuse_pool_6 );
	REGISTER_METRIC( server, num_disk_reuse_pool_7 );
	REGISTER_METRIC( server, num_disk_reuse_pool_8 );
	REGISTER_METRIC( server, num_disk_reuse_pool_9 );
	REGISTER_METRIC( server, num_disk_reuse_pool_10 );

	REGISTER_METRIC( server, num_call_put_data );
	REGISTER_METRIC( server, num_call_get_data );
	REGISTER_METRIC( server, num_call_replication_data );
	REGISTER_METRIC( server, num_call_query_lfms );
	REGISTER_METRIC( server, num_call_set_clock );
	REGISTER_METRIC( server, num_call_test_hook );
	REGISTER_METRIC( server, num_call_get_status );
	REGISTER_METRIC( server, num_call_get_cluster_config );
	REGISTER_METRIC( server, num_call_set_cluster_config );
	REGISTER_METRIC( server, num_call_register_client_cluster_config );
	REGISTER_METRIC( server, num_call_get_version );
	REGISTER_METRIC( server, num_call_unhandled );

	SET_METRIC( start_time, my_time( NULL ) );
}
