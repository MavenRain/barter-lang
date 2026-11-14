/* The C oracle of the BN254 pairing check (S8c G1, Q-S8c-2 (a)). New in
   barter-lang; no source file from another repo. See crypto/pairing.h.
   Each formula is plain: 4 products for Fp2, 9 for Fp6, 4 for Fp12, no
   table of constants (the 6 Frobenius factors and the twist constant are
   computed at the first use). The evidence is the self checks of
   test/cryptotool.c. */
#include <string.h>

#include "crypto/pairing.h"

#define FP (&BN254_FP)

/* The constants: g[i] = g^i with g = xi^((p - 1) / 6), and b2 = 3 / xi. */
typedef struct {
  int ready;
  Fp2 g[6];
  Fp2 b2;
} Consts;

static Consts CONSTS;

static int fe_is_zero(const Fe *a) {
  unsigned i;
  for (i = 0u; i < BN_LIMBS; i++) {
    if (a->w[i] != 0u)
      return 0;
  }
  return 1;
}

/* OUT from 64 hex digits (0 to 9 and a to f), the most significant first. */
static void fe_hex64(Fe *out, const char *hex) {
  unsigned i;
  memset(out, 0, sizeof *out);
  for (i = 0u; i < 64u; i++) {
    char c = hex[i];
    uint32_t d = (uint32_t)(c <= '9' ? c - '0' : c - 'a' + 10);
    unsigned k = 7u - i / 8u;
    out->w[k] = (out->w[k] << 4) | d;
  }
}

/* p - 1 as a word. */
static void p_less_one(U256 *out) {
  Fe one;
  Fe m;
  fe_from_u64(&one, 1u);
  fe_neg(&m, &one, FP);
  fe_to_u256(out, &m);
}

static int fe_from_be32(Fe *out, const unsigned char *be) {
  U256 w;
  u256_from_be(&w, be);
  return fe_from_u256(out, &w, FP);
}

static int g1_is_inf(const G1 *p) {
  return fe_is_zero(&p->x) && fe_is_zero(&p->y);
}

/* Fp2. */

void fp2_zero(Fp2 *out) {
  memset(out, 0, sizeof *out);
}

void fp2_one(Fp2 *out) {
  memset(out, 0, sizeof *out);
  out->c0.w[0] = 1u;
}

int fp2_is_zero(const Fp2 *a) {
  return fe_is_zero(&a->c0) && fe_is_zero(&a->c1);
}

int fp2_eq(const Fp2 *a, const Fp2 *b) {
  return fe_eq(&a->c0, &b->c0) && fe_eq(&a->c1, &b->c1);
}

void fp2_add(Fp2 *out, const Fp2 *a, const Fp2 *b) {
  Fe r0;
  Fe r1;
  fe_add(&r0, &a->c0, &b->c0, FP);
  fe_add(&r1, &a->c1, &b->c1, FP);
  out->c0 = r0;
  out->c1 = r1;
}

void fp2_sub(Fp2 *out, const Fp2 *a, const Fp2 *b) {
  Fe r0;
  Fe r1;
  fe_sub(&r0, &a->c0, &b->c0, FP);
  fe_sub(&r1, &a->c1, &b->c1, FP);
  out->c0 = r0;
  out->c1 = r1;
}

void fp2_neg(Fp2 *out, const Fp2 *a) {
  Fe r0;
  Fe r1;
  fe_neg(&r0, &a->c0, FP);
  fe_neg(&r1, &a->c1, FP);
  out->c0 = r0;
  out->c1 = r1;
}

void fp2_conj(Fp2 *out, const Fp2 *a) {
  Fe r0 = a->c0;
  Fe r1;
  fe_neg(&r1, &a->c1, FP);
  out->c0 = r0;
  out->c1 = r1;
}

/* (a0 + a1 u)(b0 + b1 u) = (a0 b0 - a1 b1) + (a0 b1 + a1 b0) u. */
void fp2_mul(Fp2 *out, const Fp2 *a, const Fp2 *b) {
  Fe t0;
  Fe t1;
  Fe t2;
  Fe t3;
  Fe r0;
  Fe r1;
  fe_mul(&t0, &a->c0, &b->c0, FP);
  fe_mul(&t1, &a->c1, &b->c1, FP);
  fe_mul(&t2, &a->c0, &b->c1, FP);
  fe_mul(&t3, &a->c1, &b->c0, FP);
  fe_sub(&r0, &t0, &t1, FP);
  fe_add(&r1, &t2, &t3, FP);
  out->c0 = r0;
  out->c1 = r1;
}

