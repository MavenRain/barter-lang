/* The type checker, the refusals R1-R4 and the eval command. SHARED by the
   tcc kits. */
#ifndef LANG_FRONT_CHECK_H
#define LANG_FRONT_CHECK_H

#include "front/core.h"

#define ENTRY_PARAMS_MAX 512u /* 16 before sealed-lang S2 (R23): slashUsr takes 19 words; 32 before S8b2 (Q-S8b-4 (a)): a Pt1 entry takes 16 Words, plus an Fr; 64 before S8c (Q-S8c-9 (a)): an Fp12 row takes 96 Words; 128 before S8c u3b (Q-S8c-11 (a)): a MilSt row takes 144 Words; 256 before S8c u4 (Q-S8c-15 (a)): a 10-pair pairingCheckC row takes 481 Words */

/* An entry: a program definition of type Nat, Flag, or a function of Nat and
   Flag arguments to a Nat or Flag result. */
typedef struct {
  uint32_t param_count;
  int param_flag[ENTRY_PARAMS_MAX];
  int param_word[ENTRY_PARAMS_MAX]; /* S8b: the parameter is a Word */
  int result_flag;
  int result_option;
} Entry;

/* Checks the domain declarations, then the program, into M. Returns 1, or 0
   after a diagnostic. */
/* The check mode (orbit-lang S3). In guest mode a program declaration
   cannot use a kit item (REFUSE_KIT). Kit mode is the trusted test scope of
   the kit gate (`langc --kit`). */
typedef enum {
  CHECK_GUEST,
  CHECK_KIT
} CheckMode;

/* The progress counts of `langc check --json` (S8, Q-S8-3 (a)), and the
   line and col of the failing program decl (0 when there is none). */
typedef struct {
  int parsed;
  int typed;
  int laws_discharged;
  int laws_total;
  int fail_line;
  int fail_col;
} CheckProgress;

int check_program(Arena *arena, const DeclList *decls, Machine *m, Diag *diag, CheckMode mode);
/* check_program, then the progress counts of `langc check --json` (S8,
   Q-S8-3 (a)): parsed and typed count the program decls (a block counts
   each sort), laws_discharged the typed program defs with law_only, and
   laws_total adds the failing program decl when its kernel code is
   REFUSE_LAW. */
int check_program_progress(Arena *arena, const DeclList *decls, Machine *m, Diag *diag, CheckMode mode, CheckProgress *progress);
/* Returns 1 when TYPE is the type of an entry. */
int entry_of(Machine *m, const Value *type, Entry *out);
/* Evaluates the definition NAME on ARGS and prints the result. Returns 0, 1
   after a diagnostic, or 2 after an EVAL_ARGS diagnostic. */
int eval_command(Machine *m, const char *name, char *const *args, int arg_count, FILE *out, FILE *err);

#endif
