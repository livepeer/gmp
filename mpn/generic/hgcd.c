/* hgcd.c.

   THE FUNCTIONS IN THIS FILE ARE INTERNAL WITH MUTABLE INTERFACES.  IT IS ONLY
   SAFE TO REACH THEM THROUGH DOCUMENTED INTERFACES.  IN FACT, IT IS ALMOST
   GUARANTEED THAT THEY'LL CHANGE OR DISAPPEAR IN A FUTURE GNU MP RELEASE.

Copyright 2003 Free Software Foundation, Inc.

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

#define WANT_TRACE 0

#if WANT_TRACE
# include <stdio.h>
# include <stdarg.h>
#endif

#include "gmp.h"
#include "gmp-impl.h"
#include "longlong.h"

#if WANT_TRACE
static void
trace (const char *format, ...)
{
  va_list args;
  va_start (args, format);
  gmp_vfprintf (stderr, format, args);
  va_end (args);
}
#endif

/* Comparison of _normalized_ numbers. */

#define MPN_EQUAL_P(ap, asize, bp, bsize)			\
((asize) == (bsize) && mpn_cmp ((ap), (bp), (asize)) == 0)

#define MPN_LEQ_P(ap, asize, bp, bsize)				\
((asize) < (bsize) || ((asize) == (bsize)			\
		       && mpn_cmp ((ap), (bp), (asize)) <= 0))

#define MPN_LESS_P(ap, asize, bp, bsize)			\
((asize) < (bsize) || ((asize) == (bsize)			\
		       && mpn_cmp ((ap), (bp), (asize)) < 0))

/* Extract one limb, shifting count bits left
    ________  ________
   |___xh___||___xl___|
	  |____r____|
   >count <

   The count includes any nail bits, so it should work fine if
   count is computed using count_leading_zeros.
*/

#define MPN_EXTRACT_LIMB(count, xh, xl)				\
  ((((xh) << ((count) - GMP_NAIL_BITS)) & GMP_NUMB_MASK) |	\
   ((xl) >> (GMP_LIMB_BITS - (count))))

/* Checks if a - b < c.  Prerequisite a >= b.  Overwrites c.

   Let W = 2^GMP_NUMB_BITS, k = csize.

   Write a = A W^k + a', b = B W^k + b', so that a - b < c is
   equivalent to

     (A - B) W^k < c + b' - a'     (*)

   For the right hand side, we have - W^k < c + b' - a' < 2 W^k. We
   can divide into cases based on either side of (*).

   L1. A = B: a - b < c iff a' < c + b'.

   L2. A - B = 1: a - b < c iff W^k < c + b' - a', or W^k <= c
      + b' - 1 - a'

   L3. A - B > 1: a - b > c.

   R1. a' >= c + b': a - b >= c.

   R2. 0 < c + b' - a' <= W^k: a - b < c iff A == B.

   R3. W^k < c + b' - a': a - b < c iff A - B < 2
*/

static int
mpn_diff_smaller_p (mp_srcptr ap, mp_size_t asize,
		    mp_srcptr bp, mp_size_t bsize,
		    mp_ptr cp, mp_size_t csize)
{
  mp_limb_t ch;

  ASSERT (MPN_LEQ_P (bp, bsize, ap, asize));

  if (csize == 0)
    return 0;

  if (asize < csize)
    return 1;

  if (asize == csize)
    {
      ASSERT (bsize <= csize);

      if (bsize != 0)
	{
	  /* A - B == 0, check a' < c + b' */
	  ch = mpn_add (cp, cp, csize, bp, bsize);
	  if (ch)
	    return 1;
	}

      return mpn_cmp (ap, cp, csize) < 0;
    }

  if (bsize <= csize)
    {
      /* B == 0, so A - B = A */
      if (asize > csize + 1 || ap[csize] > 1)
	return 0;

      if (bsize == 0)
	return 0;

      /* A - B == 1, so check W^k <= c + b' - 1 - a' */
      ASSERT_NOCARRY (mpn_sub_1(cp, cp, csize, 1));
      ch = mpn_add (cp, cp, csize, bp, bsize);

      return ch == 1 && mpn_cmp (cp, ap, csize) >= 0;
    }

  /* Compute A - B, and abort as soon as we know the difference is larger than 1 */

  if (asize > bsize)
    {
      /* The only way we can have A - B = 1 is if A = (1, 0, ..., 0),
	 B = (0, MAX, ..., MAX) */
      mp_size_t i;

      if (asize > bsize + 1 || ap[bsize] > 1)
	return 0;

      for (i = csize; i < bsize; i++)
	if (ap[i] != 0 || bp[i] != GMP_NUMB_MAX)
	  return 0;

      /* A - B == 1, so check W^k <= c + b' - 1 - a' */
      ASSERT_NOCARRY (mpn_sub_1(cp, cp, csize, 1));
      ch = mpn_add_n (cp, cp, bp, csize);

      return ch == 1 && mpn_cmp (cp, ap, csize) >= 0;
    }

  /* Equal high limbs cancel out, so ignore them */
  while (asize >= csize && ap[asize - 1] == bp[asize - 1])
    asize--;

  if (asize < csize)
    return 1;

  /* Now asize = bsize >= csize */
  if (asize == csize)
    {
      /* A - B == 0, check a' < c + b' */
      ch = mpn_add_n (cp, cp, bp, csize);

      return ch || mpn_cmp (ap, cp, csize) < 0;
    }

  /* asize == bsize > csize. */
  {
    mp_size_t i;

    /* We know that A - B >= 1. Do we have A - B > 1? */
    /* The only way we can have A - B = 1 is if A = (X, 0, ..., 0), B =
       (X-1, MAX, ..., MAX). */

    if (ap[asize - 1] - bp[asize - 1] > 1)
      /* A - B > 1 */
      return 0;

    for (i = csize; i < asize - 1; i++)
      if (ap[i] != 0 || bp[i] != GMP_NUMB_MAX)
	return 0;

    /* A - B == 1, so check W^k <= c + b' - 1 - a' */
    ASSERT_NOCARRY (mpn_sub_1(cp, cp, csize, 1));
    ch = mpn_add_n (cp, cp, bp, csize);

    return ch == 1 && mpn_cmp (cp, ap, csize) >= 0;
  }
}

#if WANT_ASSERT
static int
slow_diff_smaller_p (mp_srcptr ap, mp_size_t asize,
		     mp_srcptr bp, mp_size_t bsize,
		     mp_ptr cp, mp_size_t csize)
{
  if (csize == 0)
    return 0;
  else if (bsize == 0)
    return MPN_LESS_P (ap, asize, cp, csize);
  else
    {
      int res;
      mp_ptr tp;
      mp_size_t tsize;
      TMP_DECL (marker);
      TMP_MARK (marker);

      tp = TMP_ALLOC_LIMBS (asize);
      mpn_sub (tp, ap, asize, bp, bsize);
      tsize = asize;
      MPN_NORMALIZE (tp, tsize);

      res = MPN_LESS_P (tp, tsize, cp, csize);
      TMP_FREE (marker);

      return res;
    }
}

static int
wrap_mpn_diff_smaller_p (mp_srcptr ap, mp_size_t asize,
			 mp_srcptr bp, mp_size_t bsize,
			 mp_ptr cp, mp_size_t csize)
{
  int r1;
  int r2;

#if WANT_TRACE
  trace ("wrap_mpn_diff_smaller_p:\n"
	 "  a = %Nd;\n"
	 "  b = %Nd;\n"
	 "  c = %Nd;\n", ap, asize, bp, bsize, cp, csize);
#endif

  r1 = slow_diff_smaller_p (ap, asize, bp, bsize, cp, csize);
  r2 = mpn_diff_smaller_p (ap, asize, bp, bsize, cp, csize);
  ASSERT (r1 == r2);
  return r1;
}
#define mpn_diff_smaller_p wrap_mpn_diff_smaller_p
#endif /* WANT_ASSERT */


/* Compute au + bv. u and v are single limbs, a and b are n limbs each.
   Stores n+1 limbs in rp, and returns the (n+2)'nd limb. */
/* FIXME: With nails, we can instead return limb n+1, possibly including
   one non-zero nail bit. */
static mp_limb_t
mpn_addmul2_n_1 (mp_ptr rp, mp_size_t n,
		 mp_ptr ap, mp_limb_t u,
		 mp_ptr bp, mp_limb_t v)
{
  mp_limb_t h;
  mp_limb_t cy;

  h = mpn_mul_1 (rp, ap, n, u);
  cy = mpn_addmul_1 (rp, bp, n, v);
  h += cy;
#if GMP_NAIL_BITS == 0
  rp[n] = h;
  return (h < cy);
#else /* GMP_NAIL_BITS > 0 */
  rp[n] = h & GMP_NUMB_MASK;
  return h >> GMP_NUMB_BITS;
#endif /* GMP_NAIL_BITS > 0 */
}



/* qstack operations */

/* Prepares stack for pushing SIZE limbs */
static mp_ptr
qstack_push_start (struct qstack *stack,
		   mp_size_t size)
{
  ASSERT_QSTACK (stack);

  ASSERT (stack->limb_next <= stack->limb_alloc);

  if (size > stack->limb_alloc - stack->limb_next)
    {
      qstack_rotate (stack,
		     size - (stack->limb_alloc - stack->limb_next));
      ASSERT (stack->size_next < QSTACK_MAX_QUOTIENTS);
    }
  else if (stack->size_next >= QSTACK_MAX_QUOTIENTS)
    {
      qstack_rotate (stack, 0);
    }

  ASSERT (size <= stack->limb_alloc - stack->limb_next);
  ASSERT (stack->size_next < QSTACK_MAX_QUOTIENTS);

  return stack->limb + stack->limb_next;
}

