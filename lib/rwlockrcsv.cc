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
 * CREATE:   Fri 12 Jun 2015
 * REVISION:
 *
 */

#include "rwlockrcsv.h"
#include "flckutil.h"
#include "flckdbg.h"

using namespace std;

//---------------------------------------------------------
// Class FLRwlRcsvHelper
//---------------------------------------------------------
// [NOTE]
// To avoid static object initialization order problem(SIOF)
//
class FLRwlRcsvHelper
{
	protected:
		int				LockVal;													// like mutex
		flrwlrcsv_vec_t	FLRwlStack;

	protected:
		FLRwlRcsvHelper(void) : LockVal(FLCK_NOSHARED_MUTEX_VAL_UNLOCKED) {}
		virtual ~FLRwlRcsvHelper(void) {}

		static FLRwlRcsvHelper& GetFlShm(void)
		{
			static FLRwlRcsvHelper	helper;											// singlton
			return helper;
		}

	public:
		static flrwlrcsv_vec_t& GetStack(void)
		{
			return GetFlShm().FLRwlStack;
		}

		static void Lock(void)
		{
			while(!fullock::flck_trylock_noshared_mutex(&(GetFlShm().LockVal)));	// no call sched_yield()
		}

		static void Unlock(void)
		{
			fullock::flck_unlock_noshared_mutex(&(GetFlShm().LockVal));
		}
};

//---------------------------------------------------------
// FLRwlRcsv : Class Methos
//---------------------------------------------------------
// [NOTE]
// To avoid static object initialization order problem(SIOF)
//
flrwlrcsv_vec_t& FLRwlRcsv::Stack(void)
{
	return FLRwlRcsvHelper::GetStack();
}

void FLRwlRcsv::StackLock(void)
{
	FLRwlRcsvHelper::Lock();
}

void FLRwlRcsv::StackUnlock(void)
{
	FLRwlRcsvHelper::Unlock();
}

//---------------------------------------------------------
// FLRwlRcsv : Methos
//---------------------------------------------------------
FLRwlRcsv::FLRwlRcsv(int fd, off_t offset, size_t length) : lock_tid(get_threadid()), lock_fd(fd), lock_offset(offset), lock_length(length), lock_type(FLCK_UNLOCK), is_locked(false), has_locker(false), pMaster(NULL)
{
}

FLRwlRcsv::FLRwlRcsv(int fd, off_t offset, size_t length, bool is_read) : lock_tid(get_threadid()), lock_fd(fd), lock_offset(offset), lock_length(length), lock_type(is_read ? FLCK_READ_LOCK : FLCK_WRITE_LOCK), is_locked(false), has_locker(false), pMaster(NULL)
{
	bool	is_mutex_locked	= false;
	if(FLCK_INVALID_HANDLE != fd){
		if(is_read){
			RawReadLock(is_mutex_locked);
		}else{
			RawWriteLock(is_mutex_locked);
		}
	}
	if(is_mutex_locked){
		FLRwlRcsv::StackUnlock();
	}
}

FLRwlRcsv::~FLRwlRcsv(void)
{
	bool	is_mutex_locked = false;
	RawUnlock(is_mutex_locked);

	if(is_mutex_locked){
		FLRwlRcsv::StackUnlock();
	}
}

bool FLRwlRcsv::compare(const FLRwlRcsv& other) const
{
	if(lock_tid == other.lock_tid && lock_fd == other.lock_fd){
		if(lock_offset <= other.lock_offset){
			if(other.lock_offset < (lock_offset + static_cast<off_t>(lock_length))){
				return true;
			}
		}else{
			if(lock_offset < (other.lock_offset + static_cast<off_t>(other.lock_length))){
				return true;
			}
		}
	}
	return false;
}

bool FLRwlRcsv::Set(int fd, off_t offset, size_t length, bool is_read, bool& is_mutex_locked)
{
	if(!RawUnlock(is_mutex_locked)){
		ERR_FLCKPRN("Could not unlock.");
		return false;
	}
	lock_fd		= fd;
	lock_offset	= offset;
	lock_length	= length;
	lock_type	= is_read ? FLCK_READ_LOCK : FLCK_WRITE_LOCK;
	return true;
}

