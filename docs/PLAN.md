# barter-lang plan

barter-lang is a Cubical TT language for one job: a private intent swap
off chain, under an off-chain threshold authority that amends itself. An
owner signs an intent with a sealed input amount and a sealed floor. The
floor decays over a window of log heights. A solver gives a sealed quote.
FHE or MPC computes the crossing check on sealed values. A t-of-n
committee certifies the fill into the log, and only the settled fill is
opened. A t-of-n amendment changes the swap parameters, and the amend rule
in force judges the new rule. The checker proves the law facts before any
intent. A host runs one Wasm module, and the module replays the log and
checks the certificates, the proofs and the shares. No program can have a
runtime error.

The design is `design/DESIGN.md`: the role map, the base types, the
operations, the governance layer, the laws, the four C programs and the
request, verbatim. The swap design is `design/intent-swap-design.md`,
verbatim. This file gives the readings of the request, the route, the
slices and the open questions. "design sec N" is section N of
`design/DESIGN.md`. Sections 1.2 and 1.3 list the questions for the USER,
and section 1.1 has the rulings.

Status: S1 DONE (2026-11-06): the USER ruled sections 1.2 and 1.3; the
rulings are in section 1.1. Next: S2 (port the kernel).

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
| 2026-11-06 | Q-S0-1 "(a) Verdict sum (Recommended)": a failed check is a value, `Verdict A = accept A \| reject Reason`, with the reasons of section 2. Edits: section 2 (the reasons are no longer a draft). | USER, S1 |
| 2026-11-06 | Q-S0-2 "(a) Port gavel S3 total (Recommended)": S3 ports the gavel-lang S3 total arithmetic (gavel 86c2f30), restricted to compare, select, add and multiply by a public scalar, each with a static bound obligation. | USER, S1 |
| 2026-11-06 | Q-S0-3 "(a) gavel sec 3 classes (Recommended)": the gavel section 3 classes; `Enc` is the identity cell monad with a sealed eliminator, and `open` is a law-only monad morphism `Enc -> Id`. | USER, S1 |
| 2026-11-06 | Q-S0-4 "(a) Design sec 2 (Recommended)": the role map of design sec 2. | USER, S1 |
| 2026-11-06 | Q-S0-5 "(a) Design sec 3 list (Recommended)": the base types of design sec 3, with the sealed decoy index (OI-6). Q-S0-17 (b) adds `Word` for the prelude only. | USER, S1 |
| 2026-11-06 | Q-S0-6 "(a) T2 relation (Recommended)": `ZKProof` is a T2 relation: intent well-formedness and ownership, and correct evaluation of the crossing check. Edits: section 8.2 (T2); design sec 2 (the `ZKProof` row cites Q-S0-6, not Q-S0-9). | USER, S1 |
| 2026-11-06 | Q-S0-7 "(a) Hedberg reading (Recommended)": decay is a function of height, given by the cross-multiplied test of design sec 4.1. | USER, S1 |
| 2026-11-06 | Q-S0-8 "(a) Log height only (Recommended)": progress is log height; there is no wall clock. | USER, S1 |
| 2026-11-06 | Q-S0-9 "(a) Closed-form rule + floor (Recommended)": the rule is data in `Params` in a closed form, `amend` reads the rule that it can change, a new rule applies from h + 1, law G1 is the orbit-lang fixed point, and one static floor t >= t_min is in the type of `Params`. Edits: design sec 5 (it cites Q-S0-9, not Q-S0-6). | USER, S1 |
| 2026-11-06 | Q-S0-10 "(a) No fee in v1 (Recommended)": there is no fee in v1. Edits: design sec 5 (no fee field) and sec 8. | USER, S1 |
| 2026-11-06 | Q-S0-11 "(b) Governed in Params": the `langc serve` price list is a field of `Params`; the amend rule can change it like any other field, and a new price applies from h + 1. Not the recommendation. Edits: section 2, 8.4, 9, 10, 12 (S7, S16), 13 (OI-18 to OI-20); design sec 3 and 5. Q-S1-6 and Q-S1-7 come from this ruling. | USER, S1 |
| 2026-11-06 | Q-S0-12 "(a) ZK eligibility + FHE/MPC tally (Recommended)": ballots use ZK eligibility and an FHE or MPC tally, as in ratify-lang and gavel-lang. | USER, S1 |
| 2026-11-06 | Q-S0-13 "(a) Abstract Hash (Recommended)": an abstract `Hash`; Poseidon at T2 and, by Q-S0-17 (b), in the crypto prelude, not in `barterd`. | USER, S1 |
| 2026-11-06 | Q-S0-14 "(a) Noir (Recommended)": T2 exports Noir. | USER, S1 |
| 2026-11-06 | Q-S0-15 "(a) TFHE (Recommended)": T3 uses TFHE; CKKS is refused. | USER, S1 |
| 2026-11-06 | Q-S0-16 "(a) Pin WRK authenticated garbling (Recommended)": T4 targets the authenticated garbling of Wang, Ranellucci and Katz (CCS 2017): dishonest majority, malicious. The MPC corruption model is ASSUMED (M1), not EXTERNAL. Edits: section 6 (M1; the S4 row loses its (b) clause), 8.2 (T4), 11, 12 (S14), 13 (OI-12, OI-13). | USER, S1 |
| 2026-11-06 | Q-S0-17 "(b) Guest prelude over Word": the crypto (ZK verify, FHE, MPC, Poseidon, signatures) is barter-lang guest terms over a new total `Word` type in the Wasm module, checked by the kernel and lowered by langc; `barterd` has no crypto code (Q-S1-1). Not the recommendation. It replaces the clause "The ZK, FHE and MPC crypto is behind a fixed import ABI" of the S0 T1 row. Only the prelude names `Word` (wrapping 32-bit: add, sub, mul, mulhi, and, or, xor, not, shifts, rotates, compare; no division); a user program that names it gets `REFUSE_WORD`. Edits: section 1 head text, 2, 4, 6 (Z1), 7, 8.2 (T1), 8.3, 10, 11, 12 (S8b, S8c, S8d, S9, S10), 13 (OI-15, OI-17); design sec 2, 3 and 7. Q-S1-1, Q-S1-2, Q-S1-4 and Q-S1-5 come from this ruling. | USER, S1 |
| 2026-11-06 | Q-S0-18 "(a) ratify-lang kernel (Recommended)": S2 ports the ratify-lang kernel at a2322ef (the permit-lang kernel without the affine layer). Edits: section 8.1, 12 (S2). | USER, S1 |
| 2026-11-06 | Q-S0-19 "(a) Own C99 interpreter (Recommended)": `barterd` runs the module on its own C99 Wasm interpreter; node and wasmtime are oracles in the gate. | USER, S1 |
| 2026-11-06 | Q-S0-20 "(a) JSON POST + /mcp (Recommended)": JSON POST routes for the five tools and MCP streamable HTTP at `/mcp`, each behind x402. | USER, S1 |
| 2026-11-06 | Q-S0-21 "(a) Tiered (Recommended)": $0.01 for check, fmt, explain, list-base and show-law; $0.05 for build (T1 module); $0.20 for the T2 to T4 exports. By Q-S0-11 (b) and Q-S1-6 (a), this list is the genesis value in `Params`. Edits: section 8.4, 9. | USER, S1 |
| 2026-11-06 | Q-S0-22 "(a) USDC on Base mainnet (Recommended)": USDC on Base mainnet; base-sepolia in tests. | USER, S1 |
| 2026-11-06 | Q-S0-23 "(a) packages + remotes (Recommended)": `server.json` has a `packages` entry (the stdio `langc mcp` bundle) and a `remotes` entry (the `/mcp` URL after the USER deploys it), name `io.github.MavenRain/barter-lang`. | USER, S1 |
| 2026-11-06 | Q-S0-24 "(a) OpenAPI 3.1 + 402 (Recommended)": `openapi.yaml` 3.1; each route lists its 402 response with the x402 body schema and the price. By Q-S0-11 (b), that price is the genesis price (OI-19). | USER, S1 |
| 2026-11-06 | Q-S0-25 "(a) No ai-donate.json (Recommended)": `.github/FUNDING.yml`, `funding.json`, `llms.txt`; no `.well-known/ai-donate.json`. | USER, S1 |
| 2026-11-06 | Q-S0-26 "(a) Local count log (Recommended)": `langc serve` writes a local count log; nothing leaves the host. | USER, S1 |
| 2026-11-06 | Q-S0-27 "(a) Candidate milestones (Recommended)": this plan lists candidate milestones only; the USER runs any campaign. | USER, S1 |
| 2026-11-06 | Q-S0-28 "(a) What an agent gets (Recommended)": the README gets the section "What an agent gets" in ROI terms. | USER, S1 |
| 2026-11-06 | Q-S0-29 "(a) Stub + public test row (Recommended)": a local C stub facilitator, and from S18 also a public test-network facilitator row in the gate; the gate fails when the network or a test input is absent. Edits: section 9 (USER steps), 10, 12 (S18), 13 (OI-21). | USER, S1 |
| 2026-11-06 | Q-S0-30 "(a) count_tokens, equal outputs (Recommended)": the token comparison uses count_tokens on programs with equal outputs. | USER, S1 |
| 2026-11-06 | Q-S0-31 "(a) Four buckets (Recommended)": the four buckets of section 6; `laws_discharged` counts DISCHARGED rows only. | USER, S1 |
| 2026-11-06 | Q-S0-32 "(a) Twelve codes (Recommended)": the codes of section 7. Q-S0-17 (b) adds `REFUSE_WORD`, thus thirteen codes. Edits: section 7. | USER, S1 |
| 2026-11-06 | Q-S0-33 "(a) S3 changes it (Recommended)": R0 makes a trap a failure on each target; S3 changes the kit row "a trap is equal to a trap". | USER, S1 |
| 2026-11-06 | Q-S1-1 "(a) All, incl. TFHE shares (Recommended)": the module checks Poseidon, the certificate signatures, the Noir proofs (BN254 pairing), the MPC outputs and each TFHE decryption share, with a Noir proof for each share; `barterd` has no crypto code. Edits: section 4, 6 (Z1), 8.2 (T2, T3), 12 (S8c, S12, S13), 13 (OI-14); design sec 7. | USER, S1 |
| 2026-11-06 | Q-S1-2 "(a) Party program too (Recommended)": the module also runs the WRK rounds of one member, and `barterd` carries the messages. Edits: section 4, 8.2 (T1, T4), 8.3 (member mode), 10, 12 (S14b); design sec 7. Q-S1-3 comes from this ruling. | USER, S1 |
| 2026-11-06 | Q-S1-3 "(a) TCP links (Recommended)": in member mode `barterd` opens TCP links to the other members, with TLS by a proxy. Edits: section 6 (M2), 8.3, 9 (USER steps), 10, 12 (S14c), 13 (OI-16). | USER, S1 |
| 2026-11-06 | Q-S1-4 "(a) OS seed per run (Recommended)": in member mode `barterd` reads 32 bytes from the OS (getentropy) for each run and gives them to the module as an input; the prelude expands them with ChaCha20 on `Word`. Edits: section 4, 6 (M3), 8.3, 12 (S8d, S14c); design sec 7. | USER, S1 |
| 2026-11-06 | Q-S1-5 "(a) quorum-lang C oracle (Recommended)": quorum-lang `src/crypto` (8d1a113) is the reference of the prelude and a C oracle in the gate. Edits: section 4, 10, 11, 12 (S8b). | USER, S1 |
| 2026-11-06 | Q-S1-6 "(a) Range $0.01-$0.20 (Recommended)": the Q-S0-21 list is the genesis value in `Params`; each price has a static range $0.01 to $0.20 in the type of `Params`; an amendment outside the range is `reject bad_price`, a ninth reason. Edits: section 2, 7 (`REFUSE_FLOOR`), 9, 10, 12 (S7); design sec 5. | USER, S1 |
| 2026-11-06 | Q-S1-7 "(a) Ask local barterd (Recommended)": `langc serve` asks a local `barterd` for the price list at the log head, and the 402 body names that height. Edits: section 8.3, 8.4, 9 (USER steps), 10, 12 (S10, S16, the order), 13 (OI-18); design sec 7. | USER, S1 |

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

