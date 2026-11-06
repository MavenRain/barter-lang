# barter-lang domain design

> Paste the domain design below the line, verbatim. Do not edit it.

This file is the only source of the core types and the core operations of
barter-lang. `SPEC.md` sections 5 and 6 cite its sections. Do not add a core
type or a core operation that this file does not give.

When the design changes, paste the new version. Then record the change as an
open item in `SPEC.md` section 9.

---

## 1. Source

The USER gave no paper and no new design artifact (2026-11-06). This file
is written from three sibling sources. They are readings, not rulings.
`docs/PLAN.md` section 1 lists the questions that the USER rules.

- The swap: `design/intent-swap-design.md`. It is intent-lang 67445ae
  `design/DESIGN.md` lines 22 to 123, verbatim. The USER pasted it on
  2026-10-22. It is a denotational design of a private intent swap with an
  on-chain reactor. barter-lang reads it off chain.
- Self-reference: orbit-lang 2162fb2 `design/DESIGN.md` line 32 (the fixed
  point of the governance endo-operation) and lines 64, 89 and 96 to 100
  (`amend`, with the law that the meaning of `amend` is `gov`).
- Self-amending threshold authority: ratify-lang a2322ef (amendment by log
  height; ZK, FHE and MPC ballots) and gavel-lang 310e708 and d71bd96 (the
  same off-chain governance layer over a different protocol).

Section 9 gives the request of the USER, verbatim.

## 2. The role map (on chain to off chain)

| Design role (on chain) | barter-lang role (off chain) |
|---|---|
| Reactor contract executes the fill | A t-of-n committee certifies a `Fill` entry into the `Log`. |
| Token balances in contracts | A ledger that `barterd` replays from the certified log. |
| Atomic transfers, revert on failure | Both legs of a fill are in ONE log entry or in no entry. |
| `Enc Amount` under FHE | An FHE ciphertext or MPC shares under the committee key. |
| Homomorphic `cmp*` on chain | The crossing check at T3 (FHE) or T4 (MPC). |
| Selective decryption | Threshold decryption of the settled fill only. An unfilled intent is never opened. |
| Intent signature (ecrecover) | An owner key and a signature over the intent commitment. |
| Nonce freshness | A nullifier set replayed from the log. |
| `ZKProof` | A T2 relation, or t attestations of correct evaluation (PLAN Q-S0-9). |
| Block progress, the interval `I` | Log height, in a decay `Window`. No wall clock. |
| Reclaim path (constant path) | An `Unfilled` intent expires at its deadline height. Nothing moves. |

## 3. Base types (draft)

| Type | Meaning |
|---|---|
| `Token` | A token identifier. Decidable equality. |
| `Amount` | A bounded natural number. A set with decidable equality. |
| `Height` | A log height. A bounded natural number. |
| `Window` | A decay window: start height and length in heights, length > 0. |
| `Nullifier` | A one-time tag of an intent. |
| `Key`, `Sig` | An owner public key and a signature (opaque). |
| `Commitment`, `Root` | A hiding commitment and a Merkle root (opaque, over `Hash`). |
| `Intent` | inTok, outTok, sealed amtIn, sealed minOut, sealed decoyIdx, startOut, window, deadline, nullifier, owner, signature. |
| `Quote` | The sealed outAmt and the solver key. |
| `Transfer` | Token, amount, from, to. |
| `Fill` | An intent, a quote, the two transfers, a height. |
| `Outcome` | `Settled Transfer Transfer` or `Unfilled`. |
| `Sealed A` | A commitment, an FHE ciphertext or MPC shares that carry an `A`. |
| `Statement`, `Certificate` | A statement over the log, and t of n committee signatures on it. |
| `Verdict A` | `accept A` or `reject Reason`. |
| `Params` | The governed parameters and the amendment rule as data (section 5). |
| `Amendment`, `Ballot` | A proposed new `Params`, and a member vote on it. |
| `Log` | The certified, height-ordered list of entries. |

## 4. Operations (draft)

- `decayAt : Intent -> Height -> Amount -> Bool`: the affine test of
  section 4.1. Its meaning is the decay path of the design at that height.
