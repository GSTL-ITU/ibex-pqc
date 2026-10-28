/*
 * Cycle counts of the three Dilithium (ML-DSA) operations: key generation,
 * signing and verification, plus the peak stack usage.
 *
 * Each call is measured on its own, with nothing but the call between the two
 * counter reads; all printing happens after the measurements. The Dilithium
 * sources come from the dilithium submodule (see lib/pqc/dilithium.mk). Message length and
 * context are the same as in test_dilithium.c.
 *
 * The three operations run N_TEST times (make ... N_TEST=10), each run with
 * its own randombytes() seed (lib/pqc/bench.h), so its own keys, message and
 * signing randomness. Run 0 uses the default seed. Prints every run, then
 * median, min and max per operation. Signing time depends on how many
 * rejection-sampling rounds an input needs, so it varies between runs.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "randombytes.h"
#include "sign.h"
#include "bench.h"
#include "cycles.h"
#include "stack.h"

#define MLEN   59
#define CTXLEN 14

int main(void)
{
    static uint8_t ctx[CTXLEN];
    static uint8_t m[MLEN];
    static uint8_t m2[MLEN + CRYPTO_BYTES];
    static uint8_t sm[MLEN + CRYPTO_BYTES];
    static uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    static uint8_t sk[CRYPTO_SECRETKEYBYTES];
    static uint64_t cyc_keypair[N_TEST], cyc_sign[N_TEST], cyc_open[N_TEST];
    size_t smlen, mlen;
    int ret, failed = 0;
    uint64_t t0, overhead;

    snprintf((char *)ctx, CTXLEN, "test_dilitium");

    stack_paint();
    cycles_enable();
    overhead = cycles_overhead();

    for (unsigned i = 0; i < N_TEST; i++) {
        randombytes_seed(bench_seed(i));
        randombytes(m, MLEN);

        t0 = cycles_read();
        crypto_sign_keypair(pk, sk);
        cyc_keypair[i] = cycles_read() - t0 - overhead;

        t0 = cycles_read();
        crypto_sign(sm, &smlen, m, MLEN, ctx, CTXLEN, sk);
        cyc_sign[i] = cycles_read() - t0 - overhead;

        t0 = cycles_read();
        ret = crypto_sign_open(m2, &mlen, sm, smlen, ctx, CTXLEN, pk);
        cyc_open[i] = cycles_read() - t0 - overhead;

        if (ret != 0 || mlen != MLEN || smlen != MLEN + CRYPTO_BYTES ||
            memcmp(m, m2, MLEN) != 0)
            failed = 1;
    }

    /* Everything below is outside the measured regions */
    printf("Dilithium%d, %u runs\n", DILITHIUM_MODE, N_TEST);
    for (unsigned i = 0; i < N_TEST; i++)
        printf("run %u seed 0x%08x: keypair %llu, sign %llu, open %llu cycles\n",
               i, (unsigned)bench_seed(i), (unsigned long long)cyc_keypair[i],
               (unsigned long long)cyc_sign[i], (unsigned long long)cyc_open[i]);
    /* Run 0 alone, the same lines as the single-run test printed */
    cycles_print("crypto_sign_keypair", cyc_keypair[0]);
    cycles_print("crypto_sign        ", cyc_sign[0]);
    cycles_print("crypto_sign_open   ", cyc_open[0]);
    bench_summary("crypto_sign_keypair", cyc_keypair, N_TEST);
    bench_summary("crypto_sign        ", cyc_sign, N_TEST);
    bench_summary("crypto_sign_open   ", cyc_open, N_TEST);
    cycles_print("measurement overhead (subtracted)", overhead);
    printf("peak stack usage: %u of %u bytes\n",
           (unsigned)stack_max_used(), (unsigned)stack_size());

    if (stack_overflowed()) {
        printf("ERROR: stack overflow, raise STACK_SIZE in test.mk\n");
        return 1;
    }
    /* The measurements only count if the operations were correct */
    if (failed) {
        printf("ERROR: signature verification failed\n");
        return 1;
    }
    printf("signature verified\n");
    return 0;
}