void fp2_sqr(Fp2 *out, const Fp2 *a) {
  fp2_mul(out, a, a);
}

void fp2_mul_fe(Fp2 *out, const Fp2 *a, const Fe *k) {
  Fe r0;
  Fe r1;
  fe_mul(&r0, &a->c0, k, FP);
  fe_mul(&r1, &a->c1, k, FP);
  out->c0 = r0;
  out->c1 = r1;
}

/* (9 + u)(a0 + a1 u) = (9 a0 - a1) + (a0 + 9 a1) u. */
void fp2_mul_xi(Fp2 *out, const Fp2 *a) {
  Fe nine;
  Fe t0;
  Fe t1;
  Fe r0;
  Fe r1;
  fe_from_u64(&nine, 9u);
  fe_mul(&t0, &a->c0, &nine, FP);
  fe_mul(&t1, &a->c1, &nine, FP);
  fe_sub(&r0, &t0, &a->c1, FP);
  fe_add(&r1, &a->c0, &t1, FP);
  out->c0 = r0;
  out->c1 = r1;
}

/* The conjugate of A divided by a0^2 + a1^2. */
void fp2_inv(Fp2 *out, const Fp2 *a) {
  Fe t0;
  Fe t1;
  Fe n;
  Fe ni;
  Fe r0;
  Fe t2;
  Fe r1;
  fe_mul(&t0, &a->c0, &a->c0, FP);
  fe_mul(&t1, &a->c1, &a->c1, FP);
  fe_add(&n, &t0, &t1, FP);
  fe_inv(&ni, &n, FP);
  fe_mul(&r0, &a->c0, &ni, FP);
  fe_mul(&t2, &a->c1, &ni, FP);
  fe_neg(&r1, &t2, FP);
  out->c0 = r0;
  out->c1 = r1;
}

void fp2_pow(Fp2 *out, const Fp2 *a, const U256 *e) {
  Fp2 r;
  unsigned i;
  fp2_one(&r);
  for (i = 256u; i > 0u; i--) {
    unsigned k = i - 1u;
    fp2_sqr(&r, &r);
    if ((e->limb[k / 64u] >> (k % 64u)) & 1u)
      fp2_mul(&r, &r, a);
  }
  *out = r;
}

/* Returns 1 and a root of A in Fp, or 0. p = 3 mod 4: when A is a square,
   A^((p + 1) / 4) is a root. */
static int fe_sqrt(Fe *out, const Fe *a) {
  U256 e;
  U256 k;
  Fe r;
  Fe s;
  p_less_one(&e);
  u256_from_u64(&k, 2u);
  u256_add(&e, &e, &k);
  u256_from_u64(&k, 4u);
  u256_div(&e, &e, &k);
  fe_pow(&r, a, &e, FP);
  fe_mul(&s, &r, &r, FP);
  if (!fe_eq(&s, a))
    return 0;
  *out = r;
  return 1;
}

/* A root of the Fp value A0 in Fp2: a root in Fp, or u times a root of
   -A0. One of A0 and -A0 is a square. */
static int fp2_sqrt_base(Fp2 *out, const Fe *a0) {
  Fe a = *a0;
  Fe na;
  Fe s;
  fp2_zero(out);
  if (fe_sqrt(&s, &a)) {
    out->c0 = s;
    return 1;
  }
  fe_neg(&na, &a, FP);
  if (!fe_sqrt(&s, &na))
    return 0;
  out->c1 = s;
  return 1;
}

/* A root of (A0 + S) / 2 or of (A0 - S) / 2: the one that is a square. */
static int half_root(Fe *out, const Fe *a0, const Fe *s) {
  Fe two;
  Fe half;
  Fe t0;
  Fe t1;
  fe_from_u64(&two, 2u);
  fe_inv(&half, &two, FP);
  fe_add(&t0, a0, s, FP);
  fe_mul(&t1, &t0, &half, FP);
  if (fe_sqrt(out, &t1))
    return 1;
  fe_sub(&t0, a0, s, FP);
  fe_mul(&t1, &t0, &half, FP);
  return fe_sqrt(out, &t1);
}

/* With S a root of a0^2 + a1^2: y0 is a root of (a0 + S) / 2 or of
   (a0 - S) / 2, and y1 = a1 / (2 y0). The last step does the check
   y^2 = A. */
