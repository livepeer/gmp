/* gmp-mparam.h -- Compiler/machine parameter header file.

Copyright 2000, 2001, 2002, 2003, 2004, 2005, 2009 Free Software Foundation,
Inc.

This file is part of the GNU MP Library.

The GNU MP Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

The GNU MP Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the GNU MP Library.  If not, see http://www.gnu.org/licenses/.  */

#define BITS_PER_MP_LIMB 64
#define BYTES_PER_MP_LIMB 8

/* 1300MHz Itanium2 (babe.fsffrance.org)

/* Generated by tuneup.c, 2009-03-04, gcc 4.2 */

#define MUL_KARATSUBA_THRESHOLD          44
#define MUL_TOOM3_THRESHOLD             137
#define MUL_TOOM44_THRESHOLD            230

#define SQR_BASECASE_THRESHOLD           25
#define SQR_KARATSUBA_THRESHOLD         119
#define SQR_TOOM3_THRESHOLD             146
#define SQR_TOOM4_THRESHOLD             284

#define MULLOW_BASECASE_THRESHOLD        19
#define MULLOW_DC_THRESHOLD             120
#define MULLOW_MUL_N_THRESHOLD          357

#define DIV_SB_PREINV_THRESHOLD           0  /* preinv always */
#define DIV_DC_THRESHOLD                 70
#define POWM_THRESHOLD                  312

#define MATRIX22_STRASSEN_THRESHOLD      29
#define HGCD_THRESHOLD                  118
#define GCD_DC_THRESHOLD                595
#define GCDEXT_DC_THRESHOLD             584
#define JACOBI_BASE_METHOD                1

#define MOD_1_NORM_THRESHOLD              0  /* always */
#define MOD_1_UNNORM_THRESHOLD            0  /* always */
#define MOD_1_1_THRESHOLD                 8
#define MOD_1_2_THRESHOLD                 9
#define MOD_1_4_THRESHOLD                20
#define USE_PREINV_DIVREM_1               1  /* native */
#define USE_PREINV_MOD_1                  1  /* preinv always */
#define DIVEXACT_1_THRESHOLD              0  /* always (native) */
#define MODEXACT_1_ODD_THRESHOLD          0  /* always */

#define GET_STR_DC_THRESHOLD             17
#define GET_STR_PRECOMPUTE_THRESHOLD     25
#define SET_STR_DC_THRESHOLD           1488
#define SET_STR_PRECOMPUTE_THRESHOLD   3590

#define MUL_FFT_TABLE  { 528, 1184, 1856, 3840, 11264, 28672, 114688, 327680, 0 }
#define MUL_FFT_MODF_THRESHOLD          784
#define MUL_FFT_THRESHOLD              6656

#define SQR_FFT_TABLE  { 592, 1248, 2368, 3840, 11264, 28672, 81920, 327680, 0 }
#define SQR_FFT_MODF_THRESHOLD          608
#define SQR_FFT_THRESHOLD              4992
