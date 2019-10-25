#pragma once

#define OPT_INT( SHORT_NAME, LONG_NAME, VALUE, HELP_TEXT )                                         \
	{                                                                                              \
		.short_name = SHORT_NAME, .long_name = LONG_NAME, .value = VALUE, .type = OPTION_INTEGER,  \
		.help_text = HELP_TEXT, .num_choices = 0                                                   \
	}

#define OPT_STR( SHORT_NAME, LONG_NAME, VALUE, HELP_TEXT )                                         \
	{                                                                                              \
		.short_name = SHORT_NAME, .long_name = LONG_NAME, .value = VALUE, .type = OPTION_STRING,   \
		.help_text = HELP_TEXT, .num_choices = 0                                                   \
	}

#define OPT_FLAG( SHORT_NAME, LONG_NAME, VALUE, HELP_TEXT )                                        \
	{                                                                                              \
		.short_name = SHORT_NAME, .long_name = LONG_NAME, .value = VALUE, .type = OPTION_FLAG,     \
		.help_text = HELP_TEXT, .num_choices = 0                                                   \
	}

#define OPT_STR_CHOICES( SHORT_NAME, LONG_NAME, VALUE, HELP_TEXT, NUM_CHOICES, CHOICES )           \
	{                                                                                              \
		.short_name = SHORT_NAME, .long_name = LONG_NAME, .value = VALUE, .type = OPTION_STRING,   \
		.help_text = HELP_TEXT, .num_choices = NUM_CHOICES, .choices = CHOICES                     \
	}

#define OPT_END                                                                                    \
	{                                                                                              \
		.short_name = '\0', .long_name = NULL, .value = NULL, .type = OPTION_END,                  \
		.help_text = NULL, .num_choices = 0, .choices = NULL                                       \
	}

enum parse_opt_type
{
	OPTION_END,
	OPTION_INTEGER,
	OPTION_STRING,
	OPTION_FLAG,
};

struct option
{
	char short_name;
	const char* long_name;
	void* value;
	enum parse_opt_type type;
	const char* help_text;

	int num_choices;
	const char** choices;
};

// list of NULL-terminated option pointers
// *argv will be set to the next un-parsed command line value
// returns 0 on success
int parse_options( struct option* options, const char*** argv );

struct cmd_struct
{
	const char* name;
	int ( *fn )( const char***, const char** );
};

struct cmd_struct* get_command( struct cmd_struct* commands, const char* name );

void print_help( struct option* options );
