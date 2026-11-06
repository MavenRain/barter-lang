# barter-lang plan

barter-lang is a Cubical TT language for one job: a private intent swap
off chain, under an off-chain threshold authority that amends itself. An
owner signs an intent with a sealed input amount and a sealed floor. The
floor decays over a window of log heights. A solver gives a sealed quote.
FHE or MPC computes the crossing check on sealed values. A t-of-n
committee certifies the fill into the log, and only the settled fill is
opened. A t-of-n amendment changes the swap parameters, and the amend rule
in force judges the new rule. The checker proves the law facts before any
intent. A host replays the log and checks the certificates. No program can
have a runtime error.

The design is `design/DESIGN.md`: the role map, the base types, the
operations, the governance layer, the laws, the four C programs and the
request, verbatim. The swap design is `design/intent-swap-design.md`,
verbatim. This file gives the readings of the request, the route, the
slices and the open questions. "design sec N" is section N of
`design/DESIGN.md`. Section 1.2 lists the questions for the USER. A
reading that is not ruled stays a recommendation.

Status: S0 (seed): the tcc-wasm kit and this plan. Next: S1 (the USER
rules section 1.2).

## 1. Rulings and questions

### 1.1 Rulings

| Date | Ruling | Source |
|---|---|---|
| 2026-11-06 | Name: barter-lang. Repo MavenRain/barter-lang, private at creation. The USER makes it public. Not chosen: pact-lang, accord-lang. | USER, S0 |
| 2026-11-06 | T1 (primary): a Wasm module and a TinyCC C host `barterd`, from the lang-template `hosts/tcc-wasm` kit (HEAD 5265a2a). Swap matching and settlement, the amend rule and the log replay compile to one Wasm module. The ZK, FHE and MPC crypto is behind a fixed import ABI. Differential tests under node and wasmtime. | USER, S0 |
| 2026-11-06 | Extra targets (later slices): T2 ZK relation export (intent validity, fill correctness, ballot eligibility); T3 FHE circuit (the sealed crossing check and the amendment tally; TFHE, static depth bound, CKKS refused); T4 MPC circuit (Bristol Fashion, intent matching and the amendment tally); T5 JSON report (swap facts, governance facts, law counts, per-height verdicts). | USER, S0 |
| 2026-11-06 | Not chosen: native C through TinyCC (a new host kit), a JS module (tcc-js), a ZK circuit as the primary target. | USER, S0 |
| 2026-11-06 | x402 route: `langc serve`, an HTTP server in C that tcc builds. It links libcurl for the facilitator verify and settle calls and runs behind a TLS proxy. Same tools as `langc mcp`, plus MCP streamable HTTP at `/mcp`. The repo ships `server.json`, `openapi.yaml`, `llms.txt` and `funding.json`. Not chosen: a Cloudflare Worker, files only in v1. | USER, S0 |
| 2026-11-06 | Host: TinyCC. `langc`, `langc mcp`, `langc serve` and `barterd` are C99 that TinyCC builds. | USER, S0 |
| 2026-11-06 | Each deploy, each registry submission, each AgentFund campaign and the payTo wallet are USER steps. The repo holds no wallet address that Claude made up. | USER, S0 |

The seed is `bin/new-lang.sh barter-lang tcc-wasm` from lang-template
5265a2a. The kit gives the parser, the checker, the evaluator, the lowering
and the Wasm writer. No kit emits a C host, a ZK relation, an FHE circuit,
a Bristol circuit, a JSON report, an MCP server or an HTTP server, so
these are new code.

Carried default, not ruled for barter-lang: proofs are guest-language terms
only (quorum-lang ruling, 2026-10-25). Lean export is not a target.

### 1.2 Questions for the USER (S0)

Option (a) is the recommendation. Where a gavel-lang S1 ruling answers the
same question, option (a) is that ruling, cited as `gavel d71bd96:<line>`
(gavel-lang `docs/PLAN.md` at d71bd96). Where barter-lang differs, the
item says why.

