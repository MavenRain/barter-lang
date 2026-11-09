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
`design/DESIGN.md`. Sections 1.2 to 1.11 list the questions for the USER,
and section 1.1 has the rulings.

Status: S8b DONE (2026-11-08): the machine type `Word` with its 14
operations and `REFUSE_WORD`, the domain family rename to `SwapWord`,
the BN254 field `Fr` and `Fp` in the prelude, and the C oracle rows
(sections 4, 7, 9 and 10). Q-S4-1..4, Q-S5-1..10, Q-S6-1..6,
Q-S7-1..7, Q-S8-1..4 and Q-S8b-1..8 are ruled (a). Sections 1.10 and
1.11 have the S8 and S8b questions, all ruled. Next: S8b2 (the crypto
prelude part 1, second commit).

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
| 2026-11-06 | S3 base: the USER commit df78228 (tree c42151a2) is the base of S3. It adds three changes to the S2 tree that was staged: the IR row covers all 30 examples, `lazy.lang` is an IR refusal, and `test/ir/spine.lang` has 13 boundary proofs. The gate counts on df78228 are the S2 counts, but `ir: 30 lowered` and `ir refusals: 6 checked`. | USER, S3 |
| 2026-11-06 | Q-S2-1 "(a) Keep cut (Recommended)": S2 keeps the cut ratify-lang 55310c5..71f2c22 on the kernel paths. | USER, S3 |
| 2026-11-06 | Q-S2-2 "(a) Keep ported row (Recommended)": the kit row `pick 2 4 9` wants 4, and `flag_norm` stays. | USER, S3 |
| 2026-11-06 | Q-S2-3 "(a) Rewrite in S3 (Recommended)": S3 rewrites the ported plan citations of ratify-lang and permit-lang to text that names no plan item, or to the items of this plan. | USER, S3 |
| 2026-11-06 | Q-S2-4 "(a) Keep until S5 (Recommended)": the QIIT test family `Charter`, `Ratified`, `ratify` and `adopt` stays until S5 changes the domain. | USER, S3 |
| 2026-11-06 | Q-S3-1 "(a) Nat stays (Recommended)": the Nat words of the port stay kernel words for the prelude and the tests. S5 gives `Amount` and `Height` their own words (compare, select, add, a product by a public scalar), each with a static bound obligation that the checker discharges (OI-7). | USER, S4 |
| 2026-11-06 | Q-S4-1 "(a) S8 maps codes (Recommended)": S4 keeps the kernel codes. S8 maps `REFUSE_CELL` to `REFUSE_FORMER`, and `REFUSE_LAW` on a sealed eliminator or on `open` to `REFUSE_UNSEAL` or `REFUSE_OPEN`. | USER, S5 |
| 2026-11-06 | Q-S4-2 "(a) S6 renames (Recommended)": S6 changes the DAO slot name `Outcome` to `DaoOutcome` with the sums `Outcome` and `Verdict`. | USER, S5 |
| 2026-11-06 | Q-S4-3 "(a) Program law may (Recommended)": a program law can name `open`. `REFUSE_OPEN` is `open` in a def that is not a law, `settle` or the tally. | USER, S5 |
| 2026-11-06 | Q-S4-4 "(a) Base types only (Recommended)": S5 limits the parameter of `Sealed` to the base types of design sec 3 (the shape is Q-S5-3). | USER, S5 |
| 2026-11-06 | Q-S5-1 "(a) Interval rule (Recommended)": checker constants: `Amount` 64 bits, `Height` 32 bits, target word 128 bits. The checker gives each `Amount` or `Height` term of a program an upper bound (an input at its width, a literal its value, an add the sum, a product the product) and gives `REFUSE_BOUND` when a bound passes the target word (OI-7). | USER, S5 |
| 2026-11-06 | Q-S5-2 "(a) Height scalar (Recommended)": `amScale : Height -> Amount -> Amount` and `htScale : Height -> Height -> Height`. No product of two amounts. A sealed amount is scaled through `encMap`, so the scalar stays public. | USER, S5 |
| 2026-11-06 | Q-S5-3 "(a) Checker rule (Recommended)": `Sealed` keeps its parameter. In a program, `Sealed X`, `Enc X` or `seal X` needs X in the list `Amount`, `Height`, `Token`, `Sum Unit Unit` (the sealed Bool of `cross`); another X gives `REFUSE_CELL` (S8 maps it to `REFUSE_FORMER`, Q-S4-1 (a)). | USER, S5 |
| 2026-11-06 | Q-S5-4 "(a) Scalars + opaque (Recommended)": S5 builds `Amount`, `Height`, `Window` (start, length > 0), `Token` (decidable equality), `Nullifier` and the opaque `Key`, `Sig`, `Commitment` and `Root`. The records `Intent`, `Quote`, `Transfer` and `Fill` go to S6; `Statement` and `Certificate` go to S7. | USER, S5 |
| 2026-11-06 | Q-S5-5 (a) (Recommended): a direct read of a base family in a program (an elim, a fold or a projection of `Amount`, `Height`, `Window`, `Token`, `Nullifier`, `Key`, `Sig`, `Commitment` or `Root`, or `amSucc` or `htSucc` on a term that is not a literal) gives the new kernel code `REFUSE_READ`. S8 maps it to the codes of section 7 (`REFUSE_DIV` for `Amount` and `Height`), the shape of Q-S4-1 (a). | USER, S5 |
| 2026-11-06 | Q-S5-6 (a) (Recommended): in the interval rule, a variable that is not a parameter of the program def (a lambda binder in the body, a case binder of an elim or a fold such as `ih`) has the bound 2^128 - 1 (the word). Thus an elim loop cannot hide an iterated product. | USER, S5 |
| 2026-11-06 | Q-S5-7 (a) (Recommended): a program def that gives an `Amount` or a `Height` keeps its bound (its parameters at their widths) in a table. At a call, each `Amount` or `Height` argument must have a bound at most the width of its parameter, else `REFUSE_BOUND`. A program def with an `Amount` or `Height` parameter that is used as a value (not a call with all its arguments) gives `REFUSE_BOUND`. | USER, S5 |
| 2026-11-06 | Q-S5-8 (a) (Recommended): the test block family `Window` of the port becomes `Shutter`; the name `Window` is the base type of design sec 3. | USER, S5 |
| 2026-11-07 | S6 base: the USER commit e4454c9 (tree 92a3ad4c) is the base of S6. It adds S5 review fixes to the S5 tree that was staged (7b3f3810): `Window` bounds at calls, the `Enc` type-argument rule, 7 `test/check` refusals and 2 eval rows. The gate counts on e4454c9 are the S5 counts, but `check refusals: 177 checked` and `evals: 99 checked`. | USER, S6 |
| 2026-11-07 | Q-S5-9 (a) (Recommended): keep the refusal. A word operation (`amAdd`, `htAdd`, `amScale`, `htScale` or `wLen`) used as a value, not in a call with all its arguments, gives `REFUSE_BOUND`. | USER, S6 |
| 2026-11-07 | Q-S5-10 (a) (Recommended): a guest program gets a `Token`, `Nullifier`, `Key`, `Sig`, `Commitment` or `Root` value only as an input (a parameter or the state). No change. | USER, S6 |
| 2026-11-07 | Q-S6-1 (a) (Recommended): the interval rule gives a projection of a record field of type `Amount` or `Height` its width (2^64 - 1 or 2^32 - 1) when the record is a parameter of the def. | USER, S6 |
| 2026-11-07 | Q-S6-2 (a) (Recommended): the interval rule gives a projection of a `Window` field of a parameter record the widths of a `Window` parameter (start 2^32 - 1, length 2^32). | USER, S6 |
| 2026-11-07 | Q-S6-3 (a) (Recommended): `cross` uses the domain word `encMap2`, the map of two sealed values. The domain defines it by `encBind`; T3 and T4 read it as one affine circuit with no bind. | USER, S6 |
| 2026-11-07 | Q-S6-4 (a) (Recommended): N1 is N1a (a spent nullifier gives `reject Outcome reused`, DISCHARGED) and closed `refl` rows and eval rows for the spend direction. The full replay form comes in S7. The ids stay `Nat`. | USER, S6 |
| 2026-11-07 | Q-S6-5 (a) (Recommended): S2 is the path from the open of `cross` to `decayAt` of the open values (DISCHARGED by elim), and closed rows for the arithmetic reading. | USER, S6 |
| 2026-11-07 | Q-S6-6 (a) (Recommended): the S6 `settle` uses stand-ins: `Spent` for the ledger, the plain `open` for the certified open, `sgVrf` for the proof check, and `badPair` for equal tokens until the S7 `Params`. | USER, S6 |
| 2026-11-07 | Q-S7-1 (a) (Recommended): `Params` has the committee (`prMembers`), the threshold `prT`, the amend quorum `prQuorum`, the token pairs `prPairs`, the price list `prPrices` and two proof fields: `prFloor` (`tMin` <= `prT`, `tMin` = 2) and `prRange`. The price list has three tiers in cents: `pcLow` 1, `pcBuild` 5 and `pcExport` 20, each in the range 1 to 20. A closed `Params` with a proof field that the kernel cannot check gives `REFUSE_FLOOR`. `badPair` reads `prPairs`. | USER, S7 |
| 2026-11-07 | Q-S7-2 (a) (Recommended): an `Amendment` holds the new values without proofs. `amend p a c h` is a chain of checks `badCert`, `belowTMin` and `badPrice`; when all pass, a dependent check word gives the proof fields of the new `Params`. | USER, S7 |
| 2026-11-07 | Q-S7-3 (a) (Recommended): a `Statement` is `smFill` (a nullifier and a height) or `smAmend` (a `Root` digest and a height). A `Certificate` is a statement and a list of signatures (`Key`, `Sig`). `ctCheck` counts the distinct members (gavel-lang `checkCert`; `sgVrf` until S8b). `certOpen` gives `reject badCert` or `accept` of `open`. In `settle`, `badCert` comes before `belowFloor`. | USER, S7 |
| 2026-11-07 | Q-S7-4 (a) (Recommended): a `Log` is a list of entries in height order: `enFill`, `enAmend`, `enDeposit` and `enWithdraw`. The ledger is `Book`: for each key and token, the total credit and the total debit, and the spent nullifiers. A debit x is valid when debit + x <= credit (`amLe`). This closes OI-8. | USER, S7 |
| 2026-11-07 | Q-S7-5 (a) (Recommended): a fill with all checks at yes and a leg above the balance of its payer gives `accept Outcome unfilled`; the `Book` does not change and the nullifier is not spent. A withdraw above the balance gives `reject noFunds`, a tenth reason. | USER, S7 |
| 2026-11-07 | Q-S7-6 (a) (Recommended): `replay p l` folds the log with the `Params` in force, the next `Params`, the height of the last amendment and the `Book`; the first rejected entry rejects the replay. G1: the replay of the amendment entries is the fold of `amend` (by induction on the log). G2: a fill at the height of an amendment settles with the old `Params`. G3: `prFloor` (by the type). | USER, S7 |
| 2026-11-07 | Q-S7-7 (a) (Recommended): `n1Replay`: at each step of `replay`, a fill whose nullifier is in the spent set of the `Book` gives `reject reused` (one `cong`). The spend direction has closed `refl` rows. The ids stay `Nat`. | USER, S7 |
| 2026-11-08 | Q-S8-1 "(a) Full map (Recommended)": `REFUSE_READ` gives `REFUSE_DIV` on `Amount`, `Height` and `Window`, and `REFUSE_UNSEAL` on `Token`, `Nullifier`, `Key`, `Sig`, `Commitment` and `Root`. `REFUSE_CELL` maps to `REFUSE_FORMER`, `REFUSE_TIME` to `REFUSE_CLOCK`, `REFUSE_REC` to `REFUSE_PARTIAL`, `REFUSE_DATA` on a program family or cofamily to `REFUSE_BASE`, and `REFUSE_LAW` to `REFUSE_UNSEAL` or `REFUSE_OPEN`. The lexer gives `REFUSE_CKKS` on a decimal literal. `REFUSE_RETRO` has no cause (G2). Edits: section 7. | USER, S8 |
| 2026-11-08 | Q-S8-2 "(a) Both + kernel suffix (Recommended)": the text line and the JSON give the code of section 7. The text line ends with `(kernel K)` when the kernel code K is different, and the JSON has the field `kernel`. The mapped rows of `test/check/expect.txt` change. Edits: section 7, 10. | USER, S8 |
| 2026-11-08 | Q-S8-3 "(a) diagnostics + progress (Recommended)": one JSON line with `diagnostics` (zero or one object: `code`, `kernel`, `file`, `line`, `col`, `def`, `message`, `hint`) and `progress` (`parsed`, `typed`, `laws_discharged`, `laws_total`); the hints are in `src/front/codes.c`; exit 0, 1 or 2. Edits: section 7, 9. | USER, S8 |
| 2026-11-08 | Q-S8-4 "(a) Port, keep files (Recommended)": the tranche-lang printer with the comments, `langc fmt` and `langc fmt --check`, idempotence rows on each example and the domain; S8 does not rewrite the examples. Edits: section 9, 10. | USER, S8 |
| 2026-11-07 | The replay state record is `GovState` (constructor `govState`, fields `ssCur`, `ssNext`, `ssAt` and `ssBook`), because `State` is the guest program state (`REFUSE_STATE`). | USER, S7 session 3 |
| 2026-11-07 | The domain amendment operation is `govAmend`, because `amend` is a guest name in `examples/oneuse.lang`, `examples/regimes.lang` and 10 `test/check` files (rule R4, `REFUSE_NAME`). | USER, S7 session 4 |
| 2026-11-07 | A withdrawal certificate signs a third `Statement` form, `smWithdraw (smWdKey : Key) (smWdTok : Token) (smWdAmt : Amount) (smWdAt : Height)`, checked at `prT` (as a fill). The order in `gsWithdraw` is `badCert`, then `noFunds`. | USER, S7 session 5 |
| 2026-11-07 | The domain step and replay words are `govStep` and `govReplay` (rule R4: `step` is a state field in `test/check/state-fun-field.lang`, and `test/pathtool.c` makes a guest `def replay`). `amendFold` keeps its name. | USER, S7 session 5 |
| 2026-11-08 | Q-S8b-1 "(a) Port gavel 904d899 (Recommended)": `CORE_WORD` and `VAL_WORD` (uint32, wrapping), `Word` and the 14 operations `wAdd` to `wEq`, the literals `31w` and `0x1fw` with `LEX_WORD_RANGE`, Wasm i64 with a 32-bit mask, a `TERM_WORD` case in `langc fmt`, `REFUSE_WORD` by name for a guest program. Edits: section 4, 7, 9, 10. | USER, S8b |
| 2026-11-08 | Q-S8b-2 "(a) Rename family SwapWord (Recommended)": the domain family `Word` (the orbit words) becomes `SwapWord`, as gavel Q-S8-2 (a); the machine type is `Word`. | USER, S8b |
| 2026-11-08 | Q-S8b-3 "(a) gavel Montgomery CIOS (Recommended)": `Fe` is 8 Words, least significant first, Montgomery form inside (CIOS), canonical at each boundary; `Fr` and `Fp` are `Fe`; a prelude section at the end of `domain/domain.lang`. | USER, S8b |
| 2026-11-08 | Q-S8b-4 "(a) Native feMulC (Recommended)": a native field product `feMulC` (C CIOS in `eval.c`, a `Word` operation, a fixed charge of 64 steps); the guest `feMul` is the specification and oracle rows compare the two; the Wasm lowering calls the guest `feMul`; `ENTRY_PARAMS_MAX` 32 to 64 (gavel Q-S8a-4 (a), Q-S8a-5 (a)). | USER, S8b |
| 2026-11-08 | Q-S8b-5 "(a) circomlib t=3 (Recommended)": Poseidon on `Fr`, t = 3, alpha 5, R_F 8, R_P 57, the quorum Grain constants as 204 `Fe` literals in Montgomery form from a C generator under `gen/`; `Hash` carries an `Fr`; a message word is poseidon2(tag, poseidon2(x, y)). | USER, S8b |
| 2026-11-08 | Q-S8b-6 "(a) Schnorr G1 + keccak (Recommended)": closes OI-17. s*G = R + e*P, e = keccak256(R.x, R.y, P.x, P.y, m) mod r; `Key` holds a `Pt1`, `Sig` holds (R : `Pt1`, s : `Fr`); ASSUMED rows for Poseidon collision resistance, keccak as a random oracle and Schnorr EUF-CMA. | USER, S8b |
| 2026-11-08 | Q-S8b-7 "(a) Byte copy in src/crypto (Recommended)": quorum 8d1a113 `src/crypto/{u256,bn254,keccak,poseidon}.{c,h}`; `test/cryptotool.c` rows; `gate.sh` evaluates each row with `langc --kit eval`; the kit file `test/crypto/crypto.lang` is outside the Wasm set; `langc` does not link `src/crypto`. | USER, S8b |
| 2026-11-08 | Q-S8b-8 "(a) Two commits (Recommended)": S8b is `Word`, `REFUSE_WORD`, the rename, `Fr` and `Fp`, the oracle. S8b2 is `feMulC`, G1, Poseidon, keccak-256 and Schnorr; in `csHas` the `sgVrf` stand-in becomes the Schnorr check. The settle proof check keeps a stand-in until S8c; `badShare` comes with S8c. Edits: section 12. | USER, S8b |

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