int fp2_sqrt(Fp2 *out, const Fp2 *a) {
  Fe t0;
  Fe t1;
  Fe n;
  Fe s;
  Fe d;
  Fe di;
  Fp2 r;
  Fp2 chk;
  if (fe_is_zero(&a->c1))
    return fp2_sqrt_base(out, &a->c0);
  fe_mul(&t0, &a->c0, &a->c0, FP);
  fe_mul(&t1, &a->c1, &a->c1, FP);
  fe_add(&n, &t0, &t1, FP);
  if (!fe_sqrt(&s, &n))
    return 0;
  if (!half_root(&r.c0, &a->c0, &s))
    return 0;
  fe_add(&d, &r.c0, &r.c0, FP);
  fe_inv(&di, &d, FP);
  fe_mul(&r.c1, &a->c1, &di, FP);
  fp2_sqr(&chk, &r);
  if (!fp2_eq(&chk, a))
    return 0;
  *out = r;
  return 1;
}

static const Consts *consts(void) {
  if (!CONSTS.ready) {
    U256 e;
    U256 six;
    Fp2 xi;
    Fp2 g;
    Fp2 xi_inv;
    unsigned i;
    p_less_one(&e);
    u256_from_u64(&six, 6u);
    u256_div(&e, &e, &six);
    fp2_zero(&xi);
    fe_from_u64(&xi.c0, 9u);
    fe_from_u64(&xi.c1, 1u);
    fp2_pow(&g, &xi, &e);
    fp2_one(&CONSTS.g[0]);
    for (i = 1u; i < 6u; i++)
      fp2_mul(&CONSTS.g[i], &CONSTS.g[i - 1u], &g);
    fp2_inv(&xi_inv, &xi);
    fp2_add(&CONSTS.b2, &xi_inv, &xi_inv);
    fp2_add(&CONSTS.b2, &CONSTS.b2, &xi_inv);
    CONSTS.ready = 1;
  }
  return &CONSTS;
}

/* Fp6. */

void fp6_zero(Fp6 *out) {
  memset(out, 0, sizeof *out);
}

void fp6_one(Fp6 *out) {
  memset(out, 0, sizeof *out);
  out->c0.c0.w[0] = 1u;
}

void fp6_add(Fp6 *out, const Fp6 *a, const Fp6 *b) {
  fp2_add(&out->c0, &a->c0, &b->c0);
  fp2_add(&out->c1, &a->c1, &b->c1);
  fp2_add(&out->c2, &a->c2, &b->c2);
}

void fp6_sub(Fp6 *out, const Fp6 *a, const Fp6 *b) {
  fp2_sub(&out->c0, &a->c0, &b->c0);
  fp2_sub(&out->c1, &a->c1, &b->c1);
  fp2_sub(&out->c2, &a->c2, &b->c2);
}

void fp6_neg(Fp6 *out, const Fp6 *a) {
  fp2_neg(&out->c0, &a->c0);
  fp2_neg(&out->c1, &a->c1);
  fp2_neg(&out->c2, &a->c2);
}

/* c0 = a0 b0 + xi (a1 b2 + a2 b1); c1 = a0 b1 + a1 b0 + xi a2 b2;
   c2 = a0 b2 + a1 b1 + a2 b0. */
void fp6_mul(Fp6 *out, const Fp6 *a, const Fp6 *b) {
  Fp2 t;
  Fp2 s;
  Fp6 r;
  fp2_mul(&t, &a->c1, &b->c2);
  fp2_mul(&s, &a->c2, &b->c1);
  fp2_add(&t, &t, &s);
  fp2_mul_xi(&t, &t);
  fp2_mul(&s, &a->c0, &b->c0);
  fp2_add(&r.c0, &s, &t);
  fp2_mul(&t, &a->c2, &b->c2);
  fp2_mul_xi(&t, &t);
  fp2_mul(&s, &a->c0, &b->c1);
  fp2_add(&t, &t, &s);
  fp2_mul(&s, &a->c1, &b->c0);
  fp2_add(&r.c1, &t, &s);
  fp2_mul(&t, &a->c0, &b->c2);
  fp2_mul(&s, &a->c1, &b->c1);
  fp2_add(&t, &t, &s);
  fp2_mul(&s, &a->c2, &b->c0);
  fp2_add(&r.c2, &t, &s);
  *out = r;
}

/* v (a0 + a1 v + a2 v^2) = xi a2 + a0 v + a1 v^2. */
void fp6_mul_v(Fp6 *out, const Fp6 *a) {
  Fp6 r;
  fp2_mul_xi(&r.c0, &a->c2);
  r.c1 = a->c0;
  r.c2 = a->c1;
  *out = r;
}