The USER ruled each item in S1 (2026-11-06). Each item ends with its
ruling, and section 1.1 has one row for each.

- Q-S0-1 R0 failures (section 2). (a) A failed check is a value: `Verdict A
  = accept A | reject Reason`, with the reasons of section 2 (gavel
  d71bd96:36). (b) One reason only, `reject`, with no code. RULED (a).
- Q-S0-2 Amount arithmetic (section 2). (a) Port the gavel-lang S3 total
  arithmetic (gavel 86c2f30 "make arithmetic total") restricted to compare,
  select, add and multiply by a public scalar, each with a static bound
  obligation. This is enough for the affine test of design sec 4.1. (b)
  Compare and select only, as gavel ruled (gavel d71bd96:35), with decay as
  a step table in `Params`. barter-lang differs from gavel: a decay floor
  needs a product of a height and an amount. RULED (a).
- Q-S0-3 R1 formers (section 3). (a) The gavel section 3 classes (gavel
  d71bd96:37). `Enc` is the identity cell monad with a sealed eliminator;
  `open` is a law-only monad morphism `Enc -> Id` (intent-lang 67445ae
  `docs/PLAN.md` lines 35 and 282 to 289). (b) `Sealed` is an opaque base
  type with no former. RULED (a).
- Q-S0-4 Role map. (a) Design sec 2 (same shape as gavel d71bd96:38). (b)
  One sequencer orders the log; the committee certifies fills only. RULED (a).
