#pragma once

struct KeyValue
{
	const char* key;
	const char* value;
};

int parse_lfm( const char* s,
			   const char** metric_name,
			   struct KeyValue* metric_labels,
			   int* num_labels,
			   int max_key_value_pairs );
