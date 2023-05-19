// See LICENSE for license details.

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef BAREMETAL
#include <sys/mman.h>
#endif
#include "include/gemmini_testutils.h"

#define LEN 1000
#define SLIDES (LEN - 1)

void naive_conv1d(elem_t input[2][LEN],
		elem_t output[1][SLIDES]) {
  for (int i = 0; i < LEN; i++) {
    input[0][i] = i;
  }
  
  for (int i = 0; i < SLIDES; i++) {
    output[0][i] = input[0][i] + input[0][i+1];
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

  static elem_t In[2][LEN];
  static elem_t Out[1][SLIDES];

  uint64_t start_cpu = read_cycles();
  naive_conv1d(In, Out);
  uint64_t end_cpu = read_cycles();
  printf("CPU conv took %llu cycles\n", end_cpu - start_cpu);

  static elem_t weights[2][2];
  weights[0][0] = 1;
  weights[0][1] = 1;
  weights[1][0] = 0;
  weights[1][1] = 0;

  uint64_t start_g = read_cycles();
  tiled_conv_auto(
		  1, 1, LEN, 1,
		  1, 1, SLIDES,
		  1, 1, 1, 0, 2,
		  false, false, false, false, false,
		  (elem_t*)In,
		  (elem_t*)weights,
		  NULL,
		  (elem_t*)Out,
		  0, 0, 0, 0, 0, WS);
  uint64_t end_g = read_cycles();
  printf("Hardware conv took %llu cycles\n", end_g - start_g);

  ////////

  //printf("Initialize our input and output matrices in main memory\n");
  //elem_t In[DIM][DIM];
  //elem_t Out[DIM][DIM];

  printMatrix(In);
  printMatrix(Out);

  //elem_t Identity[DIM][DIM];
  //for (size_t i = 0; i < DIM; i++)
  //  for (size_t j = 0; j < DIM; j++)
  //    Identity[i][j] = i == j;

  //printf("Calculate the scratchpad addresses of all our matrices\n");
  //printf("  Note: The scratchpad is \"row-addressed\", where each address contains one matrix row\n");
  //size_t In_sp_addr = 0;
  //size_t Out_sp_addr = DIM;
  //size_t Identity_sp_addr = 2*DIM;

  //printf("Move \"In\" matrix from main memory into Gemmini's scratchpad\n");
  //gemmini_config_ld(DIM * sizeof(elem_t));
  //gemmini_config_st(DIM * sizeof(elem_t));
  //gemmini_mvin(In, In_sp_addr);

  //printf("Move \"Identity\" matrix from main memory into Gemmini's scratchpad\n");
  //gemmini_mvin(Identity, Identity_sp_addr);

  //printf("Multiply \"In\" matrix with \"Identity\" matrix with a bias of 0\n");
  //gemmini_config_ex(OUTPUT_STATIONARY, 0, 0);
  //gemmini_preload_zeros(Out_sp_addr);
  //gemmini_compute_preloaded(In_sp_addr, Identity_sp_addr);

  //printf("Move \"Out\" matrix from Gemmini's scratchpad into main memory\n");
  //gemmini_config_st(DIM * sizeof(elem_t));
  //gemmini_mvout(Out, Out_sp_addr);

  //printf("Fence till Gemmini completes all memory operations\n");
  //gemmini_fence();

  //printf("Check whether \"In\" and \"Out\" matrices are identical\n");
  //if (!is_equal(In, Out)) {
  //  printf("Input and output matrices are different!\n");
  //  printf("\"In\" matrix:\n");
  //  printMatrix(In);
  //  printf("\"Out\" matrix:\n");
  //  printMatrix(Out);
  //  printf("\n");

  //  exit(1);
  //}

  //printf("Input and output matrices are identical, as expected\n");
  exit(0);
}

