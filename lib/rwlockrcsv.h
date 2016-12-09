/*
 * FULLOCK - Fast User Level LOCK library by Yahoo! JAPAN
 *
 * Copyright 2015 Yahoo! JAPAN corporation.
 *
 * FULLOCK is fast locking library on user level by Yahoo! JAPAN.
 * FULLOCK is following specifications.
 *
 * For the full copyright and license information, please view
 * the LICENSE file that was distributed with this source code.
 *
 * AUTHOR:   Takeshi Nakatani
 * CREATE:   Fri 12 Jun 2015
 * REVISION:
 *
 */

#ifndef	RWLOCKRCSV_H
#define	RWLOCKRCSV_H

#include <vector>

#include "flcklocktype.h"
#include "flckshm.h"
#include "flckbaselist.tcc"

//---------------------------------------------------------
// Typedefs
//---------------------------------------------------------
class FLRwlRcsv;

typedef std::vector<FLRwlRcsv*>		flrwlrcsv_vec_t;

//---------------------------------------------------------
// Class FLRwlRcsvStack
//---------------------------------------------------------
class FLRwlRcsvStack
{
	protected:
		int					LockVal;		// like mutex

	public:
		flrwlrcsv_vec_t		FLRwlStack;

	public:
		FLRwlRcsvStack(void) : LockVal(FLCK_NOSHARED_MUTEX_VAL_UNLOCKED) {}
		virtual ~FLRwlRcsvStack(void) {}

		inline void lock(void) { while(!fullock::flck_trylock_noshared_mutex(&LockVal)); }	// no call sched_yield()
		inline void unlock(void) { fullock::flck_unlock_noshared_mutex(&LockVal); }
};

//---------------------------------------------------------
// Class FLRwlRcsv
//---------------------------------------------------------
class FLRwlRcsv
{
	protected:
		static FLRwlRcsvStack	Stack;

		tid_t					lock_tid;
		int						lock_fd;
		off_t					lock_offset;
		size_t					lock_length;
		FLCKLOCKTYPE			lock_type;
		bool					is_locked;			// object is locked status
		bool					has_locker;			// whichever object has lock owner(need to unlock) when object is locked status

		flrwlrcsv_vec_t			PendingStack;		// for recursive, pending lists by this
		FLRwlRcsv*				pMaster;			// for recursive, master pends this

	protected:
		bool compare(const FLRwlRcsv& other) const;
		inline void move_panding_stack(flrwlrcsv_vec_t& dest, flrwlrcsv_vec_t& src, FLRwlRcsv* pNewMaster)
		{
			for(flrwlrcsv_vec_t::iterator iter = src.begin(); iter != src.end(); iter = src.erase(iter)){
				(*iter)->pMaster = pNewMaster;
				dest.push_back(*iter);
			}
		}

		bool RawUnlock(bool& is_mutex_locked);
		bool RawReadLock(bool& is_mutex_locked);
		bool RawWriteLock(bool& is_mutex_locked);
		bool Set(int fd, off_t offset, size_t length, bool is_read, bool& is_mutex_locked);

	public:
		FLRwlRcsv(int fd = FLCK_INVALID_HANDLE, off_t offset = 0, size_t length = 0);
		FLRwlRcsv(int fd, off_t offset, size_t length, bool is_read);
		virtual ~FLRwlRcsv(void);

		bool Lock(int fd, off_t offset, size_t length, bool is_read = true);
		bool Lock(bool is_read);
		bool Lock(void);
		bool ReadLock(void);
		bool WriteLock(void);
		bool Unlock(void);
};

#endif	// RWLOCKRCSV_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
