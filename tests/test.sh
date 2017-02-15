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

##############################################################
## library path & programs path
##
if [ "X${SRCTOP}" = "X" ]; then
	MYSCRIPTDIR=`dirname $0`
	SRCTOP=`cd ${MYSCRIPTDIR}/..; pwd`
fi
if [ "X${OBJDIR}" = "X" ]; then
	LD_LIBRARY_PATH="${SRCTOP}/lib/.libs"
	TESTPROGDIR=${MYSCRIPTDIR}
else
	LD_LIBRARY_PATH="${SRCTOP}/lib/${OBJDIR}"
	TESTPROGDIR=${MYSCRIPTDIR}/${OBJDIR}
fi
export LD_LIBRARY_PATH

##############################################################
## variables
##
DATE=`date`
PROCID=$$
FILEPATH="/tmp/fullock_test_$PROCID"
MUTEXNAME="fullock_test_mutex"
CONDNAME="fullock_test_cond"

if [ "X$1" = "X" ]; then
#	LOGFILE="/dev/null"
	LOGFILE="/tmp/fullock_test_$PROCID.log"
else
	LOGFILE="$1"
fi

echo "================= $DATE ====================" > $LOGFILE

##############################################################
### Make test file for rwlock
###
echo "test" > $FILEPATH

##############################################################
### Simple test for rwlock reader
###
echo "-- Simple test for rwlock reader --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/singletest -f $FILEPATH -read >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Simple test for rwlock reader --->> ERROR"
	echo "Simple test for rwlock reader --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Simple test for rwlock reader --->> OK"
echo "Simple test for rwlock reader --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Simple test for rwlock writer
###
echo "-- Simple test for rwlock writer --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/singletest -f $FILEPATH -write >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Simple test for rwlock writer --->> ERROR"
	echo "Simple test for rwlock writer --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Simple test for rwlock writer --->> OK"
echo "Simple test for rwlock writer --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Simple test for mutex
###
echo "-- Simple test for mutex --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/singletest -m $MUTEXNAME >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Simple test for mutex --->> ERROR"
	echo "Simple test for mutex --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Simple test for mutex --->> OK"
echo "Simple test for mutex --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Simple test for cond
###
echo "-- Simple test for cond --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/singletest -c $CONDNAME >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Simple test for cond --->> ERROR"
	echo "Simple test for cond --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Simple test for cond --->> OK"
echo "Simple test for cond --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Multi thread test for rwlock
###
echo "-- Multi thread test for rwlock --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/mttest -f $FILEPATH -r 5 -w 5 -l 10 >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Multi thread test for rwlock --->> ERROR"
	echo "Multi thread test for rwlock --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Multi thread test for rwlock --->> OK"
echo "Multi thread test for rwlock --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Multi thread test for mutex
###
echo "-- Multi thread test for mutex --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/mttest -m $MUTEXNAME -t 5 -l 10 >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Multi thread test for mutex --->> ERROR"
	echo "Multi thread test for mutex --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Multi thread test for mutex --->> OK"
echo "Multi thread test for mutex --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Multi thread test for cond
###
echo "-- Multi thread test for cond(signal) --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/cond_mttest -s -w 10 >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Multi thread test for cond(signal) --->> ERROR"
	echo "Multi thread test for cond(signal) --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Multi thread test for cond(signal) --->> OK"
echo "Multi thread test for cond(signal) --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

###
echo "-- Multi thread test for cond(broadcast) --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/cond_mttest -b -w 10 >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Multi thread test for cond(broadcast) --->> ERROR"
	echo "Multi thread test for cond(broadcast) --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Multi thread test for cond(broadcast) --->> OK"
echo "Multi thread test for cond(broadcast) --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Multi process test for rwlock
###
echo "-- Multi process test for rwlock --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/mptest -f $FILEPATH -r 5 -w 5 -l 10 >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Multi process test for rwlock --->> ERROR"
	echo "Multi process test for rwlock --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Multi process test for rwlock --->> OK"