// [NOTE]
// If the target is locking as recursion and one of locking is write lock, you must take care for this case.
// On this case, if the target is unlocked from writer locking, the target keeps locking as writer yet.
// Untill all of panding stack are unlocked, the target is locked as writer.
// So that the target is locked as writer until unlocking all.
//
bool FLRwlRcsv::RawUnlock(bool& is_mutex_locked)
{
	FlShm	shm;
	int		result;

	if(FLCK_UNLOCK == lock_type || !is_locked){
		//MSG_FLCKPRN("Already unlocked.");
		return true;
	}

	// lock mutex for global stack
	if(!is_mutex_locked){
		FLRwlRcsv::StackLock();
		is_mutex_locked = true;
	}

	if(pMaster){
		// this lock object is pending, remove it from master list
		for(flrwlrcsv_vec_t::iterator iter = pMaster->PendingStack.begin(); iter != pMaster->PendingStack.end(); ++iter){
			if(this == (*iter)){
				// found own
				pMaster->PendingStack.erase(iter);
				break;
			}
		}
		pMaster = NULL;

	}else{
		// search same lock in stack
		for(flrwlrcsv_vec_t::iterator iter = FLRwlRcsv::Stack().begin(); iter != FLRwlRcsv::Stack().end(); ++iter){
			if(this == (*iter)){
				// erase own
				FLRwlRcsv::Stack().erase(iter);
				break;
			}
		}

		if(0 < PendingStack.size()){
			// if has panding stack, swap master to next in panding list.
			FLRwlRcsv*	pNewMaster = NULL;

			// check panding stack, and seach writer
			for(flrwlrcsv_vec_t::iterator iter = PendingStack.begin(); iter != PendingStack.end(); ++iter){
				if(FLCK_WRITE_LOCK == (*iter)->lock_type){
					pNewMaster = *iter;
					PendingStack.erase(iter);
					break;
				}
			}

			// next(if there is no writer)
			if(!pNewMaster){
				pNewMaster = PendingStack.back();
				PendingStack.pop_back();
			}

			// do swap master
			move_panding_stack(pNewMaster->PendingStack, PendingStack, pNewMaster);
			PendingStack.clear();

			pNewMaster->has_locker	= true;
			pNewMaster->is_locked	= true;
			pNewMaster->lock_type	= lock_type;			// [NOTICE] there is a possibility case read type to write.
			pNewMaster->pMaster		= NULL;
			FLRwlRcsv::Stack().push_back(pNewMaster);

		}else{
			// if object has lock -> unlock writer/reader lock
			if(has_locker){
				// unlock mutex for global stack(temporary)
				if(is_mutex_locked){
					FLRwlRcsv::StackUnlock();
					is_mutex_locked = false;
				}
				if(0 != (result = shm.Unlock(lock_fd, lock_offset, lock_length))){
					ERR_FLCKPRN("Could not unlock rwlock for fd(%d) offset(%zd) length(%zu), error code=%d", lock_fd, lock_offset, lock_length, result);

					// for recovering
					if(!is_mutex_locked){
						FLRwlRcsv::StackLock();
						is_mutex_locked = true;
					}
					// add own to global stack
					FLRwlRcsv::Stack().push_back(this);
					return false;
				}

			}else{
				// nothing to do
			}
		}
	}
	has_locker	= false;
	is_locked	= false;

	return true;
}