### 1.4 Questions for the USER (S2)

Q-S2-1 The cut of the port. S2 takes the change of ratify-lang
55310c5..71f2c22 on the kernel paths: the Cubical TT kernel (ratify-lang
S2) and R0 with the no-trap Wasm writer (ratify-lang S3). The kit lowering
does not match the new core, so the kit gate rows need the writer. S2 takes
nothing from 71f2c22..a2322ef (ratify-lang S4 to S8: the committee, rule,
policy and log families, the crypto words, `src/crypto`, the certificate
check). (a) Keep this cut (Recommended). (b) Take only ratify-lang S2
(a46d5df). Then `langc build` refuses until S3, and the gate loses the IR,
Wasm and differential rows until S3. RULED (a).

Q-S2-2 The kit row `pick 2 4 9` wanted a trap. The ported writer sets a
Flag argument of 2 or more to 1 (`flag_norm` in `src/wasm.c`), so the row
now wants 4. (a) Keep the ported row (Recommended). (b) Restore the kit
trap row until S3, and take `flag_norm` in S3. RULED (a).

Q-S2-3 Ported comments and fixture heads cite the plan items of ratify-lang
and permit-lang (25 lines in 12 files, for example `Q-S3-3 (b)` and
`OI-12`). These numbers name other items in this plan. (a) Rewrite them in
S3 with the trap row (Recommended). (b) Rewrite them at S21. (c) Keep them
as source citations. RULED (a).

Q-S2-4 The port replaces the kit sample with the 30 kernel examples (the
kit sample moves to `examples/seed/inventory.lang`). The kernel prelude has
the permit-lang QIIT test family `Charter`, `Ratified`, `ratify` and
`adopt`. These names are kernel test data, not ratify-lang governance. (a)
Keep them until S5 changes the domain (Recommended). (b) Rename them now.
RULED (a).

### 1.5 Questions for the USER (S3)

Q-S3-1 Where the Q-S0-2 restriction applies. The port has these Nat words,
each with the visibility `VIS_INTERNAL` in `src/front/check.c`: `natAdd`
(it gives `Option Nat`, and an overflow is `none`), `natSub` (it gives 0
when y is more than x), `natDiv` and `natMod` (each takes a NonZero
divisor), `natEq` and `natLe`. There is no product word. Q-S0-2 (a) and
section 2 ask for compare, select, add and a product by a public scalar,
each with a static bound obligation, and for no division and no
subtraction on `Amount` and `Height`. (a) Keep the Nat words of the port
as kernel words for the prelude and the tests. S5 gives `Amount` and
`Height` their own words (compare, select, add, a product by a public
scalar), each with a static bound obligation that the checker discharges
(OI-7) (Recommended). (b) S4 takes `natSub`, `natDiv` and `natMod` out of
the guest scope and adds a product by a public scalar with a static bound
on Nat. RULED (a).