/* t0 = a0^2 - xi a1 a2; t1 = xi a2^2 - a0 a1; t2 = a1^2 - a0 a2;
   d = a0 t0 + xi (a2 t1 + a1 t2); the inverse is (t0, t1, t2) / d. */
void fp6_inv(Fp6 *out, const Fp6 *a) {
  Fp2 t0;
  Fp2 t1;
  Fp2 t2;
  Fp2 s;
  Fp2 d;
  Fp2 di;
  Fp6 r;
  fp2_sqr(&t0, &a->c0);
  fp2_mul(&s, &a->c1, &a->c2);
  fp2_mul_xi(&s, &s);
  fp2_sub(&t0, &t0, &s);
  fp2_sqr(&t1, &a->c2);
  fp2_mul_xi(&t1, &t1);
  fp2_mul(&s, &a->c0, &a->c1);
  fp2_sub(&t1, &t1, &s);
  fp2_sqr(&t2, &a->c1);
  fp2_mul(&s, &a->c0, &a->c2);
  fp2_sub(&t2, &t2, &s);
  fp2_mul(&d, &a->c2, &t1);
  fp2_mul(&s, &a->c1, &t2);
  fp2_add(&d, &d, &s);
  fp2_mul_xi(&d, &d);
  fp2_mul(&s, &a->c0, &t0);
  fp2_add(&d, &d, &s);
  fp2_inv(&di, &d);
  fp2_mul(&r.c0, &t0, &di);
  fp2_mul(&r.c1, &t1, &di);
  fp2_mul(&r.c2, &t2, &di);
  *out = r;
}

/* Fp12. */

void fp12_one(Fp12 *out) {
  fp6_one(&out->a);
  fp6_zero(&out->b);
}

static int fp6_eq(const Fp6 *a, const Fp6 *b) {
  return fp2_eq(&a->c0, &b->c0) && fp2_eq(&a->c1, &b->c1) && fp2_eq(&a->c2, &b->c2);
}

int fp12_eq(const Fp12 *a, const Fp12 *b) {
  return fp6_eq(&a->a, &b->a) && fp6_eq(&a->b, &b->b);
}

int fp12_is_one(const Fp12 *a) {
  Fp12 one;
  fp12_one(&one);
  return fp12_eq(a, &one);
}

/* (a + b w)(c + d w) = (a c + v b d) + (a d + b c) w. */
void fp12_mul(Fp12 *out, const Fp12 *a, const Fp12 *b) {
  Fp6 ac;
  Fp6 bd;
  Fp6 ad;
  Fp6 bc;
  Fp12 r;
  fp6_mul(&ac, &a->a, &b->a);
  fp6_mul(&bd, &a->b, &b->b);
  fp6_mul(&ad, &a->a, &b->b);
  fp6_mul(&bc, &a->b, &b->a);
  fp6_mul_v(&bd, &bd);
  fp6_add(&r.a, &ac, &bd);
  fp6_add(&r.b, &ad, &bc);
  *out = r;
}

void fp12_sqr(Fp12 *out, const Fp12 *a) {
  fp12_mul(out, a, a);
}

void fp12_conj(Fp12 *out, const Fp12 *a) {
  Fp12 r;
  r.a = a->a;
  fp6_neg(&r.b, &a->b);
  *out = r;
}

/* (a - b w) / (a^2 - v b^2). */
void fp12_inv(Fp12 *out, const Fp12 *a) {
  Fp6 t0;
  Fp6 t1;
  Fp6 d;
  Fp6 di;
  Fp6 nb;
  Fp12 r;
  fp6_mul(&t0, &a->a, &a->a);
  fp6_mul(&t1, &a->b, &a->b);
  fp6_mul_v(&t1, &t1);
  fp6_sub(&d, &t0, &t1);
  fp6_inv(&di, &d);
  fp6_mul(&r.a, &a->a, &di);
  fp6_mul(&nb, &a->b, &di);
  fp6_neg(&r.b, &nb);
  *out = r;
}

/* The part at w^i becomes its conjugate times g^i. The parts at w^0 to
   w^5 are a.c0, b.c0, a.c1, b.c1, a.c2 and b.c2. */
