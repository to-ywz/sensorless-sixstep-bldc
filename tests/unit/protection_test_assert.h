#ifndef PROTECTION_TEST_ASSERT_H
#define PROTECTION_TEST_ASSERT_H
#include <stdio.h>
#include <stdlib.h>
/* No CRT abort/dialog on Windows, no NDEBUG bypass; CI fails immediately. */
#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr,"CHECK failed: %s (%s:%d)\n", \
                #condition,__FILE__,__LINE__); \
        exit(EXIT_FAILURE); \
    } \
} while (0)
#endif