- Q-S0-1 R0 failures (section 2). (a) A failed check is a value: `Verdict A
  = accept A | reject Reason`, with the reasons of section 2 (gavel
  d71bd96:36). (b) One reason only, `reject`, with no code.
- Q-S0-2 Amount arithmetic (section 2). (a) Port the gavel-lang S3 total
  arithmetic (gavel 86c2f30 "make arithmetic total") restricted to compare,
  select, add and multiply by a public scalar, each with a static bound
  obligation. This is enough for the affine test of design sec 4.1. (b)
  Compare and select only, as gavel ruled (gavel d71bd96:35), with decay as
  a step table in `Params`. barter-lang differs from gavel: a decay floor
  needs a product of a height and an amount.
- Q-S0-3 R1 formers (section 3). (a) The gavel section 3 classes (gavel
  d71bd96:37). `Enc` is the identity cell monad with a sealed eliminator;
  `open` is a law-only monad morphism `Enc -> Id` (intent-lang 67445ae
  `docs/PLAN.md` lines 35 and 282 to 289). (b) `Sealed` is an opaque base
  type with no former.
- Q-S0-4 Role map. (a) Design sec 2 (same shape as gavel d71bd96:38). (b)
  One sequencer orders the log; the committee certifies fills only.
- Q-S0-5 Base types. (a) The list of design sec 3. (b) The same list
  without the sealed decoy index (no decoy bundle in v1; OI-6).
- Q-S0-6 The role of `ZKProof`. (a) A T2 relation: intent well-formedness
  and ownership, and correct evaluation of the crossing check (gavel
  d71bd96:43 ruled a T2 proof of the outcome). (b) t attestations of
  correct evaluation, no proof.
- Q-S0-7 Decay. (a) The intent-lang Hedberg reading: a path in a set is
  trivial, so decay is a function of height given by the cross-multiplied
  test of design sec 4.1 (intent-lang 67445ae `docs/PLAN.md` lines 306 to
  325). (b) A step table in `Params`.
- Q-S0-8 Time (section 5). (a) Progress is log height. No wall clock
  (ratify-lang and gavel-lang). (b) Log height plus a certified wall-clock
  stamp in each entry.
- Q-S0-9 Self-amendment (design sec 5). (a) The rule is data in `Params`
  in a closed form; `amend` reads the rule that it can change; a new rule
  applies from h + 1; law G1 is the orbit-lang fixed point; one static
  floor t >= t_min in the type of `Params` (gavel d71bd96:40 ruled
  `Params` with t-of-n, a rule that amends itself, order by height). (b)
  The same with no floor. (c) An amendment can carry new barter-lang code
  (code as data). Refused in v1: it needs a checked compiler in `barterd`.
- Q-S0-10 Fee. (a) No fee in v1. (b) A fee in basis points in `Params`.
- Q-S0-11 Price list of `langc serve`. (a) A deploy setting, not in
  `Params` (gavel d71bd96:41). (b) Governed in `Params`.
- Q-S0-12 Ballots. (a) ZK eligibility and an FHE or MPC tally, as in
  ratify-lang and gavel-lang. (b) Signed ballots, t-of-n signatures only,
  no ballot secrecy.
- Q-S0-13 Hash. (a) An abstract `Hash`; Poseidon at T2 and in the crypto
  code (gavel d71bd96:42). (b) Poseidon fixed in the language.
- Q-S0-14 T2 format. (a) Noir (gavel d71bd96:44). (b) arkworks R1CS.
- Q-S0-15 T3 scheme. (a) TFHE (gavel d71bd96:45). (b) BFV. CKKS is refused
  in both: approximate arithmetic is a runtime error class.
- Q-S0-16 T4 protocol. (a) Pin one protocol: the authenticated garbling of
  Wang, Ranellucci and Katz (CCS 2017), dishonest majority, malicious
  (gavel d71bd96:46 and 63). (b) The MPC protocol and its corruption model
  are EXTERNAL (law bucket); `langc` does not check them.
