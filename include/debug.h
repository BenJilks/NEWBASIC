#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

static char error_buffer[80];

#define ERROR(...) \
{ \
	sprintf(error_buffer, __VA_ARGS__); \
	error(error_buffer); \
}

#define DEBUG 	1

#if DEBUG
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...) ;
#endif

void debug_init();
void error(const char *msg);
int has_error();

#endif // DEBUG_H

