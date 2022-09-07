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

SUBDIRS=lib docs tests buildutils

EXTRA_DIST=RELEASE_VERSION @CONFIGURECUSTOM@

CPPCHECK_CMD=	cppcheck
CPPCHECK_OPT=	--quiet \
				--error-exitcode=1 \
				--inline-suppr \
				-j 4 \
				--std=c++03 \
				--xml \
				--enable=warning,style,information,missingInclude
CPPCHECK_IGN=

cppcheck:
	$(CPPCHECK_CMD) $(CPPCHECK_OPT) $(CPPCHECK_IGN) $(SUBDIRS)

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
