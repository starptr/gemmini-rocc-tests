#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef BAREMETAL
#include <sys/mman.h>
#endif
#include "include/gemmini_testutils.h"
#define LEN 200
#define SIZE 40000

void runner(elem_t left[1][SIZE], elem_t max_pos, elem_t* out, uint64_t* c1, uint64_t* c2) {
    // take is a no-op
    int size = max_pos;

  static elem_t unflat[LEN][LEN];
  int v = 0;
  for (int i = 0; i < LEN; i++) {
    for (int j = 0; j < LEN; j++) {
      unflat[i][j] = left[0][v];
      v++;
    }
  }
  *c1 = read_cycles();
  // reduce_sum
  tiled_global_average(unflat[0], out,
    1, 1, LEN, 1);
  *c2 = read_cycles();
}
int main() {
#ifndef BAREMETAL
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
      perror("mlockall failed");
      exit(1);
    }
#endif

  printf("Size of DIM: %d\n", DIM);
  printf("Flush Gemmini TLB of stale virtual addresses\n");
  gemmini_flush(0);

  static elem_t TLeft[1][LEN];
  static elem_t TRight[1][LEN];
  static elem_t TOut[1][LEN];
  int v = 0;
  for (int i = 0; i < LEN; i++) {
      TLeft[0][i] = v;
      TRight[0][i] = v;
      v++;
  }

  // trigger cycle count
    tiled_resadd_auto(
        1, 1,
        1, 1,
        1,
        TLeft[0], TRight[0],
        TOut[0],
        false,
        WS
    );

  static elem_t Left[1][SIZE];
  static elem_t MaxPos;
  static elem_t Out;

  uint64_t c1, c2;
  uint64_t start_g = read_cycles();
  runner(Left, MaxPos, &Out, &c1, &c2);
  uint64_t end_g = read_cycles();
  printf("Hardware conv took %llu cycles\n", end_g - start_g);
  printf("Kernel took %llu cycles\n", c2 - c1);

  //print1d(Out);
  exit(0);
}