- Q-S0-5 Base types. (a) The list of design sec 3. (b) The same list
  without the sealed decoy index (no decoy bundle in v1; OI-6). RULED (a).
- Q-S0-6 The role of `ZKProof`. (a) A T2 relation: intent well-formedness
  and ownership, and correct evaluation of the crossing check (gavel
  d71bd96:43 ruled a T2 proof of the outcome). (b) t attestations of
  correct evaluation, no proof. RULED (a).
- Q-S0-7 Decay. (a) The intent-lang Hedberg reading: a path in a set is
  trivial, so decay is a function of height given by the cross-multiplied
  test of design sec 4.1 (intent-lang 67445ae `docs/PLAN.md` lines 306 to
  325). (b) A step table in `Params`. RULED (a).
- Q-S0-8 Time (section 5). (a) Progress is log height. No wall clock
  (ratify-lang and gavel-lang). (b) Log height plus a certified wall-clock
  stamp in each entry. RULED (a).
- Q-S0-9 Self-amendment (design sec 5). (a) The rule is data in `Params`
  in a closed form; `amend` reads the rule that it can change; a new rule
  applies from h + 1; law G1 is the orbit-lang fixed point; one static
  floor t >= t_min in the type of `Params` (gavel d71bd96:40 ruled
  `Params` with t-of-n, a rule that amends itself, order by height). (b)
  The same with no floor. (c) An amendment can carry new barter-lang code
  (code as data). Refused in v1: it needs a checked compiler in `barterd`. RULED (a).
