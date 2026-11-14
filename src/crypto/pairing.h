/* The C oracle of the BN254 pairing check (S8c G1, Q-S8c-2 (a)): the tower
   Fp2, Fp6, Fp12, the group G2 on the twist, the optimal ate Miller loop,
   the final power and the check of EIP-197. This is the oracle of
   test/cryptotool.c: it is plain and slow, and langc does not link it.
   See crypto/bn254.h for Fp and G1. */
#ifndef LANG_CRYPTO_PAIRING_H
#define LANG_CRYPTO_PAIRING_H

#include <stddef.h>
#include <stdint.h>

#include "crypto/bn254.h"

/* c0 + c1 u, with u^2 = -1. */
typedef struct {
  Fe c0;
  Fe c1;
} Fp2;

/* c0 + c1 v + c2 v^2, with v^3 = xi and xi = 9 + u. */
typedef struct {
  Fp2 c0;
  Fp2 c1;
  Fp2 c2;
} Fp6;

/* a + b w, with w^2 = v. */
typedef struct {
  Fp6 a;
  Fp6 b;
} Fp12;

/* A point of the twist y^2 = x^3 + 3 / xi over Fp2, in affine form.
   (0, 0) is the point at infinity (EIP-197). */
typedef struct {
  Fp2 x;
  Fp2 y;
} G2;

/* A point (X, Y, Z) of the twist in Jacobian form: the affine point
   (X / Z^2, Y / Z^3). Z = 0 is the point at infinity. */
typedef struct {
  Fp2 x;
  Fp2 y;
  Fp2 z;
} G2Jac;

/* A line value c0 + c1 w + c3 w^3 (the 3 parts of an Fp12 value that a
   line step can set). */
typedef struct {
  Fp2 c0;
  Fp2 c1;
  Fp2 c3;
} Line;

typedef struct {
  G1 p;
  G2 q;
} Pair;

/* The exponent of the loop is 6 x + 2 = 2^64 + PAIRING_LOOP_LOW. */
#define PAIRING_X 4965661367192848881ULL
#define PAIRING_LOOP_LOW ((uint64_t)(6ULL * PAIRING_X + 2ULL))

/* Fp2. OUT can be A or B. */
void fp2_zero(Fp2 *out);
void fp2_one(Fp2 *out);
int fp2_is_zero(const Fp2 *a);
int fp2_eq(const Fp2 *a, const Fp2 *b);
void fp2_add(Fp2 *out, const Fp2 *a, const Fp2 *b);
void fp2_sub(Fp2 *out, const Fp2 *a, const Fp2 *b);
void fp2_neg(Fp2 *out, const Fp2 *a);
void fp2_conj(Fp2 *out, const Fp2 *a);
void fp2_mul(Fp2 *out, const Fp2 *a, const Fp2 *b);
void fp2_sqr(Fp2 *out, const Fp2 *a);
/* A times an element of Fp. */
void fp2_mul_fe(Fp2 *out, const Fp2 *a, const Fe *k);
/* A times xi. */
void fp2_mul_xi(Fp2 *out, const Fp2 *a);
/* The inverse of A, thus 0 at 0. */
void fp2_inv(Fp2 *out, const Fp2 *a);
/* A^E for any 256-bit E. */
void fp2_pow(Fp2 *out, const Fp2 *a, const U256 *e);
/* Returns 1 and a square root of A, or 0 when A is not a square. */
int fp2_sqrt(Fp2 *out, const Fp2 *a);

/* Fp6. OUT can be A or B. */
void fp6_zero(Fp6 *out);
void fp6_one(Fp6 *out);
void fp6_add(Fp6 *out, const Fp6 *a, const Fp6 *b);
void fp6_sub(Fp6 *out, const Fp6 *a, const Fp6 *b);
void fp6_neg(Fp6 *out, const Fp6 *a);
void fp6_mul(Fp6 *out, const Fp6 *a, const Fp6 *b);
/* A times v. */
void fp6_mul_v(Fp6 *out, const Fp6 *a);
void fp6_inv(Fp6 *out, const Fp6 *a);

