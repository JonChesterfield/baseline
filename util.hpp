#ifndef UTIL_HPP_INCLUDED
#define UTIL_HPP_INCLUDED

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct raiifree {
  raiifree(char *x) : x(x) {}
  ~raiifree() { free(x); }

private:
  char *x;
};

static inline char *from_file(const char *filename) {
  FILE *f = fopen(filename, "r");
  if (!f)
    return NULL;

  struct onexit_ {
    onexit_(FILE *f) : f(f) {}
    ~onexit_() {
      if (f)
        fclose(f);
    }

  private:
    FILE *f;
  } onexit(f);

  if (ferror(f)) {
    return NULL;
  }

  char *buffer = NULL;
  unsigned blocksize = 8192;

  // warning, not unit tested
  for (uint64_t blocks_read = 0;;) {
    {
      char *tmp = (char *)realloc(buffer, ((blocks_read + 1) * blocksize));
      if (!tmp) {
        free(buffer);
        return NULL;
      }
      buffer = tmp;
    }

    size_t r = fread(&buffer[blocksize * blocks_read], 1, blocksize, f);
    blocks_read++;

    assert(r <= blocksize);
    if (r < blocksize) {
      if (feof(f)) {
        // reached end of file
        uint64_t N = ((blocks_read - 1) * blocksize) + r;
        buffer[N] = '\0';
        return buffer; // realloc down?
      } else if (ferror(f)) {
        free(buffer);
        return NULL;
      }
    }
  }
}

inline int
compare(const char *srcfile, void (*host)(unsigned, const char *, char *),
        int (*target)(const char *srcfile, unsigned, const char *, char *),
        unsigned N, const char *src) {
  char *dst = (char *)malloc(N);
  char *res = (char *)malloc(N);

  raiifree d(dst), r(res);
  if (!dst || !res) {
    printf("Malloc failed\n");
    return 1;
  }

  // host succeeds
  host(N, src, res);

  // target might not
  int rc = target(srcfile, N, (const char *)&src[0], (char *)&dst[0]);
  if (rc != 0) {
    printf("target offload failed\n");
    return rc;
  }

  // target ran, did it work
  if (memcmp(res, dst, N) != 0) {
    printf("host/target differ\n");
    for (unsigned i = 0; i < N; i++) {
      if (res[i] != dst[i]) {
        printf("byte[%u]: %d != %d\n", i, (int)res[i], (int)dst[i]);
      }
    }
    return 1;
  }

  return 0;
}

#endif