### 1.6 Questions for the USER (S4)

Q-S4-1 The refusal codes of S4. Sections 3 and 7 name `REFUSE_FORMER`,
`REFUSE_UNSEAL` and `REFUSE_OPEN`. The kernel of the port gives
`REFUSE_CELL` for a former outside the R1 table. It gives `REFUSE_LAW`
for an elim of `Sealed`, and for `open`, in a program def that is not a
law (rule O7). S4 keeps the kernel codes. (a) S8 (the refusal codes) maps
the kernel codes to the section 7 codes: `REFUSE_CELL` to
`REFUSE_FORMER`, and `REFUSE_LAW` on a sealed eliminator or on `open` to
`REFUSE_UNSEAL` or `REFUSE_OPEN` (Recommended). (b) S5 changes the codes
before the base types come. RULED (a).

Q-S4-2 The name `Outcome`. The DAO signature slot of the kernel is
`Outcome` (`SIGNATURE` in `src/front/check.c`). The design sum `Outcome`
needs the name. gavel-lang b2da56f changes the slot name to `DaoOutcome`
(3 sites in the checker, 72 fixture lines). (a) S6 takes the new name with
the sums `Outcome` and `Verdict` (Recommended). (b) S5 takes the new name.
RULED (a).

Q-S4-3 `open` in a law. `open` is law-only: a program def that names it
must be a law, and a law has no run time. Section 7 says that
`REFUSE_OPEN` is "`open` outside `settle` or the tally". (a) A program law
can name `open` (the law `openTwo` of `examples/enc.lang`). `REFUSE_OPEN`
is `open` in a def that is not a law, `settle` or the tally
(Recommended). (b) Only the domain laws can name `open`, and the checker
refuses a program law that names it. RULED (a).

Q-S4-4 The parameter of `Sealed`. S4 declares
`sealed family Sealed (A : Type 0)`, so `Enc` takes any type of universe
0. The gavel-lang R1 table admits a base type only. (a) S5 limits the
parameter of `Sealed` to the base types of design sec 3 (Recommended).
(b) Keep any type of universe 0. RULED (a).

### 1.7 Questions for the USER (S5)

Q-S5-1 (OI-7) The static bound of add and product on `Amount` and
`Height`. The domain types are unary families (the gavel-lang b2da56f
`Amount` shape), so the kernel add and product cannot overflow; a bound
matters at a fixed-width target (T3, T4). (a) Checker constants:
`Amount` 64 bits, `Height` 32 bits, target word 128 bits. The checker
gives each `Amount` or `Height` term of a program an upper bound (an
input at its width, a literal its value, an add the sum, a product the
product) and gives `REFUSE_BOUND` when a bound passes the target word
(Recommended). (b) A closed literal above its width gives
`REFUSE_BOUND`; the bound of an open term is a T3 and T4 obligation at
S13 and S14. RULED (a).

Q-S5-2 The scalar of the product. The affine test multiplies a public
`Height` (window, t) by an `Amount` (o, startOut, minOut). (a)
`amScale : Height -> Amount -> Amount` and
`htScale : Height -> Height -> Height`. No product of two amounts. A
sealed amount is scaled through `encMap`, so the scalar stays public
(Recommended). (b) `amScale : Nat -> Amount -> Amount` and
`htScale : Nat -> Height -> Height`. RULED (a).

Q-S5-3 The shape of Q-S4-4 (a). (a) `Sealed` keeps its parameter. In a
program, `Sealed X`, `Enc X` or `seal X` needs X in the list `Amount`,
`Height`, `Token` and `Sum Unit Unit` (the sealed Bool of `cross`).
Another X gives `REFUSE_CELL`, and S8 maps it to `REFUSE_FORMER`
(Q-S4-1 (a)) (Recommended). (b) A sealed family for each base type
(`SealedAmount`, `SealedHeight`, `SealedToken`, `SealedBool`) with no
parameter, with `encBind`, `encMap` and `open` for each. RULED (a).

Q-S5-4 The base types of S5. (a) `Amount`, `Height`, `Window` (start,
length > 0), `Token` (decidable equality), `Nullifier`, and the opaque
`Key`, `Sig`, `Commitment` and `Root`. The records `Intent`, `Quote`,
`Transfer` and `Fill` go to S6 with the operations; `Statement` and
`Certificate` go to S7 (Recommended). (b) S5 also builds the four
records. RULED (a).

Q-S5-5 A direct read of a base family in a program. An elim, a fold or
a projection of a base family of design sec 3, or a successor on a term
that is not a literal, can write a subtraction, a division or a read of
an opaque value. (a) One new kernel code `REFUSE_READ`; S8 maps it to
the codes of section 7 (`REFUSE_DIV` for `Amount` and `Height`), the
shape of Q-S4-1 (a) (Recommended). RULED (a).

Q-S5-6 The bound of a binder in the interval rule. (a) A variable that
is not a parameter of the program def (a lambda binder in the body, a
case binder of an elim or a fold such as `ih`) has the bound 2^128 - 1
(the word). Thus an elim loop cannot hide an iterated product, and
`examples/enc.lang` maps with `amMax`, not `amAdd` (Recommended).
RULED (a).

Q-S5-7 The bound across a call. (a) A program def that gives an
`Amount` or a `Height` keeps its bound (its parameters at their widths)
in a table. At a call, each `Amount` or `Height` argument must have a
bound at most the width of its parameter, else `REFUSE_BOUND`. A program
def with an `Amount` or `Height` parameter that is used as a value gives
`REFUSE_BOUND` (Recommended). RULED (a).

Q-S5-8 The name `Window`. The port has a test block family `Window`, and
design sec 3 names a base type `Window`. (a) The test block family
becomes `Shutter`; `Window` is the design type (Recommended). RULED (a).

Q-S5-9 A word operation as a value. `amAdd`, `htAdd`, `amScale` or
`htScale` used as a value (not a call with 2 arguments) gives
`REFUSE_BOUND` (built; `test/check/bound-word.lang`). Through a function
variable, an elim loop can iterate an add with no bound. (a) Keep the
refusal (Recommended). (b) Admit it with another bound rule. RULED (a).

Q-S5-10 Guest values of `Token`, `Nullifier`, `Key`, `Sig`, `Commitment`
and `Root`. A guest program cannot write one: the field is `Nat`, and a
`Nat` literal is a kit item (`REFUSE_KIT`). (a) These values come only
as inputs (a parameter or the state); no change (Recommended). (b) A
domain word or a literal form for each. RULED (a).

### 1.8 Questions for the USER (S6)

Q-S6-1 The bound of a record field. Under Q-S5-6 (a), a projection of a
record field at an `Amount` or `Height` slot is not a parameter, so it has
the word bound (2^128 - 1). Then `amScale t (startOut i)` of an `Intent`
parameter `i` gives `REFUSE_BOUND`. (a) The interval rule gives a
projection of a record field of type `Amount` or `Height` its width when
the record is a parameter of the def (Recommended). (b) The records hold
the fields at the word, and a program bounds each field with `amMin`
against a literal before a product. RULED (a).

Q-S6-2 The bound of a `Window` field. `wLen (itWindow i)` of an `Intent`
parameter `i` falls to the word bound plus one in `bound_window`, and
`itWindow i` as a `Window` argument is above the width. (a) A projection
of a `Window` field of a parameter record has the widths of a `Window`
parameter (start 2^32 - 1, length 2^32) (Recommended). (b) A program
reads a window only from a `Window` parameter. RULED (a).

Q-S6-3 The two sealed values of `cross`. Design sec 4.1 reads the decay
test as one affine circuit without a bind. (a) A domain word `encMap2`
maps two sealed values; the domain defines it by `encBind`, and T3 and T4
read it as one circuit (Recommended). (b) `cross` uses `encBind`, and the
text "without bind" of design sec 4.1 is a target note. RULED (a).

Q-S6-4 The spend direction of N1. `nfId` is a `Nat`, `Nat` has no elim,
and `natEq n n` of a variable `n` does not reduce, so "after a settle the
nullifier is spent" has no general proof. (a) N1 is N1a (a spent
nullifier gives `reject Outcome reused`, DISCHARGED) and closed `refl`
rows and eval rows for the spend direction; the full replay form comes in
S7 (Recommended). (b) `nfId` and `tokId` become `Size`. Then a guest
program can make `nullifier sz0` with no `Nat` literal, which breaks
Q-S5-10 (a), and the IR form of `Token` and `Nullifier` changes.
RULED (a).

Q-S6-5 The depth of S2. (a) S2 is the path from the open of `cross` to
`decayAt` of the open values (DISCHARGED by elim), and closed rows for
the arithmetic reading (the floor is `startOut` before the start and
`minOut` after the end) (Recommended). (b) Also the elim lemmas of the
arithmetic reading (`amScale` over `htAdd`, the cancel of `amLe` over
`amAdd` and over `amScale`). RULED (a).

Q-S6-6 The stand-ins of `settle`. The ledger (S7 `replay`), the certified
open (S7) and the proof check (S8b) come in later slices. (a) The S6
`settle` uses `Spent` for the ledger, the plain `open` for the certified
open, `sgVrf` for the proof check and `badPair` for equal tokens until
the S7 `Params`; S7 and S8b replace each (Recommended). (b) `settle`
waits for S7. RULED (a).

### 1.9 Questions for the USER (S7)

S7 is governance (section 12): `Params`, `amend`, `replay`, the laws
G1 to G3, and the S6 stand-ins that S7 replaces (Q-S6-6 (a)).

