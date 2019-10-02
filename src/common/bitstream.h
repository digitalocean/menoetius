#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int bitstream_read_uint64(
	const char* buf, size_t buf_size_bits, size_t bit_index, size_t num_bits, uint64_t* value );

int bitstream_write_uint64( char* buf,
							size_t buf_size_bits,
							size_t bit_index,
							size_t num_bits,
							uint64_t value,
							size_t* num_bits_written );

int bitstream_read_int64(
	const char* buf, size_t buf_size_bits, size_t bit_index, size_t num_bits, int64_t* value );

int bitstream_write_int64( char* buf,
						   size_t buf_size_bits,
						   size_t bit_index,
						   size_t num_bits,
						   int64_t value,
						   size_t* num_bits_written );
