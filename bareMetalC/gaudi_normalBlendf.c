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

void naive_normalBlendf(elem_t base[LEN][LEN], elem_t active[LEN][LEN], elem_t opacity, elem_t out[LEN][LEN]) {
    for (int i = 0; i < LEN; i++) {
        out[0][i] = opacity * active[0][i] + (1 - opacity) * base[0][i];
    }
}

void runner(elem_t base[LEN][LEN], elem_t active[LEN][LEN], elem_t opacity, elem_t out[LEN][LEN]) {
    const elem_t* ao = GEMMINI_ACC_SCALE(active, opacity);
    const elem_t* bo = GEMMINI_ACC_SCALE(base, 1 - opacity);
    tiled_resadd_auto(
        LEN, LEN,
        opacity, 1 - opacity,
        1,
        active[0], base[0],
        out[0],
        false,
        WS
    );
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
  static elem_t opacity = 0.3;
  for (int i = 0; i < LEN; i++) {
    Left[0][i] = i;
    Right[0][i] = i;
  }

  uint64_t start_cpu = read_cycles();
  naive_normalBlendf(Left, Right, opacity, Out);
  uint64_t end_cpu = read_cycles();
  printf("CPU conv took %llu cycles\n", end_cpu - start_cpu);

  uint64_t start_g = read_cycles();
  runner(Left, Right, opacity, Out);
  uint64_t end_g = read_cycles();
  printf("Hardware conv took %llu cycles\n", end_g - start_g);

  //print1d(Out);
  exit(0);
}