- Q-S0-17 Crypto location. (a) In `barterd` behind a fixed host-import ABI,
  as the T1 ruling says (section 1.1), a port of the ratify-lang ABI and
  quorum-lang `src/crypto`. (b) In the module, as a guest-language prelude
  over a total `Word` type (gavel d71bd96:47, 65 and 69). barter-lang
  differs from gavel: the barter T1 ruling keeps the ABI clause, and (b)
  removes it.
- Q-S0-18 Kernel source for S2. (a) The ratify-lang kernel (gavel
  d71bd96:48). (b) The gavel-lang kernel at 86c2f30 (it has total
  arithmetic).
- Q-S0-19 How `barterd` runs the module. (a) Its own C99 Wasm interpreter;
  node and wasmtime are oracles in the gate (gavel d71bd96:49). (b) A
  wasmtime child process.
- Q-S0-20 `langc serve` routes. (a) JSON POST routes for the five tools and
  MCP streamable HTTP at `/mcp`, each behind x402 (gavel d71bd96:50). (b)
  `/mcp` only.
- Q-S0-21 Price. (a) Tiered: $0.01 for check, fmt, explain, list-base and
  show-law; $0.05 for build (T1 module); $0.20 for the T2 to T4 exports
  (gavel d71bd96:51). (b) A flat $0.01.
- Q-S0-22 Network. (a) USDC on Base mainnet; base-sepolia in tests (gavel
  d71bd96:52). (b) base-sepolia only in v1.
- Q-S0-23 `server.json`. (a) A `packages` entry (the stdio `langc mcp`
  bundle) and a `remotes` entry (the `/mcp` URL after the USER deploys),
  name `io.github.MavenRain/barter-lang` (gavel d71bd96:53). (b) `packages`
  only.
- Q-S0-24 `openapi.yaml`. (a) OpenAPI 3.1; each route lists its 402
  response with the x402 body schema and the price (gavel d71bd96:54). (b)
  OpenAPI 3.0.
- Q-S0-25 Funding files. (a) `.github/FUNDING.yml`, `funding.json`,
  `llms.txt`; no `.well-known/ai-donate.json` (gavel d71bd96:55). (b) Also
  `.well-known/ai-donate.json`.
- Q-S0-26 Measurement. (a) `langc serve` writes a local count log; nothing
  leaves the host (gavel d71bd96:56). (b) No count log.
- Q-S0-27 AgentFund. (a) This plan lists candidate milestones only; the
  USER runs any campaign (gavel d71bd96:57). (b) No milestones in the plan.
- Q-S0-28 Agent framing. (a) The README gets the section "What an agent
  gets" in ROI terms (gavel d71bd96:58). (b) No such section.
- Q-S0-29 x402 test network. (a) A local C stub facilitator, and also a
  public test-network facilitator row in the gate; the gate fails when the
  network or a test input is absent (gavel d71bd96:59, 67 and 68). (b) The
  local stub facilitator only.
- Q-S0-30 Token comparison. (a) The permit-lang ruling: count_tokens on
  programs with equal outputs (gavel d71bd96:60). (b) A word count.
- Q-S0-31 Law buckets. (a) The four buckets of section 6;
  `laws_discharged` counts DISCHARGED rows only (gavel d71bd96:61). (b)
  Three buckets (AT TARGET joins ASSUMED).
- Q-S0-32 Refusal codes. (a) The twelve codes of section 7 (gavel
  d71bd96:62 ruled its section 7 codes). (b) One generic code with a text.
- Q-S0-33 The kit row "a trap is equal to a trap". (a) S3 changes it: R0
  makes a trap a failure on each target (permit-lang f9d8a15 `docs/PLAN.md`
  lines 188 to 201; gavel 310e708 `docs/PLAN.md` lines 84 to 86). (b) Keep
  the row until S9 (T1).

