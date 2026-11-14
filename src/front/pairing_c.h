/* S8c (Q-S8c-2 (a)): the pairing table of pairingCheckC in
   src/front/eval.c. `build/pairinggen c` prints this file, and
   test/gate.sh compares the two: do not edit it by hand. Each Fe
   value is 8 Words, limb 0 is the low Word, in Montgomery form
   (x R mod p, R = 2^256). pr_c_frob holds frobG1 to frobG5 and
   pr_c_twist_b holds twistB of domain/domain.lang, each Fp2 as c0,
   then c1. pr_c_miller holds millerBits and pr_c_x holds bnXBits,
   the top bit first. */
#ifndef LANG_FRONT_PAIRING_C_H
#define LANG_FRONT_PAIRING_C_H

#include <stdint.h>

#define PR_C_MILLER_N 64u
#define PR_C_X_N 63u

static const uint32_t pr_c_frob[5][2][8] = {
  {{0x33144907u, 0xaf9ba696u, 0x87afb78au, 0xca6b1d73u, 0xf08a2087u, 0x11bded5eu, 0x1a1f3a7cu, 0x02f34d75u}, {0x4c492d72u, 0xa222ae23u, 0x565de15bu, 0xd00f02a4u, 0x53dfc926u, 0xdc2ff3a2u, 0xb3899551u, 0x10a75716u}},
  {{0x4563ab30u, 0xb5773b10u, 0xa9aa6454u, 0x347f91c8u, 0x242e0991u, 0x7a007127u, 0x118214ecu, 0x1956bcd8u}, {0xa0aa4757u, 0x6e849f1eu, 0x89f89141u, 0xaa1c7b6du, 0xfae0ca3au, 0xb6e713cdu, 0x4e82ebc3u, 0x26694fbbu}},
  {{0x2936b629u, 0xe4bbdd0cu, 0xe133bacbu, 0xbb30f162u, 0xf9645366u, 0x31a9d1b6u, 0xa500f8ddu, 0x253570beu}, {0x5ffe77c7u, 0xa1d77ce4u, 0x7826d1dbu, 0x07affd11u, 0xbb7edc6bu, 0x6d16bd27u, 0x85defeccu, 0x2c872002u}},
  {{0x843abe92u, 0x7361d77fu, 0x273411fbu, 0xa5bb2bd3u, 0x4b3e2399u, 0x9c941f31u, 0xbb9fd3ecu, 0x15df9cddu}, {0x4bd8c949u, 0x5dddfd15u, 0xa4445b60u, 0x62cb29a5u, 0x0c7dd2b9u, 0x37bc870au, 0x3171f0fdu, 0x24830a9du}},
  {{0x41690fe7u, 0xc970692fu, 0x27694b0bu, 0xe2403421u, 0x83c459e8u, 0x32bee66bu, 0x0ab08841u, 0x12aabcedu}, {0x40aebfa9u, 0x0d485d23u, 0xab2fcc57u, 0x05193418u, 0x8a4910f5u, 0xd3b0a40bu, 0x35d2925au, 0x2f21ebb5u}}
};
static const uint32_t pr_c_twist_b[2][8] = {{0x77b802a8u, 0x3bf938e3u, 0x3633535du, 0x020b1b27u, 0x49755260u, 0x26b7edf0u, 0x4384a86du, 0x2514c632u}, {0xd1dcff67u, 0x38e7ecccu, 0x93ce0d3eu, 0x65f0b37du, 0x22ac00aau, 0xd749d0ddu, 0x4a688d4du, 0x0141b9ceu}};
static const uint8_t pr_c_miller[64] = {1, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0};
static const uint8_t pr_c_x[63] = {1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1};

#endif
