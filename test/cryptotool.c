/* The C oracle rows (S8b, Q-S8b-7 (a)). Each line is
   `FILE NAME ARGS => RESULT`. FILE is the path of a kit file without `.lang`:
   examples/crypto for the Word ops, test/crypto/crypto for the field ops on
   Fr and Fp and for ptMulC on G1 (S8b2), test/crypto/keccak for keccak256
   (kcBlock, kcHash, kcChal; S8b2, Q-S8b2-14 (a)), test/crypto/poseidon for
   Poseidon (psPermC, psHashC, psHashG; S8b2, Q-S8b2-18 (a)) and for poseidon2C
   (psHashN, psHashD; S8b2, Q-S8b2-30 (a)), test/crypto/schnorr for the
   Schnorr check and the message words (scYes, scNo, scMsgFill, scMsgAmend,
   scMsgWd, scMsgIntent; S8b2, Q-S8b2-23 (a)), test/crypto/pairing for the
   Fp2, Fp6 and Fp12 tower of the BN254 pairing (the prF2, prF6 and prF12
   rows; S8c). NAME is a def of FILE.
   `cryptotool fixtures` prints the fixture block of domain/domain.lang
   (S8b2, Q-S8b2-4 (a)). `cryptotool pairing-fixtures` prints the fixture
   block of test/crypto/pairing.lang (S8c).
   ARGS and RESULT are Word literals (`0x%08xw`) or a Nat. A field row gives
   the inputs and then the value of
   the C oracle, each as 8 Words, least significant first. The def gives 1
   when the field op of the domain file gives the same value. test/gate.sh
   evaluates each line with `langc --kit eval`. The inputs come from a fixed
   seed and from known answers. The field values come from src/crypto/bn254.c
   (quorum-lang 8d1a113, canonical form), the digests from
   src/crypto/keccak.c and src/crypto/poseidon.c. Exit 1 when the C oracle
   does not give a known answer. The self checks of the pairing oracle
   (src/crypto/pairing.c; S8c, Q-S8c-2 (a)) print no row. They compare the
   tower fields, G2 and its subgroup check, the Miller loop, the final power
   and the byte input with plain forms and known identities. A wrong check
   gives exit 1 also. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "crypto/bn254.h"
#include "crypto/keccak.h"
#include "crypto/pairing.h"
#include "crypto/poseidon.h"
#include "eip197.h"

/* The modulus less one and less two, the most significant digit first.
   Source: ratify-lang test/cryptotool.c ORDER_LESS_ONE and PRIME_LESS_TWO.
   field_rows checks each one against BN254_FR and BN254_FP. */
#define FR_LESS_ONE "30644e72e131a029b85045b68181585d2833e84879b9709143e1f593f0000000"
#define FR_LESS_TWO "30644e72e131a029b85045b68181585d2833e84879b9709143e1f593efffffff"
#define FP_LESS_ONE "30644e72e131a029b85045b68181585d97816a916871ca8d3c208c16d87cfd46"
#define FP_LESS_TWO "30644e72e131a029b85045b68181585d97816a916871ca8d3c208c16d87cfd45"

/* The Word edge values and the shift counts of the edge rows. */
static const uint32_t EDGE[5] = {0x0u, 0x1u, 0x7fffffffu, 0x80000000u, 0xffffffffu};
static const uint32_t SHIFT[4] = {0u, 31u, 32u, 33u};

/* The state of the fixed-seed generator (splitmix64). */
typedef struct {
  uint64_t s;
} Rng;

static uint64_t next64(Rng *r) {
  uint64_t z;
  r->s += 0x9e3779b97f4a7c15ULL;
  z = r->s;
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
  return z ^ (z >> 31);
}

static uint32_t next32(Rng *r) {
  return (uint32_t)(next64(r) >> 32);
}

/* The Word ops of examples/crypto.lang. A shift count is mod 32 (the Wasm
   i32 rule). A unary op takes Y and does not use it. */
typedef uint32_t (*Op2)(uint32_t x, uint32_t y);

static uint32_t w_add(uint32_t x, uint32_t y) { return x + y; }
static uint32_t w_sub(uint32_t x, uint32_t y) { return x - y; }
static uint32_t w_mul(uint32_t x, uint32_t y) { return x * y; }
static uint32_t w_mulhi(uint32_t x, uint32_t y) { return (uint32_t)(((uint64_t)x * (uint64_t)y) >> 32); }
static uint32_t w_and(uint32_t x, uint32_t y) { return x & y; }
static uint32_t w_or(uint32_t x, uint32_t y) { return x | y; }
static uint32_t w_xor(uint32_t x, uint32_t y) { return x ^ y; }
static uint32_t w_shl(uint32_t x, uint32_t y) { return x << (y & 31u); }
static uint32_t w_shr(uint32_t x, uint32_t y) { return x >> (y & 31u); }

static uint32_t w_rotl(uint32_t x, uint32_t y) {
  uint32_t n = y & 31u;
  return n == 0u ? x : (x << n) | (x >> (32u - n));
}

static uint32_t w_rotr(uint32_t x, uint32_t y) {
  uint32_t n = y & 31u;
  return n == 0u ? x : (x >> n) | (x << (32u - n));
}

static uint32_t w_lt(uint32_t x, uint32_t y) { return x < y ? 1u : 0u; }
static uint32_t w_eq(uint32_t x, uint32_t y) { return x == y ? 1u : 0u; }

static uint32_t w_not(uint32_t x, uint32_t y) {
  (void)y;
  return ~x;
}

static uint32_t w_lit(uint32_t x, uint32_t y) {
  (void)y;
  return x ^ 0xdeadbeefu;
}

/* The argument grid of an op: two edge values, an edge value and a shift
   count, or one edge value. */
typedef enum { ARGS_TWO, ARGS_SHIFT, ARGS_ONE } Args;

/* The result of an op: a Word literal or a Nat. */
typedef enum { OUT_WORD, OUT_NAT } Out;

typedef struct {
  const char *name;
  Op2 op;
  Args args;
  Out out;
} WordOp;

static const WordOp WORD_OPS[] = {
    {"wadd", w_add, ARGS_TWO, OUT_WORD},     {"wsub", w_sub, ARGS_TWO, OUT_WORD},
    {"wmul", w_mul, ARGS_TWO, OUT_WORD},     {"wmulhi", w_mulhi, ARGS_TWO, OUT_WORD},
    {"wand", w_and, ARGS_TWO, OUT_WORD},     {"wor", w_or, ARGS_TWO, OUT_WORD},
    {"wxor", w_xor, ARGS_TWO, OUT_WORD},     {"wshl", w_shl, ARGS_SHIFT, OUT_WORD},
    {"wshr", w_shr, ARGS_SHIFT, OUT_WORD},   {"wrotl", w_rotl, ARGS_SHIFT, OUT_WORD},
    {"wrotr", w_rotr, ARGS_SHIFT, OUT_WORD}, {"wnot", w_not, ARGS_ONE, OUT_WORD},
    {"wlit", w_lit, ARGS_ONE, OUT_WORD},     {"wlt", w_lt, ARGS_TWO, OUT_NAT},
    {"weq", w_eq, ARGS_TWO, OUT_NAT},
};

static void put_word(uint32_t x) {
  printf(" 0x%08xw", (unsigned)x);
}

/* Print the row of the op W on X and Y. */
static void word_row(const WordOp *w, uint32_t x, uint32_t y) {
  uint32_t r = w->op(x, y);
  printf("examples/crypto %s", w->name);
  put_word(x);
  if (w->args != ARGS_ONE)
    put_word(y);
  switch (w->out) {
  case OUT_WORD:
    printf(" =>");
    put_word(r);
    printf("\n");
    break;
  case OUT_NAT:
    printf(" => %u\n", (unsigned)r);
    break;
  }
}

/* The edge rows of each op, then 3 rows from the generator. */
static void word_rows(Rng *rng) {
  size_t i;
  size_t j;
  size_t k;
  for (i = 0u; i < sizeof WORD_OPS / sizeof WORD_OPS[0]; i++) {
    const WordOp *w = &WORD_OPS[i];
    switch (w->args) {
    case ARGS_TWO:
      for (j = 0u; j < 5u; j++)
        for (k = 0u; k < 5u; k++)
          word_row(w, EDGE[j], EDGE[k]);
      break;
    case ARGS_SHIFT:
      for (j = 0u; j < 5u; j++)
        for (k = 0u; k < 4u; k++)
          word_row(w, EDGE[j], SHIFT[k]);
      break;
    case ARGS_ONE:
      for (j = 0u; j < 5u; j++)
        word_row(w, EDGE[j], 0u);
      break;
    }
    for (j = 0u; j < 3u; j++) {
      uint32_t x = next32(rng);
      uint32_t y = next32(rng);
      word_row(w, x, y);
    }
  }
}

/* The field element of the 64 hex digits HEX, the most significant digit
   first. */
static void fe_hex(Fe *out, const char *hex) {
  size_t i;
  memset(out, 0, sizeof *out);
  for (i = 0u; i < 64u; i++) {
    char c = hex[i];
    uint32_t d = c >= 'a' ? (uint32_t)(c - 'a' + 10) : (uint32_t)(c - '0');
    size_t limb = (63u - i) / 8u;
    out->w[limb] = (out->w[limb] << 4) | d;
  }
}

static int same(const Fe *a, const Fe *b) {
  return memcmp(a->w, b->w, sizeof a->w) == 0;
}

/* 1 when A is less than the modulus F. */
static int below(const Fe *a, const Modulus *f) {
  size_t i = BN_LIMBS;
  while (i > 0u) {
    i--;
    if (a->w[i] != f->w[i])
      return a->w[i] < f->w[i];
  }
  return 0;
}

/* A canonical element from the generator: 8 limbs, the top limb less than
   2^30, again while the value is F or more. */
static void fe_random(Fe *out, Rng *rng, const Modulus *f) {
  do {
    size_t i;
    for (i = 0u; i < BN_LIMBS; i++)
      out->w[i] = next32(rng);
    out->w[BN_LIMBS - 1u] &= 0x3fffffffu;
  } while (!below(out, f));
}

typedef enum { FIELD_ADD, FIELD_SUB, FIELD_MUL, FIELD_INV } FieldOp;

typedef struct {
  const char *prefix;
  const Modulus *mod;
} Field;

static void put_fe(const Fe *a) {
  size_t i;
  for (i = 0u; i < BN_LIMBS; i++)
    put_word(a->w[i]);
}

/* Print the row of OP on A and B (FIELD_INV does not use B). OUT gets the
   value of the C oracle. */
static void field_row(const Field *f, FieldOp op, const Fe *a, const Fe *b, Fe *out) {
  switch (op) {
  case FIELD_ADD:
    fe_add(out, a, b, f->mod);
    printf("test/crypto/crypto %sAdd", f->prefix);
    break;
  case FIELD_SUB:
    fe_sub(out, a, b, f->mod);
    printf("test/crypto/crypto %sSub", f->prefix);
    break;
  case FIELD_MUL:
    fe_mul(out, a, b, f->mod);
    printf("test/crypto/crypto %sMul", f->prefix);
    break;
  case FIELD_INV:
    fe_inv(out, a, f->mod);
    printf("test/crypto/crypto %sInv", f->prefix);
    break;
  }
  put_fe(a);
  if (op != FIELD_INV)
    put_fe(b);
  put_fe(out);
  printf(" => 1\n");
}

/* A known-answer row: BAD counts the rows where the C oracle does not give
   WANT. */
static void known(const Field *f, FieldOp op, const Fe *a, const Fe *b, const Fe *want, unsigned *bad) {
  Fe out;
  field_row(f, op, a, b, &out);
  *bad += (unsigned)!same(&out, want);
}

/* The rows of the field F. BAD counts the wrong known answers and the rows
   where the inverse op does not give the input back. */
