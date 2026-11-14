# TinyCC Wasm host kit

This kit compiles a small typed language to a WebAssembly MVP module.
One C99 program, `build/langc`, holds the parser, the checker, the
evaluator, the lowering to a first-order IR and the Wasm writer.
TinyCC builds it.

## Files

These files make the language and its Wasm target:

- the front end: `src/front/`, `src/ir.h`, `src/target.h` and
  `src/main.c`
- the target files: `src/wasm.c` and its opcode list `src/wasmop.h`
- the domain and its embed tool: `domain/` and `gen/`
- the examples: `examples/`
- the tests: `test/parse/`, `test/check/`, `test/eval/`, `test/ir/`,
  `test/notrap/`, `test/review.sh`, `test/fronttool.c`,
  `test/pathtool.c` and `test/notraptool.c`
- the differential files: `test/grid.awk` and `test/run-wasm.mjs`. They
  run in the S3 gate (see "Gate").
- the build and gate files: `Makefile`, `test/gate.sh` and `PIN`
- the host docs: `docs/host/README.md`, `docs/host/CAPABILITY.md` and
  `formers/tcc-wasm.md`

## Build

From the root of this repository:

```sh
make build
make check
make check-clang
```

`build` runs `tcc -run gen/embed.c` to copy `domain/domain.lang` into
`build/domain.c`. Then it builds `build/langc` with
`tcc -std=c99 -Wall -Werror`. The domain is part of the executable.
`check-clang` compiles the same sources with
`cc -std=c99 -Wall -Wextra -Wswitch-enum -Werror -fsyntax-only`.
`check` does both steps, then runs `test/gate.sh`.
The gate needs `node`, `wasm-validate`, `wasm-objdump` and `wat2wasm` on
the `PATH`.
`PIN` records the tool versions of the last GREEN gate.
The build and the gate need no network access.

## Source language

A program is a sequence of `def NAME : TYPE := TERM`. A line comment
starts with `--`. Lambdas are `fun (x : A) => t`. There is no text type.

In kit mode, names include types `Nat Flag Prod Sum Option List Path PathP I`, values
`flagYes flagNo pair first second inl inr either none some option nil cons
pure map bind fold unfold filter pack witness payload refl sym trans
transport cong`, Nat operations `natAdd natSub natDiv natMod natEq natLe`,
the divisor type `NonZero Nat` with `nonZero` and `nonZeroValue`, and the
Flag eliminator `flagIf b yes no`. `Type 0 : Type 1` is the universe
chain. `Sigma (x : A) B` and `(x : A) * B` are dependent pairs.
`FORMERS.md` gives the status of each former.

The compiler enforces these rules:

| Rule | Code | The compiler refuses |
|---|---|---|
| R1 | `REFUSE_DATA` | a `family` declaration in a program |
| R2 | `REFUSE_AXIOM` | an `axiom` declaration |
| R3 | `REFUSE_REC` | `def rec`, or a definition that refers to itself or to a later definition |
| R4 | `REFUSE_NAME` | a core name, a domain name, a reserved name or a repeated name as a new definition |

## CLI

```sh
build/langc check PROG
build/langc eval PROG NAME [ARGS...]
build/langc ir PROG
build/langc abi PROG
build/langc build PROG -o OUT [--runtime]
```

- `check` prints `ok`, or refuses the program.
- `eval` prints the normal form of `NAME`. Each argument is a decimal
  number. An entry has no trap (see "No traps").
- `ir` prints the first-order IR. It has one `func` for each entry.
- `abi` prints the name of each entry, one for each line.
- `build` writes the Wasm module to `OUT`. On this target, `--runtime`
  gives the same bytes.

The exit status is 0 for success, 1 for a refused program and 2 for a
usage or IO error. A diagnostic on stderr has the form
`langc: CODE: DEF: message`.

## Entries

An entry is a top-level program definition whose declared type is `Nat`,
`Flag`, or a first-order function from `Nat` and `Flag` arguments to a
`Nat` or `Flag` result. `build` exports each entry by its name. Each
parameter and the result is an `i64`. Other definitions are not exported.

`Nat` is an unsigned 64-bit integer. `natAdd` gives an `Option Nat`:
`none` on an overflow past 2^64 - 1. `natSub` stops at 0. The divisor of
`natDiv` and `natMod` is a `NonZero Nat`. `Flag` is 0 or 1.

## Compilation

Compilation is partial evaluation. The compiler normalizes the body of
each entry with the parameters as free variables. Types and universes erase.
Proof contents erase. A lambda stays only as the direct function argument of
`fold`, `unfold`, `map`, `bind`, `filter`, `either` or `option`. The
lowering puts its body into a loop or a branch. The lowering refuses two
programs with a HOST-LIMIT code (see `docs/CAPABILITY.md`):

- `REFUSE_RUNTIME_CLOSURE`: a function value must exist at run time.
- `REFUSE_RUNTIME_TREE`: a fold walks a tree that exists only at run time.

## No traps