static void
qstack_push_end (struct qstack *stack, mp_size_t size)
{
  ASSERT (stack->size_next < QSTACK_MAX_QUOTIENTS);
  ASSERT (size <= stack->limb_alloc - stack->limb_next);

  stack->size[stack->size_next++] = size;
  stack->limb_next += size;

  ASSERT_QSTACK (stack);
}

static mp_size_t
qstack_push_quotient (struct qstack *stack,
		      mp_ptr *qp,
		      mp_ptr rp, mp_size_t *rsizep,
		      mp_srcptr ap, mp_size_t asize,
		      mp_srcptr bp, mp_size_t bsize)
{
  mp_size_t qsize = asize - bsize + 1;
  mp_size_t rsize = bsize;
  *qp = qstack_push_start (stack, qsize);

  mpn_tdiv_qr (*qp, rp, 0, ap, asize, bp, bsize);
  MPN_NORMALIZE (rp, rsize);
  *rsizep = rsize;

  if ((*qp)[qsize - 1] == 0)
    qsize--;

  if (qsize == 1 && (*qp)[0] == 1)
    qsize = 0;

  qstack_push_end (stack, qsize);
  return qsize;
}

static void
qstack_drop (struct qstack *stack)
{
  ASSERT (stack->size_next);
  stack->limb_next -= stack->size[--stack->size_next];
}

/* Get top element */
static mp_size_t
qstack_get_0 (struct qstack *stack,
			    mp_srcptr *qp)
{
  mp_size_t qsize;
  ASSERT (stack->size_next);

  qsize = stack->size[stack->size_next - 1];
  *qp = stack->limb + stack->limb_next - qsize;

  return qsize;
}

/* Get element just below the top */
static mp_size_t
qstack_get_1 (struct qstack *stack,
			    mp_srcptr *qp)
{
  mp_size_t qsize;
  ASSERT (stack->size_next >= 2);

  qsize = stack->size[stack->size_next - 2];
  *qp = stack->limb + stack->limb_next
    - stack->size[stack->size_next - 1]
    - qsize;

  return qsize;
}

/* Adds d to the element on top of the stack */
static void
qstack_adjust (struct qstack *stack, mp_limb_t d)
{
  mp_size_t qsize;
  mp_ptr qp;

  ASSERT (d);
  ASSERT (stack->size_next);

  ASSERT_QSTACK (stack);

  if (stack->limb_next >= stack->limb_alloc)
    {
      qstack_rotate (stack, 1);
    }

  ASSERT (stack->limb_next < stack->limb_alloc);

  qsize = stack->size[stack->size_next - 1];
  qp = stack->limb + stack->limb_next - qsize;

  if (qsize == 0)
    {
      qp[0] = d + 1;
      stack->size[stack->size_next - 1] = 1;
      stack->limb_next++;
    }
  else
    {
      mp_limb_t cy = mpn_add_1 (qp, qp, qsize, d);
      if (cy)
	{
	  qp[qsize] = cy;
	  stack->size[stack->size_next - 1]++;
	  stack->limb_next++;
	}
    }

  ASSERT_QSTACK (stack);
}

/* hgcd2 operations */

/* Computes P = R * S. No overlap allowed. */
static mp_size_t
hgcd2_mul (struct hgcd_row *P, mp_size_t alloc,
	   const struct hgcd2_row *R,
	   const struct hgcd_row *S, mp_size_t n)
{
  int grow = 0;
  mp_limb_t h = 0;
  unsigned i;
  unsigned j;

  ASSERT (n < alloc);

  for (i = 0; i < 2; i++)
    for (j = 0; j < 2; j++)
      {
	/* Set P[i, j] = R[i, 0] S[0, j] + R[i,1] S[1, j]
		       = u_i s0j + v_i s1j */
	mp_limb_t cy;

	cy = mpn_addmul2_n_1 (P[i].uvp[j], n,
			      S[0].uvp[j], R[i].u,
			      S[1].uvp[j], R[i].v);
	if (cy)
	  {
	    ASSERT (n + 2 <= alloc);
	    P[i].uvp[j][n+1] = cy;
	    grow = 1;
	  }
	else
	  h |= P[i].uvp[j][n];
      }
  if (grow)
    return n + 2;
  else
    /* Don't add redundant zeroes */
    return n + (h != 0);
}


unsigned
mpn_hgcd_max_recursion (mp_size_t n)
{
  int count;

  count_leading_zeros (count, (mp_limb_t)
		       (1 + n / (HGCD_SCHOENHAGE_THRESHOLD  - 5)));

  return GMP_LIMB_BITS - count;
}

mp_size_t
mpn_hgcd_init_itch (mp_size_t size)
{
  /* r0 <= a, r1, r2, r3 <= b, but for simplicity, we allocate asize +
     1 for all of them. The size of the uv:s are limited to asize / 2,
     but we allocate one extra limb. */

  return 4 * (size + 1) + 8 * ((size / 2) + 1);
}

void
mpn_hgcd_init (struct hgcd *hgcd,
	       mp_size_t asize,
	       mp_limb_t *limbs)
{
  unsigned i;
  unsigned j;
  mp_size_t alloc = (asize / 2) + 1;

  hgcd->sign = 0;

  for (i = 0; i < 4; i++)
    {
      hgcd->row[i].rp = limbs;
      hgcd->row[i].rsize = asize + 1; limbs += asize + 1;
    }

  hgcd->alloc = alloc;
  hgcd->size = alloc;

  for (i = 0; i < 4; i++)
    for (j = 0; j < 2; j++)
      {
	hgcd->row[i].uvp[j] = limbs;
	limbs += alloc;
      }
}

#if WANT_ASSERT
void
__gmpn_hgcd_sanity (const struct hgcd *hgcd,
		    mp_srcptr ap, mp_size_t asize,
		    mp_srcptr bp, mp_size_t bsize,
		    unsigned start, unsigned end)
{
  int sign;
  unsigned i;
  mp_size_t L = hgcd->size;
  mp_ptr tp;
  mp_size_t talloc;
  mp_ptr t1p;
  mp_ptr t2p;
  const struct hgcd_row *r;

  ASSERT (asize >= bsize);

  ASSERT (L <= asize / 2);
  ASSERT (L);

  ASSERT (L <= asize);
  ASSERT (L <= bsize);

  /* NOTE: We really need only asize + bsize + 2*L, but since we're
   * swapping the pointers around, we allocate 2*(asize + L). */
  talloc = 2*(asize + L);
  tp = __GMP_ALLOCATE_FUNC_LIMBS (talloc);
  t1p = tp;
  t2p = t1p + (asize + L);

  sign = hgcd->sign;
  if (start % 2)
    sign = ~sign;
  for (i = start, r = &hgcd->row[start]; i < end; i++, sign = ~sign, r++)
    {
      mp_size_t t1size = asize + L;
      mp_size_t t2size = bsize + L;

      mp_size_t k;
      for (k = hgcd->size; k < hgcd->alloc; k++)
	{
	  ASSERT (r->uvp[0][k] == 0);
	  ASSERT (r->uvp[1][k] == 0);
	}

      mpn_mul (t1p, ap, asize, r->uvp[0], L);
      mpn_mul (t2p, bp, bsize, r->uvp[1], L);

      if (sign < 0)
	MPN_PTR_SWAP (t1p, t1size, t2p, t2size);

      MPN_NORMALIZE (t2p, t2size);
      ASSERT (t2size <= t1size);
      ASSERT_NOCARRY (mpn_sub (t1p, t1p, t1size, t2p, t2size));

      MPN_NORMALIZE (t1p, t1size);
      ASSERT (MPN_EQUAL_P (t1p, t1size, r->rp, r->rsize));
    }
  __GMP_FREE_FUNC_LIMBS (tp, talloc);
  for (i = start; i < end - 1; i++)
    {
      /* We should have strict inequality after each reduction step,
	 but we allow equal values for input. */
      ASSERT (MPN_LEQ_P (hgcd->row[i+1].rp, hgcd->row[i+1].rsize,
			 hgcd->row[i].rp, hgcd->row[i].rsize));
    }
}
#endif

/* Helper functions for hgcd */
/* Sets (a, b, c, d)  <--  (b, c, d, a) */
#define HGCD_SWAP4_LEFT(row)				\
do {							\
  struct hgcd_row __hgcd_swap4_left_tmp;                \
  __hgcd_swap4_left_tmp = row[0];                       \
  row[0] = row[1];					\
  row[1] = row[2];					\
  row[2] = row[3];					\
  row[3] = __hgcd_swap4_left_tmp;			\
} while (0)

/* Sets (a, b, c, d)  <--  (d, a, b, c) */
#define HGCD_SWAP4_RIGHT(row)				\
do {							\
  struct hgcd_row __hgcd_swap4_right_tmp;               \
  __hgcd_swap4_right_tmp = row[3];                      \
  row[3] = row[2];					\
  row[2] = row[1];					\
  row[1] = row[0];					\
  row[0] = __hgcd_swap4_right_tmp;			\
} while (0)

/* Sets (a, b, c, d)  <--  (c, d, a, b) */
#define HGCD_SWAP4_2(row)				\
do {							\
  struct hgcd_row __hgcd_swap4_2_tmp;                   \
  __hgcd_swap4_2_tmp = row[0];                          \
  row[0] = row[2];					\
  row[2] = __hgcd_swap4_2_tmp;				\
  __hgcd_swap4_2_tmp = row[1];				\
  row[1] = row[3];					\
  row[3] = __hgcd_swap4_2_tmp;				\
} while (0)

