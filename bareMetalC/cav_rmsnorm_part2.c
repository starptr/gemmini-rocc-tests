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

#include <math.h>

void runner(elem_t left[1][LEN], elem_t right[1][LEN], elem_t out[1][LEN], elem_t ss) {
  elem_t interm = ss / LEN; // sizeof in gen
  interm = interm + 1;
  interm = sqrt(interm);
  interm = 1 / interm;

  for (int i = 0; i < LEN; i++) {
      out[0][i] = left[0][i] * right[0][i];
  }
  out = GEMMINI_ACC_SCALE(out, interm);
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

  static elem_t Left[1][LEN];
  static elem_t Right[1][LEN];
  static elem_t Out[1][LEN];
  int v = 0;
  for (int i = 0; i < LEN; i++) {
      Left[0][i] = v;
      Right[0][i] = v;
      v++;
  }
  elem_t ss = v;

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
  runner(Left, Right, Out, ss);
  uint64_t end_g = read_cycles();
  printf("Hardware conv took %llu cycles\n", end_g - start_g);

  //print1d(Out);
  exit(0);
}