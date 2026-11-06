# barter-lang

barter-lang is a Cubical TT language for a private intent swap off chain,
under an off-chain threshold authority that amends itself. Sealed intents,
a decay floor by log height, a sealed crossing check (FHE or MPC), t-of-n
certified fills and t-of-n amendments. The checker refuses any program that
can have a runtime error. It comes from lang-template with the tcc-wasm
host.

Status: S0 (seed). The plan is `docs/PLAN.md`.

## When to reach for barter-lang (DRAFT)

| Use barter-lang when | Use something else when |
|---|---|
| You write the rules of a private swap of intents that runs off chain. | The swap settles on chain (EVM): use intent-lang. |
| The amounts and floors must stay sealed until a fill settles. | You need governance only, with no swap: use ratify-lang. |
| A t-of-n committee certifies fills and amends the rules. | You run sealed-bid auctions: use gavel-lang. |
| You need a host to replay the log and check each certificate. | You need approximate (CKKS) arithmetic: barter-lang refuses it. |

S20 makes this table final.

## Files

- `SPEC.md`: the specification. Fill each section.
- `design/DESIGN.md`: the domain design. It is the only source of the core
  types and the core operations.
- `design/intent-swap-design.md`: the swap design, verbatim.
- `docs/PLAN.md`: the readings, the route, the slices and the questions.
- `AGENTS.md`: notes for coding agents (DRAFT).
- `formers/FORMERS.md`: the type formers. `formers/tcc-wasm.md` gives their
  status on the tcc-wasm host.
- `probe/CAPABILITY.md`: the host probe.
- `docs/STATUS.md` and `docs/VALIDATION.md`: the status and the gate results.
- `docs/host/README.md`: how to build and test the host kit, and how to
  replace the sample domain in `domain/`.

## License

MIT OR Apache-2.0.
