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

int
main(int argc, char *argv[])
{
  // Select input stream
  int i = 1;
  char const *filename;
  FILE *fp;

  if (argc > 2) errx(1, "too many arguments");

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
    size_t wrap_count = 0;
    for (;;) {
      // read input bytes
      unsigned char buf[3] = {0};
      size_t nr = fread(buf, 1, sizeof buf, fp);
      if (nr < sizeof buf && ferror(stdin)) err(EXIT_FAILURE, "%s", argv[1]);
      if (nr == 0) break; // end of file, empty buffer

      // convert input bytes to integer indicies
      // Shift 3 bytes into bits
      long long unsigned int bits = 0;
      bits |= buf[0];
      bits <<= CHAR_BIT;
      bits |= buf[1];
      bits <<= CHAR_BIT;
      bits |= buf[2];

      // convert integer indices to base64 alphabet characters and write output while handling line wrapping
      char b64o[4];
      for (int i = sizeof b64o - 1; i >= 0; --i) {
        unsigned char idx = bits & 0x3f;
        b64o[i] = b64a[idx];
        bits >>= 6;
      }

      if (nr < 3) b64o[3] = '=';
      if (nr < 2) b64o[2] = '=';

      fwrite(b64o, 1, sizeof b64o, stdout);

      wrap_count += 4;
      if (wrap_count >= 76) {
        putchar('\n');
        wrap_count = 0;
      }

      if (nr < sizeof buf) break; // end of file, partial buffer
    }
    if (wrap_count != 0) putchar('\n');
    if (fp != stdin)
      fclose(fp);
  }
  // cleanup
  fflush(stdout);
  if (ferror(stdout)) err(EXIT_FAILURE, "stdout");
  return EXIT_SUCCESS;
}
