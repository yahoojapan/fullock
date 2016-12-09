#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# FULLOCK - Fast User Level LOCK library by Yahoo! JAPAN
#
# Copyright 2015 Yahoo! JAPAN corporation.
#
# FULLOCK is fast locking library on user level by Yahoo! JAPAN.
# FULLOCK is following specifications.
#
# For the full copyright and license information, please view
# the LICENSE file that was distributed with this source code.
#
# AUTHOR:   Takeshi Nakatani
# CREATE:   Wed 13 May 2015
# REVISION:
#

AC_PREREQ([2.63])
AC_INIT(fullock, m4_esyscmd([tr -d '\n' < $(pwd)/RELEASE_VERSION]))
AM_INIT_AUTOMAKE()
LT_INIT()

AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_LIBTOOL

# Checks for libraries.
AC_CHECK_LIB(dl, dlopen)

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h stdio.h string.h fcntl.h limits.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT64_T
AC_TYPE_MODE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_FORK
AC_CHECK_FUNCS([memset strcasecmp clock_gettime ftruncate mkdir munmap setenv strdup strtol])

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_FILES([Makefile
		docs/Makefile
		lib/Makefile
		lib/libfullock.pc
		tests/Makefile])

# CFLAGS/CXXFLAGS
CFLAGS="-Wall $CFLAGS"
CXXFLAGS="-Wall $CXXFLAGS"

# LIB_VERSION_INFO for libtool library version info
LIB_VERSION_INFO=`$(pwd)/make_valiables.sh -lib_version_info`
AC_SUBST([LIB_VERSION_INFO])

#
# Define for debugging messages
#
AC_ARG_ENABLE(flcklib-debug,
	[  --disable-flcklib-debug disable debugging message for fullock developer],
	[case "${enableval}" in
		no)		CFLAGS="-DFLCK_DEBUG_MSG_NO $CFLAGS"
				CXXFLAGS="-DFLCK_DEBUG_MSG_NO $CXXFLAGS"
		;;
	esac]
)

AC_OUTPUT

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#

