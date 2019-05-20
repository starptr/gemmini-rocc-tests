// See LICENSE for license details.

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "include/systolic.h"
#include "util.h"

#define BIG_DIM 32

#if (BIG_DIM % DIM) != 0
#error incorrect dimensions
#endif

#if (BIG_DIM * BIG_DIM / DIM) > ACC_ROWS
#error not enough accumulator space
#endif

int is_equal_big(elem_t x[BIG_DIM][BIG_DIM], elem_t y[BIG_DIM][BIG_DIM]) {
  for (size_t i = 0; i < BIG_DIM; ++i)
    for (size_t j = 0; j < BIG_DIM; ++j)
      if (x[i][j] != y[i][j])
          return 0;
  return 1;
}

void matshift_big(int64_t full[BIG_DIM][BIG_DIM], elem_t out[BIG_DIM][BIG_DIM], int shift) {
  int divisor = 1 << shift;

  for (size_t r = 0; r < BIG_DIM; r++)
    for (size_t c = 0; c < BIG_DIM; c++) {
      // Bitshift and round element
      int64_t abs = full[r][c] > 0 ? full[r][c] : -full[r][c];
      int64_t shifted = (abs + (divisor/2)) / divisor;
      if (full[r][c] < 0)
        shifted = -shifted;

      // Saturate and cast element
      int64_t elem = shifted > elem_t_max ? elem_t_max : (shifted < elem_t_min ? elem_t_min : shifted);
      out[r][c] = elem;
    }
}

void matrelu_big(elem_t in[BIG_DIM][BIG_DIM], elem_t out[BIG_DIM][BIG_DIM]) {
  for (size_t r = 0; r < BIG_DIM; r++)
    for (size_t c = 0; c < BIG_DIM; c++)
      out[r][c] = in[r][c] > 0 ? in[r][c] : 0;
}

// TODO this should take the cumulative scale into account
void matrelu6_big(elem_t in[BIG_DIM][BIG_DIM], elem_t out[BIG_DIM][BIG_DIM], int scale) {
  // int max = 6 * scale;
  int max = 6;

  for (size_t r = 0; r < BIG_DIM; r++)
    for (size_t c = 0; c < BIG_DIM; c++) {
      elem_t positive = in[r][c] > 0 ? in[r][c] : 0;
      out[r][c] = positive > max ? max : positive;
    }
}

void printMatrix_big(elem_t m[BIG_DIM][BIG_DIM]) {
  for (size_t i = 0; i < BIG_DIM; ++i) {
    for (size_t j = 0; j < BIG_DIM; ++j)
      printf("%d ", m[i][j]);
    printf("\n");
  }
}

void printMatrix_acc_big(acc_t m[BIG_DIM][BIG_DIM]) {
  for (size_t i = 0; i < BIG_DIM; ++i) {
    for (size_t j = 0; j < BIG_DIM; ++j)
      printf("%d ", m[i][j]);
    printf("\n");
  }
}

int main() {
  for (int activation = 0; activation <= 2; ++activation) {
    for (int shift = 0; shift <= 12; shift += 4) {
      // printf("activation: %d, shift: %d\n", activation, shift);

      static acc_t In[BIG_DIM][BIG_DIM] row_align_acc;
      static int64_t In_full[BIG_DIM][BIG_DIM];
      static elem_t Out[BIG_DIM][BIG_DIM] row_align;
      static elem_t Out_gold[BIG_DIM][BIG_DIM];

      for (size_t i = 0; i < BIG_DIM; ++i) {
        for (size_t j = 0; j < BIG_DIM; ++j) {
          In[i][j] = 0;

          int bytes = rand() % 2 ? sizeof(acc_t) : sizeof(elem_t);
          for (size_t b = 0; b < bytes; ++b) {
            In[i][j] |= (rand() % 255) << (b*8);
          }

          //In[i][j] = i*BIG_DIM+j;

          In_full[i][j] = In[i][j];
        }
      }

      matshift_big(In_full, Out_gold, shift);

      if (activation == RELU)
        matrelu_big(Out_gold, Out_gold);
      else if (activation == RELU6)
        matrelu6_big(Out_gold, Out_gold, 1 << shift);

      const int acc_addr = 1 << (ADDR_LEN-1);

      matmul_config_ld(BIG_DIM*sizeof(acc_t), 0, 0, 0, 0);
      matmul_config_ex(0, activation, shift, 0, 0, 1, 0);
      matmul_config_st(BIG_DIM*sizeof(elem_t), 0, 0, 0, 1);

      for (size_t i = 0; i < BIG_DIM; i += DIM) {
        for (size_t j = 0; j < BIG_DIM; j += DIM) {
          acc_t * dram_addr_in = &In[i][j];
          elem_t * dram_addr_out = &Out[i][j];
          int sp_addr = acc_addr + i*(BIG_DIM/DIM) + j;

          matmul_mvin(dram_addr_in, sp_addr, 1, 0, 0, 0);
          matmul_mvout(dram_addr_out, sp_addr, 0, 1, 0, 0);
        }
      }

      matmul_fence();

      // printf("Matrix:\n");
      // printMatrix_acc_big(In);
      // printf("Matrix output:\n");
      // printMatrix_big(Out);
      // printf("Matrix gold output:\n");
      // printMatrix_big(Out_gold);
      // printf("\n");

      if (!is_equal_big(Out, Out_gold)) {
        /*printf("Out:\n");
        for (size_t i = 0; i < BIG_DIM; i++) {
          for (size_t j = 0; j < BIG_DIM; j++) {
            printf("%d, ", Out[i][j]);
          }
          printf("\n");
        }

        printf("\n");

        printf("Gold:\n");
        for (size_t i = 0; i < BIG_DIM; i++) {
          for (size_t j = 0; j < BIG_DIM; j++) {
            printf("%d, ", Out[i][j]);
          }
          printf("\n");
        }*/

        printf("Matrix:\n");
        printMatrix_acc_big(In);
        printf("Matrix output:\n");
        printMatrix_big(Out);
        printf("Matrix gold output:\n");
        printMatrix_big(Out_gold);
        printf("\n");

        printf("activation: %d, shift: %d\n", activation, shift);
        exit(1);
      }
    }
  }

  exit(0);
}