static void field_rows(const Field *f, const char *less_one, const char *less_two, Rng *rng, unsigned *bad) {
  Fe zero;
  Fe one;
  Fe two;
  Fe m1;
  Fe m2;
  Fe a;
  Fe b;
  Fe out;
  Fe back;
  Fe inv;
  int k;
  fe_from_u64(&zero, 0u);
  fe_from_u64(&one, 1u);
  fe_from_u64(&two, 2u);
  fe_hex(&m1, less_one);
  fe_hex(&m2, less_two);
  back = m1;
  back.w[0] += 1u;
  *bad += (unsigned)(memcmp(back.w, f->mod->w, sizeof back.w) != 0);
  back = m2;
  back.w[0] += 2u;
  *bad += (unsigned)(memcmp(back.w, f->mod->w, sizeof back.w) != 0);
  known(f, FIELD_ADD, &zero, &zero, &zero, bad);
  known(f, FIELD_ADD, &zero, &one, &one, bad);
  known(f, FIELD_ADD, &m1, &one, &zero, bad);
  known(f, FIELD_ADD, &one, &m1, &zero, bad);
  known(f, FIELD_ADD, &m1, &m1, &m2, bad);
  known(f, FIELD_SUB, &zero, &zero, &zero, bad);
  known(f, FIELD_SUB, &one, &zero, &one, bad);
  known(f, FIELD_SUB, &zero, &one, &m1, bad);
  known(f, FIELD_SUB, &one, &one, &zero, bad);
  known(f, FIELD_SUB, &m1, &m1, &zero, bad);
  known(f, FIELD_MUL, &zero, &m1, &zero, bad);
  known(f, FIELD_MUL, &one, &m1, &m1, bad);
  known(f, FIELD_MUL, &two, &m1, &m2, bad);
  known(f, FIELD_MUL, &m1, &m1, &one, bad);
  known(f, FIELD_INV, &one, &zero, &one, bad);
  known(f, FIELD_INV, &m1, &zero, &m1, bad);
  field_row(f, FIELD_INV, &two, &zero, &out);
  fe_mul(&back, &two, &out, f->mod);
  *bad += (unsigned)!same(&back, &one);
  for (k = 0; k < 4; k++) {
    fe_random(&a, rng, f->mod);
    fe_random(&b, rng, f->mod);
    field_row(f, FIELD_ADD, &a, &b, &out);
    fe_sub(&back, &out, &b, f->mod);
    *bad += (unsigned)!same(&back, &a);
  }
  for (k = 0; k < 4; k++) {
    fe_random(&a, rng, f->mod);
    fe_random(&b, rng, f->mod);
    field_row(f, FIELD_SUB, &a, &b, &out);
    fe_add(&back, &out, &b, f->mod);
    *bad += (unsigned)!same(&back, &a);
  }
  for (k = 0; k < 4; k++) {
    fe_random(&a, rng, f->mod);
    fe_random(&b, rng, f->mod);
    field_row(f, FIELD_MUL, &a, &b, &out);
    fe_inv(&inv, &b, f->mod);
    fe_mul(&back, &out, &inv, f->mod);
    *bad += (unsigned)!same(&back, &a);
  }
  for (k = 0; k < 3; k++) {
    fe_random(&a, rng, f->mod);
    field_row(f, FIELD_INV, &a, &zero, &out);
    fe_mul(&back, &a, &out, f->mod);
    *bad += (unsigned)!same(&back, &one);
  }
}

/* A feMulC row (S8b2, Q-S8b-4 (a)). feMulC gives the Montgomery product
   A B R^-1 mod m of the raw inputs, R = 2^256. The C oracle takes the exact
   product by fe_mul, then the product with RINV, the inverse of R mod m. */
static void mulc_row(const Field *f, const Fe *rinv, const Fe *a, const Fe *b) {
  Fe out;
  fe_mul(&out, a, b, f->mod);
  fe_mul(&out, &out, rinv, f->mod);
  printf("test/crypto/crypto %sMulC", f->prefix);
  put_fe(a);
  put_fe(b);
  put_fe(&out);
  printf(" => 1\n");
}

/* The feMulC rows of the field F: each pair of the edges 0, 1 and m - 1,
   then 3 pairs from the generator. The inputs are canonical, so the CIOS
   steps of feMulC give the canonical value. The C oracle does not do the
   CIOS steps, thus a non-canonical input has no row. BAD counts a wrong
   R mod m (2^256 - 1 mod m, plus 1) and a wrong inverse of R. */
static void mulc_rows(const Field *f, const char *less_one, Rng *rng, unsigned *bad) {
  Fe edge[3];
  Fe two;
  Fe ones;
  Fe r;
  Fe rinv;
  Fe a;
  Fe b;
  U256 e;
  size_t i;
  size_t j;
  int k;
  fe_from_u64(&edge[0], 0u);
  fe_from_u64(&edge[1], 1u);
  fe_hex(&edge[2], less_one);
  fe_from_u64(&two, 2u);
  u256_from_u64(&e, 256u);
  fe_pow(&r, &two, &e, f->mod);
  memset(ones.w, 0xff, sizeof ones.w);
  fe_mul(&a, &ones, &edge[1], f->mod);
  fe_add(&a, &a, &edge[1], f->mod);
  *bad += (unsigned)!same(&a, &r);
  fe_inv(&rinv, &r, f->mod);
  fe_mul(&a, &r, &rinv, f->mod);
  *bad += (unsigned)!same(&a, &edge[1]);
  for (i = 0u; i < 3u; i++)
    for (j = 0u; j < 3u; j++)
      mulc_row(f, &rinv, &edge[i], &edge[j]);
  for (k = 0; k < 3; k++) {
    fe_random(&a, rng, f->mod);
    fe_random(&b, rng, f->mod);
    mulc_row(f, &rinv, &a, &b);
  }
}

/* A ptMulC row against the guest ptMul, its specification (S8b2, Q-S8b2-1
   (a), Q-S8b2-8 (a)): K, then P, each coordinate canonical. The def gives 1
   when ptMulC and the guest ptMul give the same point. */
static void pt_guest_row(const Fe *k, const G1 *p) {
  printf("test/crypto/crypto fpPtGuest");
  put_fe(k);
  put_fe(&p->x);
  put_fe(&p->y);
  printf(" => 1\n");
}

/* A ptMulC row against g1_mul of src/crypto/bn254.c (S8b2, Q-S8b2-8 (a)):
   K, P, then K P, each coordinate canonical. */
static void pt_mul_row(const Fe *k, const G1 *p) {
  G1 out;
  U256 e;
  fe_to_u256(&e, k);
  g1_mul(&out, p, &e);
  printf("test/crypto/crypto fpPtMul");
  put_fe(k);
  put_fe(&p->x);
  put_fe(&p->y);
  put_fe(&out.x);
  put_fe(&out.y);
  printf(" => 1\n");
}

static int g1_same(const G1 *a, const G1 *b) {
  return same(&a->x, &b->x) && same(&a->y, &b->y);
}

/* The ptMulC rows on G = (1, 2) and on the point at infinity (0, 0). The
   check of the domain text and one guest ptMul use the same 1 GiB arena of
   the evaluator, and a guest ptMul with k = 2^192 does not fit
   (Q-S8b2-8 (a), Q-S8b2-19 (a)). Thus the guest rows take k 0, 1, 0x1234,
   0xdeadbeef, 2^64 - 1 and 2^160 on G, and r - 1 on the point at infinity.
   The g1_mul rows take k 0, 1, 0x1234, 0xdeadbeef, r - 1 and 2^256 - 1 on
   G, and r - 1 on the point at infinity. BAD counts a wrong known answer of
   g1_mul: 0 G = (0, 0), 1 G = G and (r - 1) G = (1, p - 2). */
static void pt_rows(const char *r_less_one, const char *p_less_two, unsigned *bad) {
  G1 g;
  G1 inf;
  G1 neg;
  G1 out;
  Fe k[8];
  U256 e;
  size_t i;
  memset(&inf, 0, sizeof inf);
  fe_from_u64(&g.x, 1u);
  fe_from_u64(&g.y, 2u);
  fe_from_u64(&neg.x, 1u);
  fe_hex(&neg.y, p_less_two);
  fe_from_u64(&k[0], 0u);
  fe_from_u64(&k[1], 1u);
  fe_from_u64(&k[2], 0x1234u);
  fe_from_u64(&k[3], 0xdeadbeefu);
  fe_from_u64(&k[4], 0xffffffffffffffffu);
  fe_from_u64(&k[5], 0u);
  k[5].w[5] = 1u;
  fe_hex(&k[6], r_less_one);
  memset(k[7].w, 0xff, sizeof k[7].w);
  fe_to_u256(&e, &k[0]);
  g1_mul(&out, &g, &e);
  *bad += (unsigned)!g1_same(&out, &inf);
  fe_to_u256(&e, &k[1]);
  g1_mul(&out, &g, &e);
  *bad += (unsigned)!g1_same(&out, &g);
  fe_to_u256(&e, &k[6]);
  g1_mul(&out, &g, &e);
  *bad += (unsigned)!g1_same(&out, &neg);
  for (i = 0u; i < 6u; i++)
    pt_guest_row(&k[i], &g);
  pt_guest_row(&k[6], &inf);
  for (i = 0u; i < 4u; i++)
    pt_mul_row(&k[i], &g);
  pt_mul_row(&k[6], &g);
  pt_mul_row(&k[7], &g);
  pt_mul_row(&k[6], &inf);
}

/* The Fe of the 32 bytes B, the most significant byte first. */
static void be_fe(Fe *out, const unsigned char *b) {
  size_t i;
  for (i = 0u; i < BN_LIMBS; i++)
    out->w[BN_LIMBS - 1u - i] = (uint32_t)b[4u * i] << 24 | (uint32_t)b[4u * i + 1u] << 16 |
                                (uint32_t)b[4u * i + 2u] << 8 | (uint32_t)b[4u * i + 3u];
}

/* The 32 bytes of A, the most significant byte first (the inverse of be_fe). */
static void fe_be(unsigned char *b, const Fe *a) {
  size_t i;
  for (i = 0u; i < BN_LIMBS; i++) {
    uint32_t x = a->w[BN_LIMBS - 1u - i];
    b[4u * i] = (unsigned char)(x >> 24);
    b[4u * i + 1u] = (unsigned char)(x >> 16);
    b[4u * i + 2u] = (unsigned char)(x >> 8);
    b[4u * i + 3u] = (unsigned char)x;
  }
}

/* The Word of the 4 bytes B, the least significant byte first. */
static uint32_t le_word(const unsigned char *b) {
  return (uint32_t)b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24;
}

/* A less F while A is F or more. A digest is less than 2^256 < 6 r: at most
   5 subtractions, as kcModR (Q-S8b2-13 (a)). */
static void fe_reduce(Fe *a, const Modulus *f) {
  while (!below(a, f)) {
    uint32_t borrow = 0u;
    size_t i;
    for (i = 0u; i < BN_LIMBS; i++) {
      uint64_t d = (uint64_t)a->w[i] - (uint64_t)f->w[i] - (uint64_t)borrow;
      a->w[i] = (uint32_t)d;
      borrow = (uint32_t)(d >> 63);
    }
  }
}

/* A kcBlock row (S8b2, Q-S8b2-11 (a)): the C pads the N bytes of MSG to one
   136-byte block (byte N gets 0x01, byte 135 gets 0x80), then gives the 17
   lanes (each lane lo, then hi) and keccak256 of MSG. N is 135 or less. */
static void kc_block_row(const unsigned char *msg, size_t n) {
  unsigned char block[136];
  unsigned char d[32];
  Fe h;
  size_t j;
  memset(block, 0, sizeof block);
  memcpy(block, msg, n);
  block[n] ^= 0x01u;
  block[135] ^= 0x80u;
  keccak256(msg, n, d);
  be_fe(&h, d);
  printf("test/crypto/keccak kcBlock");
  for (j = 0u; j < 17u; j++) {
    put_word(le_word(block + 8u * j));
    put_word(le_word(block + 8u * j + 4u));
  }
  put_fe(&h);
  printf(" => 1\n");
}

/* A kcHash row and a kcChal row on the five words A (S8b2, Q-S8b-6 (a)):
   keccak256 of the 160 bytes (each word 32 bytes, the most significant byte
   first), then the digest (kcHash) and the digest mod r (kcChal). */
static void kc_hash_rows(const Fe a[5]) {
  unsigned char msg[160];
  unsigned char d[32];
  Fe h;
  Fe e;
  size_t i;
  for (i = 0u; i < 5u; i++)
    fe_be(msg + 32u * i, &a[i]);
  keccak256(msg, sizeof msg, d);
  be_fe(&h, d);
  e = h;
  fe_reduce(&e, &BN254_FR);
  printf("test/crypto/keccak kcHash");
  for (i = 0u; i < 5u; i++)
    put_fe(&a[i]);
  put_fe(&h);
  printf(" => 1\ntest/crypto/keccak kcChal");
  for (i = 0u; i < 5u; i++)
    put_fe(&a[i]);
  put_fe(&e);
  printf(" => 1\n");
}

/* The keccak rows (S8b2, Q-S8b2-11 (a)): 4 kcBlock rows ("", "abc", 55 and
   135 bytes, byte i = i mod 256), then 3 kcHash and 3 kcChal rows: G, G and
   0x1234; 4 words on Fp and 1 on Fr from the generator; p - 1 four times and
   r - 1. BAD counts the wrong known answers of keccak256("") and
   keccak256("abc"). */