- Q-S0-10 Fee. (a) No fee in v1. (b) A fee in basis points in `Params`. RULED (a).
- Q-S0-11 Price list of `langc serve`. (a) A deploy setting, not in
  `Params` (gavel d71bd96:41). (b) Governed in `Params`. RULED (b).
- Q-S0-12 Ballots. (a) ZK eligibility and an FHE or MPC tally, as in
  ratify-lang and gavel-lang. (b) Signed ballots, t-of-n signatures only,
  no ballot secrecy. RULED (a).
- Q-S0-13 Hash. (a) An abstract `Hash`; Poseidon at T2 and in the crypto
  code (gavel d71bd96:42). (b) Poseidon fixed in the language. RULED (a).
- Q-S0-14 T2 format. (a) Noir (gavel d71bd96:44). (b) arkworks R1CS. RULED (a).
- Q-S0-15 T3 scheme. (a) TFHE (gavel d71bd96:45). (b) BFV. CKKS is refused
  in both: approximate arithmetic is a runtime error class. RULED (a).
- Q-S0-16 T4 protocol. (a) Pin one protocol: the authenticated garbling of
  Wang, Ranellucci and Katz (CCS 2017), dishonest majority, malicious
  (gavel d71bd96:46 and 63). (b) The MPC protocol and its corruption model
  are EXTERNAL (law bucket); `langc` does not check them. RULED (a).
- Q-S0-17 Crypto location. (a) In `barterd` behind a fixed host-import ABI,
  as the T1 ruling says (section 1.1), a port of the ratify-lang ABI and
  quorum-lang `src/crypto`. (b) In the module, as a guest-language prelude
  over a total `Word` type (gavel d71bd96:47, 65 and 69). barter-lang
  differs from gavel: the barter T1 ruling keeps the ABI clause, and (b)
  removes it. RULED (b).
- Q-S0-18 Kernel source for S2. (a) The ratify-lang kernel (gavel
  d71bd96:48). (b) The gavel-lang kernel at 86c2f30 (it has total
  arithmetic). RULED (a).
- Q-S0-19 How `barterd` runs the module. (a) Its own C99 Wasm interpreter;
  node and wasmtime are oracles in the gate (gavel d71bd96:49). (b) A
  wasmtime child process. RULED (a).
- Q-S0-20 `langc serve` routes. (a) JSON POST routes for the five tools and
  MCP streamable HTTP at `/mcp`, each behind x402 (gavel d71bd96:50). (b)
  `/mcp` only. RULED (a).
- Q-S0-21 Price. (a) Tiered: $0.01 for check, fmt, explain, list-base and
  show-law; $0.05 for build (T1 module); $0.20 for the T2 to T4 exports
  (gavel d71bd96:51). (b) A flat $0.01. RULED (a).
- Q-S0-22 Network. (a) USDC on Base mainnet; base-sepolia in tests (gavel
  d71bd96:52). (b) base-sepolia only in v1. RULED (a).
- Q-S0-23 `server.json`. (a) A `packages` entry (the stdio `langc mcp`
  bundle) and a `remotes` entry (the `/mcp` URL after the USER deploys),
  name `io.github.MavenRain/barter-lang` (gavel d71bd96:53). (b) `packages`
  only. RULED (a).
- Q-S0-24 `openapi.yaml`. (a) OpenAPI 3.1; each route lists its 402
  response with the x402 body schema and the price (gavel d71bd96:54). (b)
  OpenAPI 3.0. RULED (a).
- Q-S0-25 Funding files. (a) `.github/FUNDING.yml`, `funding.json`,
  `llms.txt`; no `.well-known/ai-donate.json` (gavel d71bd96:55). (b) Also
  `.well-known/ai-donate.json`. RULED (a).
- Q-S0-26 Measurement. (a) `langc serve` writes a local count log; nothing
  leaves the host (gavel d71bd96:56). (b) No count log. RULED (a).
