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

void print1d(elem_t Out[1][SLIDES]) {
  for (int j = 0; j < SLIDES; j++) {
    printf("%d, ", Out[0][j]);
  }
  printf("\n");
}

void naive_matmul(elem_t left[LEN][LEN], elem_t right[LEN][LEN], elem_t out[LEN][LEN]) {
  for (int i = 0; i < LEN; i++) {
    for (int j = 0; j < LEN; j++) {
      for (int k = 0; k < LEN; k++) {
        out[i][j] += left[i][k] * right[k][j];
      }
    }
  }
}

void runner(elem_t left[LEN][LEN], elem_t right[LEN][LEN], elem_t out[LEN][LEN]) {
  tiled_matmul_auto(
    LEN, LEN, LEN,
    (elem_t*) left, (elem_t*) right, NULL, (elem_t*)out,
    LEN, LEN, LEN, LEN,
    1, 1, 1,
    0, 1, 0, false,
    false, false,
    false, 0,
    0,
    WS);
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

  uint64_t start_cpu = read_cycles();
  naive_matmul(Left, Right, Out);
  uint64_t end_cpu = read_cycles();
  printf("CPU conv took %llu cycles\n", end_cpu - start_cpu);

  uint64_t start_g = read_cycles();
  runner(Left, Right, Out);
  uint64_t end_g = read_cycles();
  printf("Hardware conv took %llu cycles\n", end_g - start_g);

  //print1d(Out);
  exit(0);
}