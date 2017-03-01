#include <gtest/gtest.h>

#include "oomalloc.h"
#include "../src/utils.h"

TEST(parse_size, leading_plus) {
    size_t value = 1;
    ASSERT_EQ(0, parse_size("+100", &value, 0));
    ASSERT_EQ(value, 100u);
}

TEST(parse_size, base_zero) {
    size_t value;

    value = 1;
    ASSERT_EQ(0, parse_size("0", &value, 0));
    ASSERT_EQ(value, 0u);

    value = 1;
    ASSERT_EQ(0, parse_size("100", &value, 0));
    ASSERT_EQ(value, 100u);

    value = 1;
    ASSERT_EQ(0, parse_size("0x100", &value, 0));
    ASSERT_EQ(value, 0x100u);
}

TEST(parse_size, base_ten) {
    size_t value;

    value = 1;
    ASSERT_EQ(0, parse_size("0", &value, 10));
    ASSERT_EQ(value, 0u);

    value = 1;
    ASSERT_EQ(0, parse_size("100", &value, 10));
    ASSERT_EQ(value, 100u);

    ASSERT_EQ(-1, parse_size("a", &value, 10));
    ASSERT_EQ(-1, parse_size("0x1", &value, 10));
}

TEST(parse_size, base_hex) {
    size_t value;

    value = 1;
    ASSERT_EQ(0, parse_size("5", &value, 16));
    ASSERT_EQ(value, 5u);

    value = 1;
    ASSERT_EQ(0, parse_size("af", &value, 16));
    ASSERT_EQ(value, 0xAFu);

    value = 1;
    ASSERT_EQ(0, parse_size("0xfFf", &value, 16));
    ASSERT_EQ(value, 0xFFFu);

    ASSERT_EQ(-1, parse_size("g", &value, 16));
}

TEST(parse_size, base_custom) {
    size_t value;
    int base = 10 + 'z' - 'a' + 1;
    size_t z = base - 1;

    value = 1;
    ASSERT_EQ(0, parse_size("z", &value, base));
    ASSERT_EQ(value, z);

    value = 1;
    ASSERT_EQ(0, parse_size("Z", &value, base));
    ASSERT_EQ(value, z);

    value = 1;
    ASSERT_EQ(0, parse_size("zz", &value, base));
    ASSERT_EQ(value, z * base + z);

    value = 1;
    ASSERT_EQ(0, parse_size("ZZ", &value, base));
    ASSERT_EQ(value, z * base + z);
}

TEST(parse_size, invalid) {
    size_t value;

    ASSERT_EQ(-1, parse_size(",", &value, 0));
    ASSERT_EQ(-1, parse_size(" 1", &value, 0));
    ASSERT_EQ(-1, parse_size("1 ", &value, 0));
}

TEST(size_to_string, correct) {
    char buf[32];

    ASSERT_EQ(1, size_to_string(buf, sizeof(buf), 0));
    ASSERT_STREQ("0", buf);

    ASSERT_EQ(1, size_to_string(buf, sizeof(buf), 9));
    ASSERT_STREQ("9", buf);

    // 2**64 - 1
    ASSERT_EQ((ssize_t)(sizeof("18446744073709551615") - 1),
              size_to_string(buf, sizeof(buf), 18446744073709551615ull));
    ASSERT_STREQ("18446744073709551615", buf);
}

TEST(size_to_string, buffer_too_small) {
    char buf[32];

    ASSERT_EQ(-1, size_to_string(buf, 0, 0));
    ASSERT_EQ(-1, size_to_string(buf, 1, 0));
    ASSERT_EQ(-1, size_to_string(buf, 2, 10));
    ASSERT_EQ(-1, size_to_string(buf,
                                 sizeof("18446744073709551615") - 1,
                                 18446744073709551615ull));
}

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

TEST_F(OomallocTest, memory_limit_zero_malloc) {
    oomalloc_memory_limit(0);
    ASSERT_TRUE(NULL == oomalloc_malloc(1));
}

TEST_F(OomallocTest, memory_limit_zero_calloc) {
    oomalloc_memory_limit(0);
    ASSERT_TRUE(NULL == oomalloc_calloc(1, 1));
}

TEST_F(OomallocTest, memory_limit_zero_realloc_null) {
    oomalloc_memory_limit(0);
    ASSERT_TRUE(NULL == oomalloc_realloc(NULL, 1));
}

TEST_F(OomallocTest, memory_limit_zero_realloc_nonnull) {
    void *p = oomalloc_malloc(1);
    ASSERT_TRUE(NULL != p);

    oomalloc_memory_limit(0);
    ASSERT_TRUE(NULL != oomalloc_realloc(p, 1));

    oomalloc_free(p);
}

TEST_F(OomallocTest, memory_limit_nonzero_malloc) {
    oomalloc_memory_limit(4096 + MALLOC_OVERHEAD_MARGIN);

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

TEST_F(OomallocTest, memory_limit_nonzero_calloc) {
    oomalloc_memory_limit(4096 + MALLOC_OVERHEAD_MARGIN);

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

TEST_F(OomallocTest, memory_limit_nonzero_realloc) {
    oomalloc_memory_limit(4096 + MALLOC_OVERHEAD_MARGIN);

    void *p;

    ASSERT_TRUE(NULL != (p = oomalloc_realloc(NULL, 2048)));
    ASSERT_TRUE(NULL == oomalloc_realloc(NULL, 4096));
    ASSERT_TRUE(NULL != (p = oomalloc_realloc(p, 4096)));
    ASSERT_TRUE(NULL == oomalloc_realloc(NULL, MALLOC_OVERHEAD_MARGIN));
    oomalloc_free(p);

    ASSERT_TRUE(NULL == oomalloc_realloc(NULL, 4096 + 2 * MALLOC_OVERHEAD_MARGIN));
}