void fp12_frob(Fp12 *out, const Fp12 *a) {
  const Consts *c = consts();
  Fp2 t;
  Fp12 r;
  fp2_conj(&r.a.c0, &a->a.c0);
  fp2_conj(&t, &a->b.c0);
  fp2_mul(&r.b.c0, &t, &c->g[1]);
  fp2_conj(&t, &a->a.c1);
  fp2_mul(&r.a.c1, &t, &c->g[2]);
  fp2_conj(&t, &a->b.c1);
  fp2_mul(&r.b.c1, &t, &c->g[3]);
  fp2_conj(&t, &a->a.c2);
  fp2_mul(&r.a.c2, &t, &c->g[4]);
  fp2_conj(&t, &a->b.c2);
  fp2_mul(&r.b.c2, &t, &c->g[5]);
  *out = r;
}

void fp12_pow_u64(Fp12 *out, const Fp12 *a, uint64_t x) {
  Fp12 base = *a;
  Fp12 r;
  unsigned i;
  fp12_one(&r);
  for (i = 64u; i > 0u; i--) {
    fp12_sqr(&r, &r);
    if ((x >> (i - 1u)) & 1u)
      fp12_mul(&r, &r, &base);
  }
  *out = r;
}

/* The line value as an Fp12 value: a = (c0, 0, 0), b = (c1, c3, 0). */
void fp12_mul_line(Fp12 *out, const Fp12 *a, const Line *l) {
  Fp12 v;
  fp6_zero(&v.a);
  fp6_zero(&v.b);
  v.a.c0 = l->c0;
  v.b.c0 = l->c1;
  v.b.c1 = l->c3;
  fp12_mul(out, a, &v);
}

/* G2. */

void g2_generator(G2 *out) {
  fe_hex64(&out->x.c1, "198e9393920d483a7260bfb731fb5d25f1aa493335a9e71297e485b7aef312c2");
  fe_hex64(&out->x.c0, "1800deef121f1e76426a00665e5c4479674322d4f75edadd46debd5cd992f6ed");
  fe_hex64(&out->y.c1, "090689d0585ff075ec9e99ad690c3395bc4b313370b38ef355acdadcd122975b");
  fe_hex64(&out->y.c0, "12c85ea5db8c6deb4aab71808dcb408fe3d1e7690c43d37b4ce6cc0166fa7daa");
}

int g2_is_infinity(const G2 *q) {
  return fp2_is_zero(&q->x) && fp2_is_zero(&q->y);
}

int g2_eq(const G2 *a, const G2 *b) {
  return fp2_eq(&a->x, &b->x) && fp2_eq(&a->y, &b->y);
}

int g2_on_curve(const G2 *q) {
  Fp2 l;
  Fp2 r;
  if (g2_is_infinity(q))
    return 1;
  fp2_sqr(&l, &q->y);
  fp2_sqr(&r, &q->x);
  fp2_mul(&r, &r, &q->x);
  fp2_add(&r, &r, &consts()->b2);
  return fp2_eq(&l, &r);
}

void g2_neg(G2 *out, const G2 *q) {
  Fp2 y;
  fp2_neg(&y, &q->y);
  out->x = q->x;
  out->y = y;
}

static void jac_infinity(G2Jac *out) {
  fp2_one(&out->x);
  fp2_one(&out->y);
  fp2_zero(&out->z);
}

void g2_to_jac(G2Jac *out, const G2 *q) {
  if (g2_is_infinity(q)) {
    jac_infinity(out);
    return;
  }
  out->x = q->x;
  out->y = q->y;
  fp2_one(&out->z);
}

void g2_jac_affine(G2 *out, const G2Jac *t) {
  Fp2 zi;
  Fp2 zi2;
  Fp2 zi3;
  if (fp2_is_zero(&t->z)) {
    fp2_zero(&out->x);
    fp2_zero(&out->y);
    return;
  }
  fp2_inv(&zi, &t->z);
  fp2_sqr(&zi2, &zi);
  fp2_mul(&zi3, &zi2, &zi);
  fp2_mul(&out->x, &t->x, &zi2);
  fp2_mul(&out->y, &t->y, &zi3);
}

/* M = 3 X^2; S = 4 X Y^2; X3 = M^2 - 2 S; Y3 = M (S - X3) - 8 Y^4;
   Z3 = 2 Y Z. */
