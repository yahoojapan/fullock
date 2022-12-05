#!/bin/sh
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

#--------------------------------------------------------------
# Common Variables
#--------------------------------------------------------------
#
# Instead of pipefail(for shells not support "set -o pipefail")
#
PIPEFAILURE_FILE="/tmp/.pipefailure.$(od -An -tu4 -N4 /dev/random | tr -d ' \n')"

#PRGNAME=$(basename "${0}")
SCRIPTDIR=$(dirname "${0}")
SCRIPTDIR=$(cd "${SCRIPTDIR}" || exit 1; pwd)
SRCTOP=$(cd "${SCRIPTDIR}/.." || exit 1; pwd)

#
# Directories
#
TESTDIR="${SRCTOP}/tests"
LIBOBJDIR="${SRCTOP}/lib/.libs"
#TESTOBJDIR="${TESTDIR}/.libs"

#
# LD_LIBRARY_PATH
#
LD_LIBRARY_PATH="${LIBOBJDIR}"
export LD_LIBRARY_PATH

#--------------------------------------------------------------
# Variables
#--------------------------------------------------------------
DATE=$(date)
PROCID=$$

TEST_FILEPATH="/tmp/fullock_test_${PROCID}"
MUTEXNAME="fullock_test_mutex"
CONDNAME="fullock_test_cond"

#--------------------------------------------------------------
# Input Variables
#--------------------------------------------------------------
if [ -z "$1" ]; then
#	LOGFILE="/dev/null"
	LOGFILE="/tmp/fullock_test_${PROCID}.log"
else
	LOGFILE="$1"
fi

