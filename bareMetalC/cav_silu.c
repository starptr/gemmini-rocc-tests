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

#include <math.h>

void runner(elem_t left[LEN], elem_t out[LEN], elem_t until) {
  for (int i = 0; i < until; i++) {
    out[i] = 1 / (1 + pow(2.7, left[i])) * left[i];
  }
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

  static elem_t Left[LEN][LEN];
  static elem_t Right[LEN][LEN];
  static elem_t Out[LEN][LEN];
  int v = 0;
  for (int i = 0; i < LEN; i++) {
    for (int j = 0; j < LEN; j++) {
      Left[i][j] = v;
      Right[i][j] = v;
      v++;
    }
  }
  elem_t until = LEN;

  // trigger cycle count
    tiled_resadd_auto(
        1, 1,
        1, 1,
        1,
        Left[0], Right[0],
        Out[0],
        false,
        WS
    );
  uint64_t start_g = read_cycles();
  runner(Left[0], Out[0], until);
  uint64_t end_g = read_cycles();
  printf("Hardware conv took %llu cycles\n", end_g - start_g);

  //print1d(Out);
  exit(0);
}