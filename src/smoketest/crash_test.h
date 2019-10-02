#pragma once

#include <sys/types.h>

int run_crash_test( int num_keys,
					int num_pts,
					int64_t start_time,
					const char* crash_location,
					int initial_num_before_crash,
					int incr_num_before_crash );
