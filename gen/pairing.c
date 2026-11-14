/* S8c (Q-S8c-2 (a), in the form of Q-S8b2-17 (a)): print the pairing
   constants of domain/domain.lang, one def on each line: frobG1 to frobG5
   (g^1 to g^5 with g = xi^((p - 1) / 6) and xi = 9 + u), twistB (3 / xi),
   g2Gen (the G2 generator of src/crypto/pairing.c), millerBits (the 64
   bits of 6 x + 2 below its top bit 2^64) and bnXBits (the bits of x from
   its top 1 bit). A bit list has the top bit first. Each Fe value is in
   Montgomery form (a R mod p, R = 2^256). The values come from the
   definitions and from the public functions of src/crypto/pairing.h.
   test/gate.sh compares the print with domain/domain.lang.
   In the form of Q-S8b2-30 (a), `pairinggen c` prints
   src/front/pairing_c.h, the table of pairingCheckC in src/front/eval.c:
   the same Words and bits as frobG1 to frobG5, twistB, millerBits and
   bnXBits. test/gate.sh compares the print with that file. */
#include <stdio.h>
#include <string.h>

#include "crypto/pairing.h"

/* Print A R mod p as a guest Fe in parentheses. R1 is R mod p. */
static void put_fe(const Fe *a, const Fe *r1) {
  Fe m;
  uint32_t i;
  fe_mul(&m, a, r1, &BN254_FP);
  printf("(fe");
  for (i = 0; i < BN_LIMBS; i++)
    printf(" 0x%08xw", m.w[i]);
  printf(")");
}

static void put_fp2(const Fp2 *a, const Fe *r1) {
  printf("fp2 ");
  put_fe(&a->c0, r1);
  printf(" ");
  put_fe(&a->c1, r1);
}

/* The low N bits of V as the FeBits def NAME, the top bit first. */
static void put_bits(const char *name, uint64_t v, uint32_t n) {
  uint32_t i;
  printf("def %s : FeBits := ", name);
  for (i = n; i > 0u; i--)
    printf("%s (", ((v >> (i - 1u)) & 1u) != 0u ? "fbOne" : "fbZero");
  printf("fbEnd");
  for (i = 0; i < n; i++)
    printf(")");
  printf("\n");
}

/* Print A R mod p as 8 Words in braces, limb 0 first. R1 is R mod p. */
static void put_words(const Fe *a, const Fe *r1) {
  Fe m;
  uint32_t i;
  fe_mul(&m, a, r1, &BN254_FP);
  printf("{");
  for (i = 0; i < BN_LIMBS; i++)
    printf("%s0x%08xu", i == 0u ? "" : ", ", m.w[i]);
  printf("}");
}

/* Print the Fp2 A as {c0, c1}, each as 8 Words. */
static void put_fp2_words(const Fp2 *a, const Fe *r1) {
  printf("{");
  put_words(&a->c0, r1);
  printf(", ");
  put_words(&a->c1, r1);
  printf("}");
}

/* The low N bits of V as the uint8_t table NAME, the top bit first. */
static void put_bit_table(const char *name, uint64_t v, uint32_t n) {
  uint32_t i;
  printf("static const uint8_t %s[%u] = {", name, (unsigned)n);
  for (i = n; i > 0u; i--)
    printf("%s%u", i == n ? "" : ", ", (unsigned)((v >> (i - 1u)) & 1u));
  printf("};\n");
}

/* The text of src/front/pairing_c.h. FROB holds g^1 to g^5, B is twistB
   and XN is the bit count of x. R1 is R mod p. */
static void put_c(const Fp2 *frob, const Fp2 *b, uint32_t xn, const Fe *r1) {
  uint32_t i;
  printf("/* S8c (Q-S8c-2 (a)): the pairing table of pairingCheckC in\n");
  printf("   src/front/eval.c. `build/pairinggen c` prints this file, and\n");
  printf("   test/gate.sh compares the two: do not edit it by hand. Each Fe\n");
  printf("   value is 8 Words, limb 0 is the low Word, in Montgomery form\n");
  printf("   (x R mod p, R = 2^256). pr_c_frob holds frobG1 to frobG5 and\n");
  printf("   pr_c_twist_b holds twistB of domain/domain.lang, each Fp2 as c0,\n");
  printf("   then c1. pr_c_miller holds millerBits and pr_c_x holds bnXBits,\n");
  printf("   the top bit first. */\n");
  printf("#ifndef LANG_FRONT_PAIRING_C_H\n");
  printf("#define LANG_FRONT_PAIRING_C_H\n\n");
  printf("#include <stdint.h>\n\n");
  printf("#define PR_C_MILLER_N 64u\n");
  printf("#define PR_C_X_N %uu\n\n", (unsigned)xn);
  printf("static const uint32_t pr_c_frob[5][2][8] = {\n");
  for (i = 0; i < 5u; i++) {
    printf("  ");
    put_fp2_words(&frob[i], r1);
    printf("%s\n", i + 1u < 5u ? "," : "");
  }
  printf("};\n");
  printf("static const uint32_t pr_c_twist_b[2][8] = ");
  put_fp2_words(b, r1);
  printf(";\n");
  put_bit_table("pr_c_miller", PAIRING_LOOP_LOW, 64u);
  put_bit_table("pr_c_x", PAIRING_X, xn);
  printf("\n#endif\n");
}

int main(int argc, char **argv) {
  Fe one;
  Fe less;
  Fe r1;
  U256 e;
  U256 six;
  Fp2 xi;
  Fp2 g;
  Fp2 gi;
  Fp2 b;
  Fp2 frob[5];
  G2 q;
  uint64_t x = PAIRING_X;
  uint32_t n = 0;
  uint32_t i;
  /* R mod p: 1, doubled 256 times. */
  fe_from_u64(&r1, 1u);
  for (i = 0; i < 256u; i++)
    fe_add(&r1, &r1, &r1, &BN254_FP);
  /* (p - 1) / 6: p - 1 is the negative of 1 mod p. */
  fe_from_u64(&one, 1u);
  fe_neg(&less, &one, &BN254_FP);
  fe_to_u256(&e, &less);
  u256_from_u64(&six, 6u);
  u256_div(&e, &e, &six);
  fp2_zero(&xi);
  fe_from_u64(&xi.c0, 9u);
  fe_from_u64(&xi.c1, 1u);
  fp2_pow(&g, &xi, &e);
  gi = g;
  for (i = 0; i < 5u; i++) {
    frob[i] = gi;
    fp2_mul(&gi, &gi, &g);
  }
  /* 3 / xi: the sum of 3 times 1 / xi. */
  fp2_inv(&gi, &xi);
  fp2_add(&b, &gi, &gi);
  fp2_add(&b, &b, &gi);
  while (n < 64u && (x >> n) != 0u)
    n++;
  if (argc == 2 && strcmp(argv[1], "c") == 0) {
    put_c(frob, &b, n, &r1);
    return 0;
  }
  for (i = 0; i < 5u; i++) {
    printf("def frobG%u : Fp2 := ", (unsigned)(i + 1u));
    put_fp2(&frob[i], &r1);
    printf("\n");
  }
  printf("def twistB : Fp2 := ");
  put_fp2(&b, &r1);
  printf("\n");
  g2_generator(&q);
  printf("def g2Gen : Pt2 := pt2 (");
  put_fp2(&q.x, &r1);
  printf(") (");
  put_fp2(&q.y, &r1);
  printf(")\n");
  put_bits("millerBits", PAIRING_LOOP_LOW, 64u);
  put_bits("bnXBits", x, n);
  return 0;
}
