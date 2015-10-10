#include <backtrace.h>

static void full_write(int fd, const char *buf, size_t len)
{
        while (len > 0) {
                ssize_t ret = write(fd, buf, len);

                if ((ret == -1) && (errno != EINTR))
                        break;

                buf += (size_t) ret;
                len -= (size_t) ret;
        }
}

void print_backtrace(void)
{
        static const char start[] = "BACKTRACE ------------\n";
        static const char end[] = "----------------------\n";

        void *bt[1024];
        int bt_size;
        char **bt_syms;
        int i;

        bt_size = backtrace(bt, 1024);
        bt_syms = backtrace_symbols(bt, bt_size);
        full_write(STDERR_FILENO, start, strlen(start));
        for (i = 1; i < bt_size; i++) {
                size_t len = strlen(bt_syms[i]);
                full_write(STDERR_FILENO, bt_syms[i], len);
                full_write(STDERR_FILENO, "\n", 1);
        }
        full_write(STDERR_FILENO, end, strlen(end));
	free(bt_syms);
}

char* concatenate_backtrace()
{
	void *bt[1024];
        int bt_size;
        char **bt_syms;
        int i;

        bt_size = backtrace(bt, 1024);
        bt_syms = backtrace_symbols(bt, bt_size);

	char* str_backtrace = (char*) calloc(4096, sizeof(char));
        
	for (i = 1; i < bt_size; i++) 
	{
                strcat(str_backtrace, bt_syms[i]);
        	strcat(str_backtrace, "\n");
	}

	free(bt_syms);

	return str_backtrace;

}
