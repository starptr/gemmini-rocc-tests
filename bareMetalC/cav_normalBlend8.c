#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef BAREMETAL
#include <sys/mman.h>
#endif
#include "include/gemmini_testutils.h"
#define LEN 400
#define SIZE 160000

elem_t muldiv255(elem_t a, elem_t b) {
    return (a * b) / 255;
}

//void naive_normalBlend8(elem_t base[1][LEN], elem_t active[1][LEN], elem_t opacity, elem_t out[1][LEN]) {
//    for (int i = 0; i < LEN; i++) {
//        out[0][i] = muldiv255(opacity, active[0][i]) + muldiv255(255 - opacity, base[0][i]);
//    }
//}

void runner(elem_t base[LEN][LEN], elem_t active[LEN][LEN], elem_t opacity, elem_t out[LEN][LEN], uint64_t* c1, uint64_t* c2) {
    const elem_t* ao = GEMMINI_ACC_SCALE(active, opacity);
    const elem_t* ao2 = GEMMINI_ACC_SCALE(ao, 0.0039215686);
    const elem_t* bo = GEMMINI_ACC_SCALE(base, 1 - opacity);
    const elem_t* bo2 = GEMMINI_ACC_SCALE(bo, 0.0039215686);
    *c1 = read_cycles();
    tiled_resadd_auto(
        LEN, LEN,
        opacity, 1 - opacity,
        1,
        active[0], base[0],
        out[0],
        false,
        WS
    );
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

  static elem_t Left[LEN][LEN];
  static elem_t Right[LEN][LEN];
  static elem_t Out[LEN][LEN];
  static elem_t opacity = 0.3;
  for (int i = 0; i < LEN; i++) {
    Left[0][i] = i;
    Right[0][i] = i;
  }

  //uint64_t start_cpu = read_cycles();
  //naive_normalBlend8(Left, Right, opacity, Out);
  //uint64_t end_cpu = read_cycles();
  //printf("CPU conv took %llu cycles\n", end_cpu - start_cpu);

  uint64_t c1, c2;
  uint64_t start_g = read_cycles();
  runner(Left, Right, opacity, Out, &c1, &c2);
  uint64_t end_g = read_cycles();
  printf("Hardware conv took %llu cycles\n", end_g - start_g);
  printf("Kernel took %llu cycles\n", c2 - c1);

  //print1d(Out);
  exit(0);
}