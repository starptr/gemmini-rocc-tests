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

#define CHECK_RESULT 1

#define NO_BIAS 0
#define FULL_BIAS_WIDTH 1

#if FULL_BIAS_WIDTH
typedef acc_t ACC_T;
#else
typedef elem_t ACC_T;
#error variable-bitwidth bias not currently supported
#endif

#define MAT_DIM_I 89
#define MAT_DIM_J 79
#define MAT_DIM_K 83

void print_tile(elem_t* in, int tile_dim) {
  for (size_t r = 0; r < tile_dim; r++) {
    printf("row starts at: %p\n", in +r*MAT_DIM_J);
    for (size_t c = 0; c < tile_dim; c++) {
      printf("%d ", *(in +r*MAT_DIM_J + c));
    }
    printf("\n");
  }
}

void full_matmul(elem_t A[MAT_DIM_I][MAT_DIM_K], elem_t B[MAT_DIM_K][MAT_DIM_J], ACC_T D[MAT_DIM_I][MAT_DIM_J], full_t C_full[MAT_DIM_I][MAT_DIM_J]) {
  for (size_t r = 0; r < MAT_DIM_I; r++)
    for (size_t c = 0; c < MAT_DIM_J; c++) {
      C_full[r][c] = D[r][c];
      for (size_t k = 0; k < MAT_DIM_K; k++)
        C_full[r][c] += A[r][k]*B[k][c];
    }
}

void full_printMatrix(elem_t m[MAT_DIM_I][MAT_DIM_J]) {
  for (size_t i = 0; i < MAT_DIM_I; ++i) {
    for (size_t j = 0; j < MAT_DIM_J; ++j)
      printf("%d ", m[i][j]);
    printf("\n");
  }
}

int full_is_equal(elem_t x[MAT_DIM_I][MAT_DIM_J], elem_t y[MAT_DIM_I][MAT_DIM_J]) {
  for (size_t i = 0; i < MAT_DIM_I; ++i)
    for (size_t j = 0; j < MAT_DIM_J; ++j)
      if (x[i][j] != y[i][j])
        return 0;
  return 1;
}

void full_matscale(full_t full[MAT_DIM_I][MAT_DIM_J], elem_t out[MAT_DIM_I][MAT_DIM_J], acc_scale_t scale) {
  for (size_t r = 0; r < MAT_DIM_I; r++)                             
    for (size_t c = 0; c < MAT_DIM_J; c++) {
      // Scale element
      full_t scaled = ACC_SCALE(full[r][c], scale);

      // Saturate and cast element
#ifndef ELEM_T_IS_FLOAT
      full_t elem = scaled > elem_t_max ? elem_t_max : (scaled < elem_t_min ? elem_t_min : scaled);
      out[r][c] = elem;
#else
      out[r][c] = scaled; // TODO should we also saturate when using floats?
#endif
    }
} 

int main() {
#ifndef BAREMETAL
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
      perror("mlockall failed");
      exit(1);
    }
#endif

    gemmini_flush(0);

    
    static elem_t full_C[MAT_DIM_I][MAT_DIM_J] row_align(1);
    static ACC_T full_D[MAT_DIM_I][MAT_DIM_J] row_align_acc(1);

	static elem_t full_A[MAT_DIM_I][MAT_DIM_K] row_align(1);
	static elem_t full_B[MAT_DIM_K][MAT_DIM_J] row_align(1);

	static elem_t gold[MAT_DIM_I][MAT_DIM_J] row_align(1);

	for (int i = 0; i < MAT_DIM_I; i++)
		for (int k = 0; k < MAT_DIM_K; k++)
			full_A[i][k] = -1;

	for (int k = 0; k < MAT_DIM_K; k++)
		for (int j = 0; j < MAT_DIM_J; j++)
			full_B[k][j] = -1;

	for (int i = 0; i < MAT_DIM_I; i++)
		for (int j = 0; j < MAT_DIM_J; j++) {
			gold[i][j] = 5;
			full_D[i][j] = -1;
		}

    printf("Starting gemmini matmul\n");
    unsigned long start = read_cycles();

    for (int run = 0; run < 5; run++) {

		for (int i = 0; i < MAT_DIM_I; i++)
			for (int j = 0; j < MAT_DIM_J; j++)
				full_C[i][j] = -1;

	    tiled_matmul_auto(MAT_DIM_I, MAT_DIM_J, MAT_DIM_K,
	            (elem_t*)full_A, (elem_t*)full_B, NO_BIAS ? NULL : &full_D[0][0], (elem_t*)full_C,
	            MAT_DIM_K, MAT_DIM_J, MAT_DIM_J, MAT_DIM_J,
	            MVIN_SCALE_IDENTITY, MVIN_SCALE_IDENTITY, MVIN_SCALE_IDENTITY,
	            RELU, 0.062500 /*ACC_SCALE_IDENTITY*/, 0, false,
	            WS);

#if CHECK_RESULT == 1
	    if (!full_is_equal(full_C, gold)) {
	      printf("run: %d\n", run);
	      printf("C:\n");
	      full_printMatrix(full_C);
	      printf("Gold:\n");
	      full_printMatrix(gold);
	      printf("\n");

	      exit(1);
	    }
#endif
	}

    unsigned long end = read_cycles();
    printf("Cycles taken: %u\n", end-start);

  exit(0);
}