static void kc_rows(Rng *rng, unsigned *bad) {
  static const char *const KNOWN[2][2] = {
      {"", "c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470"},
      {"abc", "4e03657aea45a94fc7d47ba826c8d667c0d1e6e33a64a036ec44f58fa12d6c45"}};
  unsigned char msg[135];
  unsigned char d[32];
  Fe h;
  Fe want;
  Fe a[5];
  size_t i;
  for (i = 0u; i < 2u; i++) {
    keccak256((const unsigned char *)KNOWN[i][0], strlen(KNOWN[i][0]), d);
    be_fe(&h, d);
    fe_hex(&want, KNOWN[i][1]);
    if (!same(&h, &want))
      (*bad)++;
  }
  for (i = 0u; i < sizeof msg; i++)
    msg[i] = (unsigned char)(i & 0xffu);
  kc_block_row((const unsigned char *)"", 0u);
  kc_block_row((const unsigned char *)"abc", 3u);
  kc_block_row(msg, 55u);
  kc_block_row(msg, 135u);
  memset(a, 0, sizeof a);
  a[0].w[0] = 1u;
  a[1].w[0] = 2u;
  a[2].w[0] = 1u;
  a[3].w[0] = 2u;
  a[4].w[0] = 0x1234u;
  kc_hash_rows(a);
  for (i = 0u; i < 4u; i++)
    fe_random(&a[i], rng, &BN254_FP);
  fe_random(&a[4], rng, &BN254_FR);
  kc_hash_rows(a);
  for (i = 0u; i < 4u; i++)
    fe_hex(&a[i], FP_LESS_ONE);
  fe_hex(&a[4], FR_LESS_ONE);
  kc_hash_rows(a);
}

/* A psPermC row (S8b2, Q-S8b2-18 (a)): the state S, then poseidon_perm of
   S. */
static void ps_perm_row(const PoseidonParams *p, const Fe s[POSEIDON_WIDTH]) {
  Fe t[POSEIDON_WIDTH];
  size_t i;
  memcpy(t, s, sizeof t);
  poseidon_perm(t, p);
  printf("test/crypto/poseidon psPermC");
  for (i = 0u; i < POSEIDON_WIDTH; i++)
    put_fe(&s[i]);
  for (i = 0u; i < POSEIDON_WIDTH; i++)
    put_fe(&t[i]);
  printf(" => 1\n");
}

/* A psHashC row on A and B: element 0 of poseidon_perm of (0, A, B), then a
   psHashN row (the native poseidon2C, Q-S8b2-30 (a)) on the same values. When
   SPEC is 1, a psHashG row (the guest feMul) on the same values too. */
static void ps_hash_rows(const PoseidonParams *p, const Fe *a, const Fe *b, int spec) {
  Fe t[POSEIDON_WIDTH];
  fe_from_u64(&t[0], 0u);
  t[1] = *a;
  t[2] = *b;
  poseidon_perm(t, p);
  printf("test/crypto/poseidon psHashC");
  put_fe(a);
  put_fe(b);
  put_fe(&t[0]);
  printf(" => 1\n");
  printf("test/crypto/poseidon psHashN");
  put_fe(a);
  put_fe(b);
  put_fe(&t[0]);
  printf(" => 1\n");
  if (spec) {
    printf("test/crypto/poseidon psHashG");
    put_fe(a);
    put_fe(b);
    put_fe(&t[0]);
    printf(" => 1\n");
  }
}

/* A psHashD row on A and B (S8b2, Q-S8b2-30 (a)): the native poseidon2C and
   the guest poseidon2 (the specification) must give the same value. The row
   has no C value. */
static void ps_direct_row(const Fe *a, const Fe *b) {
  printf("test/crypto/poseidon psHashD");
  put_fe(a);
  put_fe(b);
  printf(" => 1\n");
}

/* The Poseidon rows (S8b2, Q-S8b2-18 (a)): 5 psPermC rows ((0, 1, 2),
   (0, 0, 0), (r - 1, r - 1, r - 1) and 2 generator states), 3 psHashC rows
   ((1, 2), (0, 0) and 1 generator pair), a psHashN row after each psHashC
   row, 1 psHashG row on (1, 2) and 3 psHashD rows ((0, 0), (1, 2),
   (r - 1, r - 1); Q-S8b2-30 (a)). BAD counts a wrong known answer: element 0 of the permutation of (0, 1, 2) is
   0x115cc0f5..189a (circomlib Poseidon of 1 and 2). */
static void ps_rows(Rng *rng, unsigned *bad) {
  static PoseidonParams p;
  static const Fe KNOWN = {{0x4417189au, 0x9e19607au, 0x74324551u, 0x2a3617f2u, 0x9662e9cfu, 0x3df64c6bu, 0xe7d69041u,
                            0x115cc0f5u}};
  Fe s[POSEIDON_WIDTH];
  Fe t[POSEIDON_WIDTH];
  Fe a;
  Fe b;
  size_t i;
  size_t k;
  poseidon_params(&p, &BN254_FR);
  for (i = 0u; i < POSEIDON_WIDTH; i++)
    fe_from_u64(&s[i], (uint64_t)i);
  memcpy(t, s, sizeof t);
  poseidon_perm(t, &p);
  *bad += (unsigned)!same(&t[0], &KNOWN);
  ps_perm_row(&p, s);
  for (i = 0u; i < POSEIDON_WIDTH; i++)
    fe_from_u64(&s[i], 0u);
  ps_perm_row(&p, s);
  for (i = 0u; i < POSEIDON_WIDTH; i++)
    fe_hex(&s[i], FR_LESS_ONE);
  ps_perm_row(&p, s);
  for (k = 0u; k < 2u; k++) {
    for (i = 0u; i < POSEIDON_WIDTH; i++)
      fe_random(&s[i], rng, &BN254_FR);
    ps_perm_row(&p, s);
  }
  fe_from_u64(&a, 1u);
  fe_from_u64(&b, 2u);
  ps_hash_rows(&p, &a, &b, 1);
  fe_from_u64(&a, 0u);
  fe_from_u64(&b, 0u);
  ps_hash_rows(&p, &a, &b, 0);
  fe_random(&a, rng, &BN254_FR);
  fe_random(&b, rng, &BN254_FR);
  ps_hash_rows(&p, &a, &b, 0);
  fe_from_u64(&a, 0u);
  fe_from_u64(&b, 0u);
  ps_direct_row(&a, &b);
  fe_from_u64(&a, 1u);
  fe_from_u64(&b, 2u);
  ps_direct_row(&a, &b);
  fe_hex(&a, FR_LESS_ONE);
  fe_hex(&b, FR_LESS_ONE);
  ps_direct_row(&a, &b);
}

/* The Schnorr rows and the domain fixtures (S8b2, Q-S8b-6 (a), Q-S8b2-3 (a),
   Q-S8b2-22 (a), Q-S8b2-25 (a)). poseidon2(A, B) is element 0 of
   poseidon_perm of (0, A, B). */
static void sc_p2(Fe *out, const PoseidonParams *p, const Fe *a, const Fe *b) {
  Fe t[POSEIDON_WIDTH];
  fe_from_u64(&t[0], 0u);
  t[1] = *a;
  t[2] = *b;
  poseidon_perm(t, p);
  *out = t[0];
}

/* The message word of tag T on X and Y: poseidon2(T, poseidon2(X, Y)), as
   msWord. */
static void sc_msg_word(Fe *out, const PoseidonParams *p, uint64_t t, const Fe *x, const Fe *y) {
  Fe tag;
  Fe in;
  fe_from_u64(&tag, t);
  sc_p2(&in, p, x, y);
  sc_p2(out, p, &tag, &in);
}

/* msFill (tag 0) and msAmend (tag 1) on the Nat values X and Y. */
static void sc_msg_pair(Fe *out, const PoseidonParams *p, uint64_t t, uint64_t x, uint64_t y) {
  Fe a;
  Fe b;
  fe_from_u64(&a, x);
  fe_from_u64(&b, y);
  sc_msg_word(out, p, t, &a, &b);
}

/* msWithdraw (tag 2): kh = poseidon2(x mod r, y mod r) on the canonical
   coordinates of PK, then poseidon2(TOK, poseidon2(AMT, H)). */
static void sc_msg_withdraw(Fe *out, const PoseidonParams *p, const G1 *pk, uint64_t tok, uint64_t amt,
                            uint64_t h) {
  Fe x = pk->x;
  Fe y = pk->y;
  Fe kh;
  Fe a;
  Fe b;
  Fe c;
  fe_reduce(&x, &BN254_FR);
  fe_reduce(&y, &BN254_FR);
  sc_p2(&kh, p, &x, &y);
  fe_from_u64(&a, amt);
  fe_from_u64(&b, h);
  sc_p2(&c, p, &a, &b);
  fe_from_u64(&a, tok);
  sc_p2(&b, p, &a, &c);
  sc_msg_word(out, p, 2u, &kh, &b);
}

/* msIntent (tag 3) on V = nf, tin, tout, st, ws, wl, dl: nf and the
   right-nested chain poseidon2(tin, poseidon2(tout, ... poseidon2(wl, dl))). */
static void sc_msg_intent(Fe *out, const PoseidonParams *p, const uint64_t v[7]) {
  Fe c;
  Fe a;
  Fe nf;
  size_t i;
  fe_from_u64(&c, v[6]);
  for (i = 6u; i > 1u; i--) {
    Fe t = c;
    fe_from_u64(&a, v[i - 1u]);
    sc_p2(&c, p, &a, &t);
  }
  fe_from_u64(&nf, v[0]);
  sc_msg_word(out, p, 3u, &nf, &c);
}

/* OUT = K G, G = (1, 2). */
static void sc_mul_g(G1 *out, const Fe *k) {
  G1 g;
  U256 u;
  fe_from_u64(&g.x, 1u);
  fe_from_u64(&g.y, 2u);
  fe_to_u256(&u, k);
  g1_mul(out, &g, &u);
}

/* The key of SK: PK = SK G (Q-S8b2-22 (a): fxKeyN = n G). */
static void sc_key(G1 *pk, uint64_t sk) {
  Fe k;
  fe_from_u64(&k, sk);
  sc_mul_g(pk, &k);
}

/* The signature (R, S) by SK on M (Q-S8b2-22 (a)): nonce = keccak256(sk, m)
   mod r, R = nonce G, e = keccak256(R.x, R.y, P.x, P.y, m) mod r
   (Q-S8b-6 (a)), s = nonce + e sk mod r. Each word is 32 bytes, the most
   significant byte first. */
static void sc_sign(G1 *rp, Fe *s, uint64_t sk, const Fe *m) {
  unsigned char buf[160];
  unsigned char dg[32];
  G1 pk;
  Fe k;
  Fe nonce;
  Fe e;
  Fe es;
  sc_key(&pk, sk);
  fe_from_u64(&k, sk);
  fe_be(buf, &k);
  fe_be(buf + 32, m);
  keccak256(buf, 64u, dg);
  be_fe(&nonce, dg);
  fe_reduce(&nonce, &BN254_FR);
  sc_mul_g(rp, &nonce);
  fe_be(buf, &rp->x);
  fe_be(buf + 32, &rp->y);
  fe_be(buf + 64, &pk.x);
  fe_be(buf + 96, &pk.y);
  fe_be(buf + 128, m);
  keccak256(buf, 160u, dg);
  be_fe(&e, dg);
  fe_reduce(&e, &BN254_FR);
  fe_mul(&es, &e, &k, &BN254_FR);
  fe_add(s, &nonce, &es, &BN254_FR);
}

/* A R mod F, R = 2^256 (1 doubled 256 times): the Montgomery form of a Pt1
   coordinate. */
static void sc_mont(Fe *out, const Fe *a, const Modulus *f) {
  Fe r;
  size_t i;
  fe_from_u64(&r, 1u);
  for (i = 0u; i < 256u; i++) {
    Fe t = r;
    fe_add(&r, &t, &t, f);
  }
  fe_mul(out, a, &r, f);
}

/* Q as 16 Words: x, then y, each in Montgomery form. */
static void put_pt(const G1 *q) {
  Fe x;
  sc_mont(&x, &q->x, &BN254_FP);
  put_fe(&x);
  sc_mont(&x, &q->y, &BN254_FP);
  put_fe(&x);
}

static void put_nat(uint64_t n) {
  Fe a;
  fe_from_u64(&a, n);
  put_fe(&a);
}

/* The fixture def fxNAME N TAIL of Q (a Pt1 in Montgomery form). */
static void fx_pt(const char *name, unsigned n, const char *tail, const G1 *q) {
  Fe x;
  printf("def fx%s%u%s : Pt1 := pt1 (fe", name, n, tail);
  sc_mont(&x, &q->x, &BN254_FP);
  put_fe(&x);
  printf(") (fe");
  sc_mont(&x, &q->y, &BN254_FP);
  put_fe(&x);
  printf(")\n");
}

/* The fixture defs fxNAME N R (a Pt1) and fxNAME N S (a canonical Fr): the
   signature by key N on M. */