void g2_jac_double(G2Jac *out, const G2Jac *t) {
  Fp2 x2;
  Fp2 y2;
  Fp2 m;
  Fp2 s;
  Fp2 x3;
  Fp2 y3;
  Fp2 z3;
  Fp2 t0;
  Fp2 t1;
  fp2_sqr(&x2, &t->x);
  fp2_add(&m, &x2, &x2);
  fp2_add(&m, &m, &x2);
  fp2_sqr(&y2, &t->y);
  fp2_mul(&s, &t->x, &y2);
  fp2_add(&s, &s, &s);
  fp2_add(&s, &s, &s);
  fp2_sqr(&x3, &m);
  fp2_sub(&x3, &x3, &s);
  fp2_sub(&x3, &x3, &s);
  fp2_sqr(&t0, &y2);
  fp2_add(&t0, &t0, &t0);
  fp2_add(&t0, &t0, &t0);
  fp2_add(&t0, &t0, &t0);
  fp2_sub(&t1, &s, &x3);
  fp2_mul(&y3, &m, &t1);
  fp2_sub(&y3, &y3, &t0);
  fp2_mul(&z3, &t->y, &t->z);
  fp2_add(&z3, &z3, &z3);
  out->x = x3;
  out->y = y3;
  out->z = z3;
}

/* T and Q have the same x. R = 0: T = Q, the result is 2 T. If not:
   T = -Q, the result is the point at infinity. */
static void add_same_x(G2Jac *out, const G2Jac *t, const Fp2 *r) {
  if (fp2_is_zero(r)) {
    g2_jac_double(out, t);
    return;
  }
  jac_infinity(out);
}

/* U = xQ Z^2; S2 = yQ Z^3; H = U - X; R = S2 - Y;
   X3 = R^2 - H^3 - 2 X H^2; Y3 = R (X H^2 - X3) - Y H^3; Z3 = Z H. */
void g2_jac_add_affine(G2Jac *out, const G2Jac *t, const G2 *q) {
  Fp2 z2;
  Fp2 z3;
  Fp2 u;
  Fp2 s2;
  Fp2 h;
  Fp2 r;
  Fp2 h2;
  Fp2 h3;
  Fp2 xh2;
  Fp2 t0;
  G2Jac res;
  if (g2_is_infinity(q)) {
    *out = *t;
    return;
  }
  if (fp2_is_zero(&t->z)) {
    g2_to_jac(out, q);
    return;
  }
  fp2_sqr(&z2, &t->z);
  fp2_mul(&z3, &z2, &t->z);
  fp2_mul(&u, &q->x, &z2);
  fp2_mul(&s2, &q->y, &z3);
  fp2_sub(&h, &u, &t->x);
  fp2_sub(&r, &s2, &t->y);
  if (fp2_is_zero(&h)) {
    add_same_x(out, t, &r);
    return;
  }
  fp2_sqr(&h2, &h);
  fp2_mul(&h3, &h2, &h);
  fp2_mul(&xh2, &t->x, &h2);
  fp2_sqr(&res.x, &r);
  fp2_sub(&res.x, &res.x, &h3);
  fp2_sub(&res.x, &res.x, &xh2);
  fp2_sub(&res.x, &res.x, &xh2);
  fp2_sub(&t0, &xh2, &res.x);
  fp2_mul(&res.y, &r, &t0);
  fp2_mul(&t0, &t->y, &h3);
  fp2_sub(&res.y, &res.y, &t0);
  fp2_mul(&res.z, &t->z, &h);
  *out = res;
}

void g2_add(G2 *out, const G2 *a, const G2 *b) {
  G2Jac t;
  g2_to_jac(&t, a);
  g2_jac_add_affine(&t, &t, b);
  g2_jac_affine(out, &t);
}

void g2_mul(G2 *out, const G2 *q, const U256 *k) {
  G2 base = *q;
  G2Jac acc;
  unsigned i;
  jac_infinity(&acc);
  for (i = 256u; i > 0u; i--) {
    unsigned b = i - 1u;
    g2_jac_double(&acc, &acc);
    if ((k->limb[b / 64u] >> (b % 64u)) & 1u)
      g2_jac_add_affine(&acc, &acc, &base);
  }
  g2_jac_affine(out, &acc);
}

void g2_psi(G2 *out, const G2 *q) {
  const Consts *c = consts();
  Fp2 t;
  fp2_conj(&t, &q->x);
  fp2_mul(&out->x, &t, &c->g[2]);
  fp2_conj(&t, &q->y);
  fp2_mul(&out->y, &t, &c->g[3]);
}

/* The test [x + 1] Q + psi([x] Q) + psi^2([x] Q) = psi^3([2 x] Q), from
   "Co-factor clearing and subgroup membership testing on
   pairing-friendly curves" (El Housni, Guillevic, Piellard). The self
   check of test/cryptotool.c compares it with r Q = infinity. */
