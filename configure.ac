#
# FULLOCK - Fast User Level LOCK library
#
# Utility tools for building configure/packages by AntPickax
#
# Copyright 2018 Yahoo Japan Corporation.
#
# AntPickax provides utility tools for supporting autotools
# builds.
#
# These tools retrieve the necessary information from the
# repository and appropriately set the setting values of
# configure, Makefile, spec,etc file and so on.
# These tools were recreated to reduce the number of fixes and
# reduce the workload of developers when there is a change in
# the project configuration.
# 
# For the full copyright and license information, please view
# the license file that was distributed with this source code.
#
# AUTHOR:   Takeshi Nakatani
# CREATE:   Fri, Apr 13 2018
# REVISION:
#

#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
AC_PREREQ([2.63])
AC_INIT(fullock, m4_esyscmd([tr -d '\n' < $(pwd)/RELEASE_VERSION]))
AM_INIT_AUTOMAKE()
LT_INIT()

AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

#
# Checks for programs.
#
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB

#
# Checks for libraries.
#
AC_CHECK_LIB(dl, dlopen)

#
# Checks for header files.
#
AC_CHECK_HEADERS([stdlib.h stdio.h string.h fcntl.h limits.h inttypes.h stdint.h strings.h unistd.h])

#
# Checks for typedefs, structures, and compiler characteristics.
#
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT64_T
AC_TYPE_UINT32_T
AC_TYPE_MODE_T

#
# Checks for library functions.
#
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_FORK
AC_CHECK_FUNCS([memset strcasecmp clock_gettime ftruncate mkdir munmap setenv strdup strtol])

AC_CONFIG_MACRO_DIR([m4])

#
# Load customizable variables
#
AC_CHECK_FILE([configure.custom],
	[
		configure_custom_file="configure.custom"
		custom_git_domain="$(grep '^\s*GIT_DOMAIN\s*=' configure.custom | sed -e 's|^\s*GIT_DOMAIN\s*=\s*||g' -e 's|^\s*||g' -e 's|\s*$||g')"
		custom_git_org="$(grep '^\s*GIT_ORG\s*=' configure.custom | sed -e 's|^\s*GIT_ORG\s*=\s*||g' -e 's|^\s*||g' -e 's|\s*$||g')"
		custom_git_repo="$(grep '^\s*GIT_REPO\s*=' configure.custom | sed -e 's|^\s*GIT_REPO\s*=\s*||g' -e 's|^\s*||g' -e 's|\s*$||g')"
		custom_git_endpoint="$(grep '^\s*GIT_EP_V3_REPO\s*=' configure.custom | sed -e 's|^\s*GIT_EP_V3_REPO\s*=\s*||g' -e 's|^\s*||g' -e 's|\s*$||g')"
		custom_dev_email="$(grep '^\s*DEV_EMAIL\s*=' configure.custom | sed -e 's|^\s*DEV_EMAIL\s*=\s*||g' -e 's|^\s*||g' -e 's|\s*$||g')"
		custom_dev_name="$(grep '^\s*DEB_NAME\s*=' configure.custom | sed -e 's|^\s*DEB_NAME\s*=\s*||g' -e 's|^\s*||g' -e 's|\s*$||g')"
	],
	[
		configure_custom_file=""
		custom_git_domain="github.com"
		custom_git_org="yahoojapan"
		custom_git_repo="fullock"
		custom_git_endpoint="https://api.github.com/repos/"
		custom_dev_email="antpickax-support@mail.yahoo.co.jp"
		custom_dev_name="FULLOCK_DEVELOPER"
	]
)

#
# Symbols for buildutils
#
AC_SUBST([CONFIGURECUSTOM], "${configure_custom_file}")
AC_SUBST([GIT_DOMAIN], "${custom_git_domain}")
AC_SUBST([GIT_ORG], "${custom_git_org}")
AC_SUBST([GIT_REPO], "${custom_git_repo}")
AC_SUBST([CURRENTREV], "$($(pwd)/buildutils/make_commit_hash.sh -o "${custom_git_org}" -r "${custom_git_repo}" -ep "${custom_git_endpoint}" -s)")
AC_SUBST([DEV_EMAIL], "$(echo ${DEBEMAIL:-"${custom_dev_email}"})")
AC_SUBST([DEV_NAME], "$(echo ${DEBFULLNAME:-"${custom_dev_name}"})")
#
AC_SUBST([RPMCHANGELOG], "`$(pwd)/buildutils/make_rpm_changelog.sh $(pwd)/ChangeLog`")
AC_SUBST([SHORTDESC], "`$(pwd)/buildutils/make_description.sh $(pwd)/docs/fullock.3 -short`")
AC_SUBST([LONGDESC], "`$(pwd)/buildutils/make_description.sh $(pwd)/docs/fullock.3 -long`")
AC_SUBST([ESCLONGDESC], "`$(pwd)/buildutils/make_description.sh $(pwd)/docs/fullock.3 -esclong`")
AC_SUBST([DEBLONGDESC], "`$(pwd)/buildutils/make_description.sh $(pwd)/docs/fullock.3 -deblong`")
AC_SUBST([PKG_VERSION], "`$(pwd)/buildutils/make_variables.sh -pkg_version`")
AC_SUBST([LIB_VERSION_INFO], "`$(pwd)/buildutils/make_variables.sh -lib_version_info`")
AC_SUBST([LIB_VERSION_LINK], "`$(pwd)/buildutils/make_variables.sh -lib_version_for_link`")
AC_SUBST([LIB_MAJOR_NUMBER], "`$(pwd)/buildutils/make_variables.sh -major_number`")
AC_SUBST([PKGLICENSE], "`grep License COPYING | sed 's/ License//g'`")
AC_SUBST([DEBCOPYING], "`tail -n +5 COPYING | sed 's/^$/./g' | sed 's/^/ /g'`")
AC_SUBST([DEBHELPER_DEP], "`$(pwd)/buildutils/make_variables.sh -debhelper_dep`")
AC_SUBST([RPMPKG_GROUP], ["`$(pwd)/buildutils/make_variables.sh -rpmpkg_group | sed 's#NEEDRPMGROUP#Group: Applications/Databases#g'`"])
AC_SUBST([CONFIGUREWITHOPT], "")
AM_SUBST_NOTMAKE([CURRENTREV])
AM_SUBST_NOTMAKE([RPMCHANGELOG])
AM_SUBST_NOTMAKE([SHORTDESC])
AM_SUBST_NOTMAKE([LONGDESC])
AM_SUBST_NOTMAKE([ESCLONGDESC])
AM_SUBST_NOTMAKE([DEBLONGDESC])
AM_SUBST_NOTMAKE([DEBCOPYING])
AM_SUBST_NOTMAKE([DEBHELPER_DEP])
AM_SUBST_NOTMAKE([RPMPKG_GROUP])
AM_SUBST_NOTMAKE([CONFIGUREWITHOPT])

#
# CFLAGS/CXXFLAGS
#
CFLAGS="-Wall $CFLAGS"
CXXFLAGS="-Wall $CXXFLAGS"

#
# Checking Libraries
#
check_depend_libs=1
AC_MSG_CHECKING([for checking dependent libraries])
AC_ARG_ENABLE(check-depend-libs,
    [AS_HELP_STRING([--disable-check-depend-libs], [disable checking dependent libraries])],
	[case "${enableval}" in
		yes)	check_depend_libs=1;;
		*)		check_depend_libs=0;;
	esac]
)
AS_IF([test ${check_depend_libs} = 1], [AC_MSG_RESULT(yes)], [AC_MSG_RESULT(no)])
#
# [NOTE] fullock is not dependent on other libraries.
#

#
# Define for debugging messages
#
flcklib_debug=1
AC_MSG_CHECKING([for debugging message for fullock developer])
AC_ARG_ENABLE(flcklib-debug,
	[AS_HELP_STRING([--disable-flcklib-debug], [disable debugging message for fullock developer])],
	[case "${enableval}" in
		yes)	flcklib_debug=1;;
		*)		flcklib_debug=0;;
	esac]
)
AS_IF([test ${flcklib_debug} = 1], [AC_MSG_RESULT(yes)], [AC_MSG_RESULT(no)])
AS_IF([test ${flcklib_debug} = 0],
	[
		CFLAGS="-DFLCK_DEBUG_MSG_NO $CFLAGS"
		CXXFLAGS="-DFLCK_DEBUG_MSG_NO $CXXFLAGS"
	]
)

#
# Config files
#
AC_CONFIG_FILES([Makefile
		docs/Makefile
		lib/Makefile
		lib/libfullock.pc
		tests/Makefile
		buildutils/Makefile
		buildutils/control
		buildutils/copyright
		buildutils/libfullock.spec
		buildutils/Dockerfile.templ])

AC_OUTPUT

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