static void fx_sig(const char *name, unsigned n, const Fe *m) {
  G1 rp;
  Fe s;
  sc_sign(&rp, &s, n, m);
  fx_pt(name, n, "R", &rp);
  printf("def fx%s%uS : Fr := fe", name, n);
  put_fe(&s);
  printf("\n");
}

/* The intentA message of examples/spend.lang: nf, tin, tout, st, ws, wl, dl. */
static const uint64_t SC_INTENT[7] = {1u, 1u, 2u, 4u, 1u, 1u, 5u};

/* `cryptotool fixtures` (Q-S8b2-4 (a)): the fixture block of
   domain/domain.lang, the two @FIX@ lines too. test/gate.sh compares the
   block with this print. The keys 1, 2, 3, 4, 7 and 9 of the examples, and
   one signature for each signed (key, message) pair of examples/params.lang
   and examples/spend.lang. */
static void fixtures(void) {
  static PoseidonParams p;
  static const unsigned KEY[6] = {1u, 2u, 3u, 4u, 7u, 9u};
  static const uint64_t WD[4][2] = {{3u, 1u}, {3u, 3u}, {3u, 4u}, {6u, 1u}};
  static const char *const WD_NAME[4] = {"WdOne", "WdThree", "WdFour", "WdSix"};
  G1 pk;
  Fe m;
  size_t i;
  poseidon_params(&p, &BN254_FR);
  puts("-- @FIX@ begin");
  puts("-- The Schnorr fixtures (S8b2, Q-S8b2-4 (a), Q-S8b2-22 (a)). Do not edit this");
  puts("-- block: `cryptotool fixtures` prints it, and test/gate.sh compares the two.");
  puts("-- fxKeyN = n G. fxMN R and fxMN S are the signature by key N on message M:");
  puts("-- Amend = msAmend 9 3, Fill = msFill 1 2, Intent = msIntent of intentA,");
  puts("-- WdOne, WdThree, WdFour, WdSix = msWithdraw fxKey1 1 of (3, 1), (3, 3),");
  puts("-- (3, 4), (6, 1).");
  for (i = 0u; i < 6u; i++) {
    sc_key(&pk, KEY[i]);
    fx_pt("Key", KEY[i], "", &pk);
  }
  sc_msg_pair(&m, &p, 1u, 9u, 3u);
  fx_sig("Amend", 1u, &m);
  fx_sig("Amend", 2u, &m);
  sc_msg_pair(&m, &p, 0u, 1u, 2u);
  fx_sig("Fill", 3u, &m);
  fx_sig("Fill", 4u, &m);
  sc_msg_intent(&m, &p, SC_INTENT);
  fx_sig("Intent", 7u, &m);
  sc_key(&pk, 1u);
  for (i = 0u; i < 4u; i++) {
    sc_msg_withdraw(&m, &p, &pk, 1u, WD[i][0], WD[i][1]);
    fx_sig(WD_NAME[i], 1u, &m);
    fx_sig(WD_NAME[i], 2u, &m);
  }
  puts("-- @FIX@ end");
}

/* A schnorr.lang row: NAME, then P and R (Montgomery form), S and M. Each row
   also gives a scTwin row on the same Words (S8b2, Q-S8b2-36 (a)). */
static void sc_row(const char *name, const G1 *pk, const G1 *rp, const Fe *s, const Fe *m) {
  printf("test/crypto/schnorr %s", name);
  put_pt(pk);
  put_pt(rp);
  put_fe(s);
  put_fe(m);
  printf(" => 1\n");
  printf("test/crypto/schnorr scTwin");
  put_pt(pk);
  put_pt(rp);
  put_fe(s);
  put_fe(m);
  printf(" => 1\n");
}

/* The scTwin row for R at the point at infinity (S8b2, Q-S8b2-43 (a)): R =
   (0, 0), e = keccak256(0, 0, P.x, P.y, m) mod r (Q-S8b-6 (a)), s = e sk
   mod r, thus s G = e P. The row has no scYes line: it says only that
   schnorrCheckC and schnorrCheck give the same result on these Words. */
static void sc_inf_row(uint64_t sk, const Fe *m) {
  unsigned char buf[160];
  unsigned char dg[32];
  G1 pk;
  G1 inf;
  Fe k;
  Fe e;
  Fe s;
  sc_key(&pk, sk);
  memset(&inf, 0, sizeof inf);
  fe_from_u64(&k, sk);
  fe_be(buf, &inf.x);
  fe_be(buf + 32, &inf.y);
  fe_be(buf + 64, &pk.x);
  fe_be(buf + 96, &pk.y);
  fe_be(buf + 128, m);
  keccak256(buf, 160u, dg);
  be_fe(&e, dg);
  fe_reduce(&e, &BN254_FR);
  fe_mul(&s, &e, &k, &BN254_FR);
  printf("test/crypto/schnorr scTwin");
  put_pt(&pk);
  put_pt(&inf);
  put_fe(&s);
  put_fe(m);
  printf(" => 1\n");
}

/* The Schnorr rows (S8b2, Q-S8b2-23 (a)): scYes on the signature by key 3 on
   the Fill message (fxFill3); scNo on s + 1, m + 1, R off the curve, P off
   the curve, P at infinity and s = r; the scTwin row for R at infinity
   (Q-S8b2-43 (a)); then 4 message rows (the messages of the fixtures), each
   with the C value. */
static void sc_rows(void) {
  static PoseidonParams p;
  G1 pk;
  G1 rp;
  G1 bad;
  Fe s;
  Fe m;
  Fe t;
  Fe one;
  size_t i;
  poseidon_params(&p, &BN254_FR);
  fe_from_u64(&one, 1u);
  sc_key(&pk, 3u);
  sc_msg_pair(&m, &p, 0u, 1u, 2u);
  sc_sign(&rp, &s, 3u, &m);
  sc_row("scYes", &pk, &rp, &s, &m);
  fe_add(&t, &s, &one, &BN254_FR);
  sc_row("scNo", &pk, &rp, &t, &m);
  fe_add(&t, &m, &one, &BN254_FR);
  sc_row("scNo", &pk, &rp, &s, &t);
  bad = rp;
  fe_add(&bad.y, &rp.y, &one, &BN254_FP);
  sc_row("scNo", &pk, &bad, &s, &m);
  bad = pk;
  fe_add(&bad.y, &pk.y, &one, &BN254_FP);
  sc_row("scNo", &bad, &rp, &s, &m);
  memset(&bad, 0, sizeof bad);
  sc_row("scNo", &bad, &rp, &s, &m);
  memcpy(t.w, BN254_FR.w, sizeof t.w);
  sc_row("scNo", &pk, &rp, &t, &m);
  sc_inf_row(3u, &m);
  printf("test/crypto/schnorr scMsgFill");
  put_nat(1u);
  put_nat(2u);
  put_fe(&m);
  printf(" => 1\n");
  sc_msg_pair(&m, &p, 1u, 9u, 3u);
  printf("test/crypto/schnorr scMsgAmend");
  put_nat(9u);
  put_nat(3u);
  put_fe(&m);
  printf(" => 1\n");
  sc_key(&pk, 1u);
  sc_msg_withdraw(&m, &p, &pk, 1u, 3u, 1u);
  printf("test/crypto/schnorr scMsgWd");
  put_pt(&pk);
  put_nat(1u);
  put_nat(3u);
  put_nat(1u);
  put_fe(&m);
  printf(" => 1\n");
  sc_msg_intent(&m, &p, SC_INTENT);
  printf("test/crypto/schnorr scMsgIntent");
  for (i = 0u; i < 7u; i++)
    put_nat(SC_INTENT[i]);
  put_fe(&m);
  printf(" => 1\n");
}

/* S8c G1 (D9): the self checks of the pairing oracle of crypto/pairing.h.
   The checks print nothing. BAD counts each wrong result. The order is the
   order of the layers: the tower, G2 and the subgroup test, the pairing,
   the exponents, the bytes of EIP-197. */

/* A natural number of BIG limbs of 32 bits, the least significant limb
   first. Each operation is modulo 2^(32 BIG). p^4 has 1016 bits, thus each
   value of the checks fits. */
#define BIG 34u

typedef struct {
  uint32_t w[BIG];
} Big;

static void big_u64(Big *out, uint64_t n) {
  memset(out, 0, sizeof *out);
  out->w[0] = (uint32_t)n;
  out->w[1] = (uint32_t)(n >> 32);
}

static void big_mod(Big *out, const Modulus *f) {
  memset(out, 0, sizeof *out);
  memcpy(out->w, f->w, sizeof f->w);
}

static int big_same(const Big *a, const Big *b) {
  return memcmp(a->w, b->w, sizeof a->w) == 0;
}

/* OUT can be A or B. */
static void big_add(Big *out, const Big *a, const Big *b) {
  uint64_t c = 0u;
  size_t i;
  for (i = 0u; i < BIG; i++) {
    c += (uint64_t)a->w[i] + (uint64_t)b->w[i];
    out->w[i] = (uint32_t)c;
    c >>= 32;
  }
}

/* OUT can be A or B. */
static void big_sub(Big *out, const Big *a, const Big *b) {
  uint64_t borrow = 0u;
  size_t i;
  for (i = 0u; i < BIG; i++) {
    uint64_t d = (uint64_t)a->w[i] - (uint64_t)b->w[i] - borrow;
    out->w[i] = (uint32_t)d;
    borrow = (d >> 32) & 1u;
  }
}

/* OUT can be A or B. */
static void big_mul(Big *out, const Big *a, const Big *b) {
  Big r;
  size_t i;
  size_t j;
  memset(&r, 0, sizeof r);
  for (i = 0u; i < BIG; i++) {
    uint64_t c = 0u;
    for (j = 0u; i + j < BIG; j++) {
      c += (uint64_t)a->w[i] * (uint64_t)b->w[j] + (uint64_t)r.w[i + j];
      r.w[i + j] = (uint32_t)c;
      c >>= 32;
    }
  }
  *out = r;
}

/* OUT = C[0] + C[1] X + C[2] X^2 + C[3] X^3 + C[4] X^4. */
static void big_poly(Big *out, const Big *x, const uint64_t c[5]) {
  Big r;
  Big t;
  size_t i = 5u;
  big_u64(&r, 0u);
  while (i > 0u) {
    i--;
    big_mul(&r, &r, x);
    big_u64(&t, c[i]);
    big_add(&r, &r, &t);
  }
  *out = r;
}

/* The count of the limbs of A, less the high limbs that are 0. */
static size_t big_limbs(const Big *a) {
  size_t n = BIG;
  while (n > 0u && a->w[n - 1u] == 0u)
    n--;
  return n;
}

/* The count of the one bits of N. */
static unsigned pr_ones(uint64_t n) {
  unsigned c = 0u;
  while (n != 0u) {
    c += (unsigned)(n & 1u);
    n >>= 1;
  }
  return c;
}

/* A^E for the N limbs at E, the least significant limb first: one product
   A A for each bit and one more product for each one bit. This is the plain
   power. The checks compare it with the fast forms of pairing.c, thus it
   does not call fp12_sqr. */
static void pr_pow(Fp12 *out, const Fp12 *a, const uint32_t *e, size_t n) {
  Fp12 r;
  size_t i = 32u * n;
  fp12_one(&r);
  while (i > 0u) {
    i--;
    fp12_mul(&r, &r, &r);
    if (((e[i / 32u] >> (i % 32u)) & 1u) != 0u)
      fp12_mul(&r, &r, a);
  }
  *out = r;
}

/* The modulus F as a U256. */
static void pr_modulus(U256 *out, const Modulus *f) {
  size_t i;
  for (i = 0u; i < 4u; i++)
    out->limb[i] = (uint64_t)f->w[2u * i] | (uint64_t)f->w[2u * i + 1u] << 32;
}

/* OUT = A + p as 8 limbs, with no reduction: a value that is p or more. */
static void pr_plus_p(Fe *out, const Fe *a) {
  uint64_t c = 0u;
  size_t i;
  for (i = 0u; i < BN_LIMBS; i++) {
    c += (uint64_t)a->w[i] + (uint64_t)BN254_FP.w[i];
    out->w[i] = (uint32_t)c;
    c >>= 32;
  }
}

/* The Fp12 value with the parts 1 to 12. */
static void pr_sample(Fp12 *out) {
  Fp2 *c[6];
  size_t i;
  c[0] = &out->a.c0;
  c[1] = &out->a.c1;
  c[2] = &out->a.c2;
  c[3] = &out->b.c0;
  c[4] = &out->b.c1;
  c[5] = &out->b.c2;
  for (i = 0u; i < 6u; i++) {
    fe_from_u64(&c[i]->c0, (uint64_t)(2u * i + 1u));
    fe_from_u64(&c[i]->c1, (uint64_t)(2u * i + 2u));
  }
}