/* Sets (a, b, c)  <--	(b, c, a) */
#define HGCD_SWAP3_LEFT(row)				\
do {							\
  struct hgcd_row __hgcd_swap4_left_tmp;                \
  __hgcd_swap4_left_tmp = row[0];                       \
  row[0] = row[1];					\
  row[1] = row[2];					\
  row[2] = __hgcd_swap4_left_tmp;			\
} while (0)

/* Computes P = R * S. No overlap allowed.

   Temporary space is needed for two numbers smaller than the
   resulting matrix elements, i.e. bounded by 2*L <= N. */
static mp_size_t
hgcd_mul (struct hgcd_row *P, mp_size_t alloc,
	  const struct hgcd_row *R, mp_size_t rsize,
	  const struct hgcd_row *S, mp_size_t ssize,
	  mp_ptr tp, mp_size_t talloc)
{
  unsigned i;
  unsigned j;

  mp_size_t psize;
  mp_limb_t h = 0;
  int grow = 0;

  MPN_NORMALIZE (R[1].uvp[1], rsize);
  ASSERT (S[1].uvp[1][ssize - 1] != 0);

  psize = rsize + ssize;
  ASSERT (psize <= talloc);

  if (rsize >= ssize)
    {
      for (i = 0; i < 2; i++)
	for (j = 0; j < 2; j++)
	  {
	    /* Set P[i, j] = R[i, 0] S[0, j] + R[i,1] S[1, j] */
	    mp_limb_t cy;

	    mpn_mul (P[i].uvp[j], R[i].uvp[0], rsize, S[0].uvp[j], ssize);
	    mpn_mul (tp, R[i].uvp[1], rsize, S[1].uvp[j], ssize);

	    cy = mpn_add_n (P[i].uvp[j], P[i].uvp[j], tp, psize);

	    if (cy)
	      {
		ASSERT (psize + 1 < alloc);
		P[i].uvp[j][psize] = cy;
		grow = 1;
	      }
	    else
	      h |= P[i].uvp[j][psize - 1];
	  }
    }
  else
    {
      for (i = 0; i < 2; i++)
	for (j = 0; j < 2; j++)
	  {
	    /* Set P[i, j] = R[i, 0] S[0, j] + R[i,1] S[1, j] */
	    mp_limb_t cy;

	    mpn_mul (P[i].uvp[j], S[0].uvp[j], ssize, R[i].uvp[0], rsize);
	    mpn_mul (tp, S[1].uvp[j], ssize, R[i].uvp[1], rsize);

	    cy = mpn_add_n (P[i].uvp[j], P[i].uvp[j], tp, psize);

	    if (cy)
	      {
		ASSERT (psize + 1 < alloc);
		P[i].uvp[j][psize] = cy;
		grow = 1;
	      }
	    else
	      h |= P[i].uvp[j][psize - 1];
	  }
    }

  if (grow)
    return psize + 1;
  else
    return psize - (h == 0);
}

/* Computes R = W^k H + u A' - v B', which must be non-negative. W
   denotes 2^(GMP_NUMB_BITS). Temporary space needed is k + uvsize.

   H and R must not overlap. */

mp_size_t
mpn_hgcd_fix (mp_size_t k,
	      mp_ptr rp, mp_size_t ralloc,
	      mp_ptr hp, mp_size_t hsize,
	      int sign,
	      mp_srcptr up, mp_srcptr ap,
	      mp_srcptr vp, mp_srcptr bp,
	      mp_size_t uvsize,
	      mp_ptr tp, mp_size_t talloc)
{
  mp_size_t tsize;
  mp_limb_t cy;
  mp_size_t rsize;

  if (sign < 0)
    {
      MP_SRCPTR_SWAP (up, vp);
      MP_SRCPTR_SWAP (ap, bp);
    }

  tsize = k + uvsize;

  ASSERT (k + hsize <= ralloc);
  ASSERT (tsize <= talloc);
  ASSERT (tsize <= ralloc);

  ASSERT (rp != hp);

  /* r = W^k h + u a */
  if (uvsize <= k)
    mpn_mul (rp, ap, k, up, uvsize);
  else
    mpn_mul (rp, up, uvsize, ap, k);

  if (uvsize <= hsize)
    {
      cy = mpn_add (rp + k, hp, hsize, rp + k, uvsize);
      rsize = k + hsize;
    }
  else
    {
      cy = mpn_add (rp + k, rp + k, uvsize, hp, hsize);
      rsize = k + uvsize;
    }

  if (cy)
    {
      ASSERT (rsize < ralloc);
      rp[rsize++] = cy;
    }

  /* r -= v b */

  if (uvsize <= k)
    mpn_mul (tp, bp, k, vp, uvsize);
  else
    mpn_mul (tp, vp, uvsize, bp, k);

  ASSERT_NOCARRY (mpn_sub (rp, rp, rsize, tp, tsize));
  MPN_NORMALIZE (rp, rsize);

  return rsize;
}

/* Compute r2 = r0 - q r1 */
static void
hgcd_update_r (struct hgcd_row *r, mp_srcptr qp, mp_size_t qsize)
{
  mp_srcptr r0p = r[0].rp;
  mp_srcptr r1p = r[1].rp;
  mp_ptr r2p = r[2].rp;
  mp_size_t r0size = r[0].rsize;
  mp_size_t r1size = r[1].rsize;

  ASSERT (MPN_LESS_P (r1p, r1size, r0p, r0size));

  if (qsize == 0)
    {
      ASSERT_NOCARRY (mpn_sub (r2p, r0p, r0size, r1p, r1size));
    }
  else if (qsize == 1)
    {
      mp_size_t size;
      mp_limb_t cy = mpn_mul_1 (r2p, r1p, r1size, qp[0]);
      size = r1size;

      if (cy)
	{
	  ASSERT (size < r0size);
	  r2p[size++] = cy;
	}

      ASSERT_NOCARRY (mpn_sub (r2p, r0p, r0size, r2p, size));
    }
  else
    {
      mp_size_t size = r1size + qsize;
      ASSERT (size <= r0size + 1);

      if (qsize <= r1size)
	mpn_mul (r2p, r1p, r1size, qp, qsize);
      else
	mpn_mul (r2p, qp, qsize, r1p, r1size);

      if (size > r0size)
	{
	  ASSERT (size == r0size + 1);
	  size--;
	  ASSERT (r2p[size] == 0);
	}

      ASSERT_NOCARRY (mpn_sub (r2p, r0p, r0size, r2p, size));
    }

  MPN_NORMALIZE (r[2].rp, r0size);
  r[2].rsize = r0size;

  ASSERT (MPN_LESS_P (r2p, r0size, r1p, r1size));
}

/* Compute (u2, v2) = (u0, v0) + q (u1, v1)
   Return the size of the largest u,v element.
   Caller must ensure that usize + qsize <= available storage */
static mp_size_t
hgcd_update_uv (struct hgcd_row *r, mp_size_t usize,
		mp_srcptr qp, mp_size_t qsize)
{
  unsigned i;
  mp_size_t grow;

  ASSERT (r[1].uvp[1][usize - 1] != 0);

  /* Compute u2	 = u0 + q u1 */

  if (qsize == 0)
    {
      /* Represents a unit quotient */
      mp_limb_t cy;

      cy = mpn_add_n (r[2].uvp[0], r[0].uvp[0], r[1].uvp[0], usize);
      r[2].uvp[0][usize] = cy;

      cy = mpn_add_n (r[2].uvp[1], r[0].uvp[1], r[1].uvp[1], usize);
      r[2].uvp[1][usize] = cy;
      grow = cy;
    }
  else if (qsize == 1)
    {
      mp_limb_t q = qp[0];
      for (i = 0; i < 2; i++)
	{
	  mp_srcptr u0p = r[0].uvp[i];
	  mp_srcptr u1p = r[1].uvp[i];
	  mp_ptr u2p = r[2].uvp[i];
	  mp_limb_t cy;

	  /* Too bad we don't have an addmul_1 with distinct source and
	     destination */
	  cy = mpn_mul_1 (u2p, u1p, usize, q);
	  cy += mpn_add_n (u2p, u2p, u0p, usize);

	  u2p[usize] = cy;
	  grow = cy != 0;
	}
    }
  else
    {
      for (i = 0; i < 2; i++)
	{
	  mp_srcptr u0p = r[0].uvp[i];
	  mp_srcptr u1p = r[1].uvp[i];
	  mp_ptr u2p = r[2].uvp[i];

	  if (qsize <= usize)
	    mpn_mul (u2p, u1p, usize, qp, qsize);
	  else
	    mpn_mul (u2p, qp, qsize, u1p, usize);

	  ASSERT_NOCARRY (mpn_add (u2p, u2p, usize + qsize, u0p, usize));
	  grow = qsize - ((u2p[usize + qsize - 1]) == 0);
	}
    }

  usize += grow;

  /* The values should be allocated with one limb margin */
  ASSERT (mpn_cmp (r[1].uvp[0], r[2].uvp[0], usize) <= 0);
  ASSERT (mpn_cmp (r[1].uvp[1], r[2].uvp[1], usize) <= 0);
  ASSERT (r[2].uvp[1][usize - 1] != 0);

  return usize;
}

