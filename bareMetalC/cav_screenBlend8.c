#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef BAREMETAL
#include <sys/mman.h>
#endif
#include "include/gemmini_testutils.h"
#define LEN 315

elem_t muldiv255(elem_t a, elem_t b) {
    return (a * b) / 255;
}

elem_t screen(elem_t a, elem_t b) {
    return a + b - muldiv255(a, b);
}

void naive_screenBlend8(elem_t base[LEN][LEN], elem_t active[LEN][LEN], elem_t opacity, elem_t out[LEN][LEN]) {
    for (int i = 0; i < LEN; i++) {
        for (int j = 0; j < LEN; j++) {
            out[i][j] = screen(active[i][j], base[i][j]);
        }
    }
}

void runner(elem_t base[LEN][LEN], elem_t active[LEN][LEN], elem_t opacity, elem_t out[LEN][LEN], uint64_t* c1, uint64_t* c2, uint64_t* c3, uint64_t* c4) {
    static elem_t Intermediate[LEN][LEN];
    *c1 = read_cycles();
    tiled_resadd_auto(
        LEN, LEN,
        1, 1,
        1,
        active[0], base[0],
        Intermediate[0],
        false,
        WS
    );
    *c2 = read_cycles();
    for (int i = 0; i < LEN; i++) {
        for (int j = 0; j < LEN; j++) {
            out[i][j] = active[i][j] * base[i][j];
        }
    }
    *c3 = read_cycles();
    out = GEMMINI_ACC_SCALE(out, 0.0039215686);
    tiled_resadd_auto(
        LEN, LEN,
        1, 1,
        1,
        Intermediate[0], out[0],
        out[0],
        false,
        WS
    );
    *c4 = read_cycles();
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

  //uint64_t start_cpu = read_cycles();
  //naive_screenBlend8(Left, Right, opacity, Out);
  //uint64_t end_cpu = read_cycles();
  //printf("CPU conv took %llu cycles\n", end_cpu - start_cpu);

    uint64_t c1, c2, c3, c4;
  uint64_t start_g = read_cycles();
  runner(Left, Right, opacity, Out, &c1, &c2, &c3, &c4);
  uint64_t end_g = read_cycles();
  printf("Hardware conv took %llu cycles\n", end_g - start_g);
  printf("Kernel1 conv took %llu cycles\n", c2 - c1);
  printf("Kernel2 conv took %llu cycles\n", c4 - c3);

  //print1d(Out);
  exit(0);
}