#!/bin/sh
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

#
# This script puts git commit hash string to C header file.
# ex: static const char version[] = "....";
#
# Usage: make_rev.sh <filename> <valuename>
#

PRGNAME=`basename $0`

if [ $# -ne 2 ]; then
	echo "${PRGNAME}: Error - parameter is not found."
	exit 1
fi
FILEPATH=$1
VALUENAME=$2

REVISION=`git rev-parse --short HEAD`
if [ $? -ne 0 ]; then
	echo "${PRGNAME}: Warning - git commit hash code is not found, so set to \"unknown\"."
	REVISION="unknown"
fi
#echo ${REVISION}

NEWCODES="char ${VALUENAME}[] = \"${REVISION}\";"
#echo ${NEWCODES}

if [ -f ${FILEPATH} ]; then
	FILECODES=`cat ${FILEPATH}`
	#echo ${FILECODES}

	if [ "X${FILECODES}" = "X${NEWCODES}" ]; then
		#echo "${PRGNAME}: ${FILEPATH} is not updated."
		exit 0
	fi
fi

echo ${NEWCODES} > ${FILEPATH}
echo "${PRGNAME}: ${FILEPATH} is updated."

exit 0

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
