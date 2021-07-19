#include <stdint.h>
#include <stdio.h>

void exec(uint32_t N, const char * src, char * dst)
{
  // can't map void ?
#pragma omp target map(to: src[0:N]) map(from: dst[0:N])
  {
    const uint32_t * s = (const uint32_t*) src;
    uint32_t * d = (uint32_t*) dst;

    for (uint32_t i = 0; i < N / sizeof(uint32_t); i++)
      {
        d[i] = 2*s[i];
      }
  }
}

int main()
{
  uint32_t src[128];
  uint32_t dst[128];
  uint32_t res[128];
  uint32_t N = 128;

  for (uint32_t i = 0; i < N; i++)
    {
      src[i] = i;
      dst[i] = 0;
      res[i] = 2*i;
    }

  exec(N*sizeof(uint32_t), (char*)&src[0], (char*)&dst[0]);

  uint32_t err = 0;
  for (uint32_t i = 0; i < N; i++)
    {
      if (dst[i] != res[i])
        {
          err++;
          printf("error at %u, %u != %u\n", i, dst[i], res[i]); 
        }
    }
  
  if (err != 0)
    {
      return 1;
    }

  printf("success\n");
  return 0;
}
