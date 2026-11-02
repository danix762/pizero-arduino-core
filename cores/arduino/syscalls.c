#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>

int _getentropy(void *buffer, size_t length) {
    (void)buffer;
    (void)length;
    errno = ENOSYS;
    return -1;
}
/* These are the minimum stubs newlib needs on a bare-metal target */

void _exit(int code) {
    (void)code;
    while (1) {
        __asm__("wfe");   // wait forever
    }
}

int _close(int fd) {
    (void)fd;
    return -1;
}

int _fstat(int fd, struct stat *st) {
    (void)fd;
    if (st) {
        st->st_mode = S_IFCHR;   // character device
    }
    return 0;
}

int _isatty(int fd) {
    (void)fd;
    return 1;   // pretend everything is a tty
}

int _lseek(int fd, int ptr, int dir) {
    (void)fd; (void)ptr; (void)dir;
    return 0;
}

int _read(int fd, char *ptr, int len) {
    (void)fd; (void)ptr; (void)len;
    return 0;   // no input
}

int _write(int fd, char *ptr, int len) {
    (void)fd;
    // Optional: send to USB Serial later
    // for now just pretend we wrote everything
    return len;
}

/* Simple heap for malloc / newlib */
extern char _end;                 // provided by linker script
static char *heap_end = 0;

void *_sbrk(int incr) {
    if (heap_end == 0) {
        heap_end = &_end;
    }

    char *prev = heap_end;
    heap_end += incr;

    // Very basic safety – you can improve this later
    return (void *)prev;
}

/* Optional but often needed */
int _kill(int pid, int sig) {
    (void)pid; (void)sig;
    errno = EINVAL;
    return -1;
}

int _getpid(void) {
    return 1;
}
