#pragma once

#include <stdbool.h>
#include <sys/types.h>

struct menoetius_client;

bool is_process_healthy( int pid );

pid_t run_server( const char* storage_path,
				  const char* crash_location,
				  int num_crashes,
				  int clock_time );

int kill_server( pid_t pid );
int wait_for_server_to_respond( struct menoetius_client* client, int pid, int timeout_seconds );
bool start_server_if_needed( int* pid,
							 const char* storage_path,
							 int start_time,
							 const char* crash_location,
							 int num_success_before_crash );

int shutdown_server_gracefully_and_wait( int pid );

int join_server_pid( int pid );

int validate_points( int64_t start_time,
					 int key_num,
					 int expected_num_returned_pts,
					 int actual_num_returned_pts,
					 int64_t* tt,
					 double* yy );
