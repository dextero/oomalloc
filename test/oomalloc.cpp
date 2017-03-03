#include <gtest/gtest.h>

#include "oomalloc.h"
#include "oomalloc_test.h"

#define MALLOC_OVERHEAD_MARGIN 32

class OomallocTest: public ::testing::Test {
public:
    void SetUp() {
        oomalloc_reset();
    }

    void TearDown() {
        oomalloc_reset();
    }
};

TEST_F(OomallocTest, default_malloc) {
    void *p = oomalloc_malloc(4096);
    ASSERT_TRUE(NULL != p);
    oomalloc_free(p);
}

TEST_F(OomallocTest, default_calloc) {
    void *p = oomalloc_calloc(1, 4096);
    ASSERT_TRUE(NULL != p);
    oomalloc_free(p);
}

TEST_F(OomallocTest, default_realloc) {
    void *p = oomalloc_realloc(NULL, 2048);
    ASSERT_TRUE(NULL != p);
    p = oomalloc_realloc(p, 4096);
    ASSERT_TRUE(NULL != p);
    oomalloc_free(p);
}

TEST_F(OomallocTest, heap_limit_zero_malloc) {
    oomalloc_heap_limit(0);
    ASSERT_TRUE(NULL == oomalloc_malloc(1));
}

TEST_F(OomallocTest, heap_limit_zero_calloc) {
    oomalloc_heap_limit(0);
    ASSERT_TRUE(NULL == oomalloc_calloc(1, 1));
}

TEST_F(OomallocTest, heap_limit_zero_realloc_null) {
    oomalloc_heap_limit(0);
    ASSERT_TRUE(NULL == oomalloc_realloc(NULL, 1));
}

TEST_F(OomallocTest, heap_limit_zero_realloc_nonnull) {
    void *p = oomalloc_malloc(1);
    ASSERT_TRUE(NULL != p);

    oomalloc_heap_limit(0);
    ASSERT_TRUE(NULL != oomalloc_realloc(p, 1));

    oomalloc_free(p);
}

TEST_F(OomallocTest, heap_limit_nonzero_malloc) {
    oomalloc_heap_limit(4096 + MALLOC_OVERHEAD_MARGIN);

    void *p;
    void *p2;

    ASSERT_TRUE(NULL != (p = oomalloc_malloc(4096)));
    ASSERT_TRUE(NULL == oomalloc_malloc(MALLOC_OVERHEAD_MARGIN));
    oomalloc_free(p);

    ASSERT_TRUE(NULL != (p = oomalloc_malloc(2048)));
    ASSERT_TRUE(NULL != (p2 = oomalloc_malloc(2048)));
    ASSERT_TRUE(NULL == oomalloc_malloc(MALLOC_OVERHEAD_MARGIN));
    oomalloc_free(p);
    oomalloc_free(p2);

    ASSERT_TRUE(NULL == oomalloc_malloc(4096 + 2 * MALLOC_OVERHEAD_MARGIN));

    ASSERT_TRUE(NULL != (p = oomalloc_malloc(2048)));
    ASSERT_TRUE(NULL == oomalloc_malloc(2048 + 2 * MALLOC_OVERHEAD_MARGIN));
    oomalloc_free(p);
}

TEST_F(OomallocTest, heap_limit_nonzero_calloc) {
    oomalloc_heap_limit(4096 + MALLOC_OVERHEAD_MARGIN);

    void *p;
    void *p2;

    ASSERT_TRUE(NULL != (p = oomalloc_calloc(1, 4096)));
    ASSERT_TRUE(NULL == oomalloc_calloc(1, MALLOC_OVERHEAD_MARGIN));
    oomalloc_free(p);

    ASSERT_TRUE(NULL != (p = oomalloc_calloc(1, 2048)));
    ASSERT_TRUE(NULL != (p2 = oomalloc_calloc(2048, 1)));
    ASSERT_TRUE(NULL == oomalloc_calloc(1, MALLOC_OVERHEAD_MARGIN));
    oomalloc_free(p);
    oomalloc_free(p2);

    ASSERT_TRUE(NULL == oomalloc_calloc(1, 4096 + 2 * MALLOC_OVERHEAD_MARGIN));

    ASSERT_TRUE(NULL != (p = oomalloc_calloc(1, 2048)));
    ASSERT_TRUE(NULL == oomalloc_calloc(1, 2048 + 2 * MALLOC_OVERHEAD_MARGIN));
    oomalloc_free(p);
}

TEST_F(OomallocTest, heap_limit_nonzero_realloc) {
    oomalloc_heap_limit(4096 + MALLOC_OVERHEAD_MARGIN);

    void *p;

    ASSERT_TRUE(NULL != (p = oomalloc_realloc(NULL, 2048)));
    ASSERT_TRUE(NULL == oomalloc_realloc(NULL, 4096));
    ASSERT_TRUE(NULL != (p = oomalloc_realloc(p, 4096)));
    ASSERT_TRUE(NULL == oomalloc_realloc(NULL, MALLOC_OVERHEAD_MARGIN));
    oomalloc_free(p);

    ASSERT_TRUE(NULL == oomalloc_realloc(NULL, 4096 + 2 * MALLOC_OVERHEAD_MARGIN));
}