- Q-S0-27 AgentFund. (a) This plan lists candidate milestones only; the
  USER runs any campaign (gavel d71bd96:57). (b) No milestones in the plan. RULED (a).
- Q-S0-28 Agent framing. (a) The README gets the section "What an agent
  gets" in ROI terms (gavel d71bd96:58). (b) No such section. RULED (a).
- Q-S0-29 x402 test network. (a) A local C stub facilitator, and also a
  public test-network facilitator row in the gate; the gate fails when the
  network or a test input is absent (gavel d71bd96:59, 67 and 68). (b) The
  local stub facilitator only. RULED (a).
- Q-S0-30 Token comparison. (a) The permit-lang ruling: count_tokens on
  programs with equal outputs (gavel d71bd96:60). (b) A word count. RULED (a).
- Q-S0-31 Law buckets. (a) The four buckets of section 6;
  `laws_discharged` counts DISCHARGED rows only (gavel d71bd96:61). (b)
  Three buckets (AT TARGET joins ASSUMED). RULED (a).
- Q-S0-32 Refusal codes. (a) The twelve codes of section 7 (gavel
  d71bd96:62 ruled its section 7 codes). (b) One generic code with a text. RULED (a).
- Q-S0-33 The kit row "a trap is equal to a trap". (a) S3 changes it: R0
  makes a trap a failure on each target (permit-lang f9d8a15 `docs/PLAN.md`
  lines 188 to 201; gavel 310e708 `docs/PLAN.md` lines 84 to 86). (b) Keep
  the row until S9 (T1). RULED (a).

### 1.3 Questions for the USER (S1)

The two rulings that are not the recommendation (Q-S0-11 (b) and Q-S0-17
(b)) gave seven new questions. The USER ruled them in S1 (2026-11-06).
Option (a) is the recommendation, cited as in section 1.2.

- Q-S1-1 (from Q-S0-17 (b)) Module check set. (a) The module checks
  Poseidon, the certificate signatures, the Noir proofs (BN254 pairing),
  the MPC outputs and each TFHE decryption share, with a Noir proof for
  each share; `barterd` has no crypto code (gavel d71bd96:66 and 70). (b)
  The same without the share checks: a decryption share counts on t-of-n
  attestations only. RULED (a).
- Q-S1-2 (from Q-S0-17 (b)) MPC party program. (a) The module also runs
  the WRK rounds of one member, and `barterd` carries the messages (gavel
  d71bd96:64). (b) The module checks the MPC outputs only; each member
  runs the WRK party program outside barter-lang. RULED (a).
- Q-S1-3 (from Q-S1-2 (a)) Member links. (a) In member mode `barterd`
  opens TCP links to the other members, with TLS by a proxy (gavel
  d71bd96:71). (b) Each member talks only to the operator, who relays the
  messages. RULED (a).
- Q-S1-4 (from Q-S0-17 (b)) Randomness. (a) In member mode `barterd` reads
  32 bytes from the OS (getentropy) for each run and gives them to the
  module as an input; the prelude expands them with ChaCha20 on `Word`
  (gavel d71bd96:72). (b) One host import `random_bytes`. RULED (a).
- Q-S1-5 (from Q-S0-17 (b)) Prelude reference. (a) quorum-lang
  `src/crypto` is the reference and a C oracle in the gate (gavel
  d71bd96:65). (b) Known-answer vectors only, no C oracle. RULED (a).
- Q-S1-6 (from Q-S0-11 (b)) Genesis price and bound. (a) The Q-S0-21
  tiered list is the genesis value in `Params`. Each price has a static
  range $0.01 to $0.20 in the type of `Params` (design sec 9). An
  amendment outside the range is `reject bad_price`, a ninth reason. (b)
  Genesis as (a), a static floor $0.01 only, no ceiling. (c) Genesis as
  (a), no bound. RULED (a).
- Q-S1-7 (from Q-S0-11 (b)) Price source of `langc serve`. (a)
  `langc serve` asks a local `barterd` for the price list at the log head;
  the 402 body names that height. (b) `langc serve` replays the log
  itself. (c) A deploy file that the USER refreshes from `barterd`. RULED
  (a).

## 2. The reading of R0: no runtime error

R0 says that langc accepts no program that can lead to a runtime error.
The reading ports permit-lang, ratify-lang and gavel-lang: every function is
total, and every failure in the domain is a value.

- A failed proof check, a bad decryption share, an expired intent, a reused
  nullifier, a wrong token pair, a quote below the floor, a bad certificate,
  an amendment below t_min and an amendment with a price outside its range
  are each a `reject Reason` of `Verdict`, or `Unfilled` of `Outcome`. None
  is a trap.
