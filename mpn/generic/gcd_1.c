/* mpn_gcd_1 -- mpn and limb greatest common divisor.

Copyright 1994, 1996, 2000, 2001 Free Software Foundation, Inc.

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

#include "gmp.h"
#include "gmp-impl.h"
#include "longlong.h"

/* Does not work for U == 0 or V == 0.  It would be tough to make it work for
   V == 0 since gcd(x,0) = x, and U does not generally fit in an mp_limb_t.  */

mp_limb_t
mpn_gcd_1 (mp_srcptr up, mp_size_t size, mp_limb_t vlimb)
{
  mp_limb_t ulimb;
  unsigned long int u_low_zero_bits, v_low_zero_bits;

  ASSERT (size >= 1);
  ASSERT (vlimb != 0);
  ASSERT (! mpn_zero_p (up, size));

  if (size > 1)
    {
      ulimb = mpn_mod_1 (up, size, vlimb);
      if (ulimb == 0)
	return vlimb;
    }
  else
    ulimb = up[0];

  /*  Need to eliminate low zero bits.  */
  count_trailing_zeros (u_low_zero_bits, ulimb);
  ulimb >>= u_low_zero_bits;

  count_trailing_zeros (v_low_zero_bits, vlimb);
  vlimb >>= v_low_zero_bits;

  while (ulimb != vlimb)
    {
      if (ulimb > vlimb)
	{
	  ulimb -= vlimb;
	  do
	    ulimb >>= 1;
	  while ((ulimb & 1) == 0);
	}
      else /*  vlimb > ulimb.  */
	{
	  vlimb -= ulimb;
	  do
	    vlimb >>= 1;
	  while ((vlimb & 1) == 0);
	}
    }

  return  ulimb << MIN (u_low_zero_bits, v_low_zero_bits);
}