#==============================================================
# Main
#==============================================================
{
	echo "================= $DATE ===================="

	#----------------------------------------------------------
	# Make test file for rwlock
	#----------------------------------------------------------
	echo "test" > "${TEST_FILEPATH}"

	#----------------------------------------------------------
	# Simple test for rwlock reader
	#----------------------------------------------------------
	echo "[TEST] Simple test for rwlock reader"

	if ({ "${TESTDIR}"/singletest -f "${TEST_FILEPATH}" -read || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "       Result : ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Simple test for rwlock writer
	#----------------------------------------------------------
	echo "[TEST] Simple test for rwlock writer"

	if ({ "${TESTDIR}"/singletest -f "${TEST_FILEPATH}" -write || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Simple test for mutex
	#----------------------------------------------------------
	echo "[TEST] Simple test for mutex"

	if ({ "${TESTDIR}"/singletest -m "${MUTEXNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Simple test for cond
	#----------------------------------------------------------
	echo "[TEST] Simple test for cond"

	if ({ "${TESTDIR}"/singletest -c "${CONDNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Multi thread test for rwlock
	#----------------------------------------------------------
	echo "[TEST] Multi thread test for rwlock"

	if ({ "${TESTDIR}"/mttest -f "${TEST_FILEPATH}" -r 5 -w 5 -l 10 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Multi thread test for mutex
	#----------------------------------------------------------
	echo "[TEST] Multi thread test for mutex"

	if ({ "${TESTDIR}"/mttest -m "${MUTEXNAME}" -t 5 -l 10 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Multi thread test for cond
	#----------------------------------------------------------
	#
	# signal
	#
	echo "[TEST] Multi thread test for cond(signal)"

	if ({ "${TESTDIR}"/cond_mttest -s -w 10 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#
	# broadcast
	#
	echo "[TEST] Multi thread test for cond(broadcast)"

	if ({ "${TESTDIR}"/cond_mttest -b -w 10 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Multi process test for rwlock
	#----------------------------------------------------------
	echo "[TEST] Multi process test for rwlock"

	if ({ "${TESTDIR}"/mptest -f "${TEST_FILEPATH}" -r 5 -w 5 -l 10 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Multi process test for mutex
	#----------------------------------------------------------
	echo "[TEST] Multi process test for mutex"

	if ({ "${TESTDIR}"/mptest -m "${MUTEXNAME}" -t 5 -l 10 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Multi process test for cond
	#----------------------------------------------------------
	#
	# signal
	#
	echo "[TEST] Multi process test for cond(signal)"

	if ({ "${TESTDIR}"/cond_mptest -s -w 10 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#
	# broadcast
	#
	echo "[TEST] Multi process test for cond(broadcast)"

	if ({ "${TESTDIR}"/cond_mptest -b -w 10 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Environment test
	#----------------------------------------------------------
	echo "[TEST] Environment test"

	if ({ "${TESTDIR}"/fullocktest -env || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Over area count limit test for rwlock(unit no)
	#----------------------------------------------------------
	echo "[TEST] Over area count limit test for rwlock(unit no)"

	if ({ "${TESTDIR}"/fullocktest -oac -unit no || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Over area count limit test for rwlock(unit fd)
	#----------------------------------------------------------
	echo "[TEST] Over area count limit test for rwlock(unit fd)"

	if ({ "${TESTDIR}"/fullocktest -oac -unit fd || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Over area count limit test for rwlock(unit offset)
	#----------------------------------------------------------
	echo "[TEST] Over area count limit test for rwlock(unit offset)"

	if ({ "${TESTDIR}"/fullocktest -oac -unit offset || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Deadlock test for rwlock
	#----------------------------------------------------------
	echo "[TEST] Deadlock test for rwlock"

	if ({ "${TESTDIR}"/fullocktest -dl || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Automatic recover deadlock thread test for rwlock(robust no)
	#----------------------------------------------------------
	#
	#	**** YOU NEED TO CHECK MANUALLY ****
	#
	#	echo "[TEST] Automatic recover deadlock thread test for rwlock(robust no)"
	#
	#	if ({ "${TESTDIR}"/fullocktest -ar -thread -robust no || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	#		echo "    [Result] ERROR"
	#		exit 1
	#	fi
	#	echo "    [Result] OK"
	#	echo ""

	#----------------------------------------------------------
	# Automatic recover deadlock thread test for rwlock(robust low)
	#----------------------------------------------------------
	#
	#
	#	**** YOU NEED TO CHECK MANUALLY ****
	#
	#	echo "[TEST] Automatic recover deadlock thread test for rwlock(robust low)"
	#
	#	if ({ "${TESTDIR}"/fullocktest -ar -thread -robust low || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	#		echo "    [Result] ERROR"
	#		exit 1
	#	fi
	#	echo "    [Result] OK"
	#	echo ""

	#----------------------------------------------------------
	# Automatic recover deadlock thread test for rwlock(robust high)
	#----------------------------------------------------------
	echo "[TEST] Automatic recover deadlock thread test for rwlock(robust high)"

	if ({ "${TESTDIR}"/fullocktest -ar -thread -robust high || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Automatic recover deadlock process test for rwlock(robust no)
	#----------------------------------------------------------
	echo "[TEST] Automatic recover deadlock process test for rwlock(robust no)"

	if ({ "${TESTDIR}"/fullocktest -ar -process -robust no || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Automatic recover deadlock process test for rwlock(robust low)
	#----------------------------------------------------------
	echo "[TEST] Automatic recover deadlock process test for rwlock(robust low)"

	if ({ "${TESTDIR}"/fullocktest -ar -process -robust low || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Automatic recover deadlock process test for rwlock(robust high)
	#----------------------------------------------------------
	echo "[TEST] Automatic recover deadlock process test for rwlock(robust high)"

	if ({ "${TESTDIR}"/fullocktest -ar -process -robust high || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Over area count limit test for mutex
	#----------------------------------------------------------
	echo "[TEST] Over area count limit test for mutex"

	if ({ "${TESTDIR}"/fullocktest -moac || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Deadlock test for mutex
	#----------------------------------------------------------
	echo "[TEST] Deadlock test for mutex"

	if ({ "${TESTDIR}"/fullocktest -mdl || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Automatic recover deadlock thread test for mutex(robust no)
	#----------------------------------------------------------
	#
	#	**** YOU NEED TO CHECK MANUALLY ****
	#
	#	echo "[TEST] Automatic recover deadlock thread test for mutex(robust no)"
	#
	#	if ({ "${TESTDIR}"/fullocktest -mar -thread -robust no || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	#		echo "    [Result] ERROR"
	#		exit 1
	#	fi
	#	echo "    [Result] OK"
	#	echo ""

	#----------------------------------------------------------
	# Automatic recover deadlock thread test for mutex(robust low)
	#----------------------------------------------------------
	#
	#	**** YOU NEED TO CHECK MANUALLY ****
	#
	#	echo "[TEST] Automatic recover deadlock thread test for mutex(robust low)"
	#
	#	if ({ "${TESTDIR}"/fullocktest -mar -thread -robust low || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	#		echo "    [Result] ERROR"
	#		exit 1
	#	fi
	#	echo "    [Result] OK"
	#	echo ""

	#----------------------------------------------------------
	# Automatic recover deadlock thread test for mutex(robust high)
	#----------------------------------------------------------
	echo "[TEST] Automatic recover deadlock thread test for mutex(robust high)"

	if ({ "${TESTDIR}"/fullocktest -mar -thread -robust high || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Automatic recover deadlock process test for mutex(robust no)
	#----------------------------------------------------------
	echo "[TEST] Automatic recover deadlock process test for mutex(robust no)"

	if ({ "${TESTDIR}"/fullocktest -mar -process -robust no || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Automatic recover deadlock process test for mutex(robust low)
	#----------------------------------------------------------
	echo "[TEST] Automatic recover deadlock process test for mutex(robust low)"

	if ({ "${TESTDIR}"/fullocktest -mar -process -robust low || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Automatic recover deadlock process test for mutex(robust high)
	#----------------------------------------------------------
	echo "[TEST] Automatic recover deadlock process test for mutex(robust high)"

	if ({ "${TESTDIR}"/fullocktest -mar -process -robust high || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Over area count limit test for cond
	#----------------------------------------------------------
	echo "[TEST] Over area count limit test for cond"

	if ({ "${TESTDIR}"/fullocktest -coac || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Check and Kill sub processes if these are running.
	#----------------------------------------------------------
	REST_PROCESSES=$(pgrep fullocktest | tr '\n' ' ')
	if [ -z "${REST_PROCESSES}" ]; then
		/bin/sh -c "kill -HUP ${REST_PROCESSES}" 2>/dev/null
		sleep 1

		REST_PROCESSES=$(pgrep fullocktest | tr '\n' ' ')
		if [ -z "${REST_PROCESSES}" ]; then
			/bin/sh -c "kill -KILL ${REST_PROCESSES}" 2>/dev/null
		fi
	fi

	#----------------------------------------------------------
	# Test forking
	#----------------------------------------------------------
	echo "[TEST] Test forking"

	if ({ "${TESTDIR}"/forktest || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"
	echo ""

	#----------------------------------------------------------
	# Remove file
	#----------------------------------------------------------
	rm -f "${TEST_FILEPATH}"

	echo "[SUMMARY] All test is succeed"

} | tee "${LOGFILE}"

exit 0

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
