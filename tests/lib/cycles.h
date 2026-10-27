/*
 * Cycle counting with the standard RISC-V mcycle/mcycleh CSRs.
 * Same API as pulpissimo-pqc/tests/env/include/cycles.h, so tests port as is.
 *
 * The counter counts core clock cycles, so results are comparable between
 * systems and hardware as long as no output happens inside the measured
 * region. (Output in simulation is a single store, but on hardware it would
 * be a slow UART.)
 *
 * Usage:
 *   cycles_enable();                    // once
 *   uint64_t overhead = cycles_overhead();
 *   uint64_t t0 = cycles_read();
 *   f();
 *   uint64_t n = cycles_read() - t0 - overhead;
 *   ... print n afterwards (cycles_print) ...
 */
#ifndef CYCLES_H
#define CYCLES_H

#include <stdint.h>
#include <stdio.h>

#define CSR_MCOUNTINHIBIT 0x320
#define CSR_MCYCLE        0xB00
#define CSR_MCYCLEH       0xB80

/* Clear mcountinhibit.CY. Ibex counts from reset, but other cores (and code
 * that ran before) may have stopped the counter. */
static inline void cycles_enable(void)
{
    asm volatile("csrci %0, 1" :: "i"(CSR_MCOUNTINHIBIT));
}

/* 64-bit cycle count. Re-reads if the low word wrapped between the reads. */
static inline uint64_t cycles_read(void)
{
    uint32_t hi, lo, hi2;
    do {
        asm volatile("csrr %0, %1" : "=r"(hi)  : "i"(CSR_MCYCLEH));
        asm volatile("csrr %0, %1" : "=r"(lo)  : "i"(CSR_MCYCLE));
        asm volatile("csrr %0, %1" : "=r"(hi2) : "i"(CSR_MCYCLEH));
    } while (hi != hi2);
    return ((uint64_t)hi << 32) | lo;
}

/* Cycles measured for an empty region, i.e. the cost of cycles_read() itself.
 * Subtract it from measurements to get the cycles of the code in between. */
static inline uint64_t cycles_overhead(void)
{
    uint64_t t0 = cycles_read();
    uint64_t t1 = cycles_read();
    return t1 - t0;
}

/* Prints "label: n cycles" */
static inline void cycles_print(const char *label, uint64_t cycles)
{
    printf("%s: %llu cycles\n", label, (unsigned long long)cycles);
}

#endif