- `cross : Intent -> Quote -> Height -> Sealed Bool`: the crossing check
  over sealed values (T3 or T4).
- `settle : Params -> Ledger -> Intent -> Quote -> Certificate -> Height ->
  Verdict Outcome`. It gives `reject` for a bad certificate, a reused
  nullifier, an expired intent, a wrong pair or a quote below the floor.
- `open : Certificate -> Sealed A -> Verdict A`: threshold decryption. Only
  `settle` calls it, on the two legs of a fill.
- `amend : Params -> Amendment -> Certificate -> Height -> Verdict Params`.
- `replay : Params -> Log -> Verdict (Params x Ledger)`: fold the log from
  height 0.

### 4.1 Decay without division

The design gives decay as a path `startOut` to `endOut` over the interval.
`Amount` is a set, so a path in it is trivial (Hedberg). barter-lang reads
decay as a function from height to amount, given by the cross-multiplied
test (intent-lang 67445ae `docs/PLAN.md` lines 306 to 325):

    window * o + t * startOut >= window * startOut + t * minOut

with `t` the height offset in the window and `o` the quote amount. There is
no division and no subtraction. Both sides are affine in the sealed values
with public scalars, so T3 and T4 evaluate the test without `bind`.

## 5. The governance layer

`Params` holds the swap parameters (token pairs, decay window bounds, fee,
deadlines) and the amendment rule as data in a closed form: the committee
set, n, t, the quorum and the delay in heights. `amend` reads the rule that
it can change. A new rule applies from height h + 1, not before. One static
floor is in the type of `Params`: t >= t_min. No amendment can put the
threshold below t_min (PLAN Q-S0-6).

Self-reference is the orbit-lang fixed point: replay of the log with the
rule in force at each height gives the same `Params` as a fold of `amend`
over the amendment entries (law G1). Ballots use ZK eligibility and an FHE
or MPC tally, as in ratify-lang and gavel-lang.

## 6. Laws and their sources

| Id | Law | Source |
|---|---|---|
| S1 | `settle` meets the TCM law: Settled exactly when the proof checks and the opened quote meets the decayed floor. | `design/intent-swap-design.md` lines 85 to 89 |
| S2 | Auction homomorphism: the affine test at height h equals the decay path at h. | same, line 94; section 4.1 |
| S3 | Matching homomorphism: equal opened amounts give equal outcomes. | same, line 95 |
| S4 | Privacy path: the sealed view equals a decoy view of the same size. | same, line 96 |
| S5 | Atomicity: both legs are in one entry or none (refl by the shape of the log). | same, line 97 |
| S6 | `open` is a homomorphism for the sealed operations. | same, lines 25 to 29 |
| G1 | Fixed point: replay with the rule at each height equals the fold of `amend`. | orbit-lang 2162fb2 lines 32, 96 to 100 |
| G2 | No retroactive amendment: a rule certified at h applies from h + 1. | ratify-lang a2322ef |
| G3 | Threshold floor: every reachable `Params` has t >= t_min. | section 5 |
| G4 | A certificate needs t distinct members of the committee in force. | ratify-lang, gavel-lang |
| N1 | A nullifier settles at most once; a reuse gives `reject`. | section 2 |

`docs/PLAN.md` section 6 puts each law in a bucket.

## 7. Four C programs

All four are C99 that TinyCC builds.

1. `langc`: the compiler. It parses, checks, lowers and writes the targets.
2. `langc mcp`: an AUTHORING MCP server over stdio for agents that WRITE
   barter-lang (check, fmt, explain, list-base, show-law).
3. `langc serve`: the same tools over HTTP behind x402, plus MCP streamable
   HTTP at `/mcp`. It links libcurl for the facilitator calls and runs
   behind a TLS proxy.
4. `barterd`: the T1 host. It runs a compiled module on intents, quotes,
   certificates and logs.

## 8. Not in v1

- Code as data in an amendment (new barter-lang code in `Params`).
- CKKS (approximate arithmetic).
- On-chain settlement (that is intent-lang).
- A wall clock.

## 9. The request (USER, 2026-11-06, verbatim)

