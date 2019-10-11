#include "debug.h"

static int has_error_flag = 0;

void debug_init()
{
	has_error_flag = 0;
}

void error(const char *msg)
{
	printf("Error: %s\n", msg);
	has_error_flag = 1;
}

int has_error()
{
	return has_error_flag;
}

