/* HP-PA 1.1 gmp-mparam.h -- Compiler/machine parameter header file.

Copyright 1991, 1993, 1994, 1999, 2000, 2001, 2002, 2004 Free Software
Foundation, Inc.

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

#define BITS_PER_MP_LIMB 32
#define BYTES_PER_MP_LIMB 4

/* Generated by tuneup.c, 2004-02-07, gcc 2.8 (pa7100/100MHz) */

#define MUL_KARATSUBA_THRESHOLD          30
#define MUL_TOOM3_THRESHOLD              89

#define SQR_BASECASE_THRESHOLD            4
#define SQR_KARATSUBA_THRESHOLD          55
#define SQR_TOOM3_THRESHOLD             101

#define DIV_SB_PREINV_THRESHOLD           0  /* always */
#define DIV_DC_THRESHOLD                 84
#define POWM_THRESHOLD                  166

#define HGCD_THRESHOLD                  231
#define GCD_ACCEL_THRESHOLD               3
#define GCD_DC_THRESHOLD                823
#define GCDEXT_THRESHOLD                  0  /* always */
#define JACOBI_BASE_METHOD                2

#define DIVREM_1_NORM_THRESHOLD           5
#define DIVREM_1_UNNORM_THRESHOLD        11
#define MOD_1_NORM_THRESHOLD              5
#define MOD_1_UNNORM_THRESHOLD           10
#define USE_PREINV_DIVREM_1               1
#define USE_PREINV_MOD_1                  1
#define DIVREM_2_THRESHOLD                0  /* always */
#define DIVEXACT_1_THRESHOLD              0  /* always */
#define MODEXACT_1_ODD_THRESHOLD          0  /* always */

#define GET_STR_DC_THRESHOLD             13
#define GET_STR_PRECOMPUTE_THRESHOLD     23
#define SET_STR_THRESHOLD              6589

#define MUL_FFT_TABLE  { 464, 928, 1920, 4608, 14336, 40960, 0 }
#define MUL_FFT_MODF_THRESHOLD          480
#define MUL_FFT_THRESHOLD              3328

#define SQR_FFT_TABLE  { 528, 1184, 2176, 5632, 14336, 40960, 0 }
#define SQR_FFT_MODF_THRESHOLD          520
#define SQR_FFT_THRESHOLD              3328