int g2_in_subgroup(const G2 *q) {
  U256 x;
  G2 a;
  G2 b;
  G2 c;
  G2 d;
  G2 e;
  if (!g2_on_curve(q))
    return 0;
  u256_from_u64(&x, PAIRING_X);
  g2_mul(&a, q, &x);
  g2_psi(&b, &a);
  g2_add(&a, &a, q);
  g2_psi(&c, &b);
  g2_add(&d, &c, &b);
  g2_add(&d, &d, &a);
  g2_psi(&e, &c);
  g2_add(&e, &e, &e);
  return g2_eq(&e, &d);
}

int g2_from_be(G2 *out, const unsigned char *be) {
  G2 q;
  if (!fe_from_be32(&q.x.c1, be) || !fe_from_be32(&q.x.c0, be + 32) || !fe_from_be32(&q.y.c1, be + 64) ||
      !fe_from_be32(&q.y.c0, be + 96))
    return 0;
  if (!g2_in_subgroup(&q))
    return 0;
  *out = q;
  return 1;
}

/* The Miller loop. With the twist slope L, the line at the point P is
   yP - L xP w + (L xT - yT) w^3. A line step multiplies it by a value of
   Fp2 (the final power removes this factor). */

/* The tangent at T, times Z3 Z^2: c0 = Z3 Z^2 yP; c1 = -(M Z^2) xP;
   c3 = M X - 2 Y^2, with M = 3 X^2 and Z3 = 2 Y Z. */
void pairing_line_double(Line *l, G2Jac *t, const G1 *p) {
  Fp2 x2;
  Fp2 m;
  Fp2 y2;
  Fp2 z2;
  Fp2 t0;
  Fp2 t1;
  fp2_sqr(&x2, &t->x);
  fp2_add(&m, &x2, &x2);
  fp2_add(&m, &m, &x2);
  fp2_sqr(&y2, &t->y);
  fp2_sqr(&z2, &t->z);
  fp2_mul(&t0, &m, &t->x);
  fp2_add(&t1, &y2, &y2);
  fp2_sub(&l->c3, &t0, &t1);
  fp2_mul(&t0, &m, &z2);
  fp2_mul_fe(&t1, &t0, &p->x);
  fp2_neg(&l->c1, &t1);
  g2_jac_double(t, t);
  fp2_mul(&t0, &t->z, &z2);
  fp2_mul_fe(&l->c0, &t0, &p->y);
}

/* The chord of T and Q, times Z3: c0 = Z3 yP; c1 = -R xP;
   c3 = R xQ - yQ Z3, with R = yQ Z^3 - Y and Z3 = Z (xQ Z^2 - X). */
void pairing_line_add(Line *l, G2Jac *t, const G2 *q, const G1 *p) {
  Fp2 z2;
  Fp2 z3;
  Fp2 s2;
  Fp2 r;
  Fp2 t0;
  Fp2 t1;
  fp2_sqr(&z2, &t->z);
  fp2_mul(&z3, &z2, &t->z);
  fp2_mul(&s2, &q->y, &z3);
  fp2_sub(&r, &s2, &t->y);
  g2_jac_add_affine(t, t, q);
  fp2_mul_fe(&l->c0, &t->z, &p->y);
  fp2_mul_fe(&t0, &r, &p->x);
  fp2_neg(&l->c1, &t0);
  fp2_mul(&t0, &r, &q->x);
  fp2_mul(&t1, &q->y, &t->z);
  fp2_sub(&l->c3, &t0, &t1);
}

/* f = 1, T = Q. For each bit of 6 x + 2 below the top bit, from the
   high bit: f = f^2 times the tangent, T = 2 T; on a 1 bit f = f times
   the chord, T = T + Q. Then the 2 chords of the tail with psi(Q) and
   -psi^2(Q). */
void pairing_miller(Fp12 *out, const G1 *p, const G2 *q) {
  const uint64_t lo = PAIRING_LOOP_LOW;
  Fp12 f;
  G2Jac t;
  G2 q1;
  G2 q2;
  Line l;
  unsigned i;
  fp12_one(&f);
  if (g1_is_inf(p) || g2_is_infinity(q)) {
    *out = f;
    return;
  }
  g2_to_jac(&t, q);
  for (i = 64u; i > 0u; i--) {
    fp12_sqr(&f, &f);
    pairing_line_double(&l, &t, p);
    fp12_mul_line(&f, &f, &l);
    if ((lo >> (i - 1u)) & 1u) {
      pairing_line_add(&l, &t, q, p);
      fp12_mul_line(&f, &f, &l);
    }
  }
  g2_psi(&q1, q);
  g2_psi(&q2, &q1);
  g2_neg(&q2, &q2);
  pairing_line_add(&l, &t, &q1, p);
  fp12_mul_line(&f, &f, &l);
  pairing_line_add(&l, &t, &q2, p);
  fp12_mul_line(&f, &f, &l);
  *out = f;
}