/* The constant 3 / xi of the twist, by the public Fp2 functions (pairing.c
   keeps its constant private). */
static void pr_b2(Fp2 *out) {
  Fp2 xi;
  Fe three;
  fe_from_u64(&xi.c0, 9u);
  fe_from_u64(&xi.c1, 1u);
  fe_from_u64(&three, 3u);
  fp2_inv(&xi, &xi);
  fp2_mul_fe(out, &xi, &three);
}

/* 1 when y^2 = x^3 + 3 / xi at Q. */
static int pr_on_twist(const G2 *q) {
  Fp2 b2;
  Fp2 l;
  Fp2 r;
  pr_b2(&b2);
  fp2_sqr(&l, &q->y);
  fp2_sqr(&r, &q->x);
  fp2_mul(&r, &r, &q->x);
  fp2_add(&r, &r, &b2);
  return fp2_eq(&l, &r);
}

/* 1 when R Q is the point at infinity. BAD goes up when g2_in_subgroup(Q)
   does not give the same answer (Q-S8c-6). */
static int pr_order(const G2 *q, const U256 *r, unsigned *bad) {
  G2 t;
  int in;
  g2_mul(&t, q, r);
  in = g2_is_infinity(&t) != 0;
  if ((g2_in_subgroup(q) != 0) != in)
    (*bad)++;
  return in;
}

/* The 192 bytes of the pair (P, Q) in the EIP-197 order: the u part of each
   G2 coordinate is first. */
static void pr_pair_be(unsigned char *b, const G1 *p, const G2 *q) {
  fe_be(b, &p->x);
  fe_be(b + 32, &p->y);
  fe_be(b + 64, &q->x.c1);
  fe_be(b + 96, &q->x.c0);
  fe_be(b + 128, &q->y.c1);
  fe_be(b + 160, &q->y.c0);
}

/* The tower. A is the sample value. A inv(A) = 1. The p power map 12 times
   gives A, and 6 times gives fp12_conj(A). fp12_frob(A) = A^p and
   fp12_pow_u64(A, x) = A^x by the plain power. fp12_sqr(A) = A A.
   fp12_mul_line is the product with the full form of the line value.
   fp2_mul_xi is the product with 9 + u. Then 2 properties of bn254.c that
   pairing.c uses: fe_from_u64 clears each high limb, and fe_neg of 0 is
   0. */
static void pr_tower(unsigned *bad) {
  static const uint32_t X[2] = {(uint32_t)PAIRING_X, (uint32_t)(PAIRING_X >> 32)};
  Fp12 a;
  Fp12 t;
  Fp12 u;
  Fp12 full;
  Line l;
  Fp2 xi;
  Fp2 s;
  Fp2 s2;
  Fe z;
  Fe want;
  size_t i;
  pr_sample(&a);
  fp12_inv(&t, &a);
  fp12_mul(&t, &t, &a);
  if (!fp12_is_one(&t))
    (*bad)++;
  t = a;
  for (i = 0u; i < 12u; i++) {
    fp12_frob(&t, &t);
    if (i == 5u) {
      fp12_conj(&u, &a);
      if (!fp12_eq(&t, &u))
        (*bad)++;
    }
  }
  if (!fp12_eq(&t, &a))
    (*bad)++;
  fp12_frob(&t, &a);
  pr_pow(&u, &a, BN254_FP.w, BN_LIMBS);
  if (!fp12_eq(&t, &u))
    (*bad)++;
  fp12_pow_u64(&t, &a, PAIRING_X);
  pr_pow(&u, &a, X, 2u);
  if (!fp12_eq(&t, &u))
    (*bad)++;
  fp12_sqr(&t, &a);
  fp12_mul(&u, &a, &a);
  if (!fp12_eq(&t, &u))
    (*bad)++;
  fe_from_u64(&l.c0.c0, 3u);
  fe_from_u64(&l.c0.c1, 4u);
  fe_from_u64(&l.c1.c0, 5u);
  fe_from_u64(&l.c1.c1, 6u);
  fe_from_u64(&l.c3.c0, 7u);
  fe_from_u64(&l.c3.c1, 8u);
  memset(&full, 0, sizeof full);
  full.a.c0 = l.c0;
  full.b.c0 = l.c1;
  full.b.c1 = l.c3;
  fp12_mul_line(&t, &a, &l);
  fp12_mul(&u, &a, &full);
  if (!fp12_eq(&t, &u))
    (*bad)++;
  fe_from_u64(&xi.c0, 9u);
  fe_from_u64(&xi.c1, 1u);
  fe_from_u64(&s.c0, 5u);
  fe_from_u64(&s.c1, 7u);
  fp2_mul_xi(&s2, &s);
  fp2_mul(&xi, &xi, &s);
  if (!fp2_eq(&s2, &xi))
    (*bad)++;
  memset(&z, 0xff, sizeof z);
  fe_from_u64(&z, 5u);
  memset(&want, 0, sizeof want);
  want.w[0] = 5u;
  if (!same(&z, &want))
    (*bad)++;
  want.w[0] = 0u;
  fe_neg(&z, &want, &BN254_FP);
  if (!same(&z, &want))
    (*bad)++;
}

/* G2 and the subgroup test. The generator G is on the twist, r G is the
   point at infinity, and the p power map of G is p G. The sums 5 G + 7 G,
   G + G, G + (-G) and G + infinity are correct. Then 4 points R of the
   twist with x = n + u (n = 1, 2, ...): for R, for R + 5 G, for 7 G and
   for the point at infinity, g2_in_subgroup(Q) is 1 only when r Q is the
   point at infinity. One R or more is not in the subgroup (if not, the
   check has no force). For the first R, (2 p - r) R is in the subgroup:
   the twist has r (2 p - r) points. OUTSIDE gets an R that is not in the
   subgroup. */
static void pr_group(G2 *outside, unsigned *bad) {
  G2 g;
  G2 g5;
  G2 g7;
  G2 inf;
  G2 pt;
  G2 t;
  G2 u;
  Fp2 b2;
  Fp2 rhs;
  U256 r;
  U256 p;
  U256 h;
  U256 k;
  uint64_t n;
  unsigned found = 0u;
  unsigned out_n = 0u;
  pr_modulus(&r, &BN254_FR);
  pr_modulus(&p, &BN254_FP);
  memset(&inf, 0, sizeof inf);
  *outside = inf;
  g2_generator(&g);
  if (g2_is_infinity(&g) || !pr_on_twist(&g) || !g2_on_curve(&g))
    (*bad)++;
  if (!pr_order(&g, &r, bad))
    (*bad)++;
  g2_psi(&t, &g);
  if (g2_is_infinity(&t) || !pr_on_twist(&t))
    (*bad)++;
  g2_mul(&u, &g, &p);
  if (!g2_eq(&t, &u))
    (*bad)++;
  u256_from_u64(&k, 5u);
  g2_mul(&g5, &g, &k);
  u256_from_u64(&k, 7u);
  g2_mul(&g7, &g, &k);
  u256_from_u64(&k, 12u);
  g2_mul(&u, &g, &k);
  g2_add(&t, &g5, &g7);
  if (!g2_eq(&t, &u))
    (*bad)++;
  u256_from_u64(&k, 2u);
  g2_mul(&u, &g, &k);
  g2_add(&t, &g, &g);
  if (!g2_eq(&t, &u) || !pr_on_twist(&t))
    (*bad)++;
  g2_neg(&u, &g);
  g2_add(&t, &g, &u);
  if (!g2_is_infinity(&t))
    (*bad)++;
  g2_add(&t, &g, &inf);
  g2_add(&u, &inf, &g);
  if (!g2_eq(&t, &g) || !g2_eq(&u, &g))
    (*bad)++;
  pr_b2(&b2);
  u256_add(&h, &p, &p);
  u256_sub(&h, &h, &r);
  for (n = 1u; n <= 64u && found < 4u; n++) {
    fe_from_u64(&pt.x.c0, n);
    fe_from_u64(&pt.x.c1, 1u);
    fp2_sqr(&rhs, &pt.x);
    fp2_mul(&rhs, &rhs, &pt.x);
    fp2_add(&rhs, &rhs, &b2);
    if (fp2_sqrt(&pt.y, &rhs)) {
      found++;
      if (!pr_on_twist(&pt) || !g2_on_curve(&pt))
        (*bad)++;
      if (!pr_order(&pt, &r, bad)) {
        out_n++;
        *outside = pt;
      }
      g2_add(&t, &pt, &g5);
      (void)pr_order(&t, &r, bad);
      if (found == 1u) {
        g2_mul(&t, &pt, &h);
        if (g2_is_infinity(&t) || !pr_order(&t, &r, bad))
          (*bad)++;
      }
    }
  }
  if (found != 4u || out_n == 0u)
    (*bad)++;
  if (!pr_order(&g7, &r, bad))
    (*bad)++;
  if (!pr_order(&inf, &r, bad))
    (*bad)++;
}

/* The pairing, with G1 = (1, 2) and G2 the generator of G2. The Miller
   loop value is 1 when a point is the point at infinity. E = e(G1, G2) is
   not 1, and E^r = 1 by the plain power. e(5 G1, 7 G2) = E^35.
   e(5 G1, G2) = e(G1, 5 G2) = E^5. pairing_check gives 1 for (G1, G2),
   (-G1, G2), 0 for (G1, G2) only, and 1 for no pair. */
static void pr_pairing(unsigned *bad) {
  static const uint32_t K5[1] = {5u};
  static const uint32_t K35[1] = {35u};
  G1 g1;
  G1 n1;
  G1 p5;
  G1 inf1;
  G2 g2;
  G2 q5;
  G2 q7;
  G2 inf2;
  Fp12 e;
  Fp12 t;
  Fp12 u;
  Pair pairs[2];
  U256 k;
  fe_from_u64(&g1.x, 1u);
  fe_from_u64(&g1.y, 2u);
  n1 = g1;
  fe_neg(&n1.y, &g1.y, &BN254_FP);
  memset(&inf1, 0, sizeof inf1);
  memset(&inf2, 0, sizeof inf2);
  g2_generator(&g2);
  u256_from_u64(&k, 5u);
  g1_mul(&p5, &g1, &k);
  g2_mul(&q5, &g2, &k);
  u256_from_u64(&k, 7u);
  g2_mul(&q7, &g2, &k);
  pairing_miller(&t, &inf1, &g2);
  if (!fp12_is_one(&t))
    (*bad)++;
  pairing_miller(&t, &g1, &inf2);
  if (!fp12_is_one(&t))
    (*bad)++;
  pairing(&e, &g1, &g2);
  if (fp12_is_one(&e))
    (*bad)++;
  pr_pow(&t, &e, BN254_FR.w, BN_LIMBS);
  if (!fp12_is_one(&t))
    (*bad)++;
  pairing(&t, &p5, &q7);
  pr_pow(&u, &e, K35, 1u);
  if (!fp12_eq(&t, &u))
    (*bad)++;
  pairing(&t, &p5, &g2);
  pairing(&u, &g1, &q5);
  if (!fp12_eq(&t, &u))
    (*bad)++;
  pr_pow(&u, &e, K5, 1u);
  if (!fp12_eq(&t, &u))
    (*bad)++;
  pairs[0].p = g1;
  pairs[0].q = g2;
  pairs[1].p = n1;
  pairs[1].q = g2;
  if (pairing_check(pairs, 2u) != 1)
    (*bad)++;
  if (pairing_check(pairs, 1u) != 0)
    (*bad)++;
  if (pairing_check(pairs, 0u) != 1)
    (*bad)++;
}

/* The exponents, as natural numbers. With x = PAIRING_X: p and r are the
   BN polynomials 36 x^4 + 36 x^3 + 24 x^2 + 6 x + 1 and 36 x^4 + 36 x^3 +
   18 x^2 + 6 x + 1; 6 x + 2 = 2^64 + PAIRING_LOOP_LOW, and
   PAIRING_LOOP_LOW has 36 one bits. The exponent of the hard part is
   e = p^3 + (6 x^2 + 1) p^2 + p - (12 x + 18 x^2 + 36 x^3) p - (2 + 18 x +
   30 x^2 + 36 x^3): e r = p^4 - p^2 + 1, and pairing_final_hard(M) = M^e
   by the plain power (Q-S8c-7), M = the easy part of the Miller loop value
   of (G1, G2). */
