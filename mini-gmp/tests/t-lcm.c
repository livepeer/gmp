/*

Copyright 2012, Free Software Foundation, Inc.

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

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

#include "mini-random.h"

#define MAXBITS 400
#define COUNT 10000

static void
dump (const char *label, const mpz_t x)
{
  char *buf = mpz_get_str (NULL, 16, x);
  fprintf (stderr, "%s: %s\n", label, buf);
  free (buf);
}

int
main (int argc, char **argv)
{
  unsigned i;
  mpz_t a, b, g, s;

  hex_random_init ();

  mpz_init (a);
  mpz_init (b);
  mpz_init (g);
  mpz_init (s);

  for (i = 0; i < COUNT; i++)
    {
      mini_random_op (OP_LCM, MAXBITS, a, b, s);
      mpz_lcm (g, a, b);
      if (mpz_cmp (g, s))
	{
	  fprintf (stderr, "mpz_lcm failed:\n");
	  dump ("a", a);
	  dump ("b", b);
	  dump ("r", g);
	  dump ("ref", s);
	  abort ();
	}
      if (mpz_fits_ulong_p (b))
	{
	  mpz_set_si (g, 0);
	  mpz_lcm_ui (g, a, mpz_get_ui (b));
	  if (mpz_cmp (g, s))
	    {
	      fprintf (stderr, "mpz_lcm_ui failed:\n");
	      dump ("a", a);
	      dump ("b", b);
	      dump ("r", g);
	      dump ("ref", s);
	      abort ();
	    }
	}
    }

  mpz_clear (a);
  mpz_clear (b);
  mpz_clear (g);
  mpz_clear (s);

  return 0;
}