/* Compute r0 = r2 + q r1, and the corresponding uv */
static void
hgcd_backup (struct hgcd_row *r, mp_size_t usize,
	     mp_srcptr qp, mp_size_t qsize)
{
  mp_ptr r0p = r[0].rp;
  mp_srcptr r1p = r[1].rp;
  mp_srcptr r2p = r[2].rp;
  mp_size_t r0size;
  mp_size_t r1size = r[1].rsize;
  mp_size_t r2size = r[2].rsize;

  mp_ptr u0p = r[0].uvp[0];
  mp_ptr v0p = r[0].uvp[1];
  mp_srcptr u1p = r[1].uvp[0];
  mp_srcptr v1p = r[1].uvp[1];
  mp_srcptr u2p = r[2].uvp[0];
  mp_srcptr v2p = r[2].uvp[1];

  ASSERT (MPN_LESS_P (r2p, r2size, r1p, r1size));

  if (qsize == 0)
    {
      /* r0 = r2 + r1 */
      mp_limb_t cy = mpn_add (r0p, r1p, r1size, r2p, r2size);
      r0size = r1size;
      if (cy)
	r0p[r0size++] = cy;

      /* (u0,v0) = (u2,v2) - (u1, v1) */

      ASSERT_NOCARRY (mpn_sub_n (u0p, u2p, u1p, usize));
      ASSERT_NOCARRY (mpn_sub_n (v0p, v2p, v1p, usize));
    }
  else if (qsize == 1)
    {
      /* r0 = r2 + q r1

      Just like for mpn_addmul_1, the result is the same size as r1, or
      one limb larger. */

      mp_limb_t cy;

      cy = mpn_mul_1 (r0p, r1p, r1size, qp[0]);
      cy += mpn_add (r0p, r0p, r1size, r2p, r2size);

      r0size = r1size;
      if (cy)
	r0p[r0size++] = cy;

      /* (u0,v0) = (u2,v2) - q (u1, v1) */

      ASSERT_NOCARRY (mpn_mul_1 (u0p, u1p, usize, qp[0]));
      ASSERT_NOCARRY (mpn_sub_n (u0p, u2p, u0p, usize));

      ASSERT_NOCARRY (mpn_mul_1 (v0p, v1p, usize, qp[0]));
      ASSERT_NOCARRY (mpn_sub_n (v0p, v2p, v0p, usize));
    }
  else
    {
      /* r0 = r2 + q r1

	 Result must be of size r1size + q1size - 1, or one limb
	 larger. */

      mp_size_t size;

      r0size = r1size + qsize;
      if (r1size >= qsize)
	mpn_mul (r0p, r1p, r1size, qp, qsize);
      else
	mpn_mul (r0p, qp, qsize, r1p, r1size);

      ASSERT_NOCARRY (mpn_add (r0p, r0p, r0size, r2p, r2size));

      r0size -= (r0p[r0size-1] == 0);

      /* (u0,v0) = (u2,v2) - q (u1, v1) */

      /* We must have

	   usize >= #(q u1) >= qsize + #u1 - 1

	 which means that u1 must have at least

	   usize - #u1 >= qsize - 1

	 zero limbs at the high end, and similarly for v1. */

      ASSERT (qsize <= usize);
      size = usize - qsize + 1;
#if WANT_ASSERT
      {
	mp_size_t i;
	for (i = size; i < usize; i++)
	  {
	    ASSERT (u1p[i] == 0);
	    ASSERT (v1p[i] == 0);
	  }
      }
#endif
      /* NOTE: Needs an extra limb for the u,v values */

      if (qsize <= size)
	{
	  mpn_mul (u0p, u1p, size, qp, qsize);
	  mpn_mul (v0p, v1p, size, qp, qsize);
	}
      else
	{
	  mpn_mul (u0p, qp, qsize, u1p, size);
	  mpn_mul (v0p, qp, qsize, v1p, size);
	}

      /* qsize + size = usize + 1 */
      ASSERT (u0p[usize] == 0);
      ASSERT (v0p[usize] == 0);

      ASSERT_NOCARRY (mpn_sub_n (u0p, u2p, u0p, usize));
      ASSERT_NOCARRY (mpn_sub_n (v0p, v2p, v0p, usize));
    }

  r[0].rsize = r0size;
}

/* Called after HGCD_SWAP4_RIGHT, to adjust the size field. Large
   numbers in row 0 don't count, and are overwritten. */
static void
hgcd_normalize (struct hgcd *hgcd)
{
  mp_size_t size = hgcd->size;

  /* v3 should always be the largest element */
  while (size > 0 && hgcd->row[3].uvp[1][size - 1] == 0)
    {
      size--;
      /* Row 0 is about to be overwritten. We must zero out unused limbs */
      hgcd->row[0].uvp[0][size] = 0;
      hgcd->row[0].uvp[1][size] = 0;

      ASSERT (hgcd->row[1].uvp[0][size] == 0);
      ASSERT (hgcd->row[1].uvp[1][size] == 0);
      ASSERT (hgcd->row[2].uvp[0][size] == 0);
      ASSERT (hgcd->row[2].uvp[1][size] == 0);
      ASSERT (hgcd->row[3].uvp[0][size] == 0);
    }

  hgcd->size = size;
}

static mp_size_t
euclid_step (struct hgcd_row *r, mp_size_t usize,
	     struct qstack *quotients,
	     /* For sanity checking only */
	     mp_size_t alloc)
{
  mp_size_t qsize;
  mp_ptr qp;

  qsize = qstack_push_quotient (quotients, &qp,
				r[2].rp, &r[2].rsize,
				r[0].rp, r[0].rsize,
				r[1].rp, r[1].rsize);
  ASSERT (usize + qsize <= alloc);
  return hgcd_update_uv (r, usize, qp, qsize);
}

int
mpn_hgcd2_lehmer_step (struct hgcd2 *hgcd,
		       mp_srcptr ap, mp_size_t asize,
		       mp_srcptr bp, mp_size_t bsize,
		       struct qstack *quotients)
{
  mp_limb_t ah;
  mp_limb_t al;
  mp_limb_t bh;
  mp_limb_t bl;

  ASSERT (asize >= bsize);
  ASSERT (MPN_LEQ_P (bp, bsize, ap, asize));

  if (bsize < 2)
    return 0;

#if 0 && WANT_TRACE
  trace ("lehmer_step:\n"
	 "  a = %Nd\n"
	 "  b = %Nd\n",
	 ap, asize, bp, bsize);
#endif
  /* The case asize == 2 is needed to take care of values that are
     between one and two *full* limbs in size. */
  if (asize == 2 || (ap[asize-1] & GMP_NUMB_HIGHBIT))
    {
      if (bsize < asize)
	return 0;

      al = ap[asize - 2];
      ah = ap[asize - 1];

      ASSERT (asize == bsize);
      bl = bp[asize - 2];
      bh = bp[asize - 1];
    }
  else
    {
      unsigned shift;
      if (bsize + 1 < asize)
	return 0;

      /* We want two *full* limbs */
      ASSERT (asize > 2);

      count_leading_zeros (shift, ap[asize-1]);
#if 0 && WANT_TRACE
      trace ("shift = %d\n", shift);
#endif
      if (bsize == asize)
	bh = MPN_EXTRACT_LIMB (shift, bp[asize - 1], bp[asize - 2]);
      else
	{
	  ASSERT (asize == bsize + 1);
	  bh = bp[asize - 2] >> (GMP_LIMB_BITS - shift);
	}
      if (!bh)
	return 0;

      bl = MPN_EXTRACT_LIMB (shift, bp[asize - 2], bp[asize - 3]);

      al = MPN_EXTRACT_LIMB (shift, ap[asize - 2], ap[asize - 3]);
      ah = MPN_EXTRACT_LIMB (shift, ap[asize - 1], ap[asize - 2]);
    }

  return mpn_hgcd2 (hgcd, ah, al, bh, bl, quotients);
}

/* Scratch space needed to compute |u2| + |u1| or |v2| + |v1|, where
   all terms are at most floor (asize/2) limbs. */
#define HGCD_JEBELEAN_ITCH(asize) ((asize)/2 + 1)

/* Called when r[0, 1, 2] >= W^M, r[3] < W^M. Returns the number of
   the remainders that satisfy Jebelean's criterion, i.e. find the
   largest k such that

     r[k+1] >= max (-u[k+1], - v[k+1])

     r[k] - r[k-1] >= max (u[k+1] - u[k], v[k+1] - v[k])

   Returns 2, 3 or 4.
 */