Q-S7-1 The shape of `Params`. (a) A record with the committee
(`prMembers`, a list of `Key`), the threshold `prT : Height`, the amend
rule (`prQuorum : Height`, the count of signatures for an amendment),
the token pairs `prPairs` and the price list `prPrices`, with two proof
fields: `prFloor` (t_min <= `prT`) and `prRange` (each price in its
range). `tMin` is a domain constant, 2. The price list has three tiers
in cents (`Amount`): `pcLow` (check, fmt, explain, list-base and
show-law; genesis 1), `pcBuild` (genesis 5) and `pcExport` (genesis
20). Each price is in the range 1 to 20. A closed `Params` with a proof
field that the kernel cannot check gives `REFUSE_FLOOR` (section 7).
`badPair` reads `prPairs`. The decay window bounds and the deadlines
stay in the intent (an open item). (Recommended) (b) The same, with one
price for each route of `langc serve` (eight prices). (c) The same, with
the prices in units of 10^-6 USDC (10000 to 200000); eval of these
unary values is slow. RULED (a).

Q-S7-2 `amend`. (a) An `Amendment` holds the new values without
proofs: the members, t, the quorum, the pairs and the prices.
`amend p a c h` is a chain of checks: `badCert` (c is not a certificate
of the amendment statement at h by `prQuorum p` members of `p`),
`belowTMin` (t below `tMin`) and `badPrice` (a price outside its
range). When all checks pass, `amend` gives `accept` with the new
`Params`; a dependent check word gives the proof fields from the passed
checks. (Recommended) (b) An `Amendment` holds a full `Params` (the
gavel-lang shape). `amend` checks only the certificate; `belowTMin` and
`badPrice` come from the host decode of an amendment message. RULED (a).

Q-S7-3 `Statement`, `Certificate` and the certified open. (a) A
`Statement` is `smFill` (a nullifier and a height: open the two legs of
that intent at that height) or `smAmend` (a `Root` digest of the
amendment and a height). A `Certificate` is a statement and a list of
signatures (`Key`, `Sig`). `ctCheck m k s c` says yes when c is on the
statement s and has the signatures of at least k distinct members of m
(the gavel-lang `checkCert` on the S5 base; `sgVrf` stays the stand-in
until S8b). The certified open `certOpen p c x` gives `reject badCert`
when the check fails, else `accept` of `open x`. In `settle`, the
`badCert` check comes before `belowFloor`. (Recommended) (b) A
`Certificate` is an opaque id with a stand-in check, as `sgVrf`; S8b
builds the list. RULED (a).

Q-S7-4 The `Log`, the ledger and OI-8. (a) A `Log` is a list of entries
in height order: `enFill` (an intent, a quote, a certificate, a
height), `enAmend` (an amendment, a certificate, a height),
`enDeposit` (a key, a token, an amount, a height: the escrow on chain
credits the key) and `enWithdraw` (a key, a token, an amount, a
certificate, a height). The ledger is `Book` (the name `Ledger` is a
guest def in `examples/oneuse.lang`): for each key and token, the total
credit and the total debit, and the spent nullifiers. A debit of x is
valid when debit + x <= credit (`amLe`), with no subtraction (section
2). This closes OI-8. (Recommended) (b) The same, with a balance for
each key and token and a total subtraction word that gives a
`Verdict Amount`. RULED (a).

Q-S7-5 The meaning of `unfilled`, and a debit above the balance. (a)
`unfilled` is the outcome of a fill with all checks at yes and a leg
above the balance of its payer: `settle` gives `accept Outcome
unfilled`, the `Book` does not change, and the nullifier is not spent
(a new quote can fill the intent). A withdraw above the balance gives
`reject` with a tenth reason `noFunds` (section 2 gets ten reasons).
(Recommended) (b) A leg or a withdraw above the balance gives
`reject noFunds`; `unfilled` is an intent past its deadline with no
fill, which `replay` reports. (c) Nine reasons stay: a fill above the
balance is `unfilled`, and a withdraw above the balance has no effect. RULED (a).

Q-S7-6 `replay` and the statements of G1 to G3. (a) `replay p l` folds
the log from height 0 with a state: the `Params` in force, the next
`Params`, the height of the last amendment and the `Book`. An entry at
a height above that height first puts the next `Params` in force. Each
entry is checked with the `Params` in force at its height. The first
rejected entry rejects the whole replay with its reason (a certified
log holds only valid entries). G1: the replay of the amendment entries
of a log gives the same `Params` as the fold of `amend` over the log
(by induction on the log). G2: a fill at the height of an amendment
settles with the old `Params` (by the height order). G3: each `Params`
has t >= t_min (`prFloor`, by the type). (Recommended) (b) The same,
but a rejected entry has no effect and `replay` goes on. RULED (a).

Q-S7-7 N1 in the replay form. (a) Two parts. `n1Replay`: at each step
of `replay`, a fill whose nullifier is in the spent set of the `Book`
gives `reject reused` (one `cong`, as N1a). The spend direction (after
a settled fill, the nullifier is in the spent set) has closed `refl`
rows, because `nfEq n n` does not reduce for a variable `n` (Q-S6-4).
The ids stay `Nat`. (Recommended) (b) `Nullifier` gets a unary id with
an equality that reduces by induction, so the spend direction is a
general law. RULED (a).

### 1.10 Questions for the USER (S8)

S8 is the refusal codes, `check --json` and `langc fmt` (section 12).
The survey (2026-11-08): the checker gives 20 `REFUSE_` codes and the
kernel codes (`TYPE_MISMATCH`, `PATH_END` and others). Of the thirteen
codes of section 7, it gives only `REFUSE_PARTIAL`, `REFUSE_BOUND`,
`REFUSE_FLOOR` and `REFUSE_LAW`. `langc check` has no `--json` and
`langc` has no `fmt`. tranche-lang (2b14b52) has both
(`src/front/fmt.c`, `src/front/json.c`, a comment list in the lexer).

Q-S8-1 The code map. (a) `REFUSE_READ` on `Amount`, `Height` or
`Window` gives `REFUSE_DIV` (a `Window` is two heights). On `Token`,
`Nullifier`, `Key`, `Sig`, `Commitment` or `Root` it gives
`REFUSE_UNSEAL` (a read of an opaque value). The kernel codes with the
cause of a code of section 7 map too: `REFUSE_CELL` to
`REFUSE_FORMER`, `REFUSE_TIME` (`now`, `clock`, `timestamp`) to
`REFUSE_CLOCK`, `REFUSE_REC` to `REFUSE_PARTIAL`, and `REFUSE_DATA` on
a program family or cofamily to `REFUSE_BASE`. `REFUSE_LAW` on a sealed
eliminator gives `REFUSE_UNSEAL`, and on `open` `REFUSE_OPEN`
(Q-S4-1 (a)). The lexer gives `REFUSE_CKKS` on a decimal literal such
as `1.5` (now `LEX_CHAR`). `REFUSE_RETRO` has no cause: an amendment
applies from its height + 1 by construction (`gsCur`, G2), and section
7 says so. The other kernel codes stay. (Recommended) (b) Only the map
of section 7, with `REFUSE_UNSEAL` for the six id types.
`REFUSE_BASE`, `REFUSE_CLOCK`, `REFUSE_RETRO` and `REFUSE_CKKS` wait
for a later slice. (c) As (b), but `REFUSE_READ` on the six id types
stays, as a fourteenth code. RULED (a).

Q-S8-2 Where the map shows. (a) In both outputs. The text line
`langc: CODE: ...` gives the code of section 7 and ends with
`(kernel REFUSE_READ)` when the kernel code is different. The JSON
object has the field `kernel`. The rows of `test/check/expect.txt` with
a mapped code change to the new code (about 20 rows), and a gate row
checks the kernel suffix. (Recommended) (b) Only in `check --json`; the
text output and the 198 rows stay. (c) In both outputs, with no kernel
code. RULED (a).

Q-S8-3 The JSON shape. (a) One line on stdout:
`{"diagnostics":[...],"progress":{"parsed":N,"typed":N,"laws_discharged":N,"laws_total":N}}`,
the key order of tranche-lang without its `bounds`, `fails` and
`tables`. The checker stops at the first refusal, thus the list has
zero or one object, with the fields `code`, `kernel`, `file`, `line`,
`col`, `def`, `message` and `hint`. The hint is one fixed sentence for
each code, from a table in `src/front/codes.c`. Exit 0 (ok), 1
(refused: the JSON on stdout, no text on stderr) or 2 (usage, IO or
OOM: no JSON). (Recommended) (b) The same, without `progress`. (c) The
same, and `check --json` also lists each code of section 7 with its
hint. RULED (a).

Q-S8-4 `langc fmt`. (a) Port the tranche-lang printer: the lexer keeps
the `--` comments and the parser records the end of each declaration.
`langc fmt PROG` prints the canonical text, and `langc fmt --check
PROG` exits 1 when PROG is not canonical. Gate rows: for each file of
`examples/` and for the domain, the canonical text of the canonical
text is the same, and a C regression in `test/fronttool.c` shows the
same declarations before and after. S8 does not rewrite the examples.
(Recommended) (b) As (a), and S8 rewrites `examples/` and
`domain/domain.lang` to the canonical text, so that `fmt --check`
passes on each file. (c) As (a), but `fmt` drops the comments. RULED (a).

### 1.11 Questions for the USER (S8b)

S8b is the crypto prelude part 1 (section 12). The sources are gavel-lang
904d899 (`Word`, `Fr` and `Fp`, the C oracle), the gavel-lang S8a
rulings (the curve, Poseidon and Schnorr; not committed in gavel) and
quorum-lang 8d1a113 `src/crypto`. tranche-lang `U256` is checked
256-bit arithmetic with division, thus it is not a source for `Word`.

