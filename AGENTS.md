# AGENTS.md (DRAFT)

Notes for coding agents that find this repo. S20 makes this file final.

## What this repo is

barter-lang is a language for a private intent swap off chain, governed by
a t-of-n threshold authority that amends itself. `langc` refuses any
program that can have a runtime error. See `README.md` for when to use it.

## Status

S0: the repo is the lang-template tcc-wasm kit with a sample domain. The
barter-lang kernel, base types and targets are not here yet. Read
`docs/PLAN.md` section 12 for the slices.

## Build and test

- Gate: `make check` in the repo root. Run it serially when you keep the
  log.
- Tests are in C (host) and in barter-lang (guest). Do not add a node,
  Python or shell test harness.

## Rules for changes

- `design/DESIGN.md` section 9 and `design/intent-swap-design.md` are
  verbatim. Do not edit them.
- Do not add a core type or a core operation that `design/DESIGN.md` does
  not give.
- A question that is not ruled stays open in `docs/PLAN.md` section 1.2.
- Do not put a wallet address, a facilitator URL or a deploy secret in the
  repo.

## Tools for agents (planned)

- `langc check --json`: refusal codes, fix hints, progress counts (S8).
- `langc fmt`: the canonical printer (S8).
- `langc mcp`: an MCP server over stdio for authoring (S15).
- `langc serve`: the same tools over HTTP behind x402 (S16).
- `SKILL.md`: graded examples (S15).

## Funding

GitHub Sponsors is on for this repo. `funding.json` and `llms.txt` come in
S17.