echo "Multi process test for rwlock --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Multi process test for mutex
###
echo "-- Multi process test for mutex --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/mptest -m $MUTEXNAME -t 5 -l 10 >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Multi process test for mutex --->> ERROR"
	echo "Multi process test for mutex --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Multi process test for mutex --->> OK"
echo "Multi process test for mutex --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Multi process test for cond
###
echo "-- Multi process test for cond(signal) --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/cond_mptest -s -w 10 >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Multi process test for cond(signal) --->> ERROR"
	echo "Multi process test for cond(signal) --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Multi process test for cond(signal) --->> OK"
echo "Multi process test for cond(signal) --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

###
echo "-- Multi process test for cond(broadcast) --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/cond_mptest -b -w 10 >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Multi process test for cond(broadcast) --->> ERROR"
	echo "Multi process test for cond(broadcast) --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Multi process test for cond(broadcast) --->> OK"
echo "Multi process test for cond(broadcast) --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Environment test
###
echo "-- Environment test --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/fullocktest -env >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Environment test --->> ERROR"
	echo "Environment test --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Environment test --->> OK"
echo "Environment test --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Over area count limit test for rwlock(unit no)
###
echo "-- Over area count limit test for rwlock(unit no) --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/fullocktest -oac -unit no >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Over area count limit test for rwlock(unit no) --->> ERROR"
	echo "Over area count limit test for rwlock(unit no) --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Over area count limit test for rwlock(unit no) --->> OK"
echo "Over area count limit test for rwlock(unit no) --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Over area count limit test for rwlock(unit fd)
###
echo "-- Over area count limit test for rwlock(unit fd) --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/fullocktest -oac -unit fd >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Over area count limit test for rwlock(unit fd) --->> ERROR"
	echo "Over area count limit test for rwlock(unit fd) --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Over area count limit test for rwlock(unit fd) --->> OK"
echo "Over area count limit test for rwlock(unit fd) --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Over area count limit test for rwlock(unit offset)
###
echo "-- Over area count limit test for rwlock(unit offset) --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/fullocktest -oac -unit offset >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Over area count limit test for rwlock(unit offset) --->> ERROR"
	echo "Over area count limit test for rwlock(unit offset) --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Over area count limit test for rwlock(unit offset) --->> OK"
echo "Over area count limit test for rwlock(unit offset) --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Deadlock test for rwlock
###
echo "-- Deadlock test for rwlock --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/fullocktest -dl >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Deadlock test for rwlock --->> ERROR"
	echo "Deadlock test for rwlock --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Deadlock test for rwlock --->> OK"
echo "Deadlock test for rwlock --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Automatic recover deadlock thread test for rwlock(robust no)
###
#
#	**** YOU NEED TO CHECK MANUALLY ****
#
#	echo "-- Automatic recover deadlock thread test for rwlock(robust no) --" >> $LOGFILE
#	echo "" >> $LOGFILE
#	./fullocktest -ar -thread -robust no >> $LOGFILE
#	echo "" >> $LOGFILE
#	
#	if [ $? -ne 0 ]; then
#		echo "Automatic recover deadlock thread test for rwlock(robust no) --->> ERROR"
#		echo "Automatic recover deadlock thread test for rwlock(robust no) --->> ERROR" >> $LOGFILE
#		exit 1
#	fi
#	echo "Automatic recover deadlock thread test for rwlock(robust no) --->> OK"
#	echo "Automatic recover deadlock thread test for rwlock(robust no) --->> OK" >> $LOGFILE
#	echo "" >> $LOGFILE

##############################################################
### Automatic recover deadlock thread test for rwlock(robust low)
###
#
#	**** YOU NEED TO CHECK MANUALLY ****
#
#	echo "-- Automatic recover deadlock thread test for rwlock(robust low) --" >> $LOGFILE
#	echo "" >> $LOGFILE
#	./fullocktest -ar -thread -robust low >> $LOGFILE
#	echo "" >> $LOGFILE
#	
#	if [ $? -ne 0 ]; then
#		echo "Automatic recover deadlock thread test for rwlock(robust low) --->> ERROR"
#		echo "Automatic recover deadlock thread test for rwlock(robust low) --->> ERROR" >> $LOGFILE
#		exit 1
#	fi
#	echo "Automatic recover deadlock thread test for rwlock(robust low) --->> OK"
#	echo "Automatic recover deadlock thread test for rwlock(robust low) --->> OK" >> $LOGFILE
#	echo "" >> $LOGFILE