static int
hgcd_jebelean (const struct hgcd *hgcd, mp_size_t M,
	       mp_ptr tp, mp_size_t talloc)
{
  mp_size_t L;
  mp_size_t tsize;
  mp_limb_t cy;

  ASSERT (hgcd->row[0].rsize > M);
  ASSERT (hgcd->row[1].rsize > M);
  ASSERT (hgcd->row[2].rsize > M);
  ASSERT (hgcd->row[3].rsize <= M);

  ASSERT (MPN_LESS_P (hgcd->row[1].rp, hgcd->row[1].rsize,
		      hgcd->row[0].rp, hgcd->row[0].rsize));
  ASSERT (MPN_LESS_P (hgcd->row[2].rp, hgcd->row[2].rsize,
		      hgcd->row[1].rp, hgcd->row[1].rsize));
  ASSERT (MPN_LESS_P (hgcd->row[3].rp, hgcd->row[3].rsize,
		      hgcd->row[2].rp, hgcd->row[2].rsize));

  ASSERT (mpn_cmp (hgcd->row[0].uvp[1], hgcd->row[1].uvp[1], hgcd->size) <= 0);
  ASSERT (mpn_cmp (hgcd->row[1].uvp[1], hgcd->row[2].uvp[1], hgcd->size) <= 0);
  ASSERT (mpn_cmp (hgcd->row[2].uvp[1], hgcd->row[3].uvp[1], hgcd->size) <= 0);

  /* The bound is really floor (N/2), which is <= M = ceil (N/2) */
  L = hgcd->size;
  ASSERT (L <= M);

  ASSERT (L > 0);
  ASSERT (hgcd->row[3].uvp[1][L - 1] != 0);

  ASSERT (L < talloc);

#if WANT_TRACE
  trace ("hgcd_jebelean: sign = %d\n", hgcd->sign);

  if (L < 200)
    {
      unsigned i;
      for (i = 0; i<4; i++)
	trace (" r%d = %Nd; u%d = %Nd; v%d = %Nd;\n",
	       i, hgcd->row[i].rp, hgcd->row[i].rsize,
	       i, hgcd->row[i].uvp[0], hgcd->size,
	       i, hgcd->row[i].uvp[1], hgcd->size);
    }
#endif

  tsize = L;

  if (hgcd->sign >= 0)
    {
      /* Check if r1 - r2 >= u2 - u1 */
      cy = mpn_add_n (tp, hgcd->row[2].uvp[0], hgcd->row[1].uvp[0], L);
    }
  else
    {
      /* Check if r1 - r2 >= v2 - v1 */
      cy = mpn_add_n (tp, hgcd->row[2].uvp[1], hgcd->row[1].uvp[1], L);
    }
  if (cy)
    tp[tsize++] = cy;
  else
    MPN_NORMALIZE (tp, tsize);

  if (mpn_diff_smaller_p (hgcd->row[1].rp, hgcd->row[1].rsize,
			  hgcd->row[2].rp, hgcd->row[2].rsize, tp, tsize))
    return 2;

  /* Ok, r2 is correct */

  tsize = L;

  if (hgcd->sign >= 0)
    {
      /* Check r3 >= max (-u3, -v3) = u3 */
      if (hgcd->row[3].rsize > L)
	/* Condition satisfied */
	;
      else
	{
	  mp_size_t size;
	  for (size = L; size > hgcd->row[3].rsize; size--)
	    {
	      if (hgcd->row[3].uvp[0][size-1] != 0)
		return 3;
	    }
	  if (mpn_cmp (hgcd->row[3].rp, hgcd->row[3].uvp[0], size) <= 0)
	    return 3;
	}

      /* Check r3 - r2 >= v3 - v2 */
      cy = mpn_add_n (tp, hgcd->row[3].uvp[1], hgcd->row[2].uvp[1], L);
    }
  else
    {
      /* Check r3 >= max (-u3, -v3) = v3 */
      if (hgcd->row[3].rsize > L)
	/* Condition satisfied */
	;
      else
	{
	  mp_size_t size;
	  for (size = L; size > hgcd->row[3].rsize; size--)
	    {
	      if (hgcd->row[3].uvp[1][size-1] != 0)
		return 3;
	    }
	  if (mpn_cmp (hgcd->row[3].rp, hgcd->row[3].uvp[1], size) <= 0)
	    return 3;
	}

      /* Check r3 - r2 >= u3 - u2 */

      cy = mpn_add_n (tp, hgcd->row[3].uvp[0], hgcd->row[2].uvp[0], L);
    }

  if (cy)
    tp[tsize++] = cy;
  else
    MPN_NORMALIZE (tp, tsize);

  if (mpn_diff_smaller_p (hgcd->row[2].rp, hgcd->row[2].rsize,
			  hgcd->row[3].rp, hgcd->row[3].rsize, tp, tsize))
    return 3;

  /* Ok, r3 is correct */
  return 4;
}

/* Only the first row has v = 0, a = 1 * a + 0 * b */
static int
hgcd_start_row_p (struct hgcd_row *r, mp_size_t n)
{
  mp_size_t i;
  for (i = 0; i < n; i++)
    if (r->uvp[1][i] != 0)
      return 0;

  return 1;
}

/* Called when r2 has been computed, and it is too small. Top element
   on the stack is r0/r1. One backup step is needed. */
static int
hgcd_small_1 (struct hgcd *hgcd, mp_size_t M,
	      struct qstack *quotients,
	      mp_ptr tp, mp_size_t talloc)
{
  mp_srcptr qp;
  mp_size_t qsize;

  if (hgcd_start_row_p (hgcd->row, hgcd->size))
    {
      qstack_drop (quotients);
      return 0;
    }

  HGCD_SWAP4_RIGHT (hgcd->row);
  hgcd_normalize (hgcd);

  qsize = qstack_get_1 (quotients, &qp);

  hgcd_backup (hgcd->row, hgcd->size, qp, qsize);
  hgcd->sign = ~hgcd->sign;

#if WANT_ASSERT
  qstack_rotate (quotients, 0);
#endif

  return hgcd_jebelean (hgcd, M, tp, talloc);
}

/* Called when r3 has been computed, and is small enough. Two backup
   steps are needed. */
static int
hgcd_small_2 (struct hgcd *hgcd, mp_size_t M,
	      struct qstack *quotients,
	      mp_ptr tp, mp_size_t talloc)
{
  mp_srcptr qp;
  mp_size_t qsize;

  if (hgcd_start_row_p (hgcd->row + 2, hgcd->size))
    return 0;

  qsize = qstack_get_0 (quotients, &qp);
  hgcd_backup (hgcd->row+1, hgcd->size, qp, qsize);

  if (hgcd_start_row_p (hgcd->row + 1, hgcd->size))
    return 0;

  qsize = qstack_get_1 (quotients, &qp);
  hgcd_backup (hgcd->row, hgcd->size, qp, qsize);

  return hgcd_jebelean (hgcd, M, tp, talloc);
}

static void
hgcd_start (struct hgcd *hgcd,
	    mp_srcptr ap, mp_size_t asize,
	    mp_srcptr bp, mp_size_t bsize)
{
  MPN_COPY (hgcd->row[0].rp, ap, asize);
  hgcd->row[0].rsize = asize;

  MPN_COPY (hgcd->row[1].rp, bp, bsize);
  hgcd->row[1].rsize = bsize;

  hgcd->sign = 0;
  if (hgcd->size != 0)
    {
      /* We must zero out the uv array */
      unsigned i;
      unsigned j;

      for (i = 0; i < 4; i++)
	for (j = 0; j < 2; j++)
	  MPN_ZERO (hgcd->row[i].uvp[j], hgcd->size);
    }
#if WANT_ASSERT
  {
    unsigned i;
    unsigned j;
    mp_size_t k;

    for (i = 0; i < 4; i++)
      for (j = 0; j < 2; j++)
	for (k = hgcd->size; k < hgcd->alloc; k++)
	  ASSERT (hgcd->row[i].uvp[j][k] == 0);
  }
#endif

  hgcd->size = 1;
  hgcd->row[0].uvp[0][0] = 1;
  hgcd->row[1].uvp[1][0] = 1;
}

/* Performs one euclid step on r0, r1. Returns >= 0 if hgcd should be
   terminated, -1 if we should go on */
/* FIXME: Rename function */
static int
hgcd_case0 (struct hgcd *hgcd, mp_size_t M,
	    struct qstack *quotients,
	    mp_ptr tp, mp_size_t talloc)
{
  hgcd->size = euclid_step (hgcd->row, hgcd->size, quotients, hgcd->alloc);
  ASSERT (hgcd->size < hgcd->alloc);

  if (hgcd->row[2].rsize <= M)
    return hgcd_small_1 (hgcd, M, quotients, tp, talloc);
  else
    {
      /* Keep this remainder */
      hgcd->sign = ~hgcd->sign;

      HGCD_SWAP4_LEFT (hgcd->row);
      return -1;
    }
}

/* Called when values have been computed in rows 2 and 3, and the latter
   value is too large, and we know that it's not much too large. */
static void
hgcd_adjust (struct hgcd *hgcd,
	     struct qstack *quotients)
{
  /* Compute the correct r3, we have r3' = r3 - d r2, with
     d = 1 or 2. */

  mp_limb_t d;
  mp_limb_t c0;
  mp_limb_t c1;

  ASSERT_NOCARRY (mpn_sub (hgcd->row[3].rp,
			   hgcd->row[3].rp, hgcd->row[3].rsize,
			   hgcd->row[2].rp, hgcd->row[2].rsize));

  MPN_NORMALIZE (hgcd->row[3].rp, hgcd->row[3].rsize);
  if (MPN_LESS_P (hgcd->row[3].rp, hgcd->row[3].rsize,
		  hgcd->row[2].rp, hgcd->row[2].rsize))
    {
      d = 1;

      c0 = mpn_add_n (hgcd->row[3].uvp[0],
		      hgcd->row[3].uvp[0], hgcd->row[2].uvp[0],
		      hgcd->size);
      c1 = mpn_add_n (hgcd->row[3].uvp[1],
		      hgcd->row[3].uvp[1], hgcd->row[2].uvp[1],
		      hgcd->size);
    }
  else
    {
      ASSERT_NOCARRY (mpn_sub (hgcd->row[3].rp,
			       hgcd->row[3].rp, hgcd->row[3].rsize,
			       hgcd->row[2].rp, hgcd->row[2].rsize));

      MPN_NORMALIZE (hgcd->row[3].rp, hgcd->row[3].rsize);
      ASSERT (MPN_LESS_P (hgcd->row[3].rp, hgcd->row[3].rsize,
			  hgcd->row[2].rp, hgcd->row[2].rsize));

      d = 2;
      c0 = mpn_addmul_1 (hgcd->row[3].uvp[0],
			 hgcd->row[2].uvp[0],
			 hgcd->size, 2);
      c1 = mpn_addmul_1 (hgcd->row[3].uvp[1],
			 hgcd->row[2].uvp[1],
			 hgcd->size, 2);
    }
  if (c1 != 0)
    {
      hgcd->row[3].uvp[0][hgcd->size] = c0;
      hgcd->row[3].uvp[1][hgcd->size] = c1;
      hgcd->size++;
      ASSERT (hgcd->size < hgcd->alloc);
    }
  else
    {
      ASSERT (c0 == 0);
    }

