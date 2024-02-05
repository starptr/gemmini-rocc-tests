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
// LEN*LEN

void naive_runner(elem_t left[1][SIZE], elem_t right[1][SIZE], elem_t* out) {
  static elem_t interm[1][SIZE];
  for (int i = 0; i < LEN; i++) {
      interm[0][i] = left[0][i] * right[0][i];
  }
  static elem_t unflat[LEN][LEN];
  int v = 0;
  for (int i = 0; i < LEN; i++) {
    for (int j = 0; j < LEN; j++) {
      unflat[i][j] = interm[0][v];
      v++;
    }
  }
  // reduce_sum
  for (int i = 0; i < LEN; i++) {
    for (int j = 0; j < LEN; j++) {
      *out += unflat[i][j];
    }
  }
}

void runner(elem_t left[1][SIZE], elem_t right[1][SIZE], elem_t* out, uint64_t* c1, uint64_t* c2) {
  static elem_t interm[1][SIZE];
  for (int i = 0; i < LEN; i++) {
      interm[0][i] = left[0][i] * right[0][i];
  }
  static elem_t unflat[LEN][LEN];
  int v = 0;
  for (int i = 0; i < LEN; i++) {
    for (int j = 0; j < LEN; j++) {
      unflat[i][j] = interm[0][v];
      v++;
    }
  }
  // reduce_sum
  *c1 = read_cycles();
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

  static elem_t Left[1][SIZE];
  static elem_t Right[1][SIZE];
  static elem_t Out;
  int v = 0;
  for (int i = 0; i < LEN; i++) {
      Left[0][i] = v;
      Right[0][i] = v;
      v++;
  }

  // trigger cycle count
    tiled_resadd_auto(
        1, 1,
        1, 1,
        1,
        Left[0], Right[0],
        &Out,
        false,
        WS
    );

  uint64_t start_cpu = read_cycles();
  naive_runner(Left, Right, Out);
  uint64_t end_cpu = read_cycles();
  printf("CPU conv took %llu cycles\n", end_cpu - start_cpu);

  //uint64_t c1, c2;
  //uint64_t start_g = read_cycles();
  //runner(Left, Right, &Out, &c1, &c2);
  //uint64_t end_g = read_cycles();
  //printf("Hardware conv took %llu cycles\n", end_g - start_g);
  //printf("Kernel took %llu cycles\n", c2 - c1);

  //print1d(Out);
  exit(0);
}