No entry can trap (R0, `docs/PLAN.md` section 2). Each op of the IR is
total, and a value is one word with no trap flag. `natAdd` gives
`none` on an overflow. The divisor of `natDiv` and `natMod` is a
`NonZero Nat`, so the checker refuses a literal 0 with `REFUSE_PARTIAL`.
`langc eval` and the Wasm module give the same value for each entry.

The Wasm writer emits no trap form:

- No `unreachable`. A `SWITCH` on a constructor tag uses its last arm as
  the `br_table` default.
- No `memory.grow`. The memory minimum is equal to the maximum: the
  pages of the largest static heap bound of an entry.
- Each `div_u` and `rem_u` has a guard on its divisor.

`test/notraptool.c` reads each built module and refuses each of these
forms. The differential row runs each case on the module and compares
the result with the `langc eval` result. It runs node in the S3 gate.
A `trap` result on either side fails the gate.

The lowering lowers a value one time in a block. The blocks in that
block use the same result, but the two arms of a branch do not share
results.

The evaluation fuel (`EVAL_FUEL`) is a host limit, not a trap.

Decision record: `docs/PLAN.md` sections 1 and 2.

## Wasm layout

- The module has one memory of a fixed size and one mutable `i64` global,
  the heap pointer. The size is the pages of the largest heap bound of an
  entry (at least 1 page). `langc` adds the words of each heap block on the
  longest path of an entry. When an entry makes heap blocks in a loop with a
  run-time count, its heap use has no static bound: `langc ir` and `langc
  build` refuse it with `REFUSE_LOWER` (`heap use has no static bound`). A
  bound larger than 1 GiB gets the same refusal.
- Function 0 is the internal allocator `alloc (bytes : i32) -> i64`. It
  increases the heap pointer. It does not check the size, because the
  memory holds the heap bound of each entry.
- Function 1 + K is entry K. Each entry first sets the heap pointer to 0,
  so each call starts with an empty heap.
- A `Flag` argument is true when it is not 0. The entry sets it to 1.
- A word is 8 bytes. A heap block holds the tag in word 0 and the word of
  field K in word 1 + K. Each op is total; a value is one word.
- A tag switch is nested blocks and `br_table`. The last arm is the
  default, so a tag out of range runs the last arm.

## Domain slot

`domain/domain.lang` holds the families and the operations of the domain.
Only this file can declare a `family` (rule R1). `domain/README.md` tells
how to replace the sample domain.

## Gate

`make check` prints these lines when it is GREEN (`test/gate.sh`, then
`test/review.sh`, `build/fronttool` and `build/pathtool`):

