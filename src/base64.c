#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

static char const b64a[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                           "abcdefghijklmnopqrstuvwxyz"
                           "0123456789"
                           "+/";
static char const pad_char = '=';        /* Padding character */

#define SIX_BIT 6

int
main(int argc, char *argv[])
{
  // Select input stream
  int i = 1;
  char const *filename;
  FILE *fp;

  if (argc >= 3 && strcmp(argv[1], "-") != 0) err(EXIT_FAILURE, "%s", argv[1]);

  if (argc < 2 || (argc > 2 && strcmp(argv[1], "-") == 0)) {
    // No file, read from stdin
    filename = "-";
    fp = stdin;
    goto dfl_stdin;
  }

  for (; i < argc; i++) {
    filename = argv[i];

    if (strcmp(filename, "-") == 0) {
      // Filename is "-", read from stdin
      fp = stdin;
    } else {
      // Open the file
      fp = fopen(filename, "r");
      if (!fp) err(EXIT_FAILURE, "%s", filename);
    }

  // loop
  dfl_stdin:;
    unsigned char buf[BUFSIZ];
    int new_line = 0;
    for (;;) {
      // read input bytes
      size_t nr = fread(buf, 1, sizeof buf, fp);
      if (nr < sizeof buf && ferror(stdin)) err(EXIT_FAILURE, "%s", argv[1]);
      if (nr == 0) break; // end of file, empty buffer

      // convert input bytes to integer indicies
      size_t wrap_count = 0;
      for (size_t i = 0; i < nr; ++i) {
        new_line = 1;
        // Shift 3 bytes into a dword
        int bytes = 1;
        unsigned long dword = buf[i];

        // Second byte
        dword <<= CHAR_BIT;
        if (++i < nr) {
          dword |= buf[i];
          ++bytes;
        }

        // Third byte
        dword <<= CHAR_BIT;
        if (++i < nr) {
          dword |= buf[i];
          ++bytes;
        }

        // convert integer indices to base64 alphabet characters and write output while handling line wrapping
        for (int j = 0; j < ((3 * CHAR_BIT) / SIX_BIT); ++j) {
          if ((j * SIX_BIT) > (bytes * CHAR_BIT)) {
            putchar(pad_char);
          } else {
            int idx = dword >> (3 * CHAR_BIT - SIX_BIT);
            char c = b64a[idx];
            putchar(c);
            dword <<= SIX_BIT; /* Left shift */
            dword &= 0xffffff;     /* Discard upper bits > 24th position */
          }
          ++wrap_count;
          if (wrap_count == 76) {
            putchar('\n');
            wrap_count = 0;
            new_line = 0;
          }
        }
      }
      if (new_line == 1) putchar('\n');
      if (nr < sizeof buf) break; // end of file, partial buffer
    }
    if (fp != stdin)
      fclose(fp);
  }
  // cleanup
  fflush(stdout);
  if (ferror(stdout)) err(EXIT_FAILURE, "stdout");
  return EXIT_SUCCESS;
}
