#
# FULLOCK - Fast User Level LOCK library
#
# Copyright 2015 Yahoo Japan Corporation.
#
# FULLOCK is fast locking library on user level by Yahoo! JAPAN.
# FULLOCK is following specifications.
#
# For the full copyright and license information, please view
# the license file that was distributed with this source code.
#
# AUTHOR:   Takeshi Nakatani
# CREATE:   Wed 13 May 2015
# REVISION:
#

noinst_PROGRAMS = fullocktest singletest mttest mptest cond_mttest cond_mptest fcntl_mttest fcntl_mptest forktest

fullocktest_SOURCES = fullocktest.cc
fullocktest_LDADD = -L../lib/.libs -lfullock -lpthread

singletest_SOURCES = singletest.cc
singletest_LDADD = -L../lib/.libs -lfullock

mttest_SOURCES = mttest.cc
mttest_LDADD = -L../lib/.libs -lfullock -lpthread

mptest_SOURCES = mptest.cc
mptest_LDADD = -L../lib/.libs -lfullock

cond_mttest_SOURCES = cond_mttest.cc
cond_mttest_LDADD = -L../lib/.libs -lfullock -lpthread

cond_mptest_SOURCES = cond_mptest.cc
cond_mptest_LDADD = -L../lib/.libs -lfullock

fcntl_mttest_SOURCES = fcntl_mttest.cc
fcntl_mttest_LDADD = -L../lib/.libs -lfullock -lpthread

fcntl_mptest_SOURCES = fcntl_mptest.cc
fcntl_mptest_LDADD = -L../lib/.libs -lfullock

forktest_SOURCES = forktest.cc
forktest_LDADD = 

ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -I$(top_srcdir)/lib
AM_CPPFLAGS = -I$(top_srcdir)/lib

TESTS = test.sh

EXTRA_DIST = \
test.sh 

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