| Line | What the gate does |
|---|---|
| `examples: 37 checked, 0 failed` | `langc check` accepts each `examples/*.lang` file (`--kit` for each file of `examples/kit.txt`) |
| `parse refusals: 19 checked` | each `test/parse` file fails with the code in `test/parse/expect.txt`; one file has 1100 nested parentheses |
| `check refusals: 206 checked` | each `test/check` file fails with the code in `test/check/expect.txt`; the `PATH_END` message shows both normal forms |
| `kit-only refusals: 10 checked` | each `REFUSE_KIT` row refuses a kit item in guest mode only: `langc --kit check` accepts the file |
| `time hint: 1 checked` | a `REFUSE_TIME` refusal names the op-count form (`len r`, an `OpPos`) |
| `kernel suffix: 1 checked` | the text line of a mapped code ends with the kernel code: `test/check/read-amount.lang` gives `REFUSE_DIV` and `(kernel REFUSE_READ)` |
| `json refusals: 225 checked` | `langc check --json` exits 1 for each row of `test/parse/expect.txt` and `test/check/expect.txt`: the code and a hint are on stdout, and stderr is empty |
| `json lines: 5 checked` | for each line of `test/json/expect.txt`, `langc check --json` gives the exit code of the line and the stdout of its `test/json` file; `check --json` with no file and with a file that does not exist exits 2 with no JSON |
| `fmt idempotent: 38 checked` | for `domain/domain.lang` and each example, the `fmt` text is canonical: `fmt` of that text gives the same text, and `fmt --check` exits 0 with no output |
| `fmt not canonical: 1 checked` | `fmt --check` refuses `test/fmt/check.lang` at line 3 |
| `fmt usage: 1 checked` | an unknown `fmt` argument is a usage error (exit 2) |
| `evals: 181 checked` | `langc eval` gives each result in `test/eval/expect.txt` |
| `crypto oracle: 600 rows, 0 mismatches` | `build/cryptotool` (`test/cryptotool.c`) prints one row for each oracle value: a kit file, a def, its arguments and the result. `langc --kit eval` gives the result of each row |
| `pairing fuel: prPccFuel exit 1, EVAL_FUEL` | on the arguments of the first `prPcc0` row, `langc --kit eval test/crypto/pairing.lang prPccFuel` (14 pairs) exits 1 with `EVAL_FUEL`: the check of the file uses 2169939 of the 40000000 steps, thus one call of `pairingCheckC` can then do 12 pairs at most |
| `poseidon constants: 76 lines, same as domain/domain.lang` | the print of `build/poseidongen` is the lines of `domain/domain.lang` from `def psM00` to the last line of `psAll` |
| `poseidon2C table: 228 lines, same as src/front/poseidon_c.h` | the print of `build/poseidongen c` is `src/front/poseidon_c.h` |
| `pairing constants: 9 lines, same as domain/domain.lang` | the print of `build/pairinggen` is the lines of `domain/domain.lang` from `def frobG1` to `def bnXBits` |
| `pairingCheckC table: 28 lines, same as src/front/pairing_c.h` | the print of `build/pairinggen c` is `src/front/pairing_c.h` |
| `fixtures: 40 lines, same as domain/domain.lang` | the print of `build/cryptotool fixtures` is the lines of `domain/domain.lang` from `-- @FIX@ begin` to `-- @FIX@ end` |
| `pairing fixtures: 35 lines, same as test/crypto/pairing.lang` | the print of `build/cryptotool pairing-fixtures` is the lines of `test/crypto/pairing.lang` from `-- @PFIX@ begin` to `-- @PFIX@ end` |
| `builds: 1 checked` | `langc --kit build examples/functions.lang -o OUT` writes a module that `wasm-validate` accepts |
| `ir: 37 lowered` | `langc ir` lowers each `examples/*.lang` file (`--kit` for each file of `examples/kit.txt`) and gives one `func` line for each `abi` line; `examples/functions.lang` has at least one entry |
| `ir refusals: 10 checked` | `langc --kit check` accepts and `langc --kit ir` refuses `test/ir/closure.lang`, `kmap.lang`, `tree.lang`, `spine.lang`, `lazy.lang`, `hop.lang`, `ptmulc.lang`, `p2c.lang`, `scc.lang` and `pcc.lang`, each with its code |
| `ir lowerings: 2 lowered` | the `langc --kit ir` text of `test/ir/natops.lang` and `test/ir/block.lang` is `test/ir/natops.ir` and `test/ir/block.ir` |
| `notrap modules: 4 checked` | `langc build` writes the modules of `test/ir/natops.lang`, `test/ir/block.lang` and `examples/state-no-history.lang`, and `wasm-validate` accepts each one. `build/notraptool` (`test/notraptool.c`) accepts these three modules and the module of `examples/functions.lang`: no `unreachable`, no `memory.grow`, only the opcodes of `src/wasmop.h`, a divisor guard before each `i64.div_u` and `i64.rem_u`, and memory 0 with maximum = minimum. `build` lowers the `state` family of `examples/state-no-history.lang` as a plain family |
| `notrap bad rows: 7 checked` | `build/notraptool` refuses each module of `test/notrap/bad/` (`wat2wasm` makes it from its `.wat` file): exit 1 and the message of its line in `test/notrap/bad/expect.txt`. The 7 modules have `unreachable`, `memory.grow`, an `i64.div_u` and an `i64.rem_u` with no guard, a memory maximum above the minimum, a memory with no maximum, and `i32.add` (an opcode that is not in `src/wasmop.h`) |
| `spine evals: 10 checked` | `langc --kit eval` gives each eval of `test/ir/spine.lang` at 4 |
| `wasm: 40 modules valid` | all examples, `test/ir/natops.lang`, `test/ir/block.lang` and `test/ir/proof-trap.lang` pass validation and `build/notraptool` |
| `differential: 1846 cases, 0 mismatches` | node and `langc eval` agree on every grid case; a `trap` fails |
| `gate: 0 failures` | the sum of all failures of `test/gate.sh` |
| `review regressions: passed` | `test/review.sh` passes |
| `domain regressions: passed` | `build/fronttool` (the domain declaration rows) passes |
| `path regressions: passed` | `build/pathtool` (the path rows) passes |

The gate also checks the usage and IO exit codes, five `eval` refusals and
the absence of em-dashes and en-dashes outside `design/DESIGN.md`. It
checks memory 0 of the modules of `test/ir/natops.lang` and
`examples/functions.lang` (0 heap words): 1 page, minimum = maximum.
`langc eval` refuses a Flag of 2 with `EVAL_ARGS` (`pick 2 4 9`). The doc
check of lang-template (`perl bin/doc-check.pl` with this repository as
the root) must report 0 problems.

The differential grid runs at S3. `test/grid.awk` makes cases from each
module's IR; node and `langc eval` must agree on every result.

## Known limits

- A `fold` over a bare `nil` cannot infer its type (`TYPE_INFER`). Use a
  definition with a declared `List` type.
- With no expected type, `either`, `map` and `cong` take the result type
  of the function at a fresh variable. A dependent result needs a
  declared type.
- A definition with a `List`, `Prod` or other non-scalar result is not an
  entry. `ir` and `abi` skip it.
- `langc eval` stops after 40000000 steps (`EVAL_FUEL`). The Wasm module
  has no step limit, so a large loop count can run on Wasm and stop in
  `eval`. The grid of `test/grid.awk` uses large values only for
  entries with no loop.
- The arena of `langc` stops at 2 GiB. The Wasm memory has a fixed size:
  minimum = maximum = the pages of the largest heap bound of an entry, at
  least 1 page. An entry with no static heap bound refuses
  (`REFUSE_LOWER`, `test/ir/spine.lang`).
- Source files are at most 1 MiB. `Nat` literals are at most
  18446744073709551615.
