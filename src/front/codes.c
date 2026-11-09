/* The fixed hint of each diagnostic code for `langc check --json` (S8,
   Q-S8-3 (a)): the section 7 codes, the kernel codes that stay, and the
   LEX, PARSE, IO and USAGE codes. A code with no row has a null hint; the
   gate refuses a test row whose code has no hint. */
#include "front/json.h"

#include <string.h>

static const struct {
  const char *code;
  const char *hint;
} HINTS[] = {
  {"CHECK_DEPTH", "Make the term less deep. Use a def for each part."},
  {"DOMAIN_FAMILY", "Use a family that domain/domain.lang declares."},
  {"EVAL_ARGS", "Give one argument for each parameter of the definition."},
  {"EVAL_DEPTH", "Make the evaluation less deep, or give smaller arguments."},
  {"EVAL_ENTRY", "Give the name of a definition in the program."},
  {"EVAL_FUEL", "Give smaller arguments. The evaluation has a step limit."},
  {"EVAL_PRINT", "Evaluate a definition whose value has a printed form."},
  {"FIN_CASE", "Give one case for each value of the Fin type."},
  {"FIN_RANGE", "Use a Fin literal that is less than the bound."},
  {"GLUE_BINDER", "Use the interval variable of the Glue type in its faces."},
  {"GLUE_BOUNDARY", "Make each Glue face agree with the base type on its boundary."},
  {"GLUE_EQUIV", "Give an equivalence for each face of the Glue type."},
  {"HCOMP_CAP", "Make the cap agree with each face of the hcomp at 0."},
  {"HCOMP_UNIVERSE", "Use hcomp only in a universe that the kernel accepts."},
  {"INTERNAL", "Report this error. The kernel found a state that it must not have."},
  {"INTERVAL_TYPE", "Use an interval term where the type I is necessary."},
  {"IO", "Make sure that the file exists and that you can read it."},
  {"IO_SIZE", "Make the program file smaller than the size limit."},
  {"LEX_CHAR", "Remove the character, or put a space between the number and the name."},
  {"LEX_NAT_RANGE", "Use a Nat literal that is not larger than 2^64 - 1."},
  {"NAME_UNKNOWN", "Declare the name before you use it, or correct its spelling."},
  {"OOM", "Make the program smaller. The memory limit is reached."},
  {"PARSE_DEPTH", "Make the term less deep. Use a def for each part."},
  {"PARSE_EXPECT", "Write the token that the parser expects at this position."},
  {"PATH_END", "Make the endpoints of the path agree with the Path type."},
  {"PATH_UIP", "Do not compare two paths. The kernel has no UIP rule."},
  {"REFUSE_AXIOM", "Remove the axiom. Give a def with a proof."},
  {"REFUSE_BASE", "Use the families and cofamilies of domain/domain.lang."},
  {"REFUSE_BOUND", "Make the term agree with the bound of the definition."},
  {"REFUSE_CKKS", "Use an integer word such as Amount. A program has no real numbers."},
  {"REFUSE_CLOCK", "Use the block height. A program cannot read a clock."},
  {"REFUSE_DATA", "Remove the law def. Only domain/domain.lang can declare a law def."},
  {"REFUSE_DIV", "Use the operations of the domain on this word. Do not read its value."},
  {"REFUSE_FLOOR", "Make the term agree with the floor rule of the domain."},
  {"REFUSE_FORMER", "Use a type former that a program can use."},
  {"REFUSE_HISTORY_WRITE", "Do not write the history. A program can only read it."},
  {"REFUSE_KIT", "Use this form only in a kit file with --kit."},
  {"REFUSE_LAW", "Use this name only in a law def of domain/domain.lang."},
  {"REFUSE_LOWER", "Use a form that the target can lower."},
  {"REFUSE_MAP", "Use a map form that the domain accepts."},
  {"REFUSE_NAME", "Use a different name. The domain keeps this name."},
  {"REFUSE_OPEN", "Use open only in a law def of domain/domain.lang."},
  {"REFUSE_PARTIAL", "Use fold or unfold. A program cannot use def rec or a forward name."},
  {"REFUSE_RETRO", "Apply the amendment from its height + 1. It cannot change the past."},
  {"REFUSE_RUNTIME_CLOSURE", "Give a first-order value. The runtime cannot return a closure."},
  {"REFUSE_RUNTIME_TREE", "Give a smaller value. The runtime cannot return this tree."},
  {"REFUSE_SIG", "Make the type agree with the signature that the domain declares."},
  {"REFUSE_STATE", "Use the state form that the domain accepts."},
  {"REFUSE_UNSEAL", "Use decrypt, observe and sealed values only in a law def of domain/domain.lang."},
  {"REFUSE_WORD", "Use a word type that the domain declares."},
  {"SYSTEM_COVER", "Give faces that cover all of the interval."},
  {"SYSTEM_OVERLAP", "Make the faces agree where they overlap."},
  {"TRANSP_LINE", "Make the line constant where the face of transp is true."},
  {"TYPE_ARITY", "Give the number of arguments that the type declares."},
  {"TYPE_INFER", "Add a type annotation to the term."},
  {"TYPE_MISMATCH", "Make the term agree with the necessary type."},
  {"TYPE_NOT_FUNCTION", "Apply only a term that has a function type."},
  {"TYPE_UNIVERSE", "Use a type in a universe of the correct level."},
  {"UNGLUE_TYPE", "Use unglue only on a term of a Glue type."},
  {"USAGE", "Run langc with a command and a program path."},
};

const char *code_hint(const char *code) {
  size_t i;
  for (i = 0; code != NULL && i < sizeof HINTS / sizeof HINTS[0]; i++)
    if (strcmp(HINTS[i].code, code) == 0) return HINTS[i].hint;
  return NULL;
}
