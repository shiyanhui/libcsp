# Copyright (c) 2020, Yanhui Shi <lime.syh at gmail dot com>
# All rights reserved.
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

AC_PREREQ([2.69])

AC_INIT([libcsp], [m4_esyscmd_s([cat VERSION])],
        [https://github.com/libcsp/issues], [libcsp], [https://libcsp.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

AC_DISABLE_STATIC

AC_CONFIG_SRCDIR([src/csp.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_MACRO_DIRS([m4])

CFLAGS="-D_GNU_SOURCE"
CXXFLAGS="-O3"

AC_ARG_ENABLE([debug], [AS_HELP_STRING([--enable-debug], [enable the debug mode])])
AS_IF([test "x$enable_debug" == xyes], [CFLAGS+=" -g -O0 -fomit-frame-pointer"], [CFLAGS+=" -O3"])

AC_ARG_ENABLE([valgrind], [AS_HELP_STRING([--enable-valgrind], [debug with valgrind])])
AS_IF([test "x$enable_valgrind" == xyes], [AC_DEFINE([csp_enable_valgrind], [], [enable valgrind])], [])

AC_ARG_WITH([sysmalloc], [AS_HELP_STRING([--with-sysmalloc], [use system malloc])])
AS_IF([test "x$with_sysmalloc" == xyes], [AC_DEFINE([csp_with_sysmalloc], [], [use system malloc])], [])

AC_PROG_CXX([g++])
AC_PROG_CC([gcc])
AC_PROG_CC_STDC
AC_PROG_AWK
AC_PROG_SED
AC_PROG_LN_S
AC_PROG_INSTALL
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

AC_SUBST([WORKING_DIR], [/tmp/libcsp])
AC_CHECK_FILE(
  [$WORKING_DIR],
  [rm -rf $WORKING_DIR/*.sf $WORKING_DIR/*.cg $WORKING_DIR/.session],
  [$MKDIR_P $WORKING_DIR]
)

LT_INIT

AC_OUTPUT