##############################################################
### Automatic recover deadlock thread test for rwlock(robust high)
###
echo "-- Automatic recover deadlock thread test for rwlock(robust high) --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/fullocktest -ar -thread -robust high >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Automatic recover deadlock thread test for rwlock(robust high) --->> ERROR"
	echo "Automatic recover deadlock thread test for rwlock(robust high) --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Automatic recover deadlock thread test for rwlock(robust high) --->> OK"
echo "Automatic recover deadlock thread test for rwlock(robust high) --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Automatic recover deadlock process test for rwlock(robust no)
###
echo "-- Automatic recover deadlock process test for rwlock(robust no) --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/fullocktest -ar -process -robust no >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Automatic recover deadlock process test for rwlock(robust no) --->> ERROR"
	echo "Automatic recover deadlock process test for rwlock(robust no) --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Automatic recover deadlock process test for rwlock(robust no) --->> OK"
echo "Automatic recover deadlock process test for rwlock(robust no) --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Automatic recover deadlock process test for rwlock(robust low)
###
echo "-- Automatic recover deadlock process test for rwlock(robust low) --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/fullocktest -ar -process -robust low >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Automatic recover deadlock process test for rwlock(robust low) --->> ERROR"
	echo "Automatic recover deadlock process test for rwlock(robust low) --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Automatic recover deadlock process test for rwlock(robust low) --->> OK"
echo "Automatic recover deadlock process test for rwlock(robust low) --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Automatic recover deadlock process test for rwlock(robust high)
###
echo "-- Automatic recover deadlock process test for rwlock(robust high) --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/fullocktest -ar -process -robust high >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Automatic recover deadlock process test for rwlock(robust high) --->> ERROR"
	echo "Automatic recover deadlock process test for rwlock(robust high) --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Automatic recover deadlock process test for rwlock(robust high) --->> OK"
echo "Automatic recover deadlock process test for rwlock(robust high) --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Over area count limit test for mutex
###
echo "-- Over area count limit test for mutex --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/fullocktest -moac >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Over area count limit test for mutex --->> ERROR"
	echo "Over area count limit test for mutex --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Over area count limit test for mutex --->> OK"
echo "Over area count limit test for mutex --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Deadlock test for mutex
###
echo "-- Deadlock test for mutex --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/fullocktest -mdl >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Deadlock test for mutex --->> ERROR"
	echo "Deadlock test for mutex --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Deadlock test for mutex --->> OK"
echo "Deadlock test for mutex --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Automatic recover deadlock thread test for mutex(robust no)
###
#
#	**** YOU NEED TO CHECK MANUALLY ****
#
#	echo "-- Automatic recover deadlock thread test for mutex(robust no) --" >> $LOGFILE
#	echo "" >> $LOGFILE
#	./fullocktest -mar -thread -robust no >> $LOGFILE
#	echo "" >> $LOGFILE
#	
#	if [ $? -ne 0 ]; then
#		echo "Automatic recover deadlock thread test for mutex(robust no) --->> ERROR"
#		echo "Automatic recover deadlock thread test for mutex(robust no) --->> ERROR" >> $LOGFILE
#		exit 1
#	fi
#	echo "Automatic recover deadlock thread test for mutex(robust no) --->> OK"
#	echo "Automatic recover deadlock thread test for mutex(robust no) --->> OK" >> $LOGFILE
#	echo "" >> $LOGFILE

