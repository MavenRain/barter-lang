/* S8b2 (Q-S8b-5 (a), Q-S8b2-17 (a)): print the Poseidon constants of
   domain/domain.lang: the 9 MDS defs psM00 to psM22, then psAll (65 round
   nodes: 4 prFull, 57 prPart, 4 prFull). The values come from
   poseidon_params of src/crypto/poseidon.c on BN254_FR. Each value is in
   Montgomery form (x R mod r, R = 2^256). test/gate.sh compares the print
   with domain/domain.lang.
   S8b2 (Q-S8b2-30 (a)): `poseidongen c` prints src/front/poseidon_c.h, the
   table of poseidon2C in src/front/eval.c: the same 204 values as C arrays
   (limb 0 is the low Word), r, n0' (-r^-1 mod 2^32) and R^2 mod r.
   test/gate.sh compares that print with the file. */
#include <stdio.h>
#include <string.h>

#include "crypto/poseidon.h"

/* OUT = X R mod r. R1 is R mod r. */
static void to_mont(Fe *out, const Fe *x, const Fe *r1) {
  fe_mul(out, x, r1, &BN254_FR);
}

static void put_fe(const Fe *a) {
  uint32_t i;
  printf("fe");
  for (i = 0; i < BN_LIMBS; i++)
    printf(" 0x%08xw", a->w[i]);
}

/* The 8 Words of W as a C initializer, limb 0 first. */
static void put_words(const uint32_t *w) {
  uint32_t i;
  printf("{");
  for (i = 0; i < BN_LIMBS; i++)
    printf("0x%08xu%s", w[i], i + 1u < BN_LIMBS ? ", " : "}");
}

/* The text of src/front/poseidon_c.h. R1 is R mod r. */
static void put_c(const PoseidonParams *p, const Fe *r1) {
  Fe m;
  Fe r2;
  uint32_t inv = 1u;
  uint32_t i;
  uint32_t j;
  /* r^-1 mod 2^32: each Newton step doubles the number of good low bits
     (1, 2, 4, 8, 16, 32). */
  for (i = 0; i < 5u; i++)
    inv *= 2u - BN254_FR.w[0] * inv;
  /* R^2 mod r: fe_mul gives the canonical product. */
  fe_mul(&r2, r1, r1, &BN254_FR);
  printf("/* S8b2 (Q-S8b2-30 (a)): the Poseidon table of poseidon2C in\n");
  printf("   src/front/eval.c. `build/poseidongen c` prints this file, and\n");
  printf("   test/gate.sh compares the two: do not edit it by hand. Each value\n");
  printf("   is 8 Words, limb 0 is the low Word. The MDS and round values are in\n");
  printf("   Montgomery form (x R mod r, R = 2^256), in the order of psM00 to\n");
  printf("   psM22 and psAll in domain/domain.lang. */\n");
  printf("#ifndef LANG_FRONT_POSEIDON_C_H\n");
  printf("#define LANG_FRONT_POSEIDON_C_H\n\n");
  printf("#include <stdint.h>\n\n");
  printf("#define PS_C_ROUNDS %uu\n", (unsigned)POSEIDON_ROUNDS);
  printf("#define PS_C_HALF %uu\n", (unsigned)(POSEIDON_RF / 2u));
  printf("#define PS_C_PART %uu\n", (unsigned)POSEIDON_RP);
  printf("#define PS_C_N0 0x%08xu\n\n", (unsigned)(0u - inv));
  printf("static const uint32_t ps_c_r[8] = ");
  put_words(BN254_FR.w);
  printf(";\n");
  printf("static const uint32_t ps_c_r2[8] = ");
  put_words(r2.w);
  printf(";\n");
  printf("static const uint32_t ps_c_mds[%u][8] = {\n", (unsigned)(POSEIDON_WIDTH * POSEIDON_WIDTH));
  for (i = 0; i < POSEIDON_WIDTH; i++)
    for (j = 0; j < POSEIDON_WIDTH; j++) {
      to_mont(&m, &p->mds[i][j], r1);
      printf("  ");
      put_words(m.w);
      printf("%s\n", i * POSEIDON_WIDTH + j + 1u < POSEIDON_WIDTH * POSEIDON_WIDTH ? "," : "");
    }
  printf("};\n");
  printf("static const uint32_t ps_c_rc[%u][8] = {\n", (unsigned)POSEIDON_CONSTANTS);
  for (i = 0; i < POSEIDON_CONSTANTS; i++) {
    to_mont(&m, &p->rc[i], r1);
    printf("  ");
    put_words(m.w);
    printf("%s\n", i + 1u < POSEIDON_CONSTANTS ? "," : "");
  }
  printf("};\n\n");
  printf("#endif\n");
}

int main(int argc, char **argv) {
  static PoseidonParams p;
  Fe r1;
  Fe m;
  uint32_t half = POSEIDON_RF / 2u;
  uint32_t r;
  uint32_t i;
  uint32_t j;
  poseidon_params(&p, &BN254_FR);
  /* R mod r: 1, doubled 256 times. */
  fe_from_u64(&r1, 1u);
  for (i = 0; i < 256u; i++)
    fe_add(&r1, &r1, &r1, &BN254_FR);
  if (argc == 2 && strcmp(argv[1], "c") == 0) {
    put_c(&p, &r1);
    return 0;
  }
  for (i = 0; i < POSEIDON_WIDTH; i++)
    for (j = 0; j < POSEIDON_WIDTH; j++) {
      to_mont(&m, &p.mds[i][j], &r1);
      printf("def psM%u%u : Fe := ", (unsigned)i, (unsigned)j);
      put_fe(&m);
      printf("\n");
    }
  printf("def psAll : PsRounds :=\n");
  for (r = 0; r < POSEIDON_ROUNDS; r++) {
    printf("  (%s", r < half || r >= half + POSEIDON_RP ? "prFull" : "prPart");
    for (i = 0; i < POSEIDON_WIDTH; i++) {
      to_mont(&m, &p.rc[r * POSEIDON_WIDTH + i], &r1);
      printf(" (");
      put_fe(&m);
      printf(")");
    }
    printf("\n");
  }
  printf("  prEnd");
  for (r = 0; r < POSEIDON_ROUNDS; r++)
    printf(")");
  printf("\n");
  return 0;
}