bool FLRwlRcsv::RawReadLock(bool& is_mutex_locked)
{
	FlShm	shm;
	int		result;

	// check locking
	if(is_locked){
		if(FLCK_READ_LOCK == lock_type){
			//MSG_FLCKPRN("Already locked.");
			return true;
		}
		if(FLCK_WRITE_LOCK == lock_type && 0 < PendingStack.size()){
			//MSG_FLCKPRN("Requested read lock, but now write lock and have pending stack. so keep this type.");
			return true;
		}
	}

	// lock mutex for global stack
	if(!is_mutex_locked){
		FLRwlRcsv::StackLock();
		is_mutex_locked = true;
	}

	// Unlock if write lock
	if(FLCK_WRITE_LOCK == lock_type && is_locked){
		if(!RawUnlock(is_mutex_locked)){
			ERR_FLCKPRN("Failed to unlock.");
			return false;
		}
	}
	lock_type = FLCK_READ_LOCK;

	// check all status in global stack
	for(flrwlrcsv_vec_t::iterator iter = FLRwlRcsv::Stack().begin(); iter != FLRwlRcsv::Stack().end(); ++iter){
		if(compare(*(*iter))){
			// found same tid read or write lock
			if(!(*iter)->is_locked){
				WAN_FLCKPRN("Same tid and type(read ow write) found, but it does not have lock. but continue...");
			}
			is_locked	= (*iter)->is_locked;
			has_locker	= false;
			pMaster		= *iter;
			(*iter)->PendingStack.push_back(this);

			return true;
		}
	}
	// add own to global stack
	FLRwlRcsv::Stack().push_back(this);

	// unlock mutex for global stack
	if(is_mutex_locked){
		FLRwlRcsv::StackUnlock();
		is_mutex_locked = false;
	}

	// try read lock
	if(0 != (result = shm.ReadLock(lock_fd, lock_offset, lock_length))){
		ERR_FLCKPRN("Could not reader lock for fd(%d) offset(%zd) length(%zu), error code=%d", lock_fd, lock_offset, lock_length, result);

		// for recovering
		if(!is_mutex_locked){
			FLRwlRcsv::StackLock();
			is_mutex_locked = true;
		}
		// remove own from global stack
		for(flrwlrcsv_vec_t::iterator iter = FLRwlRcsv::Stack().begin(); iter != FLRwlRcsv::Stack().end(); ++iter){
			if(this == (*iter)){
				FLRwlRcsv::Stack().erase(iter);
				break;
			}
		}
		has_locker	= false;
		is_locked	= false;

		return false;
	}
	has_locker	= true;
	is_locked	= true;

	return true;
}

bool FLRwlRcsv::RawWriteLock(bool& is_mutex_locked)
{
	FlShm	shm;
	int		result;

	// check locking
	if(is_locked){
		if(FLCK_WRITE_LOCK == lock_type){
			//MSG_FLCKPRN("Already locked.");
			return true;
		}
		if(FLCK_READ_LOCK == lock_type && pMaster && FLCK_WRITE_LOCK == pMaster->lock_type){
			//MSG_FLCKPRN("Already locked.");
			lock_type = FLCK_WRITE_LOCK;				// [NOTICE] Already write locked, so only change type
			return true;
		}
	}

	// lock mutex for global stack
	if(!is_mutex_locked){
		FLRwlRcsv::StackLock();
		is_mutex_locked = true;
	}

	// Unlock if read locked
	if(FLCK_READ_LOCK == lock_type && is_locked){
		if(!RawUnlock(is_mutex_locked)){
			ERR_FLCKPRN("Failed to unlock.");
			return false;
		}
	}
	lock_type = FLCK_WRITE_LOCK;

	// check all status in global stack
	flrwlrcsv_vec_t	PandingReaderList;
	for(flrwlrcsv_vec_t::iterator iter = FLRwlRcsv::Stack().begin(); iter != FLRwlRcsv::Stack().end(); ){
		if(compare(*(*iter))){
			if(FLCK_READ_LOCK == (*iter)->lock_type){
				// found same tid reader
				PandingReaderList.push_back(*iter);
				iter = FLRwlRcsv::Stack().erase(iter);
				continue;

			}else if(FLCK_WRITE_LOCK == (*iter)->lock_type){
				// found same tid write lock owner
				if(!(*iter)->is_locked){
					WAN_FLCKPRN("Same tid writer found, but it does not have lock. but continue...");
				}
				// set own to this writer object pending
				has_locker	= false;
				is_locked	= true;
				pMaster		= *iter;
				(*iter)->PendingStack.push_back(this);

				return true;
			}
		}
		++iter;
	}
	// add own to global stack
	FLRwlRcsv::Stack().push_back(this);

	// unlock mutex for global stack
	if(is_mutex_locked){
		FLRwlRcsv::StackUnlock();
		is_mutex_locked = false;
	}

	// unlock all same tid reader
	for(flrwlrcsv_vec_t::iterator iter = PandingReaderList.begin(); iter != PandingReaderList.end(); ++iter){
		if((*iter)->is_locked && (*iter)->has_locker){
			if(0 != (result = shm.Unlock((*iter)->lock_fd, (*iter)->lock_offset, (*iter)->lock_length))){
				ERR_FLCKPRN("Could not unlock reader for fd(%d) offset(%zd) length(%zu), error code=%d, but continue...", (*iter)->lock_fd, (*iter)->lock_offset, (*iter)->lock_length, result);
			}
		}
		(*iter)->has_locker	= false;
		(*iter)->is_locked	= true;
		(*iter)->pMaster	= this;
		PendingStack.push_back(*iter);

		// add pending list
		move_panding_stack(PendingStack, (*iter)->PendingStack, this);
		(*iter)->PendingStack.clear();
	}

	// try write lock
	if(0 != (result = shm.WriteLock(lock_fd, lock_offset, lock_length))){
		ERR_FLCKPRN("Could not writer lock for fd(%d) offset(%zd) length(%zu), error code=%d", lock_fd, lock_offset, lock_length, result);

		// for recovering
		if(!is_mutex_locked){
			FLRwlRcsv::StackLock();
			is_mutex_locked = true;
		}
		// remove own from global stack
		for(flrwlrcsv_vec_t::iterator iter = FLRwlRcsv::Stack().begin(); iter != FLRwlRcsv::Stack().end(); ++iter){
			if(this == (*iter)){
				FLRwlRcsv::Stack().erase(iter);
				break;
			}
		}
		// re-lock panding list(and put into global stack)
		for(flrwlrcsv_vec_t::iterator iter = PendingStack.begin(); iter != PendingStack.end(); iter = PendingStack.erase(iter)){
			(*iter)->has_locker	= false;
			(*iter)->is_locked	= false;
			(*iter)->pMaster	= NULL;
			// do re-lock
			if(!(*iter)->Lock()){
				ERR_FLCKPRN("Failed to re-lock for recovering.");
			}
		}
		has_locker	= false;
		is_locked	= false;

		return false;
	}
	has_locker	= true;
	is_locked	= true;

	return true;
}

