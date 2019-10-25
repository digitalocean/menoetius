#include "option_parser.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct option* get_option_long( struct option* options, const char* long_name );
struct option* get_option_short( struct option* options, const char short_name );
int apply_value( struct option* opt, const char*** argv );
int parse_long_arg( struct option* options, const char* long_name, const char*** argv );
int parse_short_arg( struct option* options, const char short_name, const char*** argv );
struct option* get_option_long( struct option* options, const char* long_name );
struct option* get_option_short( struct option* options, const char short_name );

int apply_value_int( struct option* opt, const char*** argv )
{
	if( *argv == NULL ) {
		fprintf( stderr, "--%s is missing a value\n", opt->long_name );
		return 1;
	}
	const char* s = **argv;
	( *argv )++;

	int n = atoi( s );
	if( !n && strcmp( s, "0" ) ) {
		fprintf( stderr, "value passed to --%s (%s) is not valid\n", opt->long_name, s );
		return -1;
	}
	*(int*)opt->value = atoi( s );
	return 0;
}

int apply_value_string( struct option* opt, const char*** argv )
{
	if( *argv == NULL ) {
		fprintf( stderr, "--%s is missing a value\n", opt->long_name );
		return 1;
	}

	if( opt->num_choices ) {
		bool found = false;
		for( int i = 0; i < opt->num_choices; i++ ) {
			if( strcmp( **argv, opt->choices[i] ) == 0 ) {
				found = true;
			}
		}
		if( !found ) {
			fprintf( stderr,
					 "invalid %s value \"%s\"\nTry --help for more information.\n",
					 opt->long_name,
					 **argv );
			return 1;
		}
	}

	*(const char**)opt->value = **argv;
	( *argv )++;
	return 0;
}

int apply_value_flag( struct option* opt )
{
	( *(int*)opt->value )++;
	return 0;
}

int apply_value( struct option* opt, const char*** argv )
{
	switch( opt->type ) {
	case OPTION_INTEGER:
		return apply_value_int( opt, argv );
	case OPTION_STRING:
		return apply_value_string( opt, argv );
	case OPTION_FLAG:
		return apply_value_flag( opt );
	default:
		fprintf( stderr, "%d invalid type\n", opt->type );
		abort();
	}
}

int parse_long_arg( struct option* options, const char* long_name, const char*** argv )
{
	struct option* opt = get_option_long( options, long_name );
	if( opt == NULL ) {
		fprintf( stderr, "--%s is not a valid flag\n", long_name );
		return -1;
	}

	return apply_value( opt, argv );
}

int parse_short_arg( struct option* options, const char short_name, const char*** argv )
{
	struct option* opt = get_option_short( options, short_name );
	if( opt == NULL ) {
		fprintf( stderr, "-%c is not a valid flag\n", short_name );
		return -1;
	}

	return apply_value( opt, argv );
}

struct option* get_option_long( struct option* options, const char* long_name )
{
	for( ; options->type != OPTION_END; options++ ) {
		if( strcmp( options->long_name, long_name ) == 0 ) {
			return options;
		}
	}
	return NULL;
}

struct option* get_option_short( struct option* options, const char short_name )
{
	if( !short_name ) {
		return NULL;
	}

	for( ; options->type != OPTION_END; options++ ) {
		if( options->short_name == short_name ) {
			return options;
		}
	}

	return NULL;
}

int parse_options( struct option* options, const char*** argv )
{
	int l;
	int res;
	const char* s;
	while( **argv ) {
		s = **argv;
		l = strlen( s );
		if( l <= 1 || s[0] != '-' ) {
			return 0;
		}
		( *argv )++;
		if( s[1] == '-' ) {
			res = parse_long_arg( options, s + 2, argv );
			if( res ) {
				return res;
			}
		}
		else {
			res = parse_short_arg( options, s[1], argv );
			if( res ) {
				return res;
			}
		}
	}

	return 0;
}

struct cmd_struct* get_command( struct cmd_struct* commands, const char* name )
{
	for( ; commands->name; commands++ ) {
		if( strcmp( name, commands->name ) == 0 ) {
			return commands;
		}
	}
	return NULL;
}

void print_help( struct option* options )
{
	struct option* p;
	int max_flag = 0;
	int n;

	for( p = options; p->type != OPTION_END; p++ ) {
		n = strlen( p->long_name );
		if( n > max_flag ) {
			max_flag = n;
		}
	}

	printf( "Flags:\n" );

	for( p = options; p->type != OPTION_END; p++ ) {
		printf( "  " );
		if( p->short_name ) {
			printf( "-%c, ", p->short_name );
		}
		printf( "--%s", p->long_name );

		n = max_flag - strlen( p->long_name );
		printf( "%*c", n + 2, ' ' );

		printf( "%s", p->help_text );

		bool need_closing_parenthesis = false;
		if( p->type != OPTION_FLAG || p->num_choices > 0 ) {
			need_closing_parenthesis = true;
			printf( " (" );
		}

		switch( p->type ) {
		case OPTION_INTEGER:
			printf( "default: %d", *(int*)p->value );
			break;
		case OPTION_STRING:
			printf( "default: %s", *(const char**)p->value );
			break;
		case OPTION_FLAG:
			break;
		default:
			fprintf( stderr, "%d invalid type\n", p->type );
			abort();
		}

		for( int i = 0; i < p->num_choices; i++ ) {
			if( i == 0 ) {
				printf( "; choices: " );
			}
			else {
				printf( ", " );
			}
			printf( "%s", p->choices[i] );
		}

		if( need_closing_parenthesis ) {
			printf( ")" );
		}

		printf( "\n" );
	}
}
