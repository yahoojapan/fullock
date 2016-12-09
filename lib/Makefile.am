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

## AUTOMAKE_OPTIONS =

pkginclude_HEADERS = flckcommon.h flckstructure.h fullock.h flckshm.h flcklocktype.h flckpidcache.h flcklistfilelock.h flcklistlocker.h flcklistnmtx.h flcklistofflock.h flcklistncond.h flcklistwaiter.h flckthread.h flckutil.h flckdbg.h rwlockrcsv.h flckbaselist.tcc
pkgincludedir = $(includedir)/fullock

EXTRA_DIST = 

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libfullock.pc
DISTCLEANFILES = $(pkgconfig_DATA)

lib_LTLIBRARIES = libfullock.la
libfullock_la_SOURCES = fullock.cc flckshm.cc flckshmdump.cc flckshminit.cc flcklistfilelock.cc flcklistlocker.cc flcklistnmtx.cc flcklistofflock.cc flcklistncond.cc flcklistwaiter.cc flckthread.cc flckutil.cc flckdbg.cc rwlockrcsv.cc fullockversion.cc
libfullock_la_LDFLAGS = -version-info $(LIB_VERSION_INFO)
libfullock_la_LIBADD = -lrt -lpthread

ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = 
AM_CPPFLAGS = 

### version(commit hash)
.PHONY: fullockversion

fullockversion.cc: fullockversion
	@../make_rev.sh fullockversion.cc fullock_commit_hash

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
