#pragma once

struct env_builder
{
	int i;
	int cap;
	char** envs;
};

void env_builder_init( struct env_builder* eb );

void env_builder_free( struct env_builder* eb );

void env_builder_addf( struct env_builder* eb, const char* fmt, ... );
