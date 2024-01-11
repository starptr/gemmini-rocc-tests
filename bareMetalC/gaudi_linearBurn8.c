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

elem_t muldiv255(elem_t a, elem_t b) {
    return (a * b) / 255;
}

void naive_linearBurn8(elem_t base[LEN][LEN], elem_t active[LEN][LEN], elem_t opacity, elem_t out[LEN][LEN]) {
    for (int i = 0; i < LEN; i++) {
        for (int j = 0; j < LEN; j++) {
            out[i][j] = active[i][j] + base[i][j] - 255;
        }
    }
}

void runner(elem_t base[LEN][LEN], elem_t active[LEN][LEN], elem_t opacity, elem_t out[LEN][LEN]) {
    tiled_resadd_auto(
        LEN, LEN,
        opacity, 1 - opacity,
        1,
        active[0], base[0],
        out[0],
        false,
        WS
    );
    out = GEMMINI_ACC_SCALE(out, 2);
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
  elem_t val = 0;
  for (int i = 0; i < LEN; i++) {
    for (int j = 0; j < LEN; j++) {
      Left[i][j] = val;
      Right[i][j] = val + 1;
      val++;
    }
  }

  uint64_t start_cpu = read_cycles();
  naive_linearBurn8(Left, Right, opacity, Out);
  uint64_t end_cpu = read_cycles();
  printf("CPU conv took %llu cycles\n", end_cpu - start_cpu);

  uint64_t start_g = read_cycles();
  runner(Left, Right, opacity, Out);
  uint64_t end_g = read_cycles();
  printf("Hardware conv took %llu cycles\n", end_g - start_g);

  //print1d(Out);
  exit(0);
}