bool FLRwlRcsv::Lock(int fd, off_t offset, size_t length, bool is_read)
{
	bool	is_mutex_locked	= false;
	bool	bresult			= false;

	if(true == (bresult = Set(fd, offset, length, is_read, is_mutex_locked))){
		if(is_read){
			bresult = RawReadLock(is_mutex_locked);
		}else{
			bresult = RawWriteLock(is_mutex_locked);
		}
	}

	if(is_mutex_locked){
		FLRwlRcsv::StackUnlock();
	}
	return bresult;
}

bool FLRwlRcsv::Lock(bool is_read)
{
	bool	is_mutex_locked	= false;
	bool	bresult			= false;

	if(is_read){
		bresult = RawReadLock(is_mutex_locked);
	}else{
		bresult = RawWriteLock(is_mutex_locked);
	}

	if(is_mutex_locked){
		FLRwlRcsv::StackUnlock();
	}
	return bresult;
}

bool FLRwlRcsv::Lock(void)
{
	bool	is_mutex_locked	= false;
	bool	bresult			= false;

	if(FLCK_READ_LOCK == lock_type){
		bresult = RawReadLock(is_mutex_locked);
	}else if(FLCK_WRITE_LOCK == lock_type){
		bresult = RawWriteLock(is_mutex_locked);
	}

	if(is_mutex_locked){
		FLRwlRcsv::StackUnlock();
	}
	return bresult;
}

bool FLRwlRcsv::ReadLock(void)
{
	bool	is_mutex_locked	= false;
	bool	bresult			= RawReadLock(is_mutex_locked);

	if(is_mutex_locked){
		FLRwlRcsv::StackUnlock();
	}
	return bresult;
}

bool FLRwlRcsv::WriteLock(void)
{
	bool	is_mutex_locked	= false;
	bool	bresult			= RawWriteLock(is_mutex_locked);

	if(is_mutex_locked){
		FLRwlRcsv::StackUnlock();
	}
	return bresult;
}

bool FLRwlRcsv::Unlock(void)
{
	bool	is_mutex_locked	= false;
	bool	bresult			= RawUnlock(is_mutex_locked);

	if(is_mutex_locked){
		FLRwlRcsv::StackUnlock();
	}
	return bresult;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
