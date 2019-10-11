#include "tokenizer.h"
#include <stdio.h>
#include <ctype.h>

#define BACK_LOG_SIZE 80

// File data
static FILE *in = NULL;
static char back_log[BACK_LOG_SIZE];
static int back_log_index;
static int is_eof;

void tokenizer_open(const char *file_path)
{
	tokenizer_close();
	
	in = fopen(file_path, "r");
	back_log_index = 0;
	is_eof = 0;
}

char tokenizer_next()
{
	if (back_log_index > 0)
		return back_log[--back_log_index];

	char c = fgetc(in);
	if (c == EOF)
		is_eof = 1;
	return c;
}

void tokenizer_push_back(char c)
{
	back_log[back_log_index++] = c;
}

void tokenizer_skip_white_space()
{
	char c;

	while (isspace(c = tokenizer_next()) && !is_eof)
		continue;
	tokenizer_push_back(c);
}

void tokenizer_read_until(char *out, char until, int ignore_space)
{
	int buffer_pointer = 0;
	int c;

	while ((c = tokenizer_next()) != until)
	{
		if (isspace(c) && !ignore_space)
			break;

		out[buffer_pointer++] = c;
	}

	out[buffer_pointer] = '\0';
	tokenizer_push_back(c);
}

void tokenizer_word(char *out)
{
	int index = 0;
	char c;

	tokenizer_skip_white_space();
	while (!isspace(c = tokenizer_next()) && !is_eof)
		out[index++] = c;
	tokenizer_push_back(c);
	
	out[index] = '\0';
}

int tokenizer_has_next()
{
	return !is_eof; 
}

void tokenizer_close()
{
	if (in != NULL)
	{
		fclose(in);
		in = NULL;
	}
}