/* F^((p^6 - 1)(p^2 + 1)). */
void pairing_final_easy(Fp12 *out, const Fp12 *f) {
  Fp12 c;
  Fp12 i;
  Fp12 f1;
  Fp12 f2;
  fp12_conj(&c, f);
  fp12_inv(&i, f);
  fp12_mul(&f1, &c, &i);
  fp12_frob(&f2, &f1);
  fp12_frob(&f2, &f2);
  fp12_mul(out, &f2, &f1);
}

/* M^e with e = (p^4 - p^2 + 1) / r
     = p^3 + (6 x^2 + 1) p^2 + (1 - 12 x - 18 x^2 - 36 x^3) p
       + (-2 - 18 x - 30 x^2 - 36 x^3).
   M is a result of the easy part: its inverse is its conjugate. The
   chain makes y0 y1^2 y2^6 y3^12 y4^18 y5^30 y6^36. */
void pairing_final_hard(Fp12 *out, const Fp12 *m) {
  Fp12 fx;
  Fp12 fx2;
  Fp12 fx3;
  Fp12 mp;
  Fp12 mp2;
  Fp12 mp3;
  Fp12 y0;
  Fp12 y1;
  Fp12 y2;
  Fp12 y3;
  Fp12 y4;
  Fp12 y5;
  Fp12 y6;
  Fp12 t0;
  Fp12 t1;
  fp12_pow_u64(&fx, m, PAIRING_X);
  fp12_pow_u64(&fx2, &fx, PAIRING_X);
  fp12_pow_u64(&fx3, &fx2, PAIRING_X);
  fp12_frob(&mp, m);
  fp12_frob(&mp2, &mp);
  fp12_frob(&mp3, &mp2);
  fp12_mul(&y0, &mp, &mp2);
  fp12_mul(&y0, &y0, &mp3);
  fp12_conj(&y1, m);
  fp12_frob(&y2, &fx2);
  fp12_frob(&y2, &y2);
  fp12_frob(&y3, &fx);
  fp12_conj(&y3, &y3);
  fp12_frob(&y4, &fx2);
  fp12_mul(&y4, &fx, &y4);
  fp12_conj(&y4, &y4);
  fp12_conj(&y5, &fx2);
  fp12_frob(&y6, &fx3);
  fp12_mul(&y6, &fx3, &y6);
  fp12_conj(&y6, &y6);
  fp12_sqr(&t0, &y6);
  fp12_mul(&t0, &t0, &y4);
  fp12_mul(&t0, &t0, &y5);
  fp12_mul(&t1, &y3, &y5);
  fp12_mul(&t1, &t1, &t0);
  fp12_mul(&t0, &t0, &y2);
  fp12_sqr(&t1, &t1);
  fp12_mul(&t1, &t1, &t0);
  fp12_sqr(&t1, &t1);
  fp12_mul(&t0, &t1, &y1);
  fp12_mul(&t1, &t1, &y0);
  fp12_sqr(&t0, &t0);
  fp12_mul(out, &t0, &t1);
}

void pairing_final(Fp12 *out, const Fp12 *f) {
  Fp12 m;
  pairing_final_easy(&m, f);
  pairing_final_hard(out, &m);
}

void pairing(Fp12 *out, const G1 *p, const G2 *q) {
  Fp12 f;
  pairing_miller(&f, p, q);
  pairing_final(out, &f);
}

int pairing_check(const Pair *pairs, size_t n) {
  Fp12 f;
  Fp12 m;
  size_t i;
  fp12_one(&f);
  for (i = 0u; i < n; i++) {
    pairing_miller(&m, &pairs[i].p, &pairs[i].q);
    fp12_mul(&f, &f, &m);
  }
  pairing_final(&f, &f);
  return fp12_is_one(&f);
}

int pairing_check_be(const unsigned char *in, size_t n) {
  Fp12 f;
  Fp12 m;
  Pair pr;
  size_t i;
  if (n % 192u != 0u)
    return -1;
  fp12_one(&f);
  for (i = 0u; i < n; i += 192u) {
    if (!g1_from_be(&pr.p, in + i) || !g2_from_be(&pr.q, in + i + 64u))
      return -1;
    pairing_miller(&m, &pr.p, &pr.q);
    fp12_mul(&f, &f, &m);
  }
  pairing_final(&f, &f);
  return fp12_is_one(&f);
}