  /* Remains to adjust the quotient on stack */
  qstack_adjust (quotients, d);
}

/* Called with r0 and r1 of approximately the right size */
static int
hgcd_final (struct hgcd *hgcd, mp_size_t M,
	    struct qstack *quotients,
	    mp_ptr tp, mp_size_t talloc)
{
  ASSERT (hgcd->row[0].rsize > M);
  ASSERT (hgcd->row[1].rsize == M + 1);

  ASSERT (MPN_LESS_P (hgcd->row[1].rp, hgcd->row[1].rsize,
		      hgcd->row[0].rp, hgcd->row[0].rsize));

  /* Use euclid steps to get to the desired size. */
  /* FIXME: We could perhaps perform one lehmer step */
  hgcd->size = euclid_step (hgcd->row, hgcd->size,
			    quotients, hgcd->alloc);
  ASSERT (hgcd->size < hgcd->alloc);

  if (hgcd->row[2].rsize <= M)
    return hgcd_small_1 (hgcd, M, quotients,
			 tp, talloc);

  for (;;)
    {
      ASSERT (MPN_LESS_P (hgcd->row[1].rp, hgcd->row[1].rsize,
			  hgcd->row[0].rp, hgcd->row[0].rsize));
      ASSERT (MPN_LESS_P (hgcd->row[2].rp, hgcd->row[2].rsize,
			  hgcd->row[1].rp, hgcd->row[1].rsize));

      ASSERT (mpn_cmp (hgcd->row[0].uvp[1], hgcd->row[1].uvp[1],
		       hgcd->size) <= 0);
      ASSERT (mpn_cmp (hgcd->row[1].uvp[1], hgcd->row[2].uvp[1],
		       hgcd->size) <= 0);

      hgcd->size = euclid_step (hgcd->row + 1, hgcd->size,
				quotients,
				hgcd->alloc);
      ASSERT (hgcd->size < hgcd->alloc);

      if (hgcd->row[3].rsize <= M)
	{
#if WANT_ASSERT
	  qstack_rotate (quotients, 0);
#endif
	  return hgcd_jebelean (hgcd, M, tp, talloc);
	}
      hgcd->sign = ~hgcd->sign;

      HGCD_SWAP4_LEFT (hgcd->row);
    }
}


mp_size_t
mpn_hgcd_lehmer_itch (mp_size_t asize)
{
  /* Scratch space is needed for calling hgcd_lehmer. */
  return HGCD_JEBELEAN_ITCH (asize);
}

/* Repeatedly divides A by B, until the remainder fits in M =
   ceil(asize / 2) limbs. Stores cofactors in HGCD, and pushes the
   quotients on STACK. On success, HGCD->row[0, 1, 2] correspond to
   remainders that are larger than M limbs, while HGCD->row[3]
   correspond to a remainder that fit in M limbs.

   Returns 0 on failure (if B or A mod B fits in M limbs), otherwise
   returns 2, 3 or 4 depending on how many of the r:s that satisfy
   Jebelean's criterion. */
int
mpn_hgcd_lehmer (struct hgcd *hgcd,
		 mp_srcptr ap, mp_size_t asize,
		 mp_srcptr bp, mp_size_t bsize,
		 struct qstack *quotients,
		 mp_ptr tp, mp_size_t talloc)
{
  mp_size_t N = asize;
  mp_size_t M = (N + 1)/2;

  ASSERT (M);

#if WANT_TRACE
  trace ("hgcd_lehmer: asize = %d, bsize = %d, HGCD_SCHOENHAGE_THRESHOLD = %d\n",
	 asize, bsize, HGCD_SCHOENHAGE_THRESHOLD);
#endif

  if (bsize <= M)
    return 0;

  ASSERT (asize >= 2);

  /* Initialize, we keep r0 and r1 as the reduced numbers (so far). */
  hgcd_start (hgcd, ap, asize, bp, bsize);

  while (hgcd->row[1].rsize > M + 1)
    {
      struct hgcd2 R;

      /* Max size after reduction, plus one */
      mp_size_t ralloc = hgcd->row[1].rsize + 1;

      ASSERT_HGCD (hgcd, ap, asize, bp, bsize, 0, 2);

      switch (mpn_hgcd2_lehmer_step (&R,
				     hgcd->row[0].rp, hgcd->row[0].rsize,
				     hgcd->row[1].rp, hgcd->row[1].rsize,
				     quotients))
	{
	default:
	  ASSERT_FAIL (1);
	case 0:
	  {
	    /* The first remainder was small. Then there's a good chance
	       that the remainder A % B is also small. */
	    int res = hgcd_case0 (hgcd, M, quotients, tp, talloc);

	    if (res > 0)
	      ASSERT_HGCD (hgcd, ap, asize, bp, bsize, 0, 4);
	    if (res >= 0)
	      return res;

	    ASSERT_HGCD (hgcd, ap, asize, bp, bsize, 0, 2);
	    break;
	  }
	case 2:
	  /* r0 and r1 are correct, r2 may be too large. */
	  /* Compute r1 and r2, and check what happened */

	  hgcd->row[2].rsize
	    = mpn_hgcd2_fix (hgcd->row[2].rp, ralloc,
			     ~R.sign,
			     R.row[1].u, hgcd->row[0].rp, hgcd->row[0].rsize,
			     R.row[1].v, hgcd->row[1].rp, hgcd->row[1].rsize);

	  hgcd->row[3].rsize
	    = mpn_hgcd2_fix (hgcd->row[3].rp, ralloc,
			     R.sign,
			     R.row[2].u, hgcd->row[0].rp, hgcd->row[0].rsize,
			     R.row[2].v, hgcd->row[1].rp, hgcd->row[1].rsize);

	  ASSERT (hgcd->row[2].rsize > M);

	  if (MPN_LESS_P (hgcd->row[3].rp, hgcd->row[3].rsize,
			  hgcd->row[2].rp, hgcd->row[2].rsize))
	    /* r2 was correct. */
	    goto correct_r2;
	  else
	    {
	      /* Computes the uv matrix for the (incorrect) values r1, r2.
		 The elements must be smaller than the correct ones, since
		 they correspond to a too small q. */
	      hgcd->size = hgcd2_mul (hgcd->row + 2, hgcd->alloc,
				      R.row + 1, hgcd->row, hgcd->size);
	      hgcd->sign ^= ~R.sign;
	      /* We have r3 > r2, so avoid that assert */
	      ASSERT_HGCD (hgcd, ap, asize, bp, bsize, 2, 3);
	      ASSERT_HGCD (hgcd, ap, asize, bp, bsize, 3, 4);

	      /* Discard r3, and the corresponding quotient */
	      qstack_drop (quotients);

	      hgcd_adjust (hgcd, quotients);
	      ASSERT_HGCD (hgcd, ap, asize, bp, bsize, 2, 4);

	      /* FIXME: Bound on new r2? */

	      if (hgcd->row[3].rsize <= M)
		{
		  /* Backup two steps */
		  ASSERT (!hgcd_start_row_p (hgcd->row + 2, hgcd->size));
		  return hgcd_small_2 (hgcd, M, quotients, tp, talloc);
		}

	      HGCD_SWAP4_2 (hgcd->row);

	      continue;
	    }
	case 3:
	  /* Now r0, r1 and r2 are correct, while r3 may be too small
	     or too large. */

	  hgcd->row[2].rsize
	    = mpn_hgcd2_fix (hgcd->row[2].rp, ralloc,
			     ~R.sign,
			     R.row[1].u, hgcd->row[0].rp, hgcd->row[0].rsize,
			     R.row[1].v, hgcd->row[1].rp, hgcd->row[1].rsize);

	  hgcd->row[3].rsize
	    = mpn_hgcd2_fix (hgcd->row[3].rp, ralloc,
			     R.sign,
			     R.row[2].u, hgcd->row[0].rp, hgcd->row[0].rsize,
			     R.row[2].v, hgcd->row[1].rp, hgcd->row[1].rsize);

	correct_r2:
	  /* Discard r3, and the corresponding quotient */
	  qstack_drop (quotients);

	  ASSERT (hgcd->row[2].rsize > M);
	  ASSERT (hgcd->row[3].rsize > M);

	  ASSERT (MPN_LESS_P (hgcd->row[3].rp, hgcd->row[3].rsize,
			      hgcd->row[2].rp, hgcd->row[2].rsize));

	  hgcd->size = hgcd2_mul (hgcd->row + 2, hgcd->alloc,
				   R.row + 1, hgcd->row, hgcd->size);
	  hgcd->sign ^= ~R.sign;

	  ASSERT_HGCD (hgcd, ap, asize, bp, bsize, 2, 4);

	  HGCD_SWAP4_2 (hgcd->row);

	  /* FIXME: Additional euclid step here? */
	  break;
	case 4:
	  /* All of r0, r1, r3 and r3 are correct.
	     Compute r2 and r3 */

	  hgcd->row[2].rsize
	    = mpn_hgcd2_fix (hgcd->row[2].rp, ralloc,
			     R.sign,
			     R.row[2].u, hgcd->row[0].rp, hgcd->row[0].rsize,
			     R.row[2].v, hgcd->row[1].rp, hgcd->row[1].rsize);

	  hgcd->row[3].rsize
	    = mpn_hgcd2_fix (hgcd->row[3].rp, ralloc,
			     ~R.sign,
			     R.row[3].u, hgcd->row[0].rp, hgcd->row[0].rsize,
			     R.row[3].v, hgcd->row[1].rp, hgcd->row[1].rsize);

	  ASSERT (hgcd->row[2].rsize > M);

	  /* FIXME: Bound on r3? */

	  hgcd->size = hgcd2_mul (hgcd->row + 2, hgcd->alloc,
				  R.row + 2, hgcd->row, hgcd->size);
	  hgcd->sign ^= R.sign;

	  ASSERT_HGCD (hgcd, ap, asize, bp, bsize, 2, 4);

	  if (hgcd->row[3].rsize <= M)
	    {
	      /* Backup two steps */
	      /* Both steps must always be possible, but it's not
		 trivial to ASSERT that here. */
	      ASSERT (!hgcd_start_row_p (hgcd->row + 2, hgcd->size));
	      return hgcd_small_2 (hgcd, M, quotients, tp, talloc);
	    }

	  HGCD_SWAP4_2 (hgcd->row);
	}
    }

  ASSERT_HGCD (hgcd, ap, asize, bp, bsize, 0, 2);

  return hgcd_final (hgcd, M, quotients, tp, talloc);
}

