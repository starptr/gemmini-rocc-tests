#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef BAREMETAL
#include <sys/mman.h>
#endif
#include "include/gemmini_testutils.h"
#define LEN 20000

void runner(elem_t left[1][LEN], int max_pos, elem_t out[1][LEN]) {
    // take is a no-op
    int size = max_pos;

  elem_t max_val = left[0][0];
  for (int i = 0; i < LEN; i++) {
    // worst case
        out[0][i] = left[0][i] * 1;
    if (left[0][i] > max_val) {
        out[0][i] = left[0][i] * 1;
    }
  }
  //out[0][0] = max_val;
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

  static elem_t Left[1][LEN];
  static int MaxPos = LEN;
  static elem_t Out[1][LEN];

  // trigger cycle count
    tiled_resadd_auto(
        1, 1,
        1, 1,
        1,
        Left[0], Left[0],
        Out[0],
        false,
        WS
    );
  uint64_t start_g = read_cycles();
  runner(Left, MaxPos, Out);
  uint64_t end_g = read_cycles();
  printf("Hardware conv took %llu cycles\n", end_g - start_g);

  //print1d(Out);
  exit(0);
}