- Reasons (Q-S0-1 (a), Q-S1-6 (a)), nine: `bad_proof`, `bad_share`,
  `expired`, `reused`, `bad_pair`, `below_floor`, `bad_cert`,
  `below_t_min`, `bad_price`.
- The crypto is in the module (Q-S0-17 (b)): guest terms over the total
  `Word` type of section 4. A proof, a share, a signature or an MPC output
  that does not check is a `reject`, never a trap.
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

The crypto prelude (Q-S0-17 (b)) is barter-lang guest terms in the Wasm
module, checked by the kernel and lowered by langc. It is over a new total
type `Word`: wrapping 32-bit add, sub, mul, mulhi, and, or, xor, not,
shifts, rotates and compare, with no division (gavel d71bd96:69). Only the
prelude names `Word`; a user program that names it gets `REFUSE_WORD`.
The prelude gives the BN254 field and curve, Poseidon for `Hash`
(Q-S0-13 (a)), the certificate and intent signature check (OI-17), the
BN254 pairing and the Noir verifier (Q-S0-6 (a), Q-S0-14 (a)), the TFHE
share check with a Noir proof for each share (Q-S1-1 (a)), the MPC output
check, the WRK rounds of one member (Q-S1-2 (a)) and ChaCha20 over the
run seed (Q-S1-4 (a)). quorum-lang `src/crypto` is the reference and a C
oracle in the gate (Q-S1-5 (a)). `barterd` has no crypto code.

## 5. Time

Progress is log height (Q-S0-8). A decay `Window` starts at a height and
has a length in heights. An intent expires at its deadline height. There
is no wall clock; a program that names one gives `REFUSE_CLOCK`.

## 6. Laws and obligations

The laws are design sec 6. Buckets (Q-S0-31):

| Bucket | Laws | Count |
|---|---|---|
| DISCHARGED (the checker proves it) | S1, S2, S3, S5, G1, G2, G3, N1 | 8 |
| ASSUMED (a crypto assumption, stated) | G4 (signature unforgeability), Z1 (soundness of the Noir proofs, with the TFHE share proof; Q-S1-1 (a)), M1 (WRK security: malicious, up to n - 1 corrupt members, abort possible; Q-S0-16 (a)), M3 (the OS seed is uniform and fresh for each run; Q-S1-4 (a)) | 4 |
| AT TARGET (the target shows it; differential tests) | S6 (`open` is a homomorphism at T3 and T4) | 1 |
| EXTERNAL (outside langc) | S4 (privacy as indistinguishability), M2 (private, authenticated member links: the TLS proxy of each member; Q-S1-3 (a)) | 2 |

Z1, M1, M2 and M3 come from the S1 rulings. They are obligations, not laws
of design sec 6. Total: 15 rows.

Proof notes. S2: the affine test is equal to the decay path at each height
(Hedberg, then arithmetic). S5: both legs are in one log entry, so
atomicity is refl. G1: by induction on the log. G2 and G3: by the type of
`Params` and the height order. N1: by the replay of the nullifier set.

## 7. Refusals

Thirteen codes (Q-S0-32 (a); Q-S0-17 (b) adds `REFUSE_WORD`). `langc
check --json` gives each with a fix hint.

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
| `REFUSE_FLOOR` | A `Params` value outside a static bound of its type: t below t_min, or a price outside $0.01 to $0.20 (Q-S1-6 (a)). |
| `REFUSE_CKKS` | Approximate arithmetic. |
| `REFUSE_LAW` | A stated DISCHARGED law that the checker cannot prove. |
| `REFUSE_WORD` | A user program that names `Word` or a `Word` operation (Q-S0-17 (b)). |

## 8. Route

### 8.1 Kernel

S2 ports the ratify-lang Cubical TT kernel at a2322ef (Q-S0-18 (a)) onto
the tcc-wasm kit. Survey first: SAME, CHANGED, SOURCE-ONLY. S3 makes
arithmetic total (Q-S0-2) and changes the trap row (Q-S0-33).

### 8.2 The five targets

- T1: one Wasm module (the kit writer) for settle, amend and replay, run
  by `barterd` on its own C99 interpreter (Q-S0-19 (a)). The crypto is in
  the module: the prelude of section 4 (Q-S0-17 (b), Q-S1-1 (a)).
  `barterd` has no crypto code and no crypto import. In member mode the
  module also runs the WRK rounds of one member (Q-S1-2 (a)).
- T2: a ZK relation export in Noir (Q-S0-6 (a), Q-S0-14 (a)): intent
  well-formedness and ownership, the crossing check, ballot eligibility,
  and the TFHE share relation (Q-S1-1 (a)). The Noir verifier in the
  module checks each proof (Z1).
- T3: an FHE circuit with a static depth bound (Q-S0-15 (a)). The module
  checks each decryption share by its Noir proof (Q-S1-1 (a)).
