/* The C oracle rows (S8b, Q-S8b-7 (a)). Each line is
   `FILE NAME ARGS => RESULT`. FILE is the path of a kit file without `.lang`:
   examples/crypto for the Word ops, test/crypto/crypto for the field ops on
   Fr and Fp and for ptMulC on G1 (S8b2), test/crypto/keccak for keccak256
   (kcBlock, kcHash, kcChal; S8b2, Q-S8b2-14 (a)), test/crypto/poseidon for
   Poseidon (psPermC, psHashC, psHashG; S8b2, Q-S8b2-18 (a)) and for poseidon2C
   (psHashN, psHashD; S8b2, Q-S8b2-30 (a)), test/crypto/schnorr for the
   Schnorr check and the message words (scYes, scNo, scMsgFill, scMsgAmend,
   scMsgWd, scMsgIntent; S8b2, Q-S8b2-23 (a)). NAME is a def of FILE.
   `cryptotool fixtures` prints the fixture block of domain/domain.lang
   (S8b2, Q-S8b2-4 (a)).
   ARGS and RESULT are Word literals (`0x%08xw`) or a Nat. A field row gives
   the inputs and then the value of
   the C oracle, each as 8 Words, least significant first. The def gives 1
   when the field op of the domain file gives the same value. test/gate.sh
   evaluates each line with `langc --kit eval`. The inputs come from a fixed
   seed and from known answers. The field values come from src/crypto/bn254.c
   (quorum-lang 8d1a113, canonical form), the digests from
   src/crypto/keccak.c and src/crypto/poseidon.c. Exit 1 when the C oracle
   does not give a known answer. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "crypto/bn254.h"
#include "crypto/keccak.h"
#include "crypto/poseidon.h"

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

int main(int argc, char **argv) {
  Rng rng = {0x5338676176656c31ULL};
  const Field fr = {"fr", &BN254_FR};
  const Field fp = {"fp", &BN254_FP};
  unsigned bad = 0u;
  if (argc == 2 && strcmp(argv[1], "fixtures") == 0) {
    fixtures();
    return 0;
  }
  if (argc != 1) {
    fputs("usage: cryptotool [fixtures]\n", stderr);
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
  sc_rows();
  if (bad != 0u) {
    fprintf(stderr, "cryptotool: %u rows of the C oracle are wrong\n", bad);
    return 1;
  }
  return 0;
}
