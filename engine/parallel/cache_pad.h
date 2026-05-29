#ifndef PARGUS_CACHE_PAD_H
#define PARGUS_CACHE_PAD_H

#define PARGUS_CACHE_LINE_SIZE 64

#if defined(_MSC_VER)
#define PARGUS_CACHE_ALIGNED __declspec(align(PARGUS_CACHE_LINE_SIZE))
#elif defined(__GNUC__) || defined(__clang__)
#define PARGUS_CACHE_ALIGNED __attribute__((aligned(PARGUS_CACHE_LINE_SIZE)))
#else
#define PARGUS_CACHE_ALIGNED
#endif

typedef struct PARGUS_CACHE_ALIGNED {
    double value;
    char pad[PARGUS_CACHE_LINE_SIZE - sizeof(double)];
} PaddedDouble;

#endif
