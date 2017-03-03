#ifndef OOMALLOC_H
#define OOMALLOC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** Constant used with @ref oomalloc_limit_heap to disable heap memory limit. */
#define OOMALLOC_UNLIMITED SIZE_MAX

/**
 * Sets a hard limit on heap memory possible to allocate via calls to malloc,
 * calloc and realloc. The library keeps track of amount of already allocated
 * memory and returns NULL if an allocation would exceeded the limit.
 *
 * Calling this function with OOMALLOC_UNLIMITED constant disables memory
 * usage check.
 *
 * Default value is configured by the OOMALLOC_LIMIT_HEAP environment variable.
 * If OOMALLOC_LIMIT_HEAP is not set or is not a valid positive integer, the
 * library assumes no limit.
 *
 * NOTE: configured limit may not be exactly accurate, as allocation functions
 * require some extra memory for internal bookkeeping.
 *
 * @param max_bytes Maximum number of heap bytes an application is allowed
 *                  to allocate.
 */
void oomalloc_heap_limit(size_t max_bytes);

/**
 * Constant used with @ref oomalloc_fail_after to disable failing after
 * a particular number of allocations.
 */
#define OOMALLOC_DONT_FAIL SIZE_MAX

/**
 * Forces a memory allocation failure after @p n successful allocations via
 * calls to malloc, calloc or realloc. Allocation failure happens only once,
 * unless it is requested by another @ref oomalloc_fail_after call afterwards.
 *
 * When called multiple times, only the last call applies.
 *
 * Calling this function with OOMALLOC_DONT_FAIL constant disables this
 * behavior.
 *
 * Default value is configured by the OOMALLOC_FAIL_AFTER environment variable.
 * If OOMALLOC_FAIL_AFTER is not set or is not a valid positive integer, the
 * library behaves as if it was configured with OOMALLOC_DONT_FAIL.
 *
 * @param n Number of calls that should succeed before the library triggers
 *          an allocation failure. The value is relative to the call place,
 *          i.e. oomalloc_fail_after(0) call makes the next allocation fail
 *          regardless of the number of previous allocations.
 */
void oomalloc_fail_after(size_t n);

/**
 * Enables of disables verbose logs from all allocations.
 *
 * By default logging is disabled, unless OOMALLOC_LOG environment variable is
 * set to any value (including an empty string).
 */
void oomalloc_set_log_enabled(bool enabled);

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */

#endif /* OOMALLOC_H */
