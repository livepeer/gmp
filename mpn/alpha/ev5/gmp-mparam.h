/* gmp-mparam.h -- Compiler/machine parameter header file.

Copyright 1991, 1993, 1994, 1999, 2000 Free Software Foundation, Inc.

This file is part of the GNU MP Library.

The GNU MP Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version.

The GNU MP Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the GNU MP Library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA. */

#define BITS_PER_MP_LIMB 64
#define BYTES_PER_MP_LIMB 8

/* Generated by tuneup.c, 2000-11-11. */

#ifndef KARATSUBA_MUL_THRESHOLD
#define KARATSUBA_MUL_THRESHOLD     14
#endif
#ifndef TOOM3_MUL_THRESHOLD
#define TOOM3_MUL_THRESHOLD         74
#endif

#ifndef KARATSUBA_SQR_THRESHOLD
#define KARATSUBA_SQR_THRESHOLD     28
#endif
#ifndef TOOM3_SQR_THRESHOLD
#define TOOM3_SQR_THRESHOLD         45
#endif

#ifndef DC_THRESHOLD
#define DC_THRESHOLD                52
#endif

#ifndef FIB_THRESHOLD
#define FIB_THRESHOLD              196
#endif

#ifndef POWM_THRESHOLD
#define POWM_THRESHOLD              77
#endif

#ifndef GCD_ACCEL_THRESHOLD
#define GCD_ACCEL_THRESHOLD          4
#endif
#ifndef GCDEXT_THRESHOLD
#define GCDEXT_THRESHOLD             2
#endif

#ifndef FFT_MUL_TABLE
#define FFT_MUL_TABLE  { 560, 1248, 3264, 5376, 15360, 45056, 0 }
#endif
#ifndef FFT_MODF_MUL_THRESHOLD
#define FFT_MODF_MUL_THRESHOLD     784
#endif
#ifndef FFT_MUL_THRESHOLD
#define FFT_MUL_THRESHOLD         6016
#endif

#ifndef FFT_SQR_TABLE
#define FFT_SQR_TABLE  { 592, 1376, 3264, 6400, 17408, 36864, 0 }
#endif
#ifndef FFT_MODF_SQR_THRESHOLD
#define FFT_MODF_SQR_THRESHOLD     784
#endif
#ifndef FFT_SQR_THRESHOLD
#define FFT_SQR_THRESHOLD         3456
#endif
