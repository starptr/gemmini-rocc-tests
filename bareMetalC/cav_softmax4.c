#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef BAREMETAL
#include <sys/mman.h>
#endif
#include "include/gemmini_testutils.h"
#define LEN 16

void runner(elem_t left[LEN][LEN], elem_t max_pos, elem_t sum, elem_t out[LEN][LEN]) {
    // take is a no-op
    int size = max_pos;

    // scalar op
    out = GEMMINI_ACC_SCALE(left, sum);
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

  static elem_t TLeft[LEN][LEN];
  static elem_t TRight[LEN][LEN];
  static elem_t TOut[LEN][LEN];
  int v = 0;
  for (int i = 0; i < LEN; i++) {
    for (int j = 0; j < LEN; j++) {
      TLeft[i][j] = v;
      TRight[i][j] = v;
      v++;
    }
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

  static elem_t Left[LEN][LEN];
  static elem_t MaxPos;
  static elem_t Sum;
  static elem_t Out[LEN][LEN];

  uint64_t start_g = read_cycles();
  runner(Left, MaxPos, Sum, Out);
  uint64_t end_g = read_cycles();
  printf("Hardware conv took %llu cycles\n", end_g - start_g);

  //print1d(Out);
  exit(0);
}