##############################################################
### Automatic recover deadlock thread test for mutex(robust low)
###
#
#	**** YOU NEED TO CHECK MANUALLY ****
#
#	echo "-- Automatic recover deadlock thread test for mutex(robust low) --" >> $LOGFILE
#	echo "" >> $LOGFILE
#	./fullocktest -mar -thread -robust low >> $LOGFILE
#	echo "" >> $LOGFILE
#	
#	if [ $? -ne 0 ]; then
#		echo "Automatic recover deadlock thread test for mutex(robust low) --->> ERROR"
#		echo "Automatic recover deadlock thread test for mutex(robust low) --->> ERROR" >> $LOGFILE
#		exit 1
#	fi
#	echo "Automatic recover deadlock thread test for mutex(robust low) --->> OK"
#	echo "Automatic recover deadlock thread test for mutex(robust low) --->> OK" >> $LOGFILE
#	echo "" >> $LOGFILE

##############################################################
### Automatic recover deadlock thread test for mutex(robust high)
###
echo "-- Automatic recover deadlock thread test for mutex(robust high) --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/fullocktest -mar -thread -robust high >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Automatic recover deadlock thread test for mutex(robust high) --->> ERROR"
	echo "Automatic recover deadlock thread test for mutex(robust high) --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Automatic recover deadlock thread test for mutex(robust high) --->> OK"
echo "Automatic recover deadlock thread test for mutex(robust high) --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Automatic recover deadlock process test for mutex(robust no)
###
echo "-- Automatic recover deadlock process test for mutex(robust no) --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/fullocktest -mar -process -robust no >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Automatic recover deadlock process test for mutex(robust no) --->> ERROR"
	echo "Automatic recover deadlock process test for mutex(robust no) --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Automatic recover deadlock process test for mutex(robust no) --->> OK"
echo "Automatic recover deadlock process test for mutex(robust no) --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Automatic recover deadlock process test for mutex(robust low)
###
echo "-- Automatic recover deadlock process test for mutex(robust low) --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/fullocktest -mar -process -robust low >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Automatic recover deadlock process test for mutex(robust low) --->> ERROR"
	echo "Automatic recover deadlock process test for mutex(robust low) --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Automatic recover deadlock process test for mutex(robust low) --->> OK"
echo "Automatic recover deadlock process test for mutex(robust low) --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Automatic recover deadlock process test for mutex(robust high)
###
echo "-- Automatic recover deadlock process test for mutex(robust high) --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/fullocktest -mar -process -robust high >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Automatic recover deadlock process test for mutex(robust high) --->> ERROR"
	echo "Automatic recover deadlock process test for mutex(robust high) --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Automatic recover deadlock process test for mutex(robust high) --->> OK"
echo "Automatic recover deadlock process test for mutex(robust high) --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Over area count limit test for cond
###
echo "-- Over area count limit test for cond --" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/fullocktest -coac >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Over area count limit test for cond --->> ERROR"
	echo "Over area count limit test for cond --->> ERROR" >> $LOGFILE
	echo ""
	echo "---------------- Test process log ----------------"
	cat $LOGFILE
	exit 1
fi
echo "Over area count limit test for cond --->> OK"
echo "Over area count limit test for cond --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Check and Kill sub processes if these are running.
###
REST_PROCESSES=`ps ax | grep fullocktest | grep -v grep | awk '{print $1}'`
kill -HUP $REST_PROCESSES 2> /dev/null
sleep 1
kill -9 $REST_PROCESSES 2> /dev/null

##############################################################
### Test forking
###
echo "-- Test forking -------------------------" >> $LOGFILE
echo "" >> $LOGFILE
${TESTPROGDIR}/forktest >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Test forking --->> ERROR"
	echo "Test forking --->> ERROR" >> $LOGFILE
	echo ""
	echo "---------------- Test forking log ----------------"
	cat $LOGFILE
	put_result_xml_func NG ${XMLRESULTSFILE}
	exit 1
fi
echo "Test forking --->> OK"
echo "Test forking --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

##############################################################
### Remove file
###
rm -f $FILEPATH

echo "All test --->> OK"
echo "All test --->> OK" >> $LOGFILE
echo "" >> $LOGFILE

exit 0

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
