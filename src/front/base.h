/* Arena allocation and the diagnostic sink. SHARED by the tcc kits. */
#ifndef LANG_FRONT_BASE_H
#define LANG_FRONT_BASE_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct ArenaChunk ArenaChunk;

/* Every compiler object lives in one arena. The arena frees all at once. */
typedef struct {
  ArenaChunk *head;
  size_t used_bytes;
  size_t limit_bytes;
} Arena;

void arena_init(Arena *arena, size_t limit_bytes);
/* Returns zeroed memory, or NULL when the limit is reached. */
void *arena_alloc(Arena *arena, size_t size);
char *arena_strndup(Arena *arena, const char *text, size_t len);
void arena_release(Arena *arena);

/* The first failure wins. Later failures do not overwrite it. */
typedef struct {
  int set;
  const char *code;
  /* The kernel code when CODE is a section 7 code of another name (S8,
     Q-S8-2 (a)); else the same as CODE, or NULL. */
  const char *kernel;
  char def[64];
  char msg[512];
} Diag;

void diag_init(Diag *diag);
/* Records the failure and returns 0, so a caller can write
   `return diag_fail(...)`. DEF is NULL when no definition applies. */
int diag_fail(Diag *diag, const char *code, const char *def, const char *fmt, ...);
/* As diag_fail. The section 7 CODE has the kernel code KERNEL (S8). */
int diag_fail_kernel(Diag *diag, const char *code, const char *kernel, const char *def, const char *fmt, ...);
/* Prints `langc: CODE: DEF: message`, then ` (kernel K)` when the kernel
   code K is not CODE. */
void diag_print(const Diag *diag, FILE *out);

#endif
