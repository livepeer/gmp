/* mpfr_cmp_ui -- compare a floating-point number with a machine integer

Copyright (C) 1999 Free Software Foundation.

This file is part of the MPFR Library.

The MPFR Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Library General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

The MPFR Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
License for more details.

You should have received a copy of the GNU Library General Public License
along with the MPFR Library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA. */

#include <stdio.h>
#include "gmp.h"
#include "gmp-impl.h"
#include "longlong.h"
#include "mpfr.h"
#include "mpfr-impl.h"

/* returns a positive value if b>i*2^f,
           a negative value if b<i*2^f,
           zero if b=i*2^f
*/

int 
#if __STDC__
mpfr_cmp_ui_2exp (mpfr_srcptr b, unsigned long int i, int f )
#else
mpfr_cmp_ui_2exp (b, i, f)
     mpfr_srcptr b; 
     unsigned long int i; 
     int f; 
#endif
{
  int e, k, bn; mp_limb_t c, *bp;

  if (MPFR_IS_NAN(b)) return 1;

  if (MPFR_SIGN(b) < 0) return -1;
  /* now b>=0 */
  else if (MPFR_IS_INF(b)) return 1; 
  else if (!MPFR_NOTZERO(b)) return((i) ? -1 : 0);
  /* now b>0 */
  else if (i==0) return 1;
  else { /* b>0, i>0 */
    e = MPFR_EXP(b); /* 2^(e-1) <= b < 2^e */
    if (e>f+BITS_PER_MP_LIMB) return 1;

    c = (mp_limb_t) i;
    count_leading_zeros(k, c);
    k = f+BITS_PER_MP_LIMB - k; /* 2^(k-1) <= i*2^f < 2^k */
    if (k!=e) return (e-k);

    /* now k=e */
    c <<= (f+BITS_PER_MP_LIMB-k);
    bn = (MPFR_PREC(b)-1)/BITS_PER_MP_LIMB;
    bp = MPFR_MANT(b) + bn;
    if (*bp>c) return 1;
    else if (*bp<c) return -1;

    /* most significant limbs agree, check remaining limbs from b */
    while (--bn>=0)
      if (*--bp) return 1;
    return 0;
  }
}

/* returns a positive value if b>i*2^f,
           a negative value if b<i*2^f,
           zero if b=i*2^f 
*/

int 
#if __STDC__
mpfr_cmp_si_2exp (mpfr_srcptr b, long int i, int f )
#else
mpfr_cmp_si_2exp (b, i, f)
     mpfr_srcptr b; 
     long int i; 
     int f; 
#endif
{
  int e, k, bn, si; mp_limb_t c, *bp;

  if (MPFR_IS_NAN(b)) return 1;

  si = (i<0) ? -1 : 1; /* sign of i */
  if (MPFR_SIGN(b) * i < 0 || MPFR_IS_INF(b)) return MPFR_SIGN(b); 
  /* both signs differ */
  else if (!MPFR_NOTZERO(b) || (i==0)) { /* one is zero */
    if (i==0) return ((MPFR_NOTZERO(b)) ? MPFR_SIGN(b) : 0);
    else return si; /* b is zero */
      
  }
  else { /* b and i are of same sign */
    e = MPFR_EXP(b); /* 2^(e-1) <= b < 2^e */
    if (e>f+BITS_PER_MP_LIMB) return si;

    c = (mp_limb_t) ((i<0) ? -i : i);
    count_leading_zeros(k, c);
    k = f+BITS_PER_MP_LIMB - k; /* 2^(k-1) <= i*2^f < 2^k */
    if (k!=e) return (si*(e-k));

    /* now k=e */
    c <<= (f+BITS_PER_MP_LIMB-k);
    bn = (MPFR_PREC(b)-1)/BITS_PER_MP_LIMB;
    bp = MPFR_MANT(b) + bn;
    if (*bp>c) return si;
    else if (*bp<c) return -si;

    /* most significant limbs agree, check remaining limbs from b */
    while (--bn>=0)
      if (*--bp) return si;
    return 0;
  }
}

