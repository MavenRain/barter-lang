/* The `langc check --json` writer (S8, Q-S8-3 (a)). The escapes and the
   key order of tranche-lang src/front/json.c, without bounds, fails and
   tables. */
#ifndef LANG_FRONT_JSON_H
#define LANG_FRONT_JSON_H

#include <stdio.h>

#include "front/check.h"

/* Writes TEXT as a JSON string: `"`, `\\`, newline and tab get a short
   escape, other control bytes `\u00XX`, an invalid UTF-8 byte `�`,
   valid UTF-8 raw. */
void json_string_n(const char *text, size_t len, FILE *out);
void json_string(const char *text, FILE *out);
/* The fixed hint of CODE (src/front/codes.c), or NULL when CODE has no
   row. */
const char *code_hint(const char *code);
/* Writes the one-line object of `langc check --json` and a newline:
   `{"diagnostics":[...],"progress":{...}}`. DIAG gives the one diagnostic
   when DIAG->set, else the list is empty. The diagnostic keys are code,
   kernel, file, line, col, def, message, hint in this order. kernel is
   null when the kernel code is CODE, def is null when there is no def,
   line and col are null when there is no position. PATH is the program
   file, as given. */
void json_check(FILE *out, const char *path, const Diag *diag, const CheckProgress *progress);

#endif