static void pr_chain(unsigned *bad) {
  static const uint64_t CP[5] = {1u, 6u, 24u, 36u, 36u};
  static const uint64_t CR[5] = {1u, 6u, 18u, 36u, 36u};
  static const uint64_t CLOOP[5] = {2u, 6u, 0u, 0u, 0u};
  static const uint64_t C2[5] = {1u, 0u, 6u, 0u, 0u};
  static const uint64_t C1[5] = {0u, 12u, 18u, 36u, 0u};
  static const uint64_t C0[5] = {2u, 18u, 30u, 36u, 0u};
  Big x;
  Big p;
  Big r;
  Big p2;
  Big p3;
  Big p4;
  Big e;
  Big t;
  Big one;
  Big lhs;
  Big rhs;
  G1 g1;
  G2 g2;
  Fp12 f;
  Fp12 m;
  Fp12 hard;
  Fp12 plain;
  big_u64(&x, PAIRING_X);
  big_mod(&p, &BN254_FP);
  big_mod(&r, &BN254_FR);
  big_poly(&t, &x, CP);
  if (!big_same(&t, &p))
    (*bad)++;
  big_poly(&t, &x, CR);
  if (!big_same(&t, &r))
    (*bad)++;
  big_poly(&t, &x, CLOOP);
  big_u64(&lhs, PAIRING_LOOP_LOW);
  lhs.w[2] = 1u;
  if (!big_same(&t, &lhs))
    (*bad)++;
  if (pr_ones(PAIRING_LOOP_LOW) != 36u)
    (*bad)++;
  big_mul(&p2, &p, &p);
  big_mul(&p3, &p2, &p);
  big_mul(&p4, &p2, &p2);
  big_poly(&t, &x, C2);
  big_mul(&t, &t, &p2);
  big_add(&e, &p3, &t);
  big_add(&e, &e, &p);
  big_poly(&t, &x, C1);
  big_mul(&t, &t, &p);
  big_sub(&e, &e, &t);
  big_poly(&t, &x, C0);
  big_sub(&e, &e, &t);
  big_mul(&lhs, &e, &r);
  big_u64(&one, 1u);
  big_sub(&rhs, &p4, &p2);
  big_add(&rhs, &rhs, &one);
  if (!big_same(&lhs, &rhs))
    (*bad)++;
  fe_from_u64(&g1.x, 1u);
  fe_from_u64(&g1.y, 2u);
  g2_generator(&g2);
  pairing_miller(&f, &g1, &g2);
  pairing_final_easy(&m, &f);
  pairing_final_hard(&hard, &m);
  pr_pow(&plain, &m, e.w, big_limbs(&e));
  if (!fp12_eq(&hard, &plain))
    (*bad)++;
}

/* The bytes of EIP-197. The 384 bytes of (G1, G2), (-G1, G2) give 1; the
   first 192 give 0; a length of 191 gives -1; a length of 0 gives 1. A
   pair with the point at infinity on one side gives 1. Each of these
   inputs gives -1: a G2 point of the twist that is not in the subgroup
   (OUTSIDE), a G1 point that is not on the curve, a G2 point that is not
   on the twist, and a G1 or G2 coordinate that is p or more. */
static void pr_bytes(const G2 *outside, unsigned *bad) {
  unsigned char in[384];
  unsigned char pair[192];
  G1 g1;
  G1 n1;
  G1 inf1;
  G1 off1;
  G2 g2;
  G2 inf2;
  G2 off2;
  Fe one;
  Fe big;
  fe_from_u64(&one, 1u);
  fe_from_u64(&g1.x, 1u);
  fe_from_u64(&g1.y, 2u);
  n1 = g1;
  fe_neg(&n1.y, &g1.y, &BN254_FP);
  memset(&inf1, 0, sizeof inf1);
  memset(&inf2, 0, sizeof inf2);
  g2_generator(&g2);
  pr_pair_be(in, &g1, &g2);
  pr_pair_be(in + 192, &n1, &g2);
  if (pairing_check_be(in, 384u) != 1)
    (*bad)++;
  if (pairing_check_be(in, 192u) != 0)
    (*bad)++;
  if (pairing_check_be(in, 191u) != -1)
    (*bad)++;
  if (pairing_check_be(in, 0u) != 1)
    (*bad)++;
  pr_pair_be(pair, &inf1, &g2);
  if (pairing_check_be(pair, 192u) != 1)
    (*bad)++;
  pr_pair_be(pair, &g1, &inf2);
  if (pairing_check_be(pair, 192u) != 1)
    (*bad)++;
  pr_pair_be(pair, &g1, outside);
  if (pairing_check_be(pair, 192u) != -1)
    (*bad)++;
  off1 = g1;
  fe_from_u64(&off1.y, 3u);
  pr_pair_be(pair, &off1, &g2);
  if (pairing_check_be(pair, 192u) != -1)
    (*bad)++;
  off2 = g2;
  fe_add(&off2.y.c0, &g2.y.c0, &one, &BN254_FP);
  pr_pair_be(pair, &g1, &off2);
  if (pairing_check_be(pair, 192u) != -1)
    (*bad)++;
  pr_pair_be(pair, &g1, &g2);
  pr_plus_p(&big, &g1.x);
  fe_be(pair, &big);
  if (pairing_check_be(pair, 192u) != -1)
    (*bad)++;
  pr_pair_be(pair, &g1, &g2);
  pr_plus_p(&big, &g2.x.c1);
  fe_be(pair + 64, &big);
  if (pairing_check_be(pair, 192u) != -1)
    (*bad)++;
}

/* The value of the hex digit C (0 to 9 or a to f). */
static unsigned hex_digit(char c) {
  return c >= 'a' ? (unsigned)(c - 'a' + 10) : (unsigned)(c - '0');
}

/* The N bytes of the 2 N hex digits at HEX. */
static void hex_bytes(unsigned char *out, const char *hex, size_t n) {
  size_t i;
  for (i = 0u; i < n; i++)
    out[i] = (unsigned char)((hex_digit(hex[2u * i]) << 4) | hex_digit(hex[2u * i + 1u]));
}

/* The EIP-197 vectors of test/eip197.h: each input through
   pairing_check_be, one bad row for each wrong result. pr_bytes makes
   its inputs with pr_pair_be, thus it does not see a change of the G2
   word order in both pr_pair_be and g2_from_be: these vectors do. A
   result of -2 is an input that is larger than the buffer. */
static void pr_vectors(unsigned *bad) {
  unsigned char in[EIP197_MAX_BYTES];
  size_t i;
  for (i = 0u; i < sizeof EIP197_VECTORS / sizeof EIP197_VECTORS[0]; i++) {
    const Eip197Vector *v = &EIP197_VECTORS[i];
    size_t n = strlen(v->hex) / 2u;
    int got = -2;
    if (n <= sizeof in) {
      hex_bytes(in, v->hex, n);
      got = pairing_check_be(in, n);
    }
    if (got != v->want) {
      fprintf(stderr, "cryptotool: EIP-197 vector %s gives %d, not %d\n", v->name, got, v->want);
      (*bad)++;
    }
  }
}

static void pr_checks(unsigned *bad) {
  G2 outside;
  pr_tower(bad);
  pr_group(&outside, bad);
  pr_pairing(bad);
  pr_chain(bad);
  pr_bytes(&outside, bad);
  pr_vectors(bad);
}

/* The pairing tower rows and fixtures (S8c). pf_states gives the 3 fixture
   states: A = 1 / s, where s is the Fp12 value with the parts 1 to 12,
   B = frob(A) s, and M = A^h, where h = x >> PF_XLO for x = PAIRING_X.
   With the low PF_XLO bits l of x, A^x = M^(2^PF_XLO) A^l: the guest power
   by x is 2 rows (one row is above the arena limit of langc). */
#define PF_XLO 32u

static void pf_states(Fp12 *a, Fp12 *b, Fp12 *m) {
  Fp12 s;
  pr_sample(&s);
  fp12_inv(a, &s);
  fp12_frob(b, a);
  fp12_mul(b, b, &s);
  fp12_pow_u64(m, a, PAIRING_X >> PF_XLO);
}

/* A as 16, 48 or 96 Words: each Fe in Montgomery form, in the order of the
   fp2, fp6 and fp12 constructors of domain/domain.lang. */
static void put_f2(const Fp2 *a) {
  Fe x;
  sc_mont(&x, &a->c0, &BN254_FP);
  put_fe(&x);
  sc_mont(&x, &a->c1, &BN254_FP);
  put_fe(&x);
}

static void put_f6(const Fp6 *a) {
  put_f2(&a->c0);
  put_f2(&a->c1);
  put_f2(&a->c2);
}

static void put_f12(const Fp12 *a) {
  put_f6(&a->a);
  put_f6(&a->b);
}

/* A as the text of a guest literal. */
static void fx_f2(const Fp2 *a) {
  Fe x;
  printf(" (fp2 (fe");
  sc_mont(&x, &a->c0, &BN254_FP);
  put_fe(&x);
  printf(") (fe");
  sc_mont(&x, &a->c1, &BN254_FP);
  put_fe(&x);
  printf("))");
}

static void fx_f6(const Fp6 *a) {
  printf(" (fp6");
  fx_f2(&a->c0);
  fx_f2(&a->c1);
  fx_f2(&a->c2);
  printf(")");
}

static void fx_f12(const char *name, const Fp12 *a) {
  printf("def %s : Fp12 := fp12", name);
  fx_f6(&a->a);
  fx_f6(&a->b);
  printf("\n");
}

static void fx_g1(const char *name, const G1 *p) {
  Fe x;
  printf("def %s : Pt1 := pt1 (fe", name);
  sc_mont(&x, &p->x, &BN254_FP);
  put_fe(&x);
  printf(") (fe");
  sc_mont(&x, &p->y, &BN254_FP);
  put_fe(&x);
  printf(")\n");
}

static void fx_g2(const char *name, const G2 *q) {
  printf("def %s : Pt2 := pt2", name);
  fx_f2(&q->x);
  fx_f2(&q->y);
  printf("\n");
}

static void fx_j2(const char *name, const G2Jac *t) {
  printf("def %s : Jac2 := jac2", name);
  fx_f2(&t->x);
  fx_f2(&t->y);
  fx_f2(&t->z);
  printf("\n");
}

/* 1 when one of the N bytes at B is not 0. */
static int pf_nonzero(const unsigned char *b, size_t n) {
  size_t i;
  int any = 0;
  for (i = 0u; i < n; i++)
    any |= b[i] != 0u;
  return any;
}

/* The point fixtures (S8c G2 u3a). P and Q: pair 1 of the first EIP-197
   vector of test/eip197.h with a pair 1 that g1_from_be and g2_from_be
   accept and that has no point at infinity. QBAD: the point of the twist
   outside the subgroup of the D9 self check (pr_group): x = n + u for
   n = 1, 2, ..., the last point of the first 4 that g2_in_subgroup
   refuses. T8: T of pairing_miller after the first 8 bits of
   PAIRING_LOOP_LOW. Returns the vector, or NULL when no vector is good. */
static const Eip197Vector *pf_points(G1 *p, G2 *q, G2 *qbad, G2Jac *t8) {
  unsigned char in[192];
  const Eip197Vector *v = NULL;
  Fp2 b2;
  Fp2 rhs;
  G2 pt;
  Line l;
  size_t i;
  unsigned n;
  unsigned k;
  unsigned found = 0u;
  for (i = 0u; i < sizeof EIP197_VECTORS / sizeof EIP197_VECTORS[0] && v == NULL; i++) {
    if (strlen(EIP197_VECTORS[i].hex) >= 2u * sizeof in) {
      hex_bytes(in, EIP197_VECTORS[i].hex, sizeof in);
      if (g1_from_be(p, in) && g2_from_be(q, in + 64) && pf_nonzero(in, 64u) && !g2_is_infinity(q))
        v = &EIP197_VECTORS[i];
    }
  }
  fp2_zero(&qbad->x);
  fp2_zero(&qbad->y);
  pr_b2(&b2);
  for (n = 1u; n <= 64u && found < 4u; n++) {
    fe_from_u64(&pt.x.c0, n);
    fe_from_u64(&pt.x.c1, 1u);
    fp2_sqr(&rhs, &pt.x);
    fp2_mul(&rhs, &rhs, &pt.x);
    fp2_add(&rhs, &rhs, &b2);
    if (fp2_sqrt(&pt.y, &rhs)) {
      found++;
      if (!g2_in_subgroup(&pt))
        *qbad = pt;
    }
  }
  g2_to_jac(t8, q);
  for (k = 64u; k > 56u; k--) {
    pairing_line_double(&l, t8, p);
    if ((PAIRING_LOOP_LOW >> (k - 1u)) & 1u)
      pairing_line_add(&l, t8, q, p);
  }
  return v;
}

/* The Miller states (S8c G2 u3b): the loop of pairing_miller on P and Q,
   with the same steps. F8 = F after the first 8 bits of millerBits (T is
   then pfT8). FE and TE = F and T after all 64 bits, before the tail. */