mp_size_t
mpn_hgcd_itch (mp_size_t asize)
{
  /* Scratch space is needed for calling hgcd. We need space for the
     results of all recursive calls. In addition, we need to call
     hgcd_lehmer, hgcd_jebelean, hgcd_fix and hgcd_mul, for which
     asize limbs should be enough. */

  /* Limit on the recursion depth */
  unsigned k = mpn_hgcd_max_recursion (asize);

  return asize + mpn_hgcd_init_itch (asize + 6 * k) + 12 * k;
}

/* Computes hgcd using Sch�nhage's algorithm. Should return the same
   numbers as mpn_hgcd_lehmer, but computes them asymptotically
   faster. */
int
mpn_hgcd (struct hgcd *hgcd,
	  mp_srcptr ap, mp_size_t asize,
	  mp_srcptr bp, mp_size_t bsize,
	  struct qstack *quotients,
	  mp_ptr tp, mp_size_t talloc)
{
  mp_size_t N = asize;
  mp_size_t M = (N + 1)/2;
  mp_size_t n;
  mp_size_t m;

  struct hgcd R;
  mp_size_t itch;

  ASSERT (M);
#if WANT_TRACE
  trace ("hgcd: asize = %d, bsize = %d, HGCD_SCHOENHAGE_THRESHOLD = %d\n",
	 asize, bsize, HGCD_SCHOENHAGE_THRESHOLD);
  if (asize < 100)
    trace ("  a = %Nd\n"
	   "  b = %Nd\n", ap, asize, bp, bsize);
#endif

  if (bsize <= M)
    return 0;

  ASSERT (asize >= 2);

  if (BELOW_THRESHOLD (N, HGCD_SCHOENHAGE_THRESHOLD))
    return mpn_hgcd_lehmer (hgcd, ap, asize, bp, bsize,
			    quotients, tp, talloc);

  /* Initialize, we keep r0 and r1 as the reduced numbers (so far). */
  hgcd_start (hgcd, ap, asize, bp, bsize);

  /* Reduce the size to M + m + 1. Usually, only one hgcd call is
     needed, but we may need multiple calls. When finished, the values
     are stored in r0 (potentially large) and r1 (smaller size) */

  n = N - M;
  m = (n + 1)/2;

  /* The second recursive call can use numbers of size up to n+3 */
  itch = mpn_hgcd_init_itch (n+3);

  ASSERT (itch <= talloc);
  mpn_hgcd_init (&R, n+3, tp);
  tp += itch; talloc -= itch;

  while (hgcd->row[1].rsize > M + m + 1)
    {
      /* Max size after reduction, plus one */
      mp_size_t ralloc = hgcd->row[1].rsize + 1;
      switch (mpn_hgcd (&R,
			hgcd->row[0].rp + M, hgcd->row[0].rsize - M,
			hgcd->row[1].rp + M, hgcd->row[1].rsize - M,
			quotients, tp, talloc))
	{
	default:
	  ASSERT_FAIL (1);

	case 0:
	  {
	    /* The first remainder was small. Then there's a good chance
	       that the remainder A % B is also small. */
	    int res = hgcd_case0 (hgcd, M, quotients, tp, talloc);

	    if (res > 0)
	      ASSERT_HGCD (hgcd, ap, asize, bp, bsize, 0, 4);
	    if (res >= 0)
	      return res;

	    ASSERT_HGCD (hgcd, ap, asize, bp, bsize, 0, 2);
	    break;
	  }

	case 2:
	  /* Now r0 and r1 are correct, while r2 may be too large.
	   * Compute r1 and r2, and check what happened */

	  ASSERT_HGCD (&R,
		       hgcd->row[0].rp + M, hgcd->row[0].rsize - M,
		       hgcd->row[1].rp + M, hgcd->row[1].rsize - M,
		       0, 4);

	  /* Store new values in rows 2 and 3, to avoid overlap */
	  hgcd->row[2].rsize
	    = mpn_hgcd_fix (M, hgcd->row[2].rp, ralloc,
			    R.row[1].rp, R.row[1].rsize,
			    ~R.sign,
			    R.row[1].uvp[0], hgcd->row[0].rp,
			    R.row[1].uvp[1], hgcd->row[1].rp,
			    R.size,
			    tp, talloc);

	  hgcd->row[3].rsize
	    = mpn_hgcd_fix (M, hgcd->row[3].rp, ralloc,
			    R.row[2].rp, R.row[2].rsize,
			    R.sign,
			    R.row[2].uvp[0], hgcd->row[0].rp,
			    R.row[2].uvp[1], hgcd->row[1].rp,
			    R.size,
			    tp, talloc);

	  ASSERT (hgcd->row[2].rsize > M);

	  if (MPN_LESS_P (hgcd->row[3].rp, hgcd->row[3].rsize,
			  hgcd->row[2].rp, hgcd->row[2].rsize))
	    /* r2 was correct. */
	    goto correct_r2;
	  else
	    {
	      /* r2 was too large, i.e. q0 too small. In this case we
		 must have r2 % r1 <= r2 - r1 smaller than M + m + 1. */

	      /* Computes the uv matrix for the (incorrect) values r1, r2.
		 The elements must be smaller than the correct ones, since
		 they correspond to a too small q. */
	      hgcd->size = hgcd_mul (hgcd->row + 2, hgcd->alloc,
				     R.row + 1, R.size,
				     hgcd->row, hgcd->size,
				     tp, talloc);
	      hgcd->sign ^= ~R.sign;
	      /* We have r3 > r2, so avoid that assert */
	      ASSERT_HGCD (hgcd, ap, asize, bp, bsize, 2, 3);
	      ASSERT_HGCD (hgcd, ap, asize, bp, bsize, 3, 4);

	      /* Discard r3, and the corresponding quotient */
	      qstack_drop (quotients);

	      hgcd_adjust (hgcd, quotients);
	      ASSERT_HGCD (hgcd, ap, asize, bp, bsize, 2, 4);

	      ASSERT (hgcd->row[3].rsize <= M + m + 1);

	      if (hgcd->row[3].rsize <= M)
		{
		  /* Backup two steps */
		  ASSERT (!hgcd_start_row_p (hgcd->row + 2, hgcd->size));

		  return hgcd_small_2 (hgcd, M, quotients, tp, talloc);
		}

	      HGCD_SWAP4_2 (hgcd->row);
	      goto reduction_done;
	    }
	case 3:
	  /* Now r0, r1 and r2 are correct, while r3 may be too small
	     or too large. */
	  /* Compute r1 and r2, and check what happened */
	  ASSERT_HGCD (&R,
		       hgcd->row[0].rp + M, hgcd->row[0].rsize - M,
		       hgcd->row[1].rp + M, hgcd->row[1].rsize - M,
		       0, 4);

	  /* Store new values in rows 2 and 3, to avoid overlap */
	  hgcd->row[2].rsize
	    = mpn_hgcd_fix (M, hgcd->row[2].rp, ralloc,
			    R.row[1].rp, R.row[1].rsize,
			    ~R.sign,
			    R.row[1].uvp[0], hgcd->row[0].rp,
			    R.row[1].uvp[1], hgcd->row[1].rp,
			    R.size,
			    tp, talloc);

	  hgcd->row[3].rsize
	    = mpn_hgcd_fix (M, hgcd->row[3].rp, ralloc,
			    R.row[2].rp, R.row[2].rsize,
			    R.sign,
			    R.row[2].uvp[0], hgcd->row[0].rp,
			    R.row[2].uvp[1], hgcd->row[1].rp,
			    R.size,
			    tp, talloc);

	correct_r2:
	  /* Discard r3, and the corresponding quotient */
	  qstack_drop (quotients);

	  ASSERT (hgcd->row[2].rsize > M);
	  ASSERT (hgcd->row[3].rsize > M);

	  ASSERT (MPN_LESS_P (hgcd->row[3].rp, hgcd->row[3].rsize,
			      hgcd->row[2].rp, hgcd->row[2].rsize));

	  hgcd->size = hgcd_mul (hgcd->row+2, hgcd->alloc,
				 R.row+1, R.size,
				 hgcd->row, hgcd->size,
				 tp, talloc);
	  hgcd->sign ^= ~R.sign;

	  /* We haven't computed r3, but there are three cases.

	  r3 < 0: We have no good bounds for either r2 or r1 mod r2.
	  We may need another hgcd call.

	  0 <= r3 < r2: Then r3 is correct, and small enough. An
	  additioanl euclid step could be a win.

	  r3 >= r2: Then r2 is small, so we are done with the first
	  phase.

	  For now, we don't compute r3, we just check the stop
	  condition for r2. */

	  ASSERT_HGCD (hgcd, ap, asize, bp, bsize, 2, 4);
	  HGCD_SWAP4_2 (hgcd->row);

	  break;

	case 4:
	  /* All of r0, r1, r3 and r3 are correct.
	     Compute r2 and r3 */

	  ASSERT_HGCD (&R,
		       hgcd->row[0].rp + M, hgcd->row[0].rsize - M,
		       hgcd->row[1].rp + M, hgcd->row[1].rsize - M,
		       0, 4);

	  /* Store new values in rows 2 and 3, to avoid overlap */
	  hgcd->row[2].rsize
	    = mpn_hgcd_fix (M, hgcd->row[2].rp, ralloc,
			    R.row[2].rp, R.row[2].rsize,
			    R.sign,
			    R.row[2].uvp[0], hgcd->row[0].rp,
			    R.row[2].uvp[1], hgcd->row[1].rp,
			    R.size,
			    tp, talloc);

	  hgcd->row[3].rsize
	    = mpn_hgcd_fix (M, hgcd->row[3].rp, ralloc,
			    R.row[3].rp, R.row[3].rsize,
			    ~R.sign,
			    R.row[3].uvp[0], hgcd->row[0].rp,
			    R.row[3].uvp[1], hgcd->row[1].rp,
			    R.size,
			    tp, talloc);

	  ASSERT (hgcd->row[2].rsize > M);
	  ASSERT (hgcd->row[3].rsize <= M + m + 1);

	  hgcd->size = hgcd_mul (hgcd->row+2, hgcd->alloc,
				 R.row+2, R.size,
				 hgcd->row, hgcd->size,
				 tp, talloc);
	  hgcd->sign ^= R.sign;

	  ASSERT_HGCD (hgcd, ap, asize, bp, bsize, 2, 4);

	  if (hgcd->row[3].rsize <= M)
	    {
	      /* Backup two steps */
	      /* Both steps must always be possible, but it's not
		 trivial to ASSERT that here. */
	      ASSERT (!hgcd_start_row_p (hgcd->row + 2, hgcd->size));

	      return hgcd_small_2 (hgcd, M, quotients, tp, talloc);
	    }
	  HGCD_SWAP4_2 (hgcd->row);

	  /* Short cut, we would exit the loop anyway. */
	  goto reduction_done;
	}
    }

 reduction_done:
  ASSERT (hgcd->row[0].rsize >= hgcd->row[1].rsize);
  ASSERT (hgcd->row[1].rsize > M);
  ASSERT (hgcd->row[1].rsize <= M + m + 1);

  if (hgcd->row[0].rsize > M + m + 1)
    {
      /* One euclid step to reduce size. */
      /* FIXME: Rename hgcd_case0 */
      int res = hgcd_case0 (hgcd, M, quotients,
			    tp, talloc);

      if (res > 0)
	ASSERT_HGCD (hgcd, ap, asize, bp, bsize, 0, 4);
      if (res >= 0)
	return res;

      ASSERT_HGCD (hgcd, ap, asize, bp, bsize, 0, 2);
    }

  ASSERT (hgcd->row[0].rsize >= hgcd->row[1].rsize);
  ASSERT (hgcd->row[0].rsize <= M + m + 1);
  ASSERT (hgcd->row[1].rsize > M);

  /* Second phase, reduce size until we have one number of size > M
     and one of size <= M+1 */
  while (hgcd->row[1].rsize > M + 1)
    {
      mp_size_t k = 2*M - hgcd->row[0].rsize;
      mp_size_t n1 = hgcd->row[0].rsize - k;
      int res;

      ASSERT (k + (n1 + 1)/2 == M);
      ASSERT (n1 >= 2);

      ASSERT (n1 <= 2*(m + 1));
      ASSERT (n1 <= n + 3);

      res = mpn_hgcd (&R,
		      hgcd->row[0].rp + k, hgcd->row[0].rsize - k,
		      hgcd->row[1].rp + k, hgcd->row[1].rsize - k,
		      quotients, tp, talloc);

      if (!res)
	{
	  /* The first remainder was small. Then there's a good chance
	     that the remainder A % B is also small. */

	  res = hgcd_case0 (hgcd, M, quotients,
			    tp, talloc);

	  if (res > 0)
	    ASSERT_HGCD (hgcd, ap, asize, bp, bsize, 0, 4);
	  if (res >= 0)
	    return res;

	  ASSERT_HGCD (hgcd, ap, asize, bp, bsize, 0, 2);
	  continue;
	}

      /* Now r0 and r1 are always correct. */

      /* Store new values in rows 2 and 3, to avoid overlap */
      hgcd->row[2].rsize
	= mpn_hgcd_fix (k, hgcd->row[2].rp, hgcd->row[0].rsize + 1,
			R.row[0].rp, R.row[0].rsize,
			R.sign,
			R.row[0].uvp[0], hgcd->row[0].rp,
			R.row[0].uvp[1], hgcd->row[1].rp,
			R.size,
			tp, talloc);

      hgcd->row[3].rsize
	= mpn_hgcd_fix (k, hgcd->row[3].rp, hgcd->row[1].rsize + 1,
			R.row[1].rp, R.row[1].rsize,
			~R.sign,
			R.row[1].uvp[0], hgcd->row[0].rp,
			R.row[1].uvp[1], hgcd->row[1].rp,
			R.size,
			tp, talloc);

      ASSERT (hgcd->row[2].rsize > M);
      ASSERT (hgcd->row[3].rsize > k);

      hgcd->size = hgcd_mul (hgcd->row+2, hgcd->alloc,
			     R.row, R.size, hgcd->row, hgcd->size,
			     tp, talloc);
      hgcd->sign ^= R.sign;

      ASSERT_HGCD (hgcd, ap, asize, bp, bsize, 2, 4);

      if (hgcd->row[3].rsize <= M)
	{
	  /* Backup two steps */

	  /* We don't use R.row[2] and R.row[3], so drop the
	     corresponding quotients. */
	  qstack_drop (quotients);
	  qstack_drop (quotients);

	  return hgcd_small_2 (hgcd, M, quotients, tp, talloc);
	}

      HGCD_SWAP4_2 (hgcd->row);

      /* It's crucial that we drop quotients that we don't use, and
	 it gets a little complicated because we don't use the top
	 quotient first. */

      if (res >= 3)
	{
	  /* We already know the correct q */
	  mp_size_t qsize;
	  mp_srcptr qp;

	  qsize = qstack_get_1 (quotients, &qp);

	  ASSERT (qsize + hgcd->size <= hgcd->alloc);
	  hgcd_update_r (hgcd->row, qp, qsize);
	  hgcd->size = hgcd_update_uv (hgcd->row, hgcd->size,
				       qp, qsize);
	  ASSERT (hgcd->size < hgcd->alloc);

	  ASSERT (hgcd->row[2].rsize > k);
	  if (hgcd->row[2].rsize <= M)
	    {
	      /* Discard r3 */
	      qstack_drop (quotients);
	      return hgcd_small_1 (hgcd, M, quotients,
				   tp, talloc);
	    }
	  if (res == 3)
	    /* Drop quotient for r3 */
	    qstack_drop (quotients);
	}
      else
	{
	  /* Discard r2 and r3, compute new r2 */
	  qstack_drop (quotients);
	  qstack_drop (quotients);

	  hgcd->size = euclid_step (hgcd->row, hgcd->size,
				    quotients, hgcd->alloc);
	  ASSERT (hgcd->size < hgcd->alloc);

	  if (hgcd->row[2].rsize <= M)
	    return hgcd_small_1 (hgcd, M, quotients,
				 tp, talloc);
	}

      ASSERT (hgcd->row[2].rsize > M);

      if (res == 4)
	{
	  /* We already know the correct q */
	  mp_size_t qsize;
	  mp_srcptr qp;

	  qsize = qstack_get_0 (quotients, &qp);

	  ASSERT (qsize + hgcd->size <= hgcd->alloc);
	  hgcd_update_r (hgcd->row + 1, qp, qsize);
	  hgcd->size = hgcd_update_uv (hgcd->row + 1, hgcd->size,
				       qp, qsize);
	  ASSERT (hgcd->size < hgcd->alloc);
	  ASSERT (hgcd->row[3].rsize <= M + 1);
	}
      else
	/* At this point, we have already dropped the old quotient. */
	hgcd->size = euclid_step (hgcd->row + 1, hgcd->size,
				  quotients, hgcd->alloc);

      ASSERT (hgcd->size < hgcd->alloc);

      if (hgcd->row[3].rsize <= M)
	{
#if WANT_ASSERT
	  qstack_rotate (quotients, 0);
#endif
	  ASSERT_HGCD (hgcd, ap, asize, bp, bsize, 0, 4);
	  return hgcd_jebelean (hgcd, M, tp, talloc);
	}

      HGCD_SWAP4_2 (hgcd->row);
    }

  ASSERT_HGCD (hgcd, ap, asize, bp, bsize, 0, 2);

  return hgcd_final (hgcd, M, quotients, tp, talloc);
}

int
mpn_hgcd_equal (const struct hgcd *A, const struct hgcd *B)
{
  unsigned i;

  if (A->sign != B->sign)
    return 0;

  if (A->size != B->size)
    return 0;

  for (i = 0; i < 4; i++)
    {
      unsigned j;

      if (!MPN_EQUAL_P (A->row[i].rp, A->row[i].rsize,
			B->row[i].rp, B->row[i].rsize))
	return 0;

      for (j = 0; j < 2; j++)
	if (mpn_cmp (A->row[i].uvp[j],
		     B->row[i].uvp[j], A->size) != 0)
	  return 0;
    }

  return 1;
}