## 2. The reading of R0: no runtime error

R0 says that langc accepts no program that can lead to a runtime error.
The reading ports permit-lang, ratify-lang and gavel-lang: every function is
total, and every failure in the domain is a value.

- A failed proof check, a bad decryption share, an expired intent, a reused
  nullifier, a wrong token pair, a quote below the floor, a bad certificate
  and an amendment below t_min are each a `reject Reason` of `Verdict`, or
  `Unfilled` of `Outcome`. None is a trap.
- Reasons (draft): `bad_proof`, `bad_share`, `expired`, `reused`,
  `bad_pair`, `below_floor`, `bad_cert`, `below_t_min`.
- Arithmetic on `Amount` and `Height` is total (Q-S0-2). No division and no
  subtraction. Each add and each product by a public scalar has a static
  bound obligation that the checker discharges (OI-7).
- Host input that does not decode is a `reject`, never a trap (permit-lang
  `test/notraptool.c` precedent).
- The kit row "a trap is equal to a trap" changes (Q-S0-33, OI-4).

## 3. The reading of R1

R1 admits only cell monads and their duals, as Cubical TT represents them.
S4 ports the gavel-lang section 3 table (gavel 310e708 `docs/PLAN.md`
lines 88 to 119) where the words are the same (Q-S0-3).