static void pf_mstates(Fp12 *f8, Fp12 *fe, G2Jac *te, const G1 *p, const G2 *q) {
  Line l;
  unsigned k;
  fp12_one(fe);
  *f8 = *fe;
  g2_to_jac(te, q);
  for (k = 64u; k > 0u; k--) {
    fp12_sqr(fe, fe);
    pairing_line_double(&l, te, p);
    fp12_mul_line(fe, fe, &l);
    if ((PAIRING_LOOP_LOW >> (k - 1u)) & 1u) {
      pairing_line_add(&l, te, q, p);
      fp12_mul_line(fe, fe, &l);
    }
    if (k == 57u)
      *f8 = *fe;
  }
}

/* The final exponent states (S8c G2 u3c): E = pairing_final_easy(MIL),
   X1 = E^x, X2 = X1^x and X3 = X2^x, where x is PAIRING_X. */
static void pf_fstates(Fp12 *e, Fp12 *x1, Fp12 *x2, Fp12 *x3, const Fp12 *mil) {
  pairing_final_easy(e, mil);
  fp12_pow_u64(x1, e, PAIRING_X);
  fp12_pow_u64(x2, x1, PAIRING_X);
  fp12_pow_u64(x3, x2, PAIRING_X);
}

/* The [x] points (S8c G2 u3d): XQ = [x] Q and XQBAD = [x] QBAD (g2_mul
   with k = x, as in g2_in_subgroup), where x is PAIRING_X. */
static void pf_xpoints(G2 *xq, G2 *xqbad, const G2 *q, const G2 *qbad) {
  U256 x;
  u256_from_u64(&x, PAIRING_X);
  g2_mul(xq, q, &x);
  g2_mul(xqbad, qbad, &x);
}

/* `cryptotool pairing-fixtures` (S8c): the fixture block of
   test/crypto/pairing.lang, the two @PFIX@ lines too. test/gate.sh compares
   the block with this print. */
static void pf_fixtures(void) {
  Fp12 a;
  Fp12 b;
  Fp12 m;
  G1 p;
  G2 q;
  G2 qbad;
  G2Jac t8;
  Fp12 f8;
  Fp12 fe;
  G2Jac te;
  Fp12 mil;
  Fp12 e;
  Fp12 x1;
  Fp12 x2;
  Fp12 x3;
  G2 xq;
  G2 xqbad;
  const Eip197Vector *v = pf_points(&p, &q, &qbad, &t8);
  pf_states(&a, &b, &m);
  pf_mstates(&f8, &fe, &te, &p, &q);
  pairing_miller(&mil, &p, &q);
  pf_fstates(&e, &x1, &x2, &x3, &mil);
  pf_xpoints(&xq, &xqbad, &q, &qbad);
  puts("-- @PFIX@ begin");
  puts("-- The pairing fixtures (S8c). Do not edit this block: `cryptotool");
  puts("-- pairing-fixtures` prints it, and test/gate.sh compares the two.");
  puts("-- pfA = 1 / s, where s is the Fp12 value with the parts 1 to 12.");
  puts("-- pfB = frob(pfA) s.");
  printf("-- pfM = pfA^h, where h = x >> %u and x is bnXBits.\n", PF_XLO);
  fx_f12("pfA", &a);
  fx_f12("pfB", &b);
  fx_f12("pfM", &m);
  printf("-- pfP and pfQ = pair 1 of the EIP-197 vector %s.\n", v != NULL ? v->name : "(none)");
  puts("-- pfQBad = a point of the twist outside the subgroup (the D9 self check");
  puts("-- point: x = n + u, the last of the first 4 points of that form).");
  puts("-- pfT8 = T of the Miller loop after the first 8 bits of millerBits.");
  fx_g1("pfP", &p);
  fx_g2("pfQ", &q);
  fx_g2("pfQBad", &qbad);
  fx_j2("pfT8", &t8);
  puts("-- pfF8 = F of the Miller loop after the first 8 bits (T is pfT8).");
  puts("-- pfFE and pfTE = F and T of the Miller loop after the 64 bits of");
  puts("-- millerBits (before the tail). pfMil = the Miller value of pfP and pfQ.");
  fx_f12("pfF8", &f8);
  fx_f12("pfFE", &fe);
  fx_j2("pfTE", &te);
  fx_f12("pfMil", &mil);
  puts("-- pfE = the easy part of the final exponent on pfMil (pairing_final_easy).");
  puts("-- pfX1 = pfE^x, pfX2 = pfX1^x and pfX3 = pfX2^x, where x is bnXBits.");
  fx_f12("pfE", &e);
  fx_f12("pfX1", &x1);
  fx_f12("pfX2", &x2);
  fx_f12("pfX3", &x3);
  puts("-- pfXQ = [x] pfQ and pfXQBad = [x] pfQBad (g2_mul with k = x, as in");
  puts("-- g2_in_subgroup), where x is bnXBits.");
  fx_g2("pfXQ", &xq);
  fx_g2("pfXQBad", &xqbad);
  puts("-- @PFIX@ end");
}

static void pf_row2(const char *name, const Fp2 *v) {
  printf("test/crypto/pairing %s", name);
  put_f2(v);
  printf(" => 1\n");
}

static void pf_row6(const char *name, const Fp6 *v) {
  printf("test/crypto/pairing %s", name);
  put_f6(v);
  printf(" => 1\n");
}

static void pf_row12(const char *name, const Fp12 *v) {
  printf("test/crypto/pairing %s", name);
  put_f12(v);
  printf(" => 1\n");
}

/* A test row: one Word (the load gate), and the result 1 when YES. */
static void pf_rowb(const char *name, int yes) {
  printf("test/crypto/pairing %s", name);
  put_word(0u);
  printf(" => %d\n", yes != 0);
}

static void pf_rowq(const char *name, const G2 *v) {
  printf("test/crypto/pairing %s", name);
  put_f2(&v->x);
  put_f2(&v->y);
  printf(" => 1\n");
}

static void pf_rowt(const char *name, const G2Jac *v) {
  printf("test/crypto/pairing %s", name);
  put_f2(&v->x);
  put_f2(&v->y);
  put_f2(&v->z);
  printf(" => 1\n");
}

/* The line value L as the Fp12 value of fp12_mul_line:
   fp12 (fp6 c0 0 0) (fp6 c1 c3 0). */
static void pf_rowl(const char *name, const Line *l) {
  Fp12 t;
  fp6_zero(&t.a);
  fp6_zero(&t.b);
  t.a.c0 = l->c0;
  t.b.c0 = l->c1;
  t.b.c1 = l->c3;
  pf_row12(name, &t);
}

/* A Miller state row: F, then T. */
static void pf_rowm(const char *name, const Fp12 *f, const G2Jac *t) {
  printf("test/crypto/pairing %s", name);
  put_f12(f);
  put_f2(&t->x);
  put_f2(&t->y);
  put_f2(&t->z);
  printf(" => 1\n");
}

/* The point and line rows (S8c G2 u3a): each def on pfP, pfQ, pfQBad and
   pfT8, with the value of pairing.c. A test row gives 1 when the test of
   pairing.c is true. INF is the point at infinity, JI is g2_to_jac of INF
   (the value (1, 1, 0)), JQ is g2_to_jac of Q. A C row and a G row have
   the same Words. The P row: Q with x.c0 = p, through g2_from_be. */
static void pf_point_rows(void) {
  unsigned char be[128];
  G1 p;
  G2 q;
  G2 qbad;
  G2 qn;
  G2 inf;
  G2 r;
  G2Jac t8;
  G2Jac ji;
  G2Jac jq;
  G2Jac j;
  Fp2 one;
  Line l;
  const Eip197Vector *v = pf_points(&p, &q, &qbad, &t8);
  fp2_zero(&inf.x);
  fp2_zero(&inf.y);
  fp2_one(&one);
  pf_rowb("prP2OnQ", v != NULL && g2_on_curve(&q));
  qn = q;
  fp2_add(&qn.y, &qn.y, &one);
  pf_rowb("prP2OnNo", g2_on_curve(&qn));
  pf_rowb("prP2OnInf", g2_on_curve(&inf));
  memset(be, 0, sizeof be);
  if (v != NULL)
    hex_bytes(be, v->hex + 128, sizeof be);
  /* p = FP_LESS_ONE + 1: the last byte of p - 1 is 0x46, thus no carry. */
  hex_bytes(be + 32, FP_LESS_ONE, 32u);
  be[63]++;
  pf_rowb("prP2OnP", g2_from_be(&r, be));
  g2_neg(&r, &q);
  pf_rowq("prP2Neg", &r);
  g2_psi(&r, &q);
  pf_rowq("prP2Psi", &r);
  g2_to_jac(&jq, &q);
  pf_rowt("prP2ToJac", &jq);
  g2_to_jac(&ji, &inf);
  pf_rowt("prP2ToJacInf", &ji);
  g2_jac_double(&j, &t8);
  pf_rowt("prJ2DblC", &j);
  pf_rowt("prJ2DblG", &j);
  g2_jac_double(&j, &ji);
  pf_rowt("prJ2DblInf", &j);
  g2_jac_add_affine(&j, &t8, &q);
  pf_rowt("prJ2AddGen", &j);
  g2_jac_add_affine(&j, &t8, &inf);
  pf_rowt("prJ2AddQInf", &j);
  g2_jac_add_affine(&j, &ji, &q);
  pf_rowt("prJ2AddTInf", &j);
  g2_jac_add_affine(&j, &jq, &q);
  pf_rowt("prJ2AddSame", &j);
  g2_neg(&qn, &q);
  g2_jac_add_affine(&j, &jq, &qn);
  pf_rowt("prJ2AddOpp", &j);
  g2_jac_affine(&r, &t8);
  pf_rowq("prJ2ToAff", &r);
  g2_add(&r, &q, &qbad);
  pf_rowq("prP2Add", &r);
  j = t8;
  pairing_line_double(&l, &j, &p);
  pf_rowl("prLineDblC", &l);
  pf_rowl("prLineDblG", &l);
  j = t8;
  pairing_line_add(&l, &j, &q, &p);
  pf_rowl("prLineAdd", &l);
}

/* The Miller rows (S8c G2 u3b): milDbl and milAdd on (pfF8, pfT8), milBits
   on the first 8 bits from (1, JQ) to (pfF8, pfT8), milTail on (pfFE, pfTE)
   to pfMil (the value of pairing_miller), and miller with P or Q at
   infinity (the value 1). */
static void pf_miller_rows(void) {
  G1 p;
  G1 pinf;
  G2 q;
  G2 qbad;
  G2 qinf;
  G2Jac t8;
  G2Jac te;
  G2Jac t;
  Fp12 f8;
  Fp12 fe;
  Fp12 f;
  Line l;
  (void)pf_points(&p, &q, &qbad, &t8);
  pf_mstates(&f8, &fe, &te, &p, &q);
  f = f8;
  t = t8;
  fp12_sqr(&f, &f);
  pairing_line_double(&l, &t, &p);
  fp12_mul_line(&f, &f, &l);
  pf_rowm("prMilDbl", &f, &t);
  f = f8;
  t = t8;
  pairing_line_add(&l, &t, &q, &p);
  fp12_mul_line(&f, &f, &l);
  pf_rowm("prMilAdd", &f, &t);
  pf_rowm("prMilBits", &f8, &t8);
  pairing_miller(&f, &p, &q);
  pf_row12("prMilTail", &f);
  fe_from_u64(&pinf.x, 0u);
  fe_from_u64(&pinf.y, 0u);
  fp2_zero(&qinf.x);
  fp2_zero(&qinf.y);
  pairing_miller(&f, &pinf, &q);
  pf_row12("prMillerPInf", &f);
  pairing_miller(&f, &p, &qinf);
  pf_row12("prMillerQInf", &f);
}

/* The final exponent rows (S8c G2 u3c): finEasy on pfMil to pfE (the value
   of pairing_final_easy), and finHardBy on (pfE, pfX1, pfX2, pfX3) to the
   value of pairing_final_hard on pfE. */
static void pf_final_rows(void) {
  G1 p;
  G2 q;
  G2 qbad;
  G2Jac t8;
  Fp12 mil;
  Fp12 e;
  Fp12 x1;
  Fp12 x2;
  Fp12 x3;
  Fp12 f;
  (void)pf_points(&p, &q, &qbad, &t8);
  pairing_miller(&mil, &p, &q);
  pf_fstates(&e, &x1, &x2, &x3, &mil);
  pf_row12("prFinEasy", &e);
  pairing_final_hard(&f, &e);
  pf_row12("prFinHardBy", &f);
}

/* The input check rows (S8c G2 u3d): p2MulX on Q to XQ, the subgroup test
   on (Q, XQ) and on (QBAD, XQBAD), g2_in_subgroup on Q, the input check (P
   on the curve, Q in the subgroup) on an empty list, on POFF and on QBAD,
   the Miller product on an empty list and on 2 pairs at infinity, and
   pairing_check after the input check on POFF and on QBAD. POFF is the
   point (1, 1), which is not on the curve. A test row gives 1 when the
   test of pairing.c is true. */