- T4: Bristol Fashion circuits for intent matching and the amendment
  tally, for the WRK protocol (Q-S0-16 (a), M1). The module runs the
  rounds of one member (Q-S1-2 (a)): base OT, OT extension, the
  authenticated preprocessing, the garbling and the evaluation (OI-13).
- T5: a JSON report: swap facts, governance facts, law counts, the verdict
  at each height.

### 8.3 Four C programs

`langc`, `langc mcp`, `langc serve` and `barterd`, as design sec 7. Keep
them separate: `langc mcp` and `langc serve` are for agents that WRITE
barter-lang; `barterd` runs a compiled module.

`barterd` has two modes. Replay mode reads a log, the intents, the quotes
and the certificates, runs the module and prints the verdicts. It holds
no secret. It also gives a local `langc serve` the price list in `Params`
at the log head, with that height (Q-S1-7 (a)). Member mode (Q-S1-2 (a))
carries the WRK messages of one member. It opens TCP links to the other
members, with TLS by a proxy (Q-S1-3 (a), M2). It reads 32 bytes from the
OS (getentropy) for each run and gives them to the module as the seed
(Q-S1-4 (a), M3). The seed and the member state stay in memory for the
run only.

### 8.4 The x402 route

`langc serve` gives the `langc mcp` tools over HTTP (Q-S0-20 (a)) on the
network of Q-S0-22 (a). The price list is a field of `Params` (Q-S0-11
(b)); its genesis value is the Q-S0-21 (a) list. `langc serve` asks a
local `barterd` for the price list at the log head (Q-S1-7 (a)). A request
with no payment gets a 402 response with the price and the log height of
that price. An amendment certified at height h changes a price from
h + 1 (OI-18). `langc serve` calls the facilitator verify
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
| The price in the 402 response | S16 (Q-S0-21 (a) genesis list, Q-S1-7 (a)) |
| Sec 1: MCP tools and x402 endpoints, price $0.01 to $0.20 | S15, S16; the range in `Params` at S7 (Q-S1-6 (a)) |
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

USER steps: each deploy, which runs both `barterd` and `langc serve`
(Q-S1-7 (a)); the TLS proxy of `langc serve` and the TLS proxy of the
member links (Q-S1-3 (a)); each registry submission, the payTo wallet,
the facilitator account, the test-network inputs of the S18 row (Q-S0-29
(a)), any AgentFund campaign, and the public flip of the repo.

## 10. Tests

The dev-language rule (USER ruling 2026-10-09) holds. Unit and integration
tests are in C (host) and in barter-lang (guest). No new node, Python or
shell harness. The kit's node differential stays. `make check` is the
gate; logs that are kept come from a serial run.

- x402: a C client against a local `langc serve`, a local `barterd` and a
  stub facilitator in C (Q-S0-29 (a)).
- x402 on the test network (Q-S0-29 (a)): from S18 the gate runs a C
  client against a local `langc serve` that calls a public base-sepolia
  facilitator. The client signs the EIP-3009 authorization in C99 under
  `test/` (gavel d71bd96:68). The gate fails when the network or a test
  input is absent (OI-21).
- Price rows (Q-S0-11 (b), Q-S1-6 (a), Q-S1-7 (a)): an amendment with a
  price outside $0.01 to $0.20 gives `reject bad_price`; a new price
  applies from h + 1; the 402 body names the height of its price.
- Prelude rows (Q-S1-5 (a)): each prelude operation against quorum-lang
  `src/crypto` as a C oracle on fixed vectors.
- WRK rows (Q-S1-2 (a), Q-S1-3 (a)): n `barterd` members in member mode on
  local TCP links. The outputs equal the clear evaluation of the same
  circuit.

## 11. Port sources

| Source | What |
|---|---|
| intent-lang 67445ae | The swap design, the Hedberg decay reading, `Enc` as a sealed cell monad. |
| orbit-lang 2162fb2 | Self-reference as a fixed point (law G1). |
| ratify-lang a2322ef | Self-amendment by height, the kernel (Q-S0-18 (a)). Not its host-import crypto ABI (Q-S0-17 (b)). |
| gavel-lang 310e708, d71bd96, 86c2f30 | The S0 shape, the S1 rulings, total arithmetic, the crypto prelude slices. |
| quorum-lang 8d1a113 | `src/crypto`: the reference of the crypto prelude and a C oracle in the gate (Q-S1-5 (a)); the FHE and MPC tally. |
| Wang, Ranellucci, Katz (CCS 2017) | WRK authenticated garbling (Q-S0-16 (a)). No sibling source. |
| permit-lang | The R0 no-trap reading, `REFUSE_PARTIAL`, `test/notraptool.c`, `langc mcp`, `check --json`, `SKILL.md`, the token comparison. |
| mandate-lang | The x402 shape. |

## 12. Slices

