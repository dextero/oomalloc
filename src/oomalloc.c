// for RTLD_NEXT
#define _GNU_SOURCE

#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>

#include "oomalloc.h"

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

extern malloc_t malloc;
extern calloc_t calloc;
extern realloc_t realloc;
extern free_t free;

struct libc_functions {
    malloc_t *malloc;
    calloc_t *calloc;
    realloc_t *realloc;
    free_t *free;
};

static struct oomalloc_globals {
    struct libc_functions libc;

    bool heap_limited;
    size_t heap_limit_bytes;
    size_t heap_used;

    bool alloc_fail_requested;
    size_t successes_until_next_fail;

    bool log_allocations;
    size_t allocation_idx;
} GLOBALS;

#define OOMALLOC_LOG(fmt, ...) \
    do { \
        if (GLOBALS.log_allocations) { \
            fprintf(stderr, fmt "\n", ##__VA_ARGS__); \
        } \
    } while (0)

void oomalloc_heap_limit(size_t max_bytes) {
    if (max_bytes == OOMALLOC_UNLIMITED) {
        GLOBALS.heap_limited = false;
        OOMALLOC_LOG("memory limit disabled");
    } else {
        GLOBALS.heap_limited = true;
        GLOBALS.heap_limit_bytes = max_bytes;
        OOMALLOC_LOG("memory limit set to %zu bytes", max_bytes);
    }
}

void oomalloc_fail_after(size_t n) {
    if (n == OOMALLOC_DONT_FAIL) {
        GLOBALS.alloc_fail_requested = false;
        OOMALLOC_LOG("allocation failure on demand disabled");
    } else {
        GLOBALS.alloc_fail_requested = true;
        GLOBALS.successes_until_next_fail = n;
        OOMALLOC_LOG("allocation failure requested after %zu successes", n);
    }
}

void oomalloc_set_log_enabled(bool enabled) {
    GLOBALS.log_allocations = enabled;
}

void oomalloc_reset(void) {
    oomalloc_heap_limit(OOMALLOC_UNLIMITED);
    oomalloc_fail_after(OOMALLOC_DONT_FAIL);
}

static int getenv_size(const char *name,
                       size_t *out_size) {
    const char *value = getenv(name);
    if (!value) {
        return -1;
    }

    char *endptr = NULL;
    errno = 0;
    *out_size = strtoull(value, &endptr, 0);

    if (errno
            || !endptr
            || endptr == value
            || *endptr != '\0') {
        return -1;
    } else {
        return 0;
    }
}

static void init() {
    if (GLOBALS.libc.malloc) {
        return;
    }

    GLOBALS.libc.malloc = (malloc_t*)dlsym(RTLD_NEXT, "malloc");
    GLOBALS.libc.free = (free_t*)dlsym(RTLD_NEXT, "free");
    GLOBALS.libc.calloc = (calloc_t*)dlsym(RTLD_NEXT, "calloc");
    GLOBALS.libc.realloc = (realloc_t*)dlsym(RTLD_NEXT, "realloc");

    if (!GLOBALS.libc.malloc
            || !GLOBALS.libc.free
            || !GLOBALS.libc.calloc
            || !GLOBALS.libc.realloc) {
        OOMALLOC_LOG("unable to load libc functions");
        GLOBALS.libc = (struct libc_functions){};
        return;
    }

    if (getenv("OOMALLOC_LOG") != NULL) {
        oomalloc_set_log_enabled(true);
    }

    size_t n;
    if (getenv_size("OOMALLOC_LIMIT_HEAP", &n) == 0) {
        oomalloc_heap_limit(n);
    }
    if (getenv_size("OOMALLOC_FAIL_AFTER", &n) == 0) {
        oomalloc_fail_after(n);
    }
}

enum allocation_attempt_result {
    ALLOC_CONTINUE,
    ALLOC_FAIL_REQUESTED,
};

static enum allocation_attempt_result
register_allocation_attempt(size_t bytes_requested) {
    const size_t possible_usage = GLOBALS.heap_used + bytes_requested;

    if (GLOBALS.heap_limited
            && possible_usage > GLOBALS.heap_limit_bytes) {
        OOMALLOC_LOG("memory limit exhausted: %zu used, %zu requested, %zu/%zu "
                     "total", GLOBALS.heap_used, bytes_requested,
                     possible_usage, GLOBALS.heap_limit_bytes);
        return ALLOC_FAIL_REQUESTED;
    }

    if (GLOBALS.alloc_fail_requested) {
        if (GLOBALS.successes_until_next_fail == 0) {
            GLOBALS.alloc_fail_requested = false;
            OOMALLOC_LOG("allocation failure on explicit request");
            return ALLOC_FAIL_REQUESTED;
        }

        --GLOBALS.successes_until_next_fail;
    }

    return ALLOC_CONTINUE;
}

static void register_allocated_memory(void *ptr, size_t bytes_requested) {
    size_t actual_size = malloc_usable_size(ptr);
    GLOBALS.heap_used += actual_size;

    OOMALLOC_LOG("%zu. allocation: requested %zu B, got %zu B (in use: %zu)",
                 GLOBALS.allocation_idx++, bytes_requested,
                 actual_size, GLOBALS.heap_used);
}

void *malloc(size_t size) {
    init();

    if (register_allocation_attempt(size) == ALLOC_FAIL_REQUESTED) {
        return NULL;
    }

    void *ptr = GLOBALS.libc.malloc(size);
    register_allocated_memory(ptr, size);
    return ptr;
}

void *calloc(size_t nmemb, size_t size) {
    init();

    if (register_allocation_attempt(nmemb * size) == ALLOC_FAIL_REQUESTED) {
        return NULL;
    }

    void *ptr = GLOBALS.libc.calloc(nmemb, size);
    register_allocated_memory(ptr, size);
    return ptr;
}

void *realloc(void *ptr, size_t size) {
    init();

    size_t old_size = malloc_usable_size(ptr);
    if (size >= old_size
            && register_allocation_attempt(size - old_size) == ALLOC_FAIL_REQUESTED) {
        return NULL;
    }

    ptr = GLOBALS.libc.realloc(ptr, size);
    size_t actual_size = malloc_usable_size(ptr);
    GLOBALS.heap_used += actual_size - old_size;

    OOMALLOC_LOG("%zu. reallocation: %zu B to %zu B (delta: %zd, in use: %zu)",
                 GLOBALS.allocation_idx++, old_size, actual_size,
                 (ssize_t)actual_size - old_size, GLOBALS.heap_used);

    return ptr;
}

void free(void *ptr) {
    init();

    size_t actual_size = malloc_usable_size(ptr);
    GLOBALS.heap_used -= actual_size;
    GLOBALS.libc.free(ptr);

    OOMALLOC_LOG("free: %zu B (in use: %zu)", actual_size, GLOBALS.heap_used);
}