/* Fp12. OUT can be A or B. */
void fp12_one(Fp12 *out);
int fp12_is_one(const Fp12 *a);
int fp12_eq(const Fp12 *a, const Fp12 *b);
void fp12_mul(Fp12 *out, const Fp12 *a, const Fp12 *b);
void fp12_sqr(Fp12 *out, const Fp12 *a);
/* a - b w, which is A^(p^6). */
void fp12_conj(Fp12 *out, const Fp12 *a);
void fp12_inv(Fp12 *out, const Fp12 *a);
/* A^p. */
void fp12_frob(Fp12 *out, const Fp12 *a);
/* A^X for the 64-bit X. */
void fp12_pow_u64(Fp12 *out, const Fp12 *a, uint64_t x);
/* A times the line value L. */
void fp12_mul_line(Fp12 *out, const Fp12 *a, const Line *l);

/* G2. The generator of EIP-197. */
void g2_generator(G2 *out);
int g2_is_infinity(const G2 *q);
int g2_eq(const G2 *a, const G2 *b);
/* 1 when Q is on the twist; (0, 0) is. */
int g2_on_curve(const G2 *q);
void g2_neg(G2 *out, const G2 *q);
void g2_to_jac(G2Jac *out, const G2 *q);
void g2_jac_affine(G2 *out, const G2Jac *t);
void g2_jac_double(G2Jac *out, const G2Jac *t);
/* T plus the affine point Q. Each case is correct: T or Q at infinity,
   T = Q and T = -Q. */
void g2_jac_add_affine(G2Jac *out, const G2Jac *t, const G2 *q);
void g2_add(G2 *out, const G2 *a, const G2 *b);
/* K times Q, for any 256-bit K. */
void g2_mul(G2 *out, const G2 *q, const U256 *k);
/* The twist form of the p power map: (conj(x) g^2, conj(y) g^3), with
   g = xi^((p - 1) / 6). */
void g2_psi(G2 *out, const G2 *q);
/* 1 when Q is on the twist and in the subgroup of order r, by the
   endomorphism test (Q-S8c-6 (b)); (0, 0) is. */
int g2_in_subgroup(const G2 *q);
/* 1, or 0 when a coordinate is p or more, or the point is not on the twist
   or not in the subgroup. BE is 128 bytes in the EIP-197 order: the u part
   of x, the other part of x, the u part of y, the other part of y. */
int g2_from_be(G2 *out, const unsigned char *be);

/* The steps of the Miller loop (public for the fixture rows of S8c G2).
   line_double: L is the line value of the tangent at T at the point P,
   then T becomes 2 T. line_add: L is the line value of the chord of T and
   Q at P, then T becomes T + Q. T and Q are not at infinity, and T is
   not Q or -Q. */
void pairing_line_double(Line *l, G2Jac *t, const G1 *p);
void pairing_line_add(Line *l, G2Jac *t, const G2 *q, const G1 *p);
/* The Miller loop value of (P, Q). 1 when P or Q is at infinity. */
void pairing_miller(Fp12 *out, const G1 *p, const G2 *q);
/* F^((p^12 - 1) / r): the easy part, then the hard part by the x chain
   (Q-S8c-7 (a)). */
void pairing_final_easy(Fp12 *out, const Fp12 *f);
void pairing_final_hard(Fp12 *out, const Fp12 *m);
void pairing_final(Fp12 *out, const Fp12 *f);
/* The pairing e(P, Q). */
void pairing(Fp12 *out, const G1 *p, const G2 *q);
/* 1 when the product of e(P, Q) for the N pairs is 1; 1 for N = 0. One
   Miller loop for each pair, one final power (Q-S8c-8 (a): no limit on
   N). Each P is on G1 and each Q is in G2: the caller does the input
   checks. */
int pairing_check(const Pair *pairs, size_t n);
/* The EIP-197 check of the N bytes at IN. Returns 1 or 0 for the result,
   or -1 when the input is refused: N is not a multiple of 192, a
   coordinate is p or more, or a point is not in its group. */
int pairing_check_be(const unsigned char *in, size_t n);

#endif
