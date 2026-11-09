/* The `langc check --json` writer (S8, Q-S8-3 (a)). The escapes are those
   of tranche-lang src/front/json.c. */
#include "front/json.h"

#include <stdlib.h>
#include <string.h>

static void put_char(unsigned char c, FILE *out) {
  const char *escape = c == '"' ? "\\\"" : c == '\\' ? "\\\\" : c == '\n' ? "\\n" : c == '\t' ? "\\t" : NULL;
  if (escape != NULL) {
    fputs(escape, out);
    return;
  }
  if (c < 0x20) {
    fprintf(out, "\\u%04x", c);
    return;
  }
  fputc(c, out);
}

/* The byte count of the UTF-8 character at TEXT, or 0 when the bytes are
   not valid UTF-8. */
static size_t utf8_size(const unsigned char *text, size_t len) {
  unsigned char first = text[0];
  size_t want = first < 0x80 ? 1 : first >= 0xc2 && first <= 0xdf ? 2 : first >= 0xe0 && first <= 0xef ? 3 : first >= 0xf0 && first <= 0xf4 ? 4 : 0;
  size_t i;
  if (want == 0 || want > len) return 0;
  for (i = 1; i < want; i++)
    if ((text[i] & 0xc0) != 0x80) return 0;
  if (want == 3 && ((first == 0xe0 && text[1] < 0xa0) || (first == 0xed && text[1] >= 0xa0))) return 0;
  if (want == 4 && ((first == 0xf0 && text[1] < 0x90) || (first == 0xf4 && text[1] >= 0x90))) return 0;
  return want;
}

void json_string_n(const char *text, size_t len, FILE *out) {
  const unsigned char *bytes = (const unsigned char *)text;
  fputc('"', out);
  for (size_t pos = 0; pos < len;) {
    size_t width = utf8_size(bytes + pos, len - pos);
    /* Paths and truncated diagnostic buffers can contain invalid bytes. */
    if (width == 0) {
      fputs("\\ufffd", out);
      pos++;
      continue;
    }
    for (size_t i = 0; i < width; i++) put_char(bytes[pos + i], out);
    pos += width;
  }
  fputc('"', out);
}

void json_string(const char *text, FILE *out) {
  json_string_n(text, strlen(text), out);
}

static void put_text_or_null(const char *text, FILE *out) {
  if (text == NULL) {
    fputs("null", out);
    return;
  }
  json_string(text, out);
}

static void put_position(int value, FILE *out) {
  if (value <= 0) {
    fputs("null", out);
    return;
  }
  fprintf(out, "%d", value);
}

/* The lexer and the parser start the message with `PATH:LINE:COL: `: that
   head gives the position, and *REST is the text after it. Else the
   position is that of the failing program decl in PROGRESS. */
static void diag_position(const char *path, const Diag *diag, const CheckProgress *progress, int *line, int *col, const char **rest) {
  size_t path_len = strlen(path);
  const char *p = diag->msg + path_len;
  char *end = NULL;
  long l;
  long c;
  *line = progress->fail_line;
  *col = progress->fail_col;
  *rest = diag->msg;
  if (strncmp(diag->msg, path, path_len) != 0 || *p != ':') return;
  l = strtol(p + 1, &end, 10);
  if (end == p + 1 || *end != ':') return;
  p = end;
  c = strtol(p + 1, &end, 10);
  if (end == p + 1 || strncmp(end, ": ", 2) != 0) return;
  *line = (int)l;
  *col = (int)c;
  *rest = end + 2;
}

void json_check(FILE *out, const char *path, const Diag *diag, const CheckProgress *progress) {
  fputs("{\"diagnostics\":[", out);
  if (diag->set) {
    int line;
    int col;
    const char *rest;
    int has_kernel = diag->kernel != NULL && strcmp(diag->kernel, diag->code) != 0;
    int has_def = diag->def[0] != '\0' && strcmp(diag->def, "-") != 0;
    diag_position(path, diag, progress, &line, &col, &rest);
    fputs("{\"code\":", out);
    json_string(diag->code, out);
    fputs(",\"kernel\":", out);
    put_text_or_null(has_kernel ? diag->kernel : NULL, out);
    fputs(",\"file\":", out);
    json_string(path, out);
    fputs(",\"line\":", out);
    put_position(line, out);
    fputs(",\"col\":", out);
    put_position(col, out);
    fputs(",\"def\":", out);
    put_text_or_null(has_def ? diag->def : NULL, out);
    fputs(",\"message\":", out);
    json_string(rest, out);
    fputs(",\"hint\":", out);
    put_text_or_null(code_hint(diag->code), out);
    fputc('}', out);
  }
  fprintf(out, "],\"progress\":{\"parsed\":%d,\"typed\":%d,\"laws_discharged\":%d,\"laws_total\":%d}}\n",
          progress->parsed, progress->typed, progress->laws_discharged, progress->laws_total);
}
