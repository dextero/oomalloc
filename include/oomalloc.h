#ifndef OOMALLOC_H
#define OOMALLOC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define OOMALLOC_UNLIMITED SIZE_MAX

void oomalloc_memory_limit(size_t max_bytes);

#define OOMALLOC_DONT_FAIL SIZE_MAX

void oomalloc_fail_after(size_t n);

void oomalloc_set_log_enabled(bool enabled);

void oomalloc_reset(void);


#ifdef OOMALLOC_TEST
void *oomalloc_malloc(size_t size);
void oomalloc_free(void *ptr);
void *oomalloc_calloc(size_t nmemb, size_t size);
void *oomalloc_realloc(void *ptr, size_t size);
#endif // OOMALLOC_TEST


#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */

#endif /* OOMALLOC_H */
