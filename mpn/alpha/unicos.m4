divert(-1)

dnl  m4 macros for alpha assembler on unicos.


dnl  Copyright 2000 Free Software Foundation, Inc.
dnl 
dnl  This file is part of the GNU MP Library.
dnl
dnl  The GNU MP Library is free software; you can redistribute it and/or
dnl  modify it under the terms of the GNU Lesser General Public License as
dnl  published by the Free Software Foundation; either version 2.1 of the
dnl  License, or (at your option) any later version.
dnl
dnl  The GNU MP Library is distributed in the hope that it will be useful,
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl  Lesser General Public License for more details.
dnl
dnl  You should have received a copy of the GNU Lesser General Public
dnl  License along with the GNU MP Library; see the file COPYING.LIB.  If
dnl  not, write to the Free Software Foundation, Inc., 59 Temple Place -
dnl  Suite 330, Boston, MA 02111-1307, USA.


dnl  Note that none of the standard GMP_ASM_ autoconf tests are done for
dnl  unicos, so none of the config.m4 results can be used here.


define(`ASM_START',
m4_assert_numargs(0)
`	.ident	dummy')

define(`X',
m4_assert_numargs(1)
`^X$1')

define(`FLOAT64',
m4_assert_numargs(2)
`	.psect	$1@crud,data
$1:	.t_floating $2
	.endp')

define(`PROLOGUE',
m4_assert_numargs(1)
`	.stack	192		; What does this mean?  Only Cray knows.
	.psect	$1@code,code,cache
$1::')

define(`PROLOGUE_GP',
m4_assert_numargs(1)
`PROLOGUE($1)')

define(`EPILOGUE',
m4_assert_numargs(1)
`	.endp')

define(`DATASTART',
m4_assert_numargs(1)
`	.psect	$1@crud,data
$1:')

define(`DATAEND',
m4_assert_numargs(0)
`	.endp')

define(`ASM_END',
m4_assert_numargs(0)
`	.end')

dnl  Unicos assembler lacks unop
define(`unop',
m4_assert_numargs(-1)
`bis r31,r31,r31')

define(`cvttqc',
m4_assert_numargs(-1)
`cvttq/c')

dnl  Unicos assembler seems to align using garbage, so disable aligning
define(`ALIGN',
m4_assert_numargs(1)
)

divert

