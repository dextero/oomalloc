#include <gtest/gtest.h>

TEST_CASE(oomalloc, default_malloc) {
    void *p = malloc(4096);
    ASSERT_NOT_NULL(p);
    free(p);
}

TEST_CASE(oomalloc, default_calloc) {
    void *p = calloc(1, 4096);
    ASSERT_NOT_NULL(p);
    free(p);
}

TEST_CASE(oomalloc, default_realloc) {
    void *p = realloc(NULL, 2048);
    ASSERT_NOT_NULL(p);
    p = realloc(p, 4096);
    ASSERT_NOT_NULL(p);
    free(p);
}

TEST_CASE(oomalloc, memory_limit_zero_malloc) {
    oomalloc_memory_limit(0);
    ASSERT_NULL(malloc(1));
}

TEST_CASE(oomalloc, memory_limit_zero_calloc) {
    oomalloc_memory_limit(0);
    ASSERT_NULL(calloc(1, 1));
}

TEST_CASE(oomalloc, memory_limit_zero_realloc_null) {
    oomalloc_memory_limit(0);
    ASSERT_NULL(realloc(NULL, 1));
}

TEST_CASE(oomalloc, memory_limit_zero_realloc_nonnull) {
    void *p = malloc(1);
    ASSERT_NOT_NULL(p);

    oomalloc_memory_limit(0);
    ASSERT_NULL(realloc(p, 1));

    free(p);
}

TEST_CASE(oomalloc, memory_limit_nonzero_malloc) {
    oomalloc_memory_limit(4096);

    void *p;
    void *p2;

    ASSERT_NOT_NULL(p = malloc(4096));
    ASSERT_NULL(malloc(1));
    free(p);

    ASSERT_NOT_NULL(p = malloc(2048));
    ASSERT_NOT_NULL(p2 = malloc(2048));
    ASSERT_NULL(malloc(1));
    free(p);
    free(p2);

    ASSERT_NULL(malloc(4096));

    ASSERT_NOT_NULL(p = malloc(2048));
    ASSERT_NULL(malloc(2049));
    free(p);
    free(p2);
}

TEST_CASE(oomalloc, memory_limit_nonzero_calloc) {
    oomalloc_memory_limit(4096);

    void *p;
    void *p2;

    ASSERT_NOT_NULL(p = calloc(1, 4096));
    ASSERT_NULL(calloc(1, 1));
    free(p);

    ASSERT_NOT_NULL(p = calloc(1, 2048));
    ASSERT_NOT_NULL(p2 = calloc(2048, 1));
    ASSERT_NULL(calloc(1, 1));
    free(p);
    free(p2);

    ASSERT_NULL(calloc(1, 4096));

    ASSERT_NOT_NULL(p = calloc(1, 2048));
    ASSERT_NULL(calloc(1, 2049));
    free(p);
    free(p2);
}

TEST_CASE(oomalloc, memory_limit_nonzero_realloc) {
    oomalloc_memory_limit(4096);

    void *p;

    ASSERT_NOT_NULL(p = realloc(NULL, 2048));
    ASSERT_NULL(realloc(NULL, 4096));
    ASSERT_NOT_NULL(p = calloc(p, 4096));
    ASSERT_NULL(realloc(NULL, 1));
    free(p);

    ASSERT_NULL(realloc(NULL, 4097));
}