Q-S8b-1 The `Word` port. (a) Port gavel 904d899: `CORE_WORD` and
`VAL_WORD` (uint32, wrapping, O(1)), `Word` and the 14 operations
`wAdd` to `wEq` (`wLt` and `wEq` give `Sum Unit Unit`), the literals
`31w` and `0x1fw` with `LEX_WORD_RANGE`, `IR_SCALAR_WORD` lowered to
Wasm i64 with a 32-bit mask, a `TERM_WORD` case in `langc fmt`, and
`REFUSE_WORD` by name when the program is a guest program (`guest()`,
the gavel `word_refused`). (Recommended) (b) As (a), but `REFUSE_WORD`
by origin: a guest call to a prelude def that has `Word` in its type is
also refused. RULED (a).

Q-S8b-2 The name `Word`. The domain has `family Word : Size -> Type 0`
(the orbit words, `actW`). (a) Rename the family to `SwapWord`, as gavel
Q-S8-2 (a); the machine type is `Word`, as sections 4 and 7 say.
(Recommended) (b) Keep the family; the machine type is `Word32`, and
sections 4 and 7 change. RULED (a).

Q-S8b-3 The field. (a) Copy the gavel field defs into a prelude section
at the end of `domain/domain.lang`: `Fe` is 8 Words, least significant
first, Montgomery form inside (CIOS), canonical at each boundary; `Fr`
and `Fp` are `Fe`. (Recommended) (b) As (a), with the canonical form
only (schoolbook product and reduction): more steps, no Montgomery
constants. RULED (a).

Q-S8b-4 The run cost of the curve. A G1 scalar product with a 256-bit
scalar is about 83M steps under the evaluator (from the gavel S8a
measure of 5.19M steps for a 16-bit scalar); the fuel is 20M. (a) As
gavel Q-S8a-4 (a) and Q-S8a-5 (a): a native field product `feMulC` (C
CIOS in `eval.c`, visible as a `Word` operation, a fixed charge of 64
steps); the guest `feMul` stays the specification, oracle rows compare
the two, and the Wasm lowering calls the guest `feMul`.
`ENTRY_PARAMS_MAX` goes from 32 to 64. (Recommended) (b) Guest terms
only: curve and signature rows use small scalars under the evaluator,
and full scalars run in the Wasm module only. RULED (a).

Q-S8b-5 Poseidon and `Hash`. (a) circomlib Poseidon on `Fr` (t = 3,
alpha 5, R_F 8, R_P 57), the Grain constants of quorum `poseidon.c`
written once as 204 `Fe` literals in Montgomery form by a C generator
under `gen/`. `Hash` carries an `Fr`. A message word is
poseidon2(tag, poseidon2(x, y)). (Recommended) (b) As (a), with t = 5
for the intent tuple; quorum has t = 3 only, thus no oracle. RULED (a).

Q-S8b-6 The signature scheme (OI-17). (a) Schnorr on BN254 G1 with a
keccak-256 challenge, as gavel Q-S8-6 (a) and ratify-lang: s*G = R +
e*P, e = keccak256(R.x, R.y, P.x, P.y, m) mod r. `Key` holds a `Pt1`,
`Sig` holds (R : `Pt1`, s : `Fr`). ASSUMED rows: Poseidon collision
resistance, keccak as a random oracle, Schnorr EUF-CMA. (Recommended)
(b) As (a), with a Poseidon challenge: no keccak, and cheaper in a Noir
relation at T2. (c) FROST on secp256k1 with SHA-256, as quorum
`frost.c`: a second field and SHA-256. RULED (a).

Q-S8b-7 The C oracle. (a) A byte copy of quorum 8d1a113
`src/crypto/{u256,bn254,keccak,poseidon}.{c,h}` in `src/crypto/`;
`test/cryptotool.c` writes the rows; `gate.sh` evaluates each row with
`langc --kit eval` and prints `crypto oracle: N rows, M mismatches`; the
field and curve rows are in a kit file `test/crypto/crypto.lang` outside
the Wasm set; `langc` does not link `src/crypto`. (Recommended) (b)
Compile from a sibling `../quorum-lang` checkout at 8d1a113, no copy; a
fresh clone of barter-lang cannot run the gate. RULED (a).

Q-S8b-8 The cut and the stand-ins. (a) Two commits, as gavel Q-S8-1 (a).
S8b: `Word`, `REFUSE_WORD`, the rename, `Fr` and `Fp`, the oracle.
S8b2: the native field product, G1, Poseidon, keccak-256, Schnorr; in
`csHas` the `sgVrf` stand-in becomes the Schnorr check. The settle proof
check keeps a stand-in until the S8c Noir verifier, and `badShare` comes
with the S8c share check. (Recommended) (b) One commit with all of (a). RULED (a).

## 2. The reading of R0: no runtime error

R0 says that langc accepts no program that can lead to a runtime error.
The reading ports permit-lang, ratify-lang and gavel-lang: every function is
total, and every failure in the domain is a value.

- A failed proof check, a bad decryption share, an expired intent, a reused
  nullifier, a wrong token pair, a quote below the floor, a bad certificate,
  an amendment below t_min, an amendment with a price outside its range
  and a withdrawal above the balance are each a `reject Reason` of
  `Verdict`. A fill with a leg above the balance of its payer is
  `accept Outcome unfilled` (Q-S7-5 (a)). None is a trap.
- Reasons (Q-S0-1 (a), Q-S1-6 (a), Q-S7-5 (a)), ten: `bad_proof`,
  `bad_share`, `expired`, `reused`, `bad_pair`, `below_floor`,
  `bad_cert`, `below_t_min`, `bad_price`, `no_funds`. The domain names
  are `badProof`, `badShare`, `expired`, `reused`, `badPair`,
  `belowFloor`, `badCert`, `belowTMin`, `badPrice` and `noFunds`.
- The crypto is in the module (Q-S0-17 (b)): guest terms over the total
  `Word` type of section 4. A proof, a share, a signature or an MPC output
  that does not check is a `reject`, never a trap.
- Arithmetic on `Amount` and `Height` is total (Q-S0-2). No division and no
  subtraction. Each add and each product by a public scalar has a static
  bound obligation that the checker discharges (OI-7). The S2 port has
  the total Nat words of gavel-lang 86c2f30: the src tree of gavel-lang
  86c2f30 is equal to the src tree of this port, so S3 ports no C code.
  Q-S3-1 (a): the Nat words stay kernel words, and S5 gives `Amount`
  and `Height` their own words (section 4).
- Host input that does not decode is a `reject`, never a trap (permit-lang
  `test/notraptool.c` precedent).
- S3 changed the kit row "a trap is equal to a trap" (Q-S0-33 (a)): a
  `trap` line on either side of the differential is a FAIL
  (`test/gate.sh`).

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

S4 (2026-11-06) builds `Enc` and `open` in `domain/domain.lang` over rule
O7 of the kernel: `sealed family Sealed (A : Type 0) := seal (sealOf : A)`,
`Enc A` is `Sealed A`, `encBind` and `encMap` are the bind and the map,
`open` is a `law def`, and the laws `openSeal` and `openBind` are paths. A
program def that eliminates `Sealed` or names `open` and is not a law
gives `REFUSE_LAW` (Q-S4-1). The rows: `examples/enc.lang`,
`test/check/unseal-wrong.lang`, `test/check/open-wrong.lang` and two eval
rows.

S6 (2026-11-07) builds the swap of design sec 4 at the end of
`domain/domain.lang`. The records are `Intent`, `Quote`, `Transfer` and
`Fill`; each field name has the prefix of its record (`it`, `qt`, `tr`,
`fl`). `amtIn` and `minOut` of an intent and the quote output are
`Sealed Amount`, and `startOut` is public. The decoy index is a
`Sealed Height` (OI-6). The sums:

- `Verdict A` is `accept (vdVal : A) | reject (vdWhy : Reason)`. The
  value field is `vdVal`, because `vdOk` is the name of a law.
- `Outcome` is `settled (stIn : Transfer) (stOut : Transfer) |
  unfilled`. `unfilled` comes with the replay of S7. The DAO slot
  `Outcome` of the kit is now `DaoOutcome` (Q-S4-2 (a)).
- `Reason` has nine reasons. S6 uses `reused`, `expired`, `badPair`,
  `badProof` and `belowFloor`. `badCert`, `belowTMin` and `badPrice`
  come with S7, and `badShare` comes with S8b.
- `Spent` is a list of nullifiers, a stand-in for the ledger of S7
  (Q-S6-6 (a)).

The words: `vdCheck A s w v` is `v` when the flag `s` says yes, else
`reject A w`. `spHas n l` says yes when the nullifier `n` is in `l`.
`sgVrf` is a stand-in for the S8b proof check. `encMap2` maps a word of
two arguments over two sealed values (Q-S6-3 (a)). `wClamp` clamps a
height into a window. `decayTest` is the decay floor of design sec 4
with no subtraction and no division; each side is below 2^98, in the
word. `decayAt` applies it to an intent at a height, and `cross` is its
sealed form over the quote output and `minOut`. `settleOut` gives the
two transfers of a fill.

`settle l i q h` is a chain of five checks, with the first check
outermost: `reused` (`spHas`), `expired` (the deadline), `badPair`
(equal tokens), `badProof` (`sgVrf`) and `belowFloor` (the open of
`cross`). Each tail of the chain is a named def: `settleRest` (checks 2
to 5), `settlePair` (checks 3 to 5), `settleProof` (checks 4 and 5) and
`settleFloor` (check 5). When all checks pass, the verdict is
`accept Outcome (settleOut i q)`. In S6, `settle` opens with the plain
`open`; S7 puts a certificate before the open (Q-S6-6 (a)).

S7 (2026-11-07) builds the governance of design sec 5 at the end of
`domain/domain.lang`, after `bkHas`: a domain word goes after each type
and word that it names. The records and sums:

- `Params` is `params` of `prMembers` (a `Members` list of `Key`),
  `prT` (the threshold), `prQuorum` (the count of signatures for an
  amendment), `prPairs` (a `Pairs` list of `Pair`), `prPrices` and the
  two proof fields `prFloor` (`htLe tMin (prT p)` is yes; `tMin` is 2)
  and `prRange` (`pcIn (prPrices p)` is yes). `Prices` is `prices` of
  the three tiers `pcLow`, `pcBuild` and `pcExport` in cents; `pcTier`
  says yes for 1 to `pcMax` (20). The `Pair` constructor is
  `tradePair`, because `pair` is a kit word.
