/*
 * FULLOCK - Fast User Level LOCK library
 *
 * Copyright 2015 Yahoo Japan Corporation.
 *
 * FULLOCK is fast locking library on user level by Yahoo! JAPAN.
 * FULLOCK is following specifications.
 *
 * For the full copyright and license information, please view
 * the license file that was distributed with this source code.
 *
 * AUTHOR:   Takeshi Nakatani
 * CREATE:   Wed 13 May 2015
 * REVISION:
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include "flckcommon.h"
#include "fullock.h"
#include "flckshm.h"

using namespace std;

//---------------------------------------------------------
// Functions - Version
//---------------------------------------------------------
extern char fullock_commit_hash[];

void fullock_print_version(FILE* stream)
{
	static const char format[] =
		"\n"
		"FULLOCK Version %s (commit: %s)\n"
		"\n"
		"Copyright(C) 2015 Yahoo Japan Corporation.\n"
		"\n"
		"FULLOCK is fast locking library on user level by Yahoo! JAPAN.\n"
		"FULLOCK is following specifications.\n"
		"\n";
	fprintf(stream, format, VERSION, fullock_commit_hash);
}

//---------------------------------------------------------
// Functions - variables
//---------------------------------------------------------
bool fullock_set_no_robust(void)
{
	FlShm::SetRobustMode(FlShm::ROBUST_NO);
	return true;
}

bool fullock_set_low_robust(void)
{
	FlShm::SetRobustMode(FlShm::ROBUST_LOW);
	return true;
}

bool fullock_set_high_robust(void)
{
	FlShm::SetRobustMode(FlShm::ROBUST_HIGH);
	return true;
}

bool fullock_set_noretry_allow_nomap(void)
{
	FlShm::SetNomapMode(FlShm::NOMAP_ALLOW_NORETRY);
	return true;
}

bool fullock_set_noretry_deny_nomap(void)
{
	FlShm::SetNomapMode(FlShm::NOMAP_DENY_NORETRY);
	return true;
}

bool fullock_set_retry_allow_nomap(void)
{
	FlShm::SetNomapMode(FlShm::NOMAP_ALLOW_RETRY);
	return true;
}

bool fullock_set_retry_deny_nomap(void)
{
	FlShm::SetNomapMode(FlShm::NOMAP_DENY_RETRY);
	return true;
}

bool fullock_set_no_freeunit(void)
{
	FlShm::SetFreeUnitMode(FlShm::FREE_NO);
	return true;
}

bool fullock_set_fd_freeunit(void)
{
	FlShm::SetFreeUnitMode(FlShm::FREE_FD);
	return true;
}

bool fullock_set_offset_freeunit(void)
{
	FlShm::SetFreeUnitMode(FlShm::FREE_OFFSET);
	return true;
}

bool fullock_set_robust_check_count(int val)
{
	if(-1 == FlShm::SetRobustLoopCnt(val)){
		return false;
	}
	return true;
}

bool fullock_reinitialize(const char* dirpath, const char* filename)
{
	if(!FlShm::ReInitializeObject(dirpath, filename)){
		return false;
	}
	return true;
}

bool fullock_reinitialize_ex(const char* dirpath, const char* filename, size_t filelockcnt, size_t offlockcnt, size_t lockercnt, size_t nmtxcnt, size_t ncondcnt, size_t waitercnt)
{
	if(!FlShm::ReInitializeObject(dirpath, filename, filelockcnt, offlockcnt, lockercnt, nmtxcnt, ncondcnt, waitercnt)){
		return false;
	}
	return true;
}

//---------------------------------------------------------
// Functions - named mutex
//---------------------------------------------------------
int fullock_mutex_lock(const char* pname)
{
	FlShm	shm;
	return shm.Lock(pname);
}

int fullock_mutex_trylock(const char* pname)
{
	FlShm	shm;
	return shm.TryLock(pname);
}

int fullock_mutex_timedlock(const char* pname, time_t timeout_usec)
{
	FlShm	shm;
	return shm.TimeoutLock(pname, timeout_usec);
}

int fullock_mutex_unlock(const char* pname)
{
	FlShm	shm;
	return shm.Unlock(pname);
}

//---------------------------------------------------------
// Functions - rwlock
//---------------------------------------------------------
int fullock_rwlock_rdlock(int fd, off_t offset, size_t length)
{
	FlShm	shm;
	return shm.ReadLock(fd, offset, length);
}

int fullock_rwlock_tryrdlock(int fd, off_t offset, size_t length)
{
	FlShm	shm;
	return shm.TryReadLock(fd, offset, length);
}

int fullock_rwlock_timedrdlock(int fd, off_t offset, size_t length, time_t timeout_usec)
{
	FlShm	shm;
	return shm.TimeoutReadLock(fd, offset, length, timeout_usec);
}

int fullock_rwlock_wrlock(int fd, off_t offset, size_t length)
{
	FlShm	shm;
	return shm.WriteLock(fd, offset, length);
}

int fullock_rwlock_trywrlock(int fd, off_t offset, size_t length)
{
	FlShm	shm;
	return shm.TryWriteLock(fd, offset, length);
}

int fullock_rwlock_timedwrlock(int fd, off_t offset, size_t length, time_t timeout_usec)
{
	FlShm	shm;
	return shm.TimeoutWriteLock(fd, offset, length, timeout_usec);
}

int fullock_rwlock_unlock(int fd, off_t offset, size_t length)
{
	FlShm	shm;
	return shm.Unlock(fd, offset, length);
}

bool fullock_rwlock_islocked(int fd, off_t offset, size_t length)
{
	FlShm	shm;
	return shm.IsLocked(fd, offset, length);
}

//---------------------------------------------------------
// Functions - named mutex
//---------------------------------------------------------
int fullock_cond_timedwait(const char* pcondname, const char* pmutexname, time_t timeout_usec)
{
	FlShm	shm;
	return shm.TimeoutWait(pcondname, pmutexname, timeout_usec);
}

int fullock_cond_wait(const char* pcondname, const char* pmutexname)
{
	FlShm	shm;
	return shm.Wait(pcondname, pmutexname);
}

int fullock_cond_signal(const char* pcondname)
{
	FlShm	shm;
	return shm.Signal(pcondname);
}

int fullock_cond_broadcast(const char* pcondname)
{
	FlShm	shm;
	return shm.Broadcast(pcondname);
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
