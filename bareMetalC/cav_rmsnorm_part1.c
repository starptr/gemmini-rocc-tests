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

void runner(elem_t left[LEN][LEN], elem_t right[LEN][LEN], elem_t* out) {
  static elem_t interm[LEN][LEN];
  for (int i = 0; i < LEN; i++) {
    for (int j = 0; j < LEN; j++) {
      interm[i][j] = left[i][j] * right[i][j];
    }
  }
  // reduce_sum
  tiled_global_average(interm[0], out,
    1, 1, LEN, 1);
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
  static elem_t Out;
  int v = 0;
  for (int i = 0; i < LEN; i++) {
    for (int j = 0; j < LEN; j++) {
      Left[i][j] = v;
      Right[i][j] = v;
      v++;
    }
  }

  uint64_t start_g = read_cycles();
  runner(Left, Right, &Out);
  uint64_t end_g = read_cycles();
  printf("Hardware conv took %llu cycles\n", end_g - start_g);

  //print1d(Out);
  exit(0);
}