- `Statement` is `smFill` (a nullifier and a height), `smAmend` (a
  `Root` digest and a height) or `smWithdraw` (a key, a token, an
  amount and a height; USER, S7 session 5). `Certificate` is
  `certificate` of a statement and `Sigs` (a list of `Key` and `Sig`).
  `ctCheck m k s c` says yes when the statement of `c` is `s` (`smEq`)
  and `ctCount` finds at least `k` distinct members of `m` with a
  signature (`sgVrf` stays the S8b stand-in). `certOpen` gives
  `reject badCert` or `accept` of `open`.
- `Amendment` holds the new members, t, quorum, pairs and prices
  without proofs, and the `Root` digest `adRoot`.
- `Entry` is `enFill` (an intent, a quote, a certificate and a height),
  `enAmend` (an amendment, a certificate and a height), `enDeposit` (a
  key, a token, an amount and a height) or `enWithdraw` (a key, a
  token, an amount, a certificate and a height). `Log` is a list of
  entries.
- `Book` is `book` of `Accts` (a journal of `acct k t credit debit`
  rows) and `Spent` (the spent nullifiers). `bkCredit` and `bkDebit`
  add a row, and `acIn` and `acOut` add the credits and the debits of
  one key and one token. `bkCan k t x b` says yes when `acOut` + x <=
  `acIn`, with no subtraction (Q-S7-4 (a)). `bkSpend` and `bkHas` add
  and find a nullifier.
- `GovState` is `govState` of `ssCur` (the `Params` in force), `ssNext`
  (the next `Params`), `ssAt` (the height of the last amendment) and
  `ssBook` (USER, S7 session 3).
- `Reason` gets a tenth reason, `noFunds` (Q-S7-5 (a)).

The compare words of S7 use `amLeLin`, a case on `amCase` (about x * y
eval steps). The eval of `amLe` computes each recursive value first, so
its nested elim costs about C(x + y, x) steps (`amLe 20 20` is above
the arena limit). `amLe` stays for its laws.

`govAmend p a c h` (Q-S7-2 (a)) is a chain of three checks: `badCert`
(`ctCheck` at `prQuorum p` on `smAmend (adRoot a) h`), `belowTMin` and
`badPrice`. When all pass, it gives `accept` of the new `Params`, and
`vdWith` gives the two proof fields from the paths of the passed
checks.

`settle p b i q c h` is a chain of six checks, with the first check
outermost: `reused` (`bkHas` on the `Book`), `expired`, `badPair`
(`paOk`: the token pair of the intent is in `prPairs p`), `badProof`
(`sgVrf`), `badCert` (`ctCheck` at `prT p` on `smFill (itNf i) h`) and
`belowFloor` (the open of `cross`). The tails are `settleRest`,
`settlePair`, `settleProof`, `settleCert` and `settleFloor`. When all
checks pass, `settleFunds b i q` gives `settleOut i q` when each leg is
in the balance of its payer (`fdFlag`, two `bkCan`), else `unfilled`
(Q-S7-5 (a)). The guest-call bound check of the checker names the new
tails `settleCert` and `settleFunds` (`bound_domain`).

The S7 staged review also guards `gsFill`, including its use as a
function value. An `Entry` constructor checks its base fields and the
bounded fields of its nested records before a fill can enter a `Log`.
Thus `govStep`, `rpGo` and `govReplay` cannot hide a widened sealed quote
or intent payload from settlement. Seven refusal fixtures cover these
paths, the fill helper as a value and an entry height above its width;
all seven were accepted by the original staged checker. The forwarding
and construction examples in `examples/swap.lang` retain valid input
widths.

The replay (Q-S7-6 (a)): `vdThen` binds a `Verdict`. `gsCur s h` is
`ssCur s` when h <= `ssAt s`, else `ssNext s`. `govStep s e` checks the
entry `e` with the `Params` in force at its height: `gsFill` (the
`settle` of S7; on `settled` the `Book` pays the two legs and spends
the nullifier, on `unfilled` the `Book` does not change), `gsAmend`
(`govAmend`; the result is the next `Params`, in force above the height
of the entry), `gsDeposit` (a credit) and `gsWithdraw` (`badCert` on
`smWithdraw` at `prT`, then `noFunds`, then a debit). `rpGo` folds a
log with `govStep` and `vdThen`: the first `reject` rejects the replay.
`govReplay p l` starts at `govState p p htZero bkZero`. `lgAmends`
keeps the amendment entries of a log, and `amendFold p l` folds the
amendment step (`afStep`, `afGo`) over the log and gives the next
`Params` of the last state. The names are `govAmend`, `govStep` and
`govReplay`, not `amend`, `step` and `replay`, because those are guest
names in `examples/` and `test/` (rule R4; USER, S7 sessions 4 and 5).

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

S8b (2026-11-08) builds the first part of the prelude. `Word` is a
kernel type (`CORE_WORD`, `VAL_WORD`; Q-S8b-1 (a)): a uint32 that
wraps. Its 14 operations are `wAdd`, `wSub`, `wMul`, `wMulHi`, `wAnd`,
`wOr`, `wXor`, `wNot`, `wShl`, `wShr`, `wRotl`, `wRotr`, `wLt` and
`wEq`. A compare gives `Sum Unit Unit`: `inr tt` when the test holds.
A literal is `31w` or `0x1fw`; a literal above 0xffffffff gives
`LEX_WORD_RANGE`. The Wasm form is an i64 with a 32-bit mask, and the
entry keeps the low 32 bits of a `Word` argument. The domain family
`Word` (the orbit words) is now `SwapWord` (Q-S8b-2 (a)). The prelude
section at the end of `domain/domain.lang` (Q-S8b-3 (a)) gives `Fe`: 8
`Word`s, the least significant first, in Montgomery form (CIOS) inside
and canonical at each boundary. `FeMod` holds a modulus. `Fr` and `Fp`
are `Fe` with the moduli `frMod` and `fpMod` of BN254. `feAdd`,
`feSub`, `feMul`, `fePow` and `feInv` are the field operations. The
native field product `feMulC`, G1, Poseidon, keccak-256 and the Schnorr
check come with S8b2 (Q-S8b-8 (a)).

S5 (2026-11-06) builds the base types of design sec 3 at the end of
`domain/domain.lang` (Q-S5-4 (a)). `Amount` and `Height` are unary
families (zero and successor). The words of `Amount` are `amLe`,
`amSelect`, `amMax`, `amMin`, `amAdd` and `amScale`; the words of
`Height` are `htLe`, `htSelect`, `htAdd` and `htScale`. The scalar of a
product is a public `Height` (Q-S5-2 (a)). No word subtracts or divides.
`Window` has a start and a length above 0 (`wStart`, `wLen`, the law
`wLenPos`). `Token` and `Nullifier` have a decidable equality (`tkEq`,
`nfEq`). `Key`, `Sig`, `Commitment` and `Root` are opaque. The laws of
the words are paths by elim. The test block family of the port is now
`Shutter` (Q-S5-8 (a)). The QIIT test family of Q-S2-4 (a) stays as
kernel test data.

The checker has three S5 rules. Each applies to a guest program only:

- The read rule (Q-S5-5 (a)). A program reads a base type only through
  its words. An elim, a fold or a projection of a base family, or
  `amSucc` or `htSucc` on a term that is not a literal, gives
  `REFUSE_READ`.
- The `Sealed` rule (Q-S4-4 (a), Q-S5-3 (a)). `Sealed X`, `Enc X`,
  `seal X`, `encBind` and `encMap` need X in `Amount`, `Height`, `Token`
  and `Sum Unit Unit`. Another X gives `REFUSE_CELL`.
- The interval rule (OI-7; Q-S5-1, Q-S5-6 and Q-S5-7 (a)). The widths
  are 2^64 - 1 for `Amount` and 2^32 - 1 for `Height`; the target word
  is 2^128 - 1. The checker gives each `Amount` or `Height` term of a
  program def an upper bound: a parameter of the def its width, another
  binder (a lambda binder in the body, a case binder such as `ih`) the
  word, a literal its value, an add the sum, a product the product,
  `amMax`, `amMin` and a select the larger bound. A bound above the word
  gives `REFUSE_BOUND`. A program def that gives an `Amount` or a
  `Height` keeps its bound in a table, and a call with all its arguments
  takes that bound. At a call, an `Amount` or `Height` argument with a
  bound above the width of its parameter gives `REFUSE_BOUND`. A word
  operation (`amAdd`, `htAdd`, `amScale`, `htScale`, `wLen`) or a program def
  with an `Amount`, `Height` or `Window` parameter that is used as a value, not in
  a call with all its arguments, gives `REFUSE_BOUND` (Q-S5-9 (a)). A
  law has no run time, so the interval rule does not apply to a law. For
  example, `wLen w` of a `Window` parameter `w` has the bound 2^32, above
  the `Height` width, so it gives `REFUSE_BOUND` as a `Height` argument of
  a program def.

S5 review checks the assumptions behind Window bounds at calls: a Window
argument must have a start and a length predecessor within 32 bits.
Closed Window definitions are followed to their constructors; an unknown
field gets the word bound. The implicit successor in `wLen` is checked
against the word, including in loops. `Enc`, `encBind` and `encMap` must
receive their base type arguments before they can be passed as values,
so a higher-order call cannot instantiate `Enc` at an unsupported type.

The rows: `examples/base.lang`, `examples/enc.lang` (now at `Amount`),
the `test/check` files `read-amount`, `read-height` and `read-token`
(`REFUSE_READ`), `bound-chain`, `bound-helper`, `bound-arg`,
`bound-loop`, `bound-value` and `bound-word` (`REFUSE_BOUND`), `enc-size`
(`REFUSE_CELL`), and 18 eval rows of `base`. Review regressions add
`bound-window-{arg,length,loop,unknown,value,word}` (`REFUSE_BOUND`) and
`enc-type-value` (`REFUSE_CELL`), plus valid Window helper evaluations.

