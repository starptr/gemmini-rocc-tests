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

void runner(elem_t left[LEN][LEN], elem_t right[LEN][LEN], elem_t out[LEN][LEN], elem_t ss) {
  elem_t interm = ss / LEN; // sizeof in gen
  interm = interm + 1;
  interm = sqrt(interm);
  interm = 1 / interm;

  static elem_t interm2[LEN][LEN];
  for (int i = 0; i < LEN; i++) {
    for (int j = 0; j < LEN; j++) {
      interm2[i][j] = left[i][j] * right[i][j];
    }
  }
  out = GEMMINI_ACC_SCALE(interm2, interm);
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
  elem_t ss = v;

  uint64_t start_g = read_cycles();
  runner(Left, Right, Out, ss);
  uint64_t end_g = read_cycles();
  printf("Hardware conv took %llu cycles\n", end_g - start_g);

  //print1d(Out);
  exit(0);
}