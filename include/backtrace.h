#include <execinfo.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static void full_write(int fd, const char *buf, size_t len);
void print_backtrace(void);