S6 adds two field rules to the interval rule (Q-S6-1 (a), Q-S6-2 (a)).
A projection of an `Amount` or `Height` field of a record that is a
parameter of the def has the width of the field. A projection of a
`Window` field of a parameter record has the widths of a `Window`
parameter (start 2^32 - 1, length 2^32). A field of a record that is
not a parameter (for example a case binder) has the word bound. The
rows: `examples/swap.lang` (the fields of an `Intent` parameter) and the
`test/check` files `bound-field-binder` and `bound-field-window`
(`REFUSE_BOUND`).

At a call to a program def with a record parameter, its direct `Amount`,
`Height` and `Window` fields must retain these input widths. A parameter
record can be forwarded; a constructor (including a closed program def)
is checked field by field. A computed record has no field-bound summary
and is refused at such a call. A program def with a bounded record
parameter cannot escape as a value or a partial application. These checks
prevent a record from hiding a widened value and narrowing its bound at
the next projection. The `bound-record-{amount,height,window,call,value}`
refusals cover these cases; `examples/swap.lang` also checks forwarding
and reconstruction at the input widths. Fields beyond the first 64
parameters use the word bound, matching the bound table's capacity.

The guest calls to `wClamp`, `decayTest`, `decayAt`, `cross` and the settle
chain also check the bounds used by their domain arithmetic. `decayTest`
admits a length up to 2^32 and a clamped height up to 2^33 - 1; its start
and amount arguments retain their input widths. Direct sealed `Amount`
and `Height` fields and program parameters retain their payload widths
at calls. A literal seal is checked at its payload; a computed sealed
value has no payload-bound summary and is refused at these calls.
The `bound-decay-{args,value}`, `bound-cross-sealed`,
`bound-settle-sealed` and `bound-sealed-arg` refusals prevent the hidden
products from exceeding the word. `inputCross` and `inputSettle` in
`examples/swap.lang` check calls on external inputs.

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

S6 (2026-11-07) discharges S1, S2, S3, S5 and N1. Each law is a
`law def` in `domain/domain.lang`, and the kernel checks each proof. Two
steps carry the S1 proofs: `vdOk` (a check with the flag yes gives the
rest of the chain) and `vdBad` (a check with the flag no gives `reject`
with its reason). Each step is one `cong` on the flag path. The kernel
unfolds the named tails of `settle`, thus a `trans` goes down the chain
one check at a time.

| Law | Defs | Statement | Proof |
|---|---|---|---|
| S1 | `s1Accept` | The five flags at yes give `settle l i q h` = `accept Outcome (settleOut i q)`. | A `trans` chain of five `vdOk`. |
| S1 | `s1Expired`, `s1BadPair`, `s1BadProof`, `s1BelowFloor` | The flags before a check at yes and the flag of the check at no give `reject Outcome` with the reason of the check. | A `vdOk` prefix, then `vdBad`. |
| N1 | `n1Reused` | `spHas (itNf i) l` at yes gives `reject Outcome reused` (N1a, Q-S6-4 (a)). | One `cong`. |
| S2 | `openMap2`, `s2Cross` | The open of `encMap2 A B C g m n` is `g` of the open values. The open of `cross i q h` is `decayAt` of the open quote output and the open `minOut` (Q-S6-5 (a)). | Elim on `m`, then on `n`, then `refl`. `s2Cross` is `openMap2` at `Amount`, `Amount` and `Sum Unit Unit`. |
| S3 | `s3Quote` | Two quotes from one solver with equal open outputs give the same `settle`. | Elim on the two sealed outputs, with a motive that is a function of the path, then `cong`. |
| S5 | `s5Fill` | `fillOutcome f` is `settled` of the two legs of the fill `f`. | `refl`. |

N1 in the replay form is DONE at S7 (Q-S6-4 (a)). The arithmetic reading
of S2 (the floor is `startOut` before the start of the window and
`minOut` after the end) has closed rows. The rows:

- `examples/swap.lang`: the fields of an `Intent` parameter and
  `vdCheck`; 3 eval rows (`notYes`, `vdKeep`, `vdStop`).
- `examples/decay.lang`: a window with start 1 and length 2, `startOut`
  4 and `minOut` 2. 9 eval rows of `wClamp` (below, in and above the
  window) and `decayTest` (pass and fail at the start, the middle and the
  end), and 5 closed `refl` laws (`clampMidRefl`, `floorStartRefl`,
  `floorMidRefl`, `floorMidFailRefl`, `floorEndRefl`).
- `examples/spend.lang` (kit mode): real nullifiers (`nullifier 1`,
  `nullifier 2`), an intent and two quotes at the height 2 (floor 3).
  The `spHas` rows and the `settle` rows (`reused`, accept with the two
  transfers, `belowFloor`), each with a `refl` law; 5 eval rows.

S7 (2026-11-07) discharges G1, G2, G3 and N1 in the replay form, and
states S1, N1a and S3 again on the `settle` of S7 (the S6 forms of
these laws are gone). The S1 proofs use `vdOk` and `vdBad` as in S6.
The kernel takes `cong (fun (f : Sum Unit Unit) => accept Outcome
(fdPick f i q)) p` as the path from `accept Outcome (settleFunds b i
q)` to the outcome of the funds flag.

| Law | Defs | Statement | Proof |
|---|---|---|---|
| S1 | `s1Accept` | The six flags and the funds flag at yes give `settle p b i q c h` = `accept Outcome (settleOut i q)`. | A `trans` chain of six `vdOk`, then a `cong` on the funds flag. |
| S1 | `s1Unfilled` | The six flags at yes and the funds flag at no give `accept Outcome unfilled` (Q-S7-5 (a)). | The same chain, then a `cong` on the funds flag. |
| S1 | `s1Expired`, `s1BadPair`, `s1BadProof`, `s1BadCert`, `s1BelowFloor` | The flags before a check at yes and the flag of the check at no give `reject Outcome` with the reason of the check. | A `vdOk` prefix, then `vdBad`. |
| N1 | `n1Reused` | `bkHas (itNf i) b` at yes gives `reject Outcome reused` (N1a). | One `cong`. |
| S3 | `s3Quote` | Two quotes from one solver with equal open outputs give the same `settle` (the S7 signature). | As in S6. |
| G1 | `g1Go`, `g1Fix` | `g1Go l s`: `rpGo (lgAmends l) s` = `afGo l s`. `g1Fix p l`: `vdThen GovState Params (govReplay p (lgAmends l)) (fun s => accept Params (ssNext s))` = `amendFold p l`. | `g1Go` by induction on the log; on an amendment entry, an elim on the verdict of `gsAmend`. `g1Fix` is one `cong` of `g1Go` at `govState p p htZero bkZero`. |
| G2 | `g2Cur`, `g2Same` | `g2Cur s`: `gsCur s (ssAt s)` = `ssCur s`. `g2Same s i q c`: `govStep s (enFill i q c (ssAt s))` is the fill with `ssCur s`, the `Params` before the amendment at that height. | `g2Cur` is a `cong` on the flag with `htLeRefl`; `g2Same` uses `g2Cur`. |
| G3 | `g3Floor` | Each `Params` p has `htLe tMin (prT p)` at yes. | `prFloor p` (by the type). |
| N1 | `n1Replay` | At a step of the replay, a fill whose nullifier is in the spent set of the `Book` gives `govStep s (enFill i q c h)` = `reject GovState reused` (Q-S7-7 (a)). | One `cong` on `n1Reused`. |

Four mutants, one wrong law body at a time, are each refused by the
kernel (`TYPE_MISMATCH` in `langc --kit check` of
`examples/spend.lang`): `g3Floor` with `prRange`, `g2Cur` with the two
branches swapped, `g1Go` with `accept ih s`, and `n1Replay` with
`ssCur`. The spend direction of N1 (after a settled fill, the nullifier
is in the spent set) has closed `refl` rows, because `nfEq n n` does
not reduce for a variable `n` (Q-S7-7 (a)). The rows:

- `examples/params.lang` (kit mode, new): `genesisParams` (the members
  `key 1` to `key 3`, t 2 and the prices 1, 5 and 20); the closed laws
  `floorRow`, `tierRow`, `certRow`, `dupRow` and `canRow`; 24 eval rows
  for the price and floor flags, `ctCheck` and `certOpen`, the `Book`
  (`bkCan`, `bkHas` and `noFunds`), `govAmend` (accept, `belowTMin`,
  `badPrice` and `badCert`), `govReplay` (`replayRow`, `replayLate`,
  `replaySame` and `replayPoor`) and `foldRow` (`amendFold`, equal to
  `amendRow`).
- `examples/spend.lang` (kit mode): the `settle` rows on the S7
  signature (the outputs of `reusedRow`, `freshRow` and `lowRow` do not
  change), `badCertRow`, `poorRow` (`unfilled`) and `pairRow`; the
  replay rows `stepFresh`, `stepReused`, `stepReusedRefl`,
  `stepReusedN1` (`n1Replay` on literals), `spentAfter` and
  `spentAfterRefl` (the spend direction); 6 eval rows.
- `examples/swap.lang` (`inputSettle`) and
  `test/check/bound-settle-sealed.lang` take the S7 signature of
  `settle`.

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

Kernel codes. The checker of the port also gives codes that are not in
this table. S8 (2026-11-08, Q-S8-1 (a)) maps them:

