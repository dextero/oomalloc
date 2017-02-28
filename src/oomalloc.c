#include <dlfcn.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "oomalloc.h"

typedef void *malloc_t(size_t size);
typedef void free_t(void *ptr);
typedef void *calloc_t(size_t nmemb, size_t size);
typedef void *realloc_t(void *ptr, size_t size);
typedef size_t malloc_usable_size_t(void *ptr);

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

void *glibc;
malloc_t *glibc_malloc;
calloc_t *glibc_calloc;
realloc_t *glibc_realloc;
free_t *glibc_free;
malloc_usable_size_t *glibc_malloc_usable_size;

bool limit_memory = false;
size_t limit_memory_bytes;
size_t total_memory_requested = 0;

bool alloc_fail_requested = false;
size_t successes_until_next_fail;

bool log_allocations = false;

#define OOMALLOC_LOG(fmt, ...) \
    do { \
        if (log_allocations) { \
            fprintf(stderr, fmt "\n", ##__VA_ARGS__); \
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

    char *endptr = NULL;
    errno = 0;
    *out_size = strtoull(value, &endptr, base);

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
    if (glibc) {
        return;
    }

    glibc = dlopen("libc.so.6", RTLD_LAZY);
    if (!glibc) {
        return;
    }

    glibc_malloc = (malloc_t*)dlsym(glibc, "malloc");
    glibc_free = (free_t*)dlsym(glibc, "free");
    glibc_calloc = (calloc_t*)dlsym(glibc, "calloc");
    glibc_realloc = (realloc_t*)dlsym(glibc, "realloc");
    glibc_malloc_usable_size = (malloc_usable_size_t*)dlsym(glibc, "malloc_usable_size");

    if (!glibc_malloc
            || !glibc_free
            || !glibc_calloc
            || !glibc_realloc
            || !glibc_malloc_usable_size) {
        dlclose(glibc);
        glibc = NULL;
        return;
    }

    if (getenv_size("OOMALLOC_LIMIT_MEMORY", &limit_memory_bytes, 0) == 0) {
        limit_memory = true;
    }
    if (getenv_size("OOMALLOC_FAIL_AFTER", &successes_until_next_fail, 10) == 0) {
        alloc_fail_requested = true;
    }
    if (getenv("OOMALLOC_LOG") != NULL) {
        log_allocations = true;
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
    size_t actual_size = glibc_malloc_usable_size(ptr);
    total_memory_requested += actual_size;

    OOMALLOC_LOG("allocated %zu B", actual_size);
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

    size_t old_size = glibc_malloc_usable_size(ptr);
    if (size >= old_size && should_fail(size - old_size)) {
        return NULL;
    }

    ptr = glibc_realloc(ptr, size);
    size_t actual_size = glibc_malloc_usable_size(ptr);
    total_memory_requested += actual_size - old_size;

    OOMALLOC_LOG("reallocated %zu B to %zu B (delta: %zd)",
                 old_size, actual_size, (ssize_t)actual_size - old_size);

    return ptr;
}

void free(void *ptr) {
    init();

    size_t actual_size = glibc_malloc_usable_size(ptr);
    total_memory_requested -= actual_size;
    glibc_free(ptr);

    OOMALLOC_LOG("freed %zu B", actual_size);
}