Copied from `barter-lang-work/USER-ASK.md` sections 1 and 2. Two changes:
trailing white space is removed, and the en dash is a hyphen (the kit gate
refuses en dashes). The curly quotes and the leading spaces stay.

### 9.1 The ask, verbatim

I want to write a new programming language with the following characteristics:

First, the language should not accept/compile anything that will lead to runtime errors, period.

As for type constructors/higher-kinded types, admit only cell monads and their duals as represented computationally in cubical type theory.

As for base types and operations, admit what is needed in order to express construction of an off-chain zero-knowledge, homomorphically encrypted, multi-party self-referential threshold-authorized self-amending system and the private intent swap protocol that the self-amending system governs.

The host language shall be TinyCC.  Use your judgment to surface multiple-choice questions around choosing compilation targets.

Finally, in order to facilitate agents reaching for this language, adhere to the following additional criteria:

### 9.2 The pasted block, verbatim

This time the USER pasted ONE block. It holds both parts that gavel-lang recorded as block 1 (the 7 bullets) and block 2 (sections 1 to 4 and the starting sequence). The copy below keeps the curly quotes, the en dash and the leading spaces.

```
  The plan for making agents reach for the language includes:
  - one canonical form per operation and a langc fmt canonical printer;
  - langc check --json, giving fix hints and progress counts (parsed, typed, laws
    discharged);
  - a SKILL.md with 3 to 5 graded examples;
  - an MCP server written in TinyCC (langc mcp);
  - AGENTS.md and a README table saying when to use the language;
  - a token-count comparison against Python and Rust.
  - List where agents look. That means an MCP server entry in the public
     registries, an OpenAPI description, and the price in the 402 response.


1. Make the work agent-consumable and payable (highest leverage)
* Wrap existing formalizations or verification workflows as MCP tools or simple x402-gated endpoints. Examples: “verify this Halo2 gadget against the formalization,” “search formalized properties of this circuit,” “check this snippet for known invariants,” or “request a formalization sketch of gadget X.”
* Price per use in the $0.01-$0.20 range (or higher for heavier verification). Agents (or their principals) will use tools that improve success rate or reduce verification cost.
* Publish Agent Skills (SKILL.md with name + description frontmatter) that teach agents how and when to use your libraries or tools. Skills are an open standard (originally from Anthropic, now under the Agentic AI Foundation) and load progressively, so discovery is cheap.
2. Publish machine-readable discovery and funding signals
* Add FUNDING.yml plus a simple funding.json (or equivalent) to GitHub repos.
* Put an llms.txt (and, where relevant, .well-known/ai-donate.json or similar) on any site or content property that describes the projects, transparency, and explicit welcome for agent payments.
* Include AGENTS.md or Skills in the repos so coding agents that encounter your code can surface funding options.
Agents parse these files the way humans read a README or Sponsors page.
3. Use agent-native funding rails for larger efforts
* Run specific formalization milestones on AgentFund (or similar) with escrow and clear deliverables. Agents can discover, evaluate, contribute, and vote.
* Keep human channels (GitHub Sponsors, Open Collective, Patreon) as the primary near-term path; treat agent micropayments and campaigns as complementary. Better tools that agents actually use also generate recommendations and visibility that drive human sponsors.
4. Frame the value in agent terms
Position the work as shared infrastructure for agentic systems that need verifiable correctness: formalized payment or agent protocols, ZK compliance layers, verified gadgets that other agents can rely on. Agents (and the people who fund them) respond to clear ROI on reliability and reduced verification cost more than to general appeals.
Practical starting sequence
1. Add machine-readable funding metadata (FUNDING.yml, llms.txt, funding manifest) to the main repos.
2. Expose one useful Lean/ZK capability as an MCP tool or lightweight x402 endpoint (Cloudflare Workers + facilitator, or a simple Express + x402 middleware, are common starting points).
3. Publish a short Skill that documents how to use it.
4. List the endpoint/tool in relevant MCP or x402 discovery surfaces and mention it in content that reaches agent developers.
5. Measure actual agent traffic and payments before investing heavily in larger campaigns.
```