| Kernel code | Code | Cause |
|---|---|---|
| `REFUSE_CELL` | `REFUSE_FORMER` | Each site. |
| `REFUSE_TIME` | `REFUSE_CLOCK` | Each site (`now`, `clock`, `timestamp`). |
| `REFUSE_REC` | `REFUSE_PARTIAL` | A forward name, or a `def rec`. |
| `REFUSE_DATA` | `REFUSE_BASE` | A program family or cofamily. On a program law def it stays `REFUSE_DATA`. |
| `REFUSE_LAW` | `REFUSE_UNSEAL` | A sealed eliminator, fold or projection, `decrypt` or `observe` (Q-S4-1 (a)). |
| `REFUSE_LAW` | `REFUSE_OPEN` | `open` (Q-S4-1 (a)). On a law def name that is not one of these it stays `REFUSE_LAW`. |
| `REFUSE_READ` | `REFUSE_DIV` | A read of `Amount`, `Height` or `Window` (Q-S5-5 (a)). |
| `REFUSE_READ` | `REFUSE_UNSEAL` | A read of `Token`, `Nullifier`, `Key`, `Sig`, `Commitment` or `Root`. |

The lexer gives `REFUSE_CKKS` on a decimal literal (digits, `.`, a
digit). `REFUSE_RETRO` has no cause in the checker: an amendment applies
from its height + 1 (`gsCur`, G2), so no program can state an amendment
that applies at or before its own height. S8b (2026-11-08) builds
`REFUSE_WORD` (Q-S8b-1 (a)): a user program that names `Word`, a `Word`
operation or a `Word` literal gets it. `domain/domain.lang` and a kit
file with `--kit` can use them. The lexer gives `LEX_WORD_RANGE` on a
`Word` literal above 0xffffffff. The other kernel codes stay.

The text line is `langc: CODE: MESSAGE (kernel K)` when the kernel code
K is different from CODE, else `langc: CODE: MESSAGE` (Q-S8-2 (a)).
`langc check --json PROG` writes one line on stdout (Q-S8-3 (a)):
`{"diagnostics":[...],"progress":{"parsed":N,"typed":N,"laws_discharged":N,"laws_total":N}}`.
The list has zero or one diagnostic, with the keys `code`, `kernel`,
`file`, `line`, `col`, `def`, `message` and `hint` in this order.
`kernel` is null when K is CODE, and `def` is null when there is no def.
The hint is one fixed sentence for each code, from the table in
`src/front/codes.c`. The exit status is 0 (accepted), 1 (refused: the
JSON on stdout, nothing on stderr) or 2 (usage, IO or memory: no JSON,
the text line on stderr). On a parse error the progress counts are all
zero.

S7 (2026-11-07) builds `REFUSE_FLOOR` (Q-S7-1 (a)): the checker
(`law_field` in `src/front/check.c`) refuses a `Params` term with a
proof field `prFloor` or `prRange` that the kernel cannot check. The
rows: `test/check/floor-params.lang` (t 1, below `tMin` 2, with `refl`
for `prFloor`) and `test/check/floor-range.lang` (a price tier 0, with
`refl` for `prRange`), each `REFUSE_FLOOR` in `test/check/expect.txt`.

## 8. Route

### 8.1 Kernel

S2 ports the ratify-lang Cubical TT kernel at a2322ef (Q-S0-18 (a)) onto
the tcc-wasm kit. Survey first: SAME, CHANGED, SOURCE-ONLY. S3 makes
arithmetic total (Q-S0-2) and changes the trap row (Q-S0-33).

S2 (DONE) took the change of ratify-lang 55310c5..71f2c22 on the kernel
paths: the kernel, R0 and the no-trap Wasm writer (Q-S2-1). The gate keeps
the kit row "a trap is equal to a trap". The host crypto ABI and
`src/crypto` of ratify-lang are not in the port (Q-S0-17 (b)).

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
| One canonical form per operation, `langc fmt` | S8, built (2026-11-08): `langc fmt PROG`, `langc fmt --check PROG` |
| `langc check --json`, fix hints, progress counts (parsed, typed, laws discharged) | S8, built (2026-11-08): section 7 |
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

S8b (2026-11-08) adds two kit files: `examples/crypto.lang` (one def for
each `Word` operation, in `examples/kit.txt`) and
`test/crypto/crypto.lang` (the field rows, outside the Wasm set). The
gate line `crypto oracle: N rows, M mismatches` tells an agent if a
prelude change still agrees with the C oracle (section 10). The
`check --json` hints of `REFUSE_WORD` and `LEX_WORD_RANGE` are in
`src/front/codes.c`.

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
  `src/crypto` as a C oracle on fixed vectors. S8b (Q-S8b-7 (a)):
  `src/crypto/{u256,bn254,keccak,poseidon}.{c,h}` are byte copies of
  quorum-lang 8d1a113, and `langc` does not link them.
  `build/cryptotool` (`test/cryptotool.c`) writes 424 rows `FILE NAME
  ARGS => RESULT`: 360 on `examples/crypto.lang` and 64 on
  `test/crypto/crypto.lang`. The gate row `crypto oracle` runs each row
  with `langc --kit eval` and gives 424 rows, 0 mismatches.
  `langc --kit ir test/crypto/crypto.lang` refuses with `EVAL_DEPTH` at
  `frInv` (the lowering nests deeper than 2000 levels), so the field
  file is outside the Wasm set.
- Word rows (S8b): 35 eval rows in `test/eval/expect.txt`;
  `test/check/word-type.lang` and `test/check/word-op.lang` give
  `REFUSE_WORD`; `test/parse/word-range.lang` gives `LEX_WORD_RANGE`.
  `examples/crypto.lang` is a `fmt idempotent` row and a differential
  module: a `Word` parameter takes 0w, 1w, 7w, 31w, 32w, 33w,
  0x80000000w and 0xffffffffw. A gate row runs `weq 4294967297 1` on
  the Wasm module and wants 1 (the entry keeps the low 32 bits). C rows
  in `test/fronttool.c`: the domain file can use `Word`, its literals
  and a `Word` `hcomp`; a guest `31w` gives `REFUSE_WORD`.
- WRK rows (Q-S1-2 (a), Q-S1-3 (a)): n `barterd` members in member mode on
  local TCP links. The outputs equal the clear evaluation of the same
  circuit.
- Refusal rows (S8): `test/check/expect.txt` has the mapped codes;
  `test/parse/ckks-decimal.lang` gives `REFUSE_CKKS`; the gate row
  `kernel suffix` checks the ` (kernel REFUSE_READ)` suffix on one
  `REFUSE_DIV` row.
- JSON rows (S8): the gate rows `json refusals` and `json lines` run
  `langc check --json`; a C row in `test/fronttool.c` checks the JSON
  quote.
- fmt rows (S8): `fmt idempotent` (each `examples/*.lang` and
  `domain/domain.lang`): fmt of the fmt text is the same text, and
  `langc fmt --check` of the fmt text exits 0. `fmt not canonical`:
  `langc fmt --check test/fmt/check.lang` exits 1 with
  `test/fmt/check.lang:3: not canonical`. `fmt usage`: a bad option
  exits 2. Both argument orders are accepted: `langc fmt --check PROG`
  and `langc fmt PROG --check`. C rows in `test/fronttool.c`: the decl
  dump of a file and of its fmt text are the same, and fmt refuses a
  canonical text that crosses a limit (`PARSE_DEPTH`, `FMT_SIZE`).

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
| S2 | Port the ratify-lang kernel at a2322ef (Q-S0-18 (a)); survey first | DONE (2026-11-06) |
| S3 | Total arithmetic, the trap row (Q-S0-2, Q-S0-33) | DONE (2026-11-06) |
| S4 | R1 formers, `Enc` and `open` (Q-S0-3) | DONE (2026-11-06) |
| S5 | Base types of design sec 3 | DONE (2026-11-06) |
| S6 | Swap operations and laws S1, S2, S3, S5, N1 | DONE (2026-11-07) |
| S7 | Governance: `Params` with the price list and its range (Q-S0-11 (b), Q-S1-6 (a)), `amend`, `replay`, `bad_price`, laws G1 to G3; the ledger and the certified `open` in `settle` (Q-S6-6 (a)), N1 in the replay form (Q-S6-4 (a)), OI-8 | DONE (2026-11-07) |
| S8 | Refusal codes, `check --json`, `langc fmt` | DONE (2026-11-08) |
| S8b | The crypto prelude part 1 (Q-S0-17 (b)): `Word` and `REFUSE_WORD` (Q-S8b-1 (a)), the family rename to `SwapWord` (Q-S8b-2 (a)), the BN254 field `Fr` and `Fp` (Q-S8b-3 (a)), the C oracle rows (Q-S1-5 (a), Q-S8b-7 (a)) | DONE (2026-11-08) |
| S8b2 | The crypto prelude part 1, second commit (Q-S8b-8 (a)): the native field product `feMulC` (Q-S8b-4 (a)), BN254 G1, Poseidon (Q-S0-13 (a), Q-S8b-5 (a)), keccak-256, the Schnorr check (OI-17, Q-S8b-6 (a)) in `csHas` | NEXT |
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
- OI-4 The kit row "a trap is equal to a trap" (Q-S0-33). Closed at S3
  (section 2).
- OI-5 The facilitator URL and the payTo address are USER inputs. No
  default address in the repo.
- OI-6 The decoy bundle: its size and the meaning of the sealed decoy
  index (design line 65 of `design/intent-swap-design.md`).
- OI-7 The bit width of `Amount` and `Height`, so that the products of the
  affine test have a static bound. Q-S5-1 (a) rules it (section
  1.7). S5 builds the interval rule (section 4).
- OI-8 Deposits and withdrawals: the design escrows on chain. Off chain,
  the ledger needs deposit and withdraw entries in the log. Their shape is
  a question for S7 (the `Log` and `replay`). S6 does not need it: a
  `Transfer` is a token, an amount and two keys, and no S6 reason reads a
  balance. Closed at S7 by Q-S7-4 (a): the `enDeposit` and `enWithdraw`
  entries of the `Log`, and the `Book` (section 3).
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
  (gavel d71bd96:66). It is a question at S8b. RULED by Q-S8b-6 (a):
  Schnorr on BN254 G1 with a keccak-256 challenge, built at S8b2.
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
- OI-22 The decay window bounds and the deadlines stay in the intent,
  not in `Params` (Q-S7-1 (a)). A `Params` limit on them, set by an
  amendment, is a question for a later slice.
