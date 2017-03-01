#define _GNU_SOURCE

#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/types.h>

#include "oomalloc.h"
#include "utils.h"

extern char *getenv(const char *name);

typedef void *malloc_t(size_t size);
typedef void free_t(void *ptr);
typedef void *calloc_t(size_t nmemb, size_t size);
typedef void *realloc_t(void *ptr, size_t size);

size_t malloc_usable_size(void *ptr);

#ifdef OOMALLOC_TEST
#define malloc oomalloc_malloc
#define calloc oomalloc_calloc
#define realloc oomalloc_realloc
#define free oomalloc_free
#endif

malloc_t malloc;
calloc_t calloc;
realloc_t realloc;
free_t free;

static malloc_t *glibc_malloc;
static calloc_t *glibc_calloc;
static realloc_t *glibc_realloc;
static free_t *glibc_free;

static bool limit_memory = false;
static size_t limit_memory_bytes;
static size_t total_memory_requested = 0;

static bool alloc_fail_requested = false;
static size_t successes_until_next_fail;

static bool log_allocations = false;

static void print_size(int fd, size_t size) {
    char buf[32];
    ssize_t bytes_written = size_to_string(buf, sizeof(buf), size);
    write(fd, buf, bytes_written);
}

static void print_ssize(int fd, ssize_t ssize) {
    char buf[32];
    ssize_t bytes_written = 0;

    if (ssize < 0) {
        buf[0] = '-';
        bytes_written = 1 + size_to_string(&buf[1], sizeof(buf) - 1, (size_t)-ssize);
    } else {
        bytes_written = size_to_string(buf, sizeof(buf), (size_t)ssize);
    }

    write(fd, buf, bytes_written);
}

static void fdprintf(int fd, const char *format, ...)
    __attribute__((format(printf, 2, 3)));

static void fdprintf(int fd, const char *format, ...) {
    va_list list;
    va_start(list, format);

    while (*format) {
        const char *end = format;
        while (*end && *end != '%') {
            ++end;
        }

        if (format != end) {
            write(fd, format, end - format);
            format = end;
        } else {
            assert(*format == '%');

            ++format;
            if (*format++ == 'z') {
                switch (*format++) {
                case 'd':
                    print_ssize(fd, va_arg(list, ssize_t));
                    break;
                case 'u':
                    print_size(fd, va_arg(list, size_t));
                    break;
                case '\0':
                    break;
                default:
                    write(fd, format - 3, 3);
                    break;
                }
            }
        }
    }

    va_end(list);
}

#define OOMALLOC_LOG(fmt, ...) \
    do { \
        if (log_allocations) { \
            fdprintf(2, fmt "\n", ##__VA_ARGS__); \
        } \
    } while (0)

void oomalloc_memory_limit(size_t max_bytes) {
    if (max_bytes == OOMALLOC_UNLIMITED) {
        limit_memory = false;
        OOMALLOC_LOG("memory limit disabled");
    } else {
        limit_memory = true;
        limit_memory_bytes = max_bytes;
        OOMALLOC_LOG("memory limit set to %zu bytes", max_bytes);
    }
}

void oomalloc_fail_after(size_t n) {
    if (n == OOMALLOC_DONT_FAIL) {
        alloc_fail_requested = false;
        OOMALLOC_LOG("allocation failure on demand disabled");
    } else {
        alloc_fail_requested = true;
        successes_until_next_fail = n;
        OOMALLOC_LOG("allocation failure requested after %zu successes", n);
    }
}

void oomalloc_set_log_enabled(bool enabled) {
    log_allocations = enabled;
}

void oomalloc_reset(void) {
    oomalloc_memory_limit(OOMALLOC_UNLIMITED);
    oomalloc_fail_after(OOMALLOC_DONT_FAIL);
}

static int getenv_size(const char *name,
                       size_t *out_size,
                       int base) {
    const char *value = getenv(name);
    if (!value) {
        return -1;
    }

    return parse_size(value, out_size, base);
}

static void init() {
    if (glibc_malloc) {
        return;
    }

    glibc_malloc = (malloc_t*)dlsym(RTLD_NEXT, "malloc");
    glibc_free = (free_t*)dlsym(RTLD_NEXT, "free");
    glibc_calloc = (calloc_t*)dlsym(RTLD_NEXT, "calloc");
    glibc_realloc = (realloc_t*)dlsym(RTLD_NEXT, "realloc");

    if (!glibc_malloc
            || !glibc_free
            || !glibc_calloc
            || !glibc_realloc) {
        OOMALLOC_LOG("unable to load glibc functions");
        return;
    }

    if (getenv("OOMALLOC_LOG") != NULL) {
        log_allocations = true;
    }

    size_t n;
    if (getenv_size("OOMALLOC_LIMIT_MEMORY", &n, 0) == 0) {
        oomalloc_memory_limit(n);
    }
    if (getenv_size("OOMALLOC_FAIL_AFTER", &n, 10) == 0) {
        oomalloc_fail_after(n);
    }
}

static bool should_fail(size_t extra_memory_requested) {
    if (limit_memory
            && total_memory_requested + extra_memory_requested > limit_memory_bytes) {
        OOMALLOC_LOG("memory limit exhausted: %zu used, %zu requested, %zu/%zu total",
                     total_memory_requested, extra_memory_requested,
                     total_memory_requested + extra_memory_requested,
                     limit_memory_bytes);
        return true;
    }

    if (alloc_fail_requested) {
        if (successes_until_next_fail == 0) {
            alloc_fail_requested = false;
            OOMALLOC_LOG("allocation failure on explicit request");
            return true;
        }

        --successes_until_next_fail;
    }

    OOMALLOC_LOG("requested %zu B", extra_memory_requested);
    return false;
}

static void register_allocated_memory(void *ptr) {
    size_t actual_size = malloc_usable_size(ptr);
    total_memory_requested += actual_size;

    OOMALLOC_LOG("allocated %zu B (in use: %zu)",
                 actual_size, total_memory_requested);
}

void *malloc(size_t size) {
    init();

    if (should_fail(size)) {
        return NULL;
    }

    void *ptr = glibc_malloc(size);
    register_allocated_memory(ptr);
    return ptr;
}

void *calloc(size_t nmemb, size_t size) {
    init();

    if (should_fail(nmemb * size)) {
        return NULL;
    }

    void *ptr = glibc_calloc(nmemb, size);
    register_allocated_memory(ptr);
    return ptr;
}

void *realloc(void *ptr, size_t size) {
    init();

    size_t old_size = malloc_usable_size(ptr);
    if (size >= old_size && should_fail(size - old_size)) {
        return NULL;
    }

    ptr = glibc_realloc(ptr, size);
    size_t actual_size = malloc_usable_size(ptr);
    total_memory_requested += actual_size - old_size;

    OOMALLOC_LOG("reallocated %zu B to %zu B (delta: %zd, in use: %zu)",
                 old_size, actual_size, (ssize_t)actual_size - old_size,
                 total_memory_requested);

    return ptr;
}

void free(void *ptr) {
    init();

    size_t actual_size = malloc_usable_size(ptr);
    total_memory_requested -= actual_size;
    glibc_free(ptr);

    OOMALLOC_LOG("freed %zu B (in use: %zu)",
                 actual_size, total_memory_requested);
}