- `Enc` (the design's `Enc A`) is the identity cell monad with a sealed
  eliminator. Its carrier is `Sealed A`. A user program cannot eliminate
  it.
- `open` is a law-only monad morphism `Enc -> Id`. Its meaning is the
  design's `decrypt`, and law S6 states that it is a homomorphism. Only
  `settle` and the tally call it, after a certificate.
- `Verdict` and `Outcome` are sums, not formers.
- A former outside the R1 table gives `REFUSE_FORMER`.

## 4. The minimal base signature (R2)

R2 admits what the swap and its governance need: the base types of design
sec 3 and the operations of design sec 4 (Q-S0-5). The role map is design
sec 2 (Q-S0-4). A base type outside that list gives `REFUSE_BASE`.

## 5. Time

Progress is log height (Q-S0-8). A decay `Window` starts at a height and
has a length in heights. An intent expires at its deadline height. There
is no wall clock; a program that names one gives `REFUSE_CLOCK`.

## 6. Laws and obligations

The laws are design sec 6. Buckets (Q-S0-31):

| Bucket | Laws | Count |
|---|---|---|
| DISCHARGED (the checker proves it) | S1, S2, S3, S5, G1, G2, G3, N1 | 8 |
| ASSUMED (a crypto assumption, stated) | G4 (signature unforgeability) | 1 |
| AT TARGET (the target shows it; differential tests) | S6 (`open` is a homomorphism at T3 and T4) | 1 |
| EXTERNAL (outside langc) | S4 (privacy as indistinguishability; the MPC corruption model if Q-S0-16 (b)) | 1 |

Proof notes. S2: the affine test is equal to the decay path at each height
(Hedberg, then arithmetic). S5: both legs are in one log entry, so
atomicity is refl. G1: by induction on the log. G2 and G3: by the type of
`Params` and the height order. N1: by the replay of the nullifier set.

## 7. Refusals

Twelve codes (Q-S0-32). `langc check --json` gives each with a fix hint.

| Code | Cause |
|---|---|
| `REFUSE_PARTIAL` | A function that is not total. |
| `REFUSE_FORMER` | A former outside the R1 table. |
| `REFUSE_BASE` | A base type outside design sec 3. |
| `REFUSE_DIV` | Division or subtraction on `Amount` or `Height`. |
| `REFUSE_BOUND` | An arithmetic bound obligation that is not discharged. |
| `REFUSE_OPEN` | `open` outside `settle` or the tally. |
| `REFUSE_UNSEAL` | A sealed eliminator in a user program. |
| `REFUSE_CLOCK` | A wall clock. |
| `REFUSE_RETRO` | An amendment that applies at or before its own height. |
| `REFUSE_FLOOR` | A `Params` value with t below t_min. |
| `REFUSE_CKKS` | Approximate arithmetic. |
| `REFUSE_LAW` | A stated DISCHARGED law that the checker cannot prove. |

## 8. Route

### 8.1 Kernel

S2 ports a Cubical TT kernel (Q-S0-18) onto the tcc-wasm kit. S3 makes
arithmetic total (Q-S0-2) and changes the trap row (Q-S0-33).

### 8.2 The five targets

- T1: one Wasm module (the kit writer) for settle, amend and replay, run
  by `barterd` (Q-S0-19). Crypto per Q-S0-17.
- T2: a ZK relation export (Q-S0-6, Q-S0-14).
- T3: an FHE circuit with a static depth bound (Q-S0-15).
- T4: a Bristol Fashion circuit (Q-S0-16).
- T5: a JSON report: swap facts, governance facts, law counts, the verdict
  at each height.

### 8.3 Four C programs

`langc`, `langc mcp`, `langc serve` and `barterd`, as design sec 7. Keep
them separate: `langc mcp` and `langc serve` are for agents that WRITE
barter-lang; `barterd` runs a compiled module.

### 8.4 The x402 route

`langc serve` gives the `langc mcp` tools over HTTP (Q-S0-20) at the
prices of Q-S0-21 on the network of Q-S0-22. A request with no payment gets
a 402 response with the price. `langc serve` calls the facilitator verify
and settle routes through libcurl. The facilitator URL and the payTo
address are USER inputs; `langc serve` does not start without them (OI-5).
The x402 shape follows mandate-lang `docs/VALIDATION.md` "The x402 route
(S8)". Field names are checked against the x402 text in S16 (OI-1).

### 8.5 The demo

Two owners sign intents on one pair. Two solvers quote. The crossing check
runs at T3. One fill settles at height h; the other intent expires. Then a
t-of-n amendment raises t, and the next fill needs the new t. `barterd`
replays the log and gives the same `Params` as the fold of `amend` (G1).

## 9. Agent adoption

Each item of the pasted block (design sec 9.2) maps to a slice. USER marks
an outward step that the USER does.

| Pasted item | Where |
|---|---|
| One canonical form per operation, `langc fmt` | S8 |
| `langc check --json`, fix hints, progress counts (parsed, typed, laws discharged) | S8 |
| `SKILL.md` with 3 to 5 graded examples | S15 |
| An MCP server in TinyCC (`langc mcp`) | S15 |
| `AGENTS.md` and a README table on when to use the language | S0 DRAFT, S20 final |
| Token-count comparison against Python and Rust | S19 (Q-S0-30) |
| MCP server entry in the public registries | S17 `server.json` (Q-S0-23); submission USER |
| An OpenAPI description | S17 `openapi.yaml` (Q-S0-24) |
| The price in the 402 response | S16 (Q-S0-21) |
| Sec 1: MCP tools and x402 endpoints, price $0.01 to $0.20 | S15, S16 |
| Sec 1: Agent Skills | S15 |
| Sec 2: FUNDING.yml, funding.json, llms.txt, ai-donate.json | S17 (Q-S0-25) |
| Sec 2: AGENTS.md or Skills in the repo | S15, S20 |
| Sec 3: AgentFund milestones | S20 candidates (Q-S0-27); campaign USER |
| Sec 3: human channels (GitHub Sponsors) | Sponsor flag at the seed; channels USER |
| Sec 4: frame the value in agent terms | S20 (Q-S0-28) |
| Start 1: funding metadata | S17 |
| Start 2: one capability as MCP tool or x402 endpoint | S15, S16 |
| Start 3: a short Skill | S15 |
| Start 4: list the tool in discovery surfaces | S17 files; listing USER |
| Start 5: measure agent traffic and payments | S16 count log (Q-S0-26); reading USER |

USER steps: each deploy of `langc serve`, the TLS proxy, each registry
submission, the payTo wallet, the facilitator account, any AgentFund
campaign, and the public flip of the repo.

## 10. Tests

The dev-language rule (USER ruling 2026-10-09) holds. Unit and integration
tests are in C (host) and in barter-lang (guest). The x402 test uses a C
client against a local `langc serve` and a stub facilitator in C
(Q-S0-29). No new node, Python or shell harness. The kit's node
differential stays. `make check` is the gate; logs that are kept come from
a serial run.

## 11. Port sources

| Source | What |
|---|---|
| intent-lang 67445ae | The swap design, the Hedberg decay reading, `Enc` as a sealed cell monad. |
| orbit-lang 2162fb2 | Self-reference as a fixed point (law G1). |
| ratify-lang a2322ef | Self-amendment by height, the host-import crypto ABI, the kernel. |
| gavel-lang 310e708, d71bd96, 86c2f30 | The S0 shape, the S1 rulings, total arithmetic. |
| quorum-lang | `src/crypto`, the FHE and MPC tally. |
| permit-lang | The R0 no-trap reading, `REFUSE_PARTIAL`, `test/notraptool.c`, `langc mcp`, `check --json`, `SKILL.md`, the token comparison. |
| mandate-lang | The x402 shape. |

## 12. Slices

| Slice | Work | State |
|---|---|---|
| S0 | Seed from the kit, design, this plan, README and AGENTS drafts | DONE |
| S1 | The USER rules section 1.2 | NEXT |
| S2 | Port the kernel (Q-S0-18) | |
| S3 | Total arithmetic, the trap row (Q-S0-2, Q-S0-33) | |
| S4 | R1 formers, `Enc` and `open` (Q-S0-3) | |
| S5 | Base types of design sec 3 | |
| S6 | Swap operations and laws S1, S2, S3, S5, N1 | |
| S7 | Governance: `Params`, `amend`, `replay`, laws G1 to G3 | |
| S8 | Refusal codes, `check --json`, `langc fmt` | |
| S9 | T1 lowering to one Wasm module | |
| S10 | `barterd`: interpreter, ledger replay, crypto (Q-S0-17, Q-S0-19) | |
| S11 | T5 JSON report | |
| S12 | T2 relation export | |
| S13 | T3 FHE circuit | |
| S14 | T4 Bristol circuit | |
| S15 | `langc mcp` and `SKILL.md` | |
| S16 | `langc serve` and x402 with the C stub facilitator | |
| S17 | `server.json`, `openapi.yaml`, `llms.txt`, `funding.json`, `.github/FUNDING.yml` | |
| S18 | The public test-network row (if Q-S0-29 (a)) | |
| S19 | Token comparison against Python and Rust | |
| S20 | README and AGENTS final, the demo, candidate milestones | |
| S21 | Close | |

## 13. Open items

- OI-1 The field names of the x402 402 body and of the facilitator
  routes. Check them against the x402 text in S16. Do not guess.
- OI-2 The `server.json` schema of the MCP Registry. Check it in S17.
- OI-3 The `funding.json` schema. Check it in S17.
- OI-4 The kit row "a trap is equal to a trap" (Q-S0-33).
- OI-5 The facilitator URL and the payTo address are USER inputs. No
  default address in the repo.
- OI-6 The decoy bundle: its size and the meaning of the sealed decoy
  index (design line 65 of `design/intent-swap-design.md`).
- OI-7 The bit width of `Amount` and `Height`, so that the products of the
  affine test have a static bound.
- OI-8 Deposits and withdrawals: the design escrows on chain. Off chain,
  the ledger needs deposit and withdraw entries in the log. Their shape is
  a question for S5.
- OI-9 Committee key generation (DKG): EXTERNAL or a USER step.
- OI-10 Solver choice: the first valid quote or an auction among solvers
  (`design/intent-swap-design.md` line 103).
- OI-11 `doc-check.pl` was not run at S0 (gavel and ratify S0 precedent).
