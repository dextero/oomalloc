#ifndef OOMALLOC_TEST_H
#define OOMALLOC_TEST_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void oomalloc_reset(void);

void *oomalloc_malloc(size_t size);
void oomalloc_free(void *ptr);
void *oomalloc_calloc(size_t nmemb, size_t size);
void *oomalloc_realloc(void *ptr, size_t size);

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */

#endif /* OOMALLOC_TEST_H */
