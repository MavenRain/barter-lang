Source: intent-lang design/DESIGN.md at 67445ae, lines 22 to 123, verbatim below the blank line.
Pasted by the USER on 2026-10-22: the denotational design of the private intent swap.
That design is on chain. barter-lang reads it off chain; see design/DESIGN.md section 2.

## The pasted design (verbatim)

**Denotational design** (in Conal Elliott’s sense) specifies a system by giving each type a simple, precise mathematical *meaning* (denotation) and requiring every operation to be a homomorphism for that meaning: the meaning of an instance is the instance of the meaning (the type-class-morphism / TCM principle). Correctness, algebraic laws, and freedom from abstraction leaks follow for free; implementation is constrained only by the requirement that it realize the denotation.

We formalize the design in Cubical Type Theory (interval \(\mathbb{I}\) with endpoints \(\mathsf{i0},\mathsf{i1}\), path types \(\mathsf{Path}\,A\,x\,y\) or heterogeneous \(\mathsf{PathP}\), and higher paths for homotopies). Cubical structure lets us treat continuous auction decay, atomic settlement, and privacy equivalences as paths (or higher paths) rather than as operational traces.

### Semantic Domains

- **Time / Auction Parameter.** Continuous time (or normalized block progress) is the interval \(\mathbb{I}\). A Dutch-style decay curve is a path in the space of amounts:
  \[
  \mathsf{decay} : \mathsf{Path}\,(\mathsf{Amount})\,(\mathsf{startOut})\,(\mathsf{endOut}).
  \]
  Linear decay is the straight-line path \(\lambda i.\;\mathsf{startOut} - i\cdot(\mathsf{startOut}-\mathsf{endOut})\).

- **Plain Intent Meaning.** An intent denotes a *constraint predicate* on market outcomes:
  \[
  \mu(\mathsf{Intent}) \;:\; \mathbb{I} \to \mathsf{Amount} \to \mathsf{Prop}.
  \]
  At progress \(i\), a fill with output amount \(o\) is acceptable precisely when \(o \ge \mathsf{decay}\,i\) (and the input amount matches the signed quantity, nonces are fresh, etc.). The predicate is independent of any particular solver.

- **Encrypted Intent.** FHE supplies a type constructor \(\mathsf{Enc}\,A\) equipped with homomorphic operations (\(+^*,\cdot^*,\mathsf{cmp}^*\), \ldots) such that decryption is a homomorphism:
  \[
  \mathsf{decrypt}(\mathsf{op}^*\,c_1\,c_2) \;\equiv\; \mathsf{op}\,(\mathsf{decrypt}\,c_1)\,(\mathsf{decrypt}\,c_2)
  \]
  (path equality). An encrypted intent carries \(\mathsf{Enc}\,\mathsf{Amount}\) for the input size, the minimum output floor, and (optionally) an encrypted index into a decoy bundle. Its denotation is the plain predicate encrypted under the FHE scheme: satisfaction of the encrypted constraint is computationally indistinguishable from satisfaction of the plaintext constraint (privacy as a path between the encrypted view and a dummy view).

- **ZK Proof.** A proof is an element of a proposition (or a higher path witnessing that a circuit evaluates to a valid witness). It certifies:
  - correct encryption of the declared parameters,
  - that a proposed fill satisfies the (homomorphically evaluated) auction constraint at the claimed time,
  - atomicity of the token transfers (input pulled, output delivered, or both revert).

- **Outcome.** \(\mathsf{Outcome} = \mathsf{Settled}\;(\mathsf{Transfer}\times\mathsf{Transfer}) \;\uplus\; \mathsf{Unfilled}\). Settlement is a map
  \[
  \mathsf{settle} : \mathsf{Intent} \to \mathsf{Quote} \to \mathsf{ZKProof} \to \mathsf{Outcome}
  \]
  whose denotation is the semantic matching function on the plain predicates: it produces \(\mathsf{Settled}\) exactly when the decrypted quote meets the decayed constraint, and the two transfers are path-equal (atomic).

### Core Types (Cubical Style)

```agda
-- Tokens and amounts (or indexed by token)
postulate Token : Set
postulate Amount : Set

-- Interval-based decay
decay : Path Amount startOut endOut
decay = λ i → startOut - i · (startOut - endOut)

-- Encrypted values with homomorphic structure
postulate Enc : Set → Set
postulate decrypt : {A : Set} → Enc A → A
-- Homomorphism laws as paths

-- Intent
record Intent : Set where
  field
    inTok   : Token
    outTok  : Token
    amtIn   : Enc Amount
    minOut  : Enc Amount          -- floor; decay applied homomorphically
    decoyIdx : Enc ℕ              -- index of real order in bundle (Zama-style)
    zk      : ZKProof             -- well-formedness + ownership

-- Meaning of an intent
⟦_⟧Intent : Intent → (I → Amount → Prop)
⟦ intent ⟧Intent i o =
  decrypt (intent .minOut) ≤ o
  ×  -- plus input match, nonce, etc.

-- Quote / fill (solver’s offer)
record Quote : Set where
  field
    outAmt : Enc Amount
    solver : Address

-- Settlement
data Outcome : Set where
  Settled : Transfer → Transfer → Outcome
  Unfilled : Outcome

settle : Intent → Quote → ZKProof → Outcome
-- TCM law (path):
-- ⟦ settle i q π ⟧ ≡ if (verified π ∧ ⟦i⟧Intent (time) (decrypt (q.outAmt)))
--                    then Settled (pullIn i) (pushOut q)
--                    else Unfilled
```

### TCM / Homomorphism Laws

- **Auction homomorphism.** Evaluating the decay path at a point \(i\) and then applying the constraint is definitionally the same as applying the encrypted constraint and decrypting.
- **Matching homomorphism.** The meaning of \(\mathsf{settle}\) is the pointwise application of the intent predicate to the decrypted quote. Different fillers that produce equal decrypted amounts yield path-equal outcomes.
- **Privacy path.** For any observer, the encrypted view of an intent is path-equal to the view of a decoy bundle of the same cardinality (computational indistinguishability realized as a higher path). Selective decryption is available only to the winning solver and the user.
- **Atomicity.** The pair of transfers (input pull via Permit2-style authorization, output delivery) forms a path whose endpoints are the pre-state and the post-state; any failure reverts both sides (the path is constant on failure).

### Protocol Flow (Denotationally)

1. User constructs an \(\mathsf{Intent}\) whose denotation is the desired outcome predicate, encrypts the sensitive fields under FHE, and attaches a ZK proof of well-formedness. The signed intent is broadcast (gasless).
2. Solvers observe only the encrypted view. They evaluate matching, pricing, and auction progress *homomorphically*, producing encrypted quotes. A ZK proof attests that the quote satisfies the encrypted constraint.
3. The first (or auction-winning) valid quote is submitted on-chain. The reactor verifies the ZK proof and the homomorphic comparisons. On success it executes the atomic transfers whose denotation matches the plain predicate; otherwise the transaction reverts with no state change.
4. Unfilled intents remain reclaimable; the reclaim path is the constant path at the original escrow state.

This design yields a protocol whose observable behavior is exactly the composition of the mathematical meanings of intents, decay curves, and settlements. Implementations (on-chain reactors, off-chain FHE solvers, ZK circuits) are correct precisely when they realize these denotations; privacy, fairness of the auction, and atomicity are paths in the Cubical model rather than external invariants.
