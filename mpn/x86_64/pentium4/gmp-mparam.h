/* Pentium 4-64 gmp-mparam.h -- Compiler/machine parameter header file.

Copyright 1991, 1993, 1994, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007,
2008, 2009 Free Software Foundation, Inc.

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

#define GMP_LIMB_BITS 64
#define BYTES_PER_MP_LIMB 8

/* These routines exists for all x86_64 chips, but they are slower on Pentium4
   than separate add/sub and shift.  Make sure they are not really used.  */
#undef HAVE_NATIVE_mpn_rsh1add_n
#undef HAVE_NATIVE_mpn_rsh1sub_n

/* 3200 MHz Pentium / 2048 Kibyte cache / socket 775 */

/* Generated by tuneup.c, 2009-11-26, gcc 4.2 */

#define MUL_TOOM22_THRESHOLD             12
#define MUL_TOOM33_THRESHOLD             89
#define MUL_TOOM44_THRESHOLD            214

#define SQR_BASECASE_THRESHOLD            0  /* always (native) */
#define SQR_TOOM2_THRESHOLD              22
#define SQR_TOOM3_THRESHOLD              89
#define SQR_TOOM4_THRESHOLD             250

#define MUL_FFT_TABLE  { 240, 480, 1600, 2816, 7168, 20480, 49152, 196608, 786432, 0 }
#define MUL_FFT_MODF_THRESHOLD          272
#define MUL_FFT_THRESHOLD              1920

#define SQR_FFT_TABLE  { 240, 480, 1472, 2816, 7168, 20480, 49152, 196608, 786432, 0 }
#define SQR_FFT_MODF_THRESHOLD          272
#define SQR_FFT_THRESHOLD              1920

#define MULLOW_BASECASE_THRESHOLD         2
#define MULLOW_DC_THRESHOLD              31
#define MULLOW_MUL_N_THRESHOLD         3837

#define MULMOD_BNM1_THRESHOLD            12

#define DC_DIV_QR_THRESHOLD              34
#define REDC_1_TO_REDC_2_THRESHOLD       16
#define REDC_2_TO_REDC_N_THRESHOLD       78

#define MATRIX22_STRASSEN_THRESHOLD      25
#define HGCD_THRESHOLD                  112
#define GCD_DC_THRESHOLD                253
#define GCDEXT_DC_THRESHOLD             278
#define JACOBI_BASE_METHOD                1

#define MOD_1_NORM_THRESHOLD              0  /* always */
#define MOD_1_UNNORM_THRESHOLD            0  /* always */
#define MOD_1_1_THRESHOLD                 5
#define MOD_1_2_THRESHOLD                 8
#define MOD_1_4_THRESHOLD                22
#define USE_PREINV_DIVREM_1               1  /* native */
#define USE_PREINV_MOD_1                  0
#define DIVEXACT_1_THRESHOLD              0  /* always (native) */
#define MODEXACT_1_ODD_THRESHOLD          0  /* always (native) */

#define GET_STR_DC_THRESHOLD             13
#define GET_STR_PRECOMPUTE_THRESHOLD     25
#define SET_STR_DC_THRESHOLD            802
#define SET_STR_PRECOMPUTE_THRESHOLD   1553

#define MUL_FFT_TABLE2 {{1,4}, {145,5}, {353,6}, {961,7}, {2689,8}, {6913,10}, {7169,9}, {7681,8}, {8449,9}, {13825,10}, {15361,9}, {19969,10}, {23553,9}, {26113,11}, {30721,10}, {31745,9}, {34561,10}, {48129,11}, {63489,10}, {81921,11}, {96257,10}, {97793,12}, {126977,11}, {133121,10}, {135169,11}, {137217,10}, {139777,9}, {141313,10}, {145409,11}, {161793,10}, {163841,11}, {165889,10}, {169985,11}, {172033,10}, {177153,11}, {206849,12}, {212993,11}, {217089,12}, {258049,11}, {358401,12}, {389121,11}, {391169,10}, {397313,11}, {410625,10}, {421889,11}, {450561,13}, {516097,12}, {520193,11}, {587777,12}, {651265,11}, {718849,12}, {790529,11}, {800769,12}, {815105,11}, {821249,12}, {833537,11}, {845825,12}, {915457,13}, {1040385,12}, {1437697,13}, {1564673,12}, {1830913,14}, {2088961,12}, {2355201,13}, {2613249,12}, {2879489,13}, {3137537,12}, {3405825,13}, {3661825,14}, {4186113,12}, {4454401,13}, {4714497,11}, {4979713,13}, {MP_SIZE_T_MAX, 0}}
#define MUL_FFT_TABLE2_SIZE 73

#define SQR_FFT_TABLE2 {{1,4}, {177,5}, {417,6}, {961,7}, {3073,8}, {6913,10}, {7169,9}, {7681,8}, {8449,9}, {13825,10}, {15361,9}, {19969,10}, {23553,9}, {26113,11}, {30721,10}, {48129,11}, {63489,10}, {65537,8}, {65793,10}, {80897,11}, {96257,12}, {126977,11}, {129025,10}, {136193,11}, {137217,10}, {146433,9}, {146945,11}, {161793,10}, {176129,11}, {195073,10}, {201729,11}, {202753,10}, {206849,11}, {219137,12}, {258049,11}, {260097,10}, {262145,11}, {270337,10}, {272385,11}, {279553,10}, {280577,11}, {286721,10}, {287745,11}, {359425,12}, {389121,11}, {423937,13}, {516097,12}, {520193,11}, {587777,12}, {651265,11}, {718849,12}, {782337,11}, {802817,12}, {806913,11}, {849921,12}, {915457,13}, {1040385,12}, {1437697,13}, {1564673,12}, {1830913,14}, {2080769,13}, {2088961,12}, {2355201,13}, {2613249,12}, {2752513,13}, {2785281,12}, {2801665,13}, {2818049,12}, {2838529,13}, {2863105,12}, {2867201,13}, {3137537,12}, {3403777,13}, {3661825,14}, {4177921,13}, {4186113,12}, {4452353,13}, {4714497,11}, {4979713,13}, {MP_SIZE_T_MAX, 0}}
#define SQR_FFT_TABLE2_SIZE 79
