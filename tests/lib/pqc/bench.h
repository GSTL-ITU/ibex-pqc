/*
 * Repeated cycle measurements for the *_cycles tests: one seed per run, and
 * median / min / max over the runs.
 *
 * Run i restarts randombytes() from bench_seed(i). Run 0 uses the default
 * seed (RANDOMBYTES_SEED), so it reproduces the single-run results; the other
 * seeds are derived from it. Each run therefore has its own keys (and for
 * Dilithium its own message and signing randomness).
 */
#ifndef BENCH_H
#define BENCH_H

#include <stdint.h>
#include <stdio.h>

#ifndef RANDOMBYTES_SEED
#define RANDOMBYTES_SEED 0x2545F491u
#endif

/* Number of runs, from N_TEST in test.mk */
#ifndef N_TEST
#define N_TEST 1
#endif

/* lib/pqc/randombytes.c */
void randombytes_seed(uint32_t seed);

/* Seed of run i: the default seed for run 0, otherwise a hash of i (the
 * finalizer of MurmurHash3), so neighbouring runs get unrelated seeds */
static inline uint32_t bench_seed(unsigned i)
{
    uint32_t x;
    if (i == 0)
        return RANDOMBYTES_SEED;
    x = RANDOMBYTES_SEED ^ (i * 0x9E3779B9u);
    x ^= x >> 16;
    x *= 0x85EBCA6Bu;
    x ^= x >> 13;
    x *= 0xC2B2AE35u;
    x ^= x >> 16;
    return x ? x : 1;
}

/* Prints "label: median M, min A, max B cycles (n runs)". Sorts v. */
static inline void bench_summary(const char *label, uint64_t *v, unsigned n)
{
    uint64_t median;
    for (unsigned i = 1; i < n; i++) {  /* insertion sort, n is small */
        uint64_t x = v[i];
        unsigned j = i;
        for (; j > 0 && v[j - 1] > x; j--)
            v[j] = v[j - 1];
        v[j] = x;
    }
    /* Even n: mean of the two middle values (rounded down) */
    median = (n % 2) ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) / 2;
    printf("%s: median %llu, min %llu, max %llu cycles (%u runs)\n", label,
           (unsigned long long)median, (unsigned long long)v[0],
           (unsigned long long)v[n - 1], n);
}

#endif
