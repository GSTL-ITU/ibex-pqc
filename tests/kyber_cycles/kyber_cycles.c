/*
 * Cycle counts of the three Kyber KEM operations: key generation,
 * encapsulation and decapsulation, plus the peak stack usage.
 * Ported from pulpissimo-pqc/tests/kyber_cycles.
 *
 * Each call is measured on its own, with nothing but the call between the two
 * counter reads; all printing happens after the measurements. The Kyber
 * sources come from the kyber submodule (see lib/pqc/kyber.mk).
 *
 * The three operations run N_TEST times (make ... N_TEST=10), each run with
 * its own randombytes() seed (lib/pqc/bench.h). Run 0 uses the default seed.
 * Prints every run, then median, min and max per operation.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "kem.h"
#include "bench.h"
#include "cycles.h"
#include "stack.h"

int main(void)
{
    static uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    static uint8_t sk[CRYPTO_SECRETKEYBYTES];
    static uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
    static uint8_t key_a[CRYPTO_BYTES];
    static uint8_t key_b[CRYPTO_BYTES];
    static uint64_t cyc_keypair[N_TEST], cyc_enc[N_TEST], cyc_dec[N_TEST];
    uint64_t t0, overhead;
    int keys_differ = 0;

    stack_paint();
    cycles_enable();
    overhead = cycles_overhead();

    for (unsigned i = 0; i < N_TEST; i++) {
        randombytes_seed(bench_seed(i));

        t0 = cycles_read();
        crypto_kem_keypair(pk, sk);
        cyc_keypair[i] = cycles_read() - t0 - overhead;

        t0 = cycles_read();
        crypto_kem_enc(ct, key_b, pk);
        cyc_enc[i] = cycles_read() - t0 - overhead;

        t0 = cycles_read();
        crypto_kem_dec(key_a, ct, sk);
        cyc_dec[i] = cycles_read() - t0 - overhead;

        if (memcmp(key_a, key_b, CRYPTO_BYTES))
            keys_differ = 1;
    }

    /* Everything below is outside the measured regions */
    printf("Kyber%d, %u runs\n", KYBER_K * 256, N_TEST);
    for (unsigned i = 0; i < N_TEST; i++)
        printf("run %u seed 0x%08x: keypair %llu, enc %llu, dec %llu cycles\n",
               i, (unsigned)bench_seed(i), (unsigned long long)cyc_keypair[i],
               (unsigned long long)cyc_enc[i], (unsigned long long)cyc_dec[i]);
    /* Run 0 alone, the same lines as the single-run test printed */
    cycles_print("crypto_kem_keypair", cyc_keypair[0]);
    cycles_print("crypto_kem_enc    ", cyc_enc[0]);
    cycles_print("crypto_kem_dec    ", cyc_dec[0]);
    bench_summary("crypto_kem_keypair", cyc_keypair, N_TEST);
    bench_summary("crypto_kem_enc    ", cyc_enc, N_TEST);
    bench_summary("crypto_kem_dec    ", cyc_dec, N_TEST);
    cycles_print("measurement overhead (subtracted)", overhead);
    printf("peak stack usage: %u of %u bytes\n",
           (unsigned)stack_max_used(), (unsigned)stack_size());

    if (stack_overflowed()) {
        printf("ERROR: stack overflow, raise STACK_SIZE in test.mk\n");
        return 1;
    }
    /* The measurements only count if the operations were correct */
    if (keys_differ) {
        printf("ERROR: shared keys differ\n");
        return 1;
    }
    printf("shared keys match\n");
    return 0;
}
