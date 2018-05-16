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

#ifndef	FULLOCK_H
#define	FULLOCK_H

#include <stdio.h>
#include <time.h>
#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

//---------------------------------------------------------
// Symbols
//---------------------------------------------------------
#define	FLCK_NAMED_MUTEX_MAXLENGTH			63
#define	FLCK_NAMED_COND_MAXLENGTH			63
#define	FLCK_INITCNT_DEFAULT				0
#define	FLCK_RWLOCK_NO_FD(intval)			(intval | 0x80000000)
#define	IS_FLCK_RWLOCK_NO_FD(val)			((val & 0x80000000) == 0x80000000)

//---------------------------------------------------------
// Functions - version
//---------------------------------------------------------
extern void fullock_print_version(FILE* stream);

//---------------------------------------------------------
// Functions - variables
//---------------------------------------------------------
extern bool fullock_set_no_robust(void);
extern bool fullock_set_low_robust(void);
extern bool fullock_set_high_robust(void);
extern bool fullock_set_noretry_allow_nomap(void);
extern bool fullock_set_noretry_deny_nomap(void);
extern bool fullock_set_retry_allow_nomap(void);
extern bool fullock_set_retry_deny_nomap(void);
extern bool fullock_set_no_freeunit(void);
extern bool fullock_set_fd_freeunit(void);
extern bool fullock_set_offset_freeunit(void);
extern bool fullock_set_robust_check_count(int val);
extern bool fullock_reinitialize(const char* dirpath, const char* filename);
extern bool fullock_reinitialize_ex(const char* dirpath, const char* filename, size_t filelockcnt, size_t offlockcnt, size_t lockercnt, size_t nmtxcnt, size_t ncondcnt, size_t waitercnt);

//---------------------------------------------------------
// Functions - named mutex
//---------------------------------------------------------
extern int fullock_mutex_lock(const char* pname);
extern int fullock_mutex_trylock(const char* pname);
extern int fullock_mutex_timedlock(const char* pname, time_t timeout_usec);
extern int fullock_mutex_unlock(const char* pname);

//---------------------------------------------------------
// Functions - rwlock
//---------------------------------------------------------
extern int fullock_rwlock_rdlock(int fd, off_t offset, size_t length);
extern int fullock_rwlock_tryrdlock(int fd, off_t offset, size_t length);
extern int fullock_rwlock_timedrdlock(int fd, off_t offset, size_t length, time_t timeout_usec);

extern int fullock_rwlock_wrlock(int fd, off_t offset, size_t length);
extern int fullock_rwlock_trywrlock(int fd, off_t offset, size_t length);
extern int fullock_rwlock_timedwrlock(int fd, off_t offset, size_t length, time_t timeout_usec);

extern int fullock_rwlock_unlock(int fd, off_t offset, size_t length);

extern bool fullock_rwlock_islocked(int fd, off_t offset, size_t length);

//---------------------------------------------------------
// Functions - named cond
//---------------------------------------------------------
extern int fullock_cond_timedwait(const char* pcondname, const char* pmutexname, time_t timeout_usec);
extern int fullock_cond_wait(const char* pcondname, const char* pmutexname);
extern int fullock_cond_signal(const char* pcondname);
extern int fullock_cond_broadcast(const char* pcondname);

#if defined(__cplusplus)
}
#endif	// __cplusplus

#endif	// FULLOCK_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