| Slice | Work | State |
|---|---|---|
| S0 | Seed from the kit, design, this plan, README and AGENTS drafts | DONE |
| S1 | The USER rules sections 1.2 and 1.3; the rulings are in section 1.1 | DONE (2026-11-06) |
| S2 | Port the ratify-lang kernel at a2322ef (Q-S0-18 (a)); survey first | NEXT |
| S3 | Total arithmetic, the trap row (Q-S0-2, Q-S0-33) | |
| S4 | R1 formers, `Enc` and `open` (Q-S0-3) | |
| S5 | Base types of design sec 3 | |
| S6 | Swap operations and laws S1, S2, S3, S5, N1 | |
| S7 | Governance: `Params` with the price list and its range (Q-S0-11 (b), Q-S1-6 (a)), `amend`, `replay`, `bad_price`, laws G1 to G3 | |
| S8 | Refusal codes, `check --json`, `langc fmt` | |
| S8b | The crypto prelude part 1 (Q-S0-17 (b)): `Word` and `REFUSE_WORD`, the BN254 field and curve, Poseidon (Q-S0-13 (a)), the signature check (OI-17), the C oracle rows (Q-S1-5 (a)) | |
| S8c | The crypto prelude part 2 (Q-S1-1 (a)): the BN254 pairing and the Noir verifier (the T2 relations, the TFHE share proof); the run time (OI-15) | |
| S8d | The crypto prelude part 3: ChaCha20 and the seed input (Q-S1-4 (a)), the MPC output check | |
| S9 | T1 lowering to one Wasm module with the prelude | |
| S10 | `barterd` replay mode: the C99 interpreter (Q-S0-19 (a)), the ledger replay, the price answer to `langc serve` (Q-S1-7 (a)); no crypto code (Q-S0-17 (b)) | |
| S11 | T5 JSON report | |
| S12 | T2 relation export (Q-S0-6 (a), Q-S0-14 (a)) and the TFHE share relation (Q-S1-1 (a)) | |
| S13 | T3 FHE circuit, the depth bound, the share checks (Q-S1-1 (a), OI-14) | |
| S14 | T4 Bristol circuits for WRK (Q-S0-16 (a)) | |
| S14b | T4 part 2: the WRK party program in the module (Q-S1-2 (a)): base OT, OT extension, preprocessing, garbling, evaluation (OI-13) | |
| S14c | `barterd` member mode: the TCP links (Q-S1-3 (a)), the OS seed (Q-S1-4 (a)), the WRK rows (OI-16) | |
| S15 | `langc mcp` and `SKILL.md` | |
| S16 | `langc serve` and x402 with the C stub facilitator; the price from a local `barterd` at the log head (Q-S1-7 (a), OI-18) | |
| S17 | `server.json`, `openapi.yaml`, `llms.txt`, `funding.json`, `.github/FUNDING.yml`; the price text (OI-19, OI-20) | |
| S18 | The public test-network row (Q-S0-29 (a), OI-21) | |
| S19 | Token comparison against Python and Rust | |
| S20 | README and AGENTS final, the demo, candidate milestones | |
| S21 | Close | |

Order: the `langc serve` x402 slices (S16 to S18) need S10, because
`langc serve` asks `barterd` for the price (Q-S1-7 (a)). S14c needs S10
and S14b. S9 needs S8b to S8d.

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
- OI-12 WRK has no guaranteed output: one corrupt member can abort a T4
  run (Q-S0-16 (a)). An aborted run is a loss of liveness, not of safety.
  T5 states it.
- OI-13 The base OT, the OT extension, the garbling hash and the PRF of
  the WRK port are fixed at S14b (a question then). No sibling source.
- OI-14 The TFHE parameters, the noise bound of a share and the threshold
  key generation are fixed at S13. The key generation is EXTERNAL (OI-9).
- OI-15 The run time of the BN254 pairing and the Noir verifier in the
  prelude under the C99 interpreter is not known. S8c measures it.
- OI-16 The reason for a lost link, a late message or a bad message in
  member mode is fixed at S14c.
- OI-17 The signature scheme of `Sig` and of the certificates is not
  ruled. gavel-lang uses Schnorr on BN254 G1 with a keccak-256 challenge
  (gavel d71bd96:66). It is a question at S8b.
- OI-18 A race: a 402 names the price at height h, and then an amendment
  changes the price from h + 1 before the payment comes. The price that
  the payment must meet is a question at S16.
- OI-19 `openapi.yaml` lists the genesis price only (Q-S0-24 (a)) and says
  that the 402 body carries the current price.
- OI-20 The price text in `server.json` and `llms.txt` likewise gives the
  genesis price and says that the 402 body carries the current price.
- OI-21 From S18 each gate run needs the network and the USER test inputs
  (Q-S0-29 (a)). A sandboxed session needs the facilitator host on its
  network allowlist.
