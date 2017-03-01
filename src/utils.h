#ifndef OOMALLOC_UTILS_H
#define OOMALLOC_UTILS_H

#include <stdlib.h>

static inline bool is_digit(char c) {
    return '0' <= c && c <= '9';
}

static inline bool is_lowercase_letter(char c) {
    return 'a' <= c && c <= 'z';
}

static inline bool is_uppercase_letter(char c) {
    return 'A' <= c && c <= 'Z';
}

static inline int parse_size(const char *str,
                             size_t *out_value,
                             int base) {
    if (str[0] == '+') {
        str += 1;
    }

    if ((base == 16 || base == 0)
            && str[0] == '0'
            && (str[1] == 'x' || str[1] == 'X')) {
        base = 16;
        str += 2;
    }

    if (base == 0) {
        base = 10;
    }

    *out_value = 0;
    while (*str) {
        if (is_digit(*str)) {
            *out_value = *out_value * base + (*str - '0');
        } else if (is_lowercase_letter(*str) && *str - 'a' < base - 10) {
            *out_value = *out_value * base + *str - 'a' + 10;
        } else if (is_uppercase_letter(*str) && *str - 'A' < base - 10) {
            *out_value = *out_value * base + *str - 'A' + 10;
        } else {
            return -1;
        }
        ++str;
    }

    return 0;
}

static inline ssize_t size_to_string(char *out_buf,
                                     size_t buf_size,
                                     size_t n) {
    if (buf_size < 2) {
        return -1;
    }

    if (!n) {
        out_buf[0] = '0';
        out_buf[1] = '\0';
        return 1;
    } else {
        int num_digits = 0;

        for (size_t num = n; num; num /= 10) {
            ++num_digits;
        }

        if (buf_size <= (size_t)num_digits) {
            return -1;
        }

        for (int i = 0; i < num_digits; ++i) {
            out_buf[num_digits - i - 1] = '0' + n % 10;
            n /= 10;
        }

        out_buf[num_digits] = '\0';
        return num_digits;
    }
}

#endif // OOMALLOC_UTILS_H