static void pf_check_rows(void) {
  G1 p;
  G1 poff;
  G1 pinf;
  G2 q;
  G2 qbad;
  G2 qinf;
  G2 xq;
  G2 xqbad;
  G2Jac t8;
  Fp12 f;
  Fp12 m;
  Pair pr;
  (void)pf_points(&p, &q, &qbad, &t8);
  pf_xpoints(&xq, &xqbad, &q, &qbad);
  fe_from_u64(&poff.x, 1u);
  fe_from_u64(&poff.y, 1u);
  fe_from_u64(&pinf.x, 0u);
  fe_from_u64(&pinf.y, 0u);
  fp2_zero(&qinf.x);
  fp2_zero(&qinf.y);
  pf_rowq("prP2MulX", &xq);
  pf_rowb("prP2InSubByYes", g2_in_subgroup(&q));
  pf_rowb("prP2InSubByNo", g2_in_subgroup(&qbad));
  pf_rowb("prP2InSub", g2_in_subgroup(&q));
  pf_rowb("prPairOkEnd", 1);
  pf_rowb("prPairOkPOff", g1_on_curve(&poff) && g2_in_subgroup(&q));
  pf_rowb("prPairOkQBad", g1_on_curve(&p) && g2_in_subgroup(&qbad));
  fp12_one(&f);
  pf_row12("prMilProdEnd", &f);
  pairing_miller(&m, &pinf, &q);
  fp12_mul(&f, &f, &m);
  pairing_miller(&m, &p, &qinf);
  fp12_mul(&f, &f, &m);
  pf_row12("prMilProdInf", &f);
  pr.p = poff;
  pr.q = q;
  pf_rowb("prPairChkPOff", g1_on_curve(&poff) && g2_in_subgroup(&q) && pairing_check(&pr, 1u) == 1);
  pr.p = p;
  pr.q = qbad;
  pf_rowb("prPairChkQBad", g1_on_curve(&p) && g2_in_subgroup(&qbad) && pairing_check(&pr, 1u) == 1);
}

/* The EIP-197 rows of pairing.lang: prPccN on N pairs. Each pair is P x
   and y, then Q x and y (put_pt and put_f2, Montgomery form). A row with no
   pair has 1 Word (the load gate). The result is 1 when YES. */
static void pc_row(const G1 *p, const G2 *q, unsigned n, int yes) {
  unsigned i;
  printf("test/crypto/pairing prPcc%u", n);
  if (n == 0u)
    put_word(0u);
  for (i = 0u; i < n; i++) {
    put_pt(&p[i]);
    put_f2(&q[i].x);
    put_f2(&q[i].y);
  }
  printf(" => %d\n", yes != 0);
}

/* The check of pairing_check_be on N points (2 at most): each P on the
   curve, each Q in the subgroup, and the product of the pairings is 1. */
static int pc_check(const G1 *p, const G2 *q, unsigned n) {
  Pair pr[2];
  unsigned i;
  int ok = n <= 2u;
  for (i = 0u; i < n && ok; i++) {
    ok = g1_on_curve(&p[i]) && g2_in_subgroup(&q[i]);
    pr[i].p = p[i];
    pr[i].q = q[i];
  }
  return ok && pairing_check(pr, n) == 1;
}

/* The row of LEN bytes IN in the EIP-197 encoding. When the decode refuses
   a pair, the row is prPcc0 with the result 0: a mismatch in the gate. */
static void pc_row_be(const unsigned char *in, size_t len) {
  G1 p[10];
  G2 q[10];
  unsigned n = (unsigned)(len / 192u);
  unsigned i;
  int ok = len % 192u == 0u && n <= 10u;
  for (i = 0u; i < n && ok; i++)
    ok = g1_from_be(&p[i], in + 192u * i) && g2_from_be(&q[i], in + 192u * i + 64u);
  pc_row(p, q, ok ? n : 0u, ok && pairing_check_be(in, len) == 1);
}

/* The field element of the 32 bytes BE (the decode of g2_from_be). 0 when
   the value is p or more. */
static int pc_fe_be(Fe *out, const unsigned char *be) {
  U256 w;
  u256_from_be(&w, be);
  return fe_from_u256(out, &w, &BN254_FP);
}

/* P and Q of the 192 bytes IN in the EIP-197 order, with no check of Q:
   g2_from_be refuses a Q off the twist, this decode keeps it. 0 when a
   coordinate is p or more, or P is not on the curve. */
static int pc_raw_be(G1 *p, G2 *q, const unsigned char *in) {
  return g1_from_be(p, in) && pc_fe_be(&q->x.c1, in + 64u) && pc_fe_be(&q->x.c0, in + 96u) &&
         pc_fe_be(&q->y.c1, in + 128u) && pc_fe_be(&q->y.c0, in + 160u);
}

/* The prPcc rows: each vector of test/eip197.h; 3 bad inputs (pfP with
   pfQBad, P = (1, 1) off the curve with pfQ, and jeff1 pair 1 with the last
   byte of Q y changed, thus Q off the twist: pc_raw_be keeps that Q, thus
   the guest gets it and must give 0); bilinearity: (5G, 7H) with (-35G, H)
   gives 1, (5G, 7H) with (-34G, H) gives 0. */
static void pc_rows(void) {
  unsigned char in[EIP197_MAX_BYTES];
  const Eip197Vector *v;
  G1 p[2];
  G2 q[2];
  G1 g1;
  G1 a;
  G2 g2;
  G2 qbad;
  G1 pt;
  G2 qt;
  G2Jac t8;
  U256 k;
  size_t len;
  size_t i;
  int ok;
  for (i = 0u; i < sizeof EIP197_VECTORS / sizeof EIP197_VECTORS[0]; i++) {
    len = strlen(EIP197_VECTORS[i].hex) / 2u;
    len = len <= sizeof in ? len : 1u;
    hex_bytes(in, EIP197_VECTORS[i].hex, len);
    pc_row_be(in, len);
  }
  v = pf_points(&p[0], &q[0], &qbad, &t8);
  pc_row(p, &qbad, 1u, pc_check(p, &qbad, 1u));
  fe_from_u64(&a.x, 1u);
  fe_from_u64(&a.y, 1u);
  pc_row(&a, q, 1u, pc_check(&a, q, 1u));
  len = v == NULL ? 1u : 192u;
  hex_bytes(in, v == NULL ? "00" : v->hex, len);
  in[191] ^= 1u;
  ok = len == 192u && pc_raw_be(&pt, &qt, in);
  pc_row(&pt, &qt, ok ? 1u : 0u, ok && pairing_check_be(in, len) == 1);
  fe_from_u64(&g1.x, 1u);
  fe_from_u64(&g1.y, 2u);
  g2_generator(&g2);
  u256_from_u64(&k, 5u);
  g1_mul(&p[0], &g1, &k);
  u256_from_u64(&k, 7u);
  g2_mul(&q[0], &g2, &k);
  q[1] = g2;
  u256_from_u64(&k, 35u);
  g1_mul(&a, &g1, &k);
  p[1] = a;
  fe_neg(&p[1].y, &a.y, &BN254_FP);
  pc_row(p, q, 2u, pc_check(p, q, 2u));
  u256_from_u64(&k, 34u);
  g1_mul(&a, &g1, &k);
  p[1] = a;
  fe_neg(&p[1].y, &a.y, &BN254_FP);
  pc_row(p, q, 2u, pc_check(p, q, 2u));
}

/* The pairing.lang rows: each def of the tower on the fixture states, with
   the value of pairing.c. The Fp2 states are part c0 of A.a and part c1 of
   B.a. The Fp6 states are A.a and B.b. A C row and a G row have the same
   Words: the def uses the native product or the guest product. A Ne row has
   a value that is different from the first state in one part only. */
static void pf_rows(void) {
  Fp12 a;
  Fp12 b;
  Fp12 t;
  Fp12 m;
  Fp6 t6;
  Fp2 t2;
  const Fp2 *a2;
  const Fp2 *b2;
  const Fp6 *a6;
  const Fp6 *b6;
  pf_states(&a, &b, &m);
  a2 = &a.a.c0;
  b2 = &b.a.c1;
  a6 = &a.a;
  b6 = &b.b;
  fp2_add(&t2, a2, b2);
  pf_row2("prF2Add", &t2);
  fp2_sub(&t2, a2, b2);
  pf_row2("prF2Sub", &t2);
  fp2_neg(&t2, a2);
  pf_row2("prF2Neg", &t2);
  fp2_conj(&t2, a2);
  pf_row2("prF2Conj", &t2);
  pf_row2("prF2Ne", &t2);
  fp2_neg(&t2, &t2);
  pf_row2("prF2Ne", &t2);
  fp2_mul(&t2, a2, b2);
  pf_row2("prF2MulC", &t2);
  pf_row2("prF2MulG", &t2);
  fp2_sqr(&t2, a2);
  pf_row2("prF2Sqr", &t2);
  fp2_mul_xi(&t2, a2);
  pf_row2("prF2MulXi", &t2);
  fp2_inv(&t2, a2);
  pf_row2("prF2InvC", &t2);
  pf_row2("prF2InvG", &t2);
  fp6_add(&t6, a6, b6);
  pf_row6("prF6Add", &t6);
  fp6_sub(&t6, a6, b6);
  pf_row6("prF6Sub", &t6);
  fp6_neg(&t6, a6);
  pf_row6("prF6Neg", &t6);
  fp6_mul(&t6, a6, b6);
  pf_row6("prF6MulC", &t6);
  pf_row6("prF6MulG", &t6);
  fp6_mul_v(&t6, a6);
  pf_row6("prF6MulV", &t6);
  fp6_inv(&t6, a6);
  pf_row6("prF6InvC", &t6);
  pf_row6("prF6InvG", &t6);
  t6 = *a6;
  fp2_neg(&t6.c0, &a6->c0);
  pf_row6("prF6Ne", &t6);
  t6 = *a6;
  fp2_neg(&t6.c1, &a6->c1);
  pf_row6("prF6Ne", &t6);
  t6 = *a6;
  fp2_neg(&t6.c2, &a6->c2);
  pf_row6("prF6Ne", &t6);
  fp12_mul(&t, &a, &b);
  pf_row12("prF12MulC", &t);
  pf_row12("prF12MulG", &t);
  fp12_sqr(&t, &a);
  pf_row12("prF12Sqr", &t);
  fp12_conj(&t, &a);
  pf_row12("prF12Conj", &t);
  pf_row12("prF12Ne", &t);
  t = a;
  fp6_neg(&t.a, &a.a);
  pf_row12("prF12Ne", &t);
  fp12_inv(&t, &a);
  pf_row12("prF12InvC", &t);
  pf_row12("prF12InvG", &t);
  fp12_frob(&t, &a);
  pf_row12("prF12Frob", &t);
  fp12_pow_u64(&t, &a, 181u);
  pf_row12("prF12PowS", &t);
  /* The power by x in 2 rows (see pf_states): prF12PowXa = A^h = M, and
     prF12PowXb = A^x from M and the low bits. */
  pf_row12("prF12PowXa", &m);
  fp12_pow_u64(&t, &a, PAIRING_X);
  pf_row12("prF12PowXb", &t);
  pf_point_rows();
  pf_miller_rows();
  pf_final_rows();
  pf_check_rows();
  pc_rows();
}

int main(int argc, char **argv) {
  Rng rng = {0x5338676176656c31ULL};
  const Field fr = {"fr", &BN254_FR};
  const Field fp = {"fp", &BN254_FP};
  unsigned bad = 0u;
  if (argc == 2 && strcmp(argv[1], "fixtures") == 0) {
    fixtures();
    return 0;
  }
  if (argc == 2 && strcmp(argv[1], "pairing-fixtures") == 0) {
    pf_fixtures();
    return 0;
  }
  if (argc != 1) {
    fputs("usage: cryptotool [fixtures | pairing-fixtures]\n", stderr);
    return 2;
  }
  word_rows(&rng);
  field_rows(&fr, FR_LESS_ONE, FR_LESS_TWO, &rng, &bad);
  field_rows(&fp, FP_LESS_ONE, FP_LESS_TWO, &rng, &bad);
  mulc_rows(&fr, FR_LESS_ONE, &rng, &bad);
  mulc_rows(&fp, FP_LESS_ONE, &rng, &bad);
  pt_rows(FR_LESS_ONE, FP_LESS_TWO, &bad);
  kc_rows(&rng, &bad);
  ps_rows(&rng, &bad);
  pr_checks(&bad);
  sc_rows();
  pf_rows();
  if (bad != 0u) {
    fprintf(stderr, "cryptotool: %u rows of the C oracle are wrong\n", bad);
    return 1;
  }
  return 0;
}
