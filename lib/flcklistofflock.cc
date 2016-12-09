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
 * CREATE:   Mon 6 Jul 2015
 * REVISION:
 *
 */

#include <errno.h>

#include "flcklistofflock.h"
#include "flcklistlocker.h"
#include "flckutil.h"
#include "flckdbg.h"

using namespace std;
using namespace fullock;

//---------------------------------------------------------
// FlListOffLock class
//---------------------------------------------------------
void FlListOffLock::dump(std::ostream &out, int level) const
{
	if(!pcurrent){
		return;
	}
	string	spacer1 = spaces_string(level);
	string	spacer2 = spaces_string(level + 1);

	out << spacer1 << "FLOFFLOCK(" << to_hexstring(pcurrent) << ")={" << std::endl;

	fllistbaseofflock::dump(out, level + 1);

	out << spacer2 << "offset            = " << pcurrent->offset	<< std::endl;
	out << spacer2 << "length            = " << pcurrent->length	<< std::endl;
	out << spacer2 << "lockval           = " << pcurrent->lockval	<< std::endl;
	out << spacer2 << "protect           = " << (pcurrent->protect ? "true" : "false") << std::endl;

	FlListLocker	tmpobj;

	out << spacer2 << "reader_list={" << std::endl;
	for(PFLLOCKER ptmp = to_abs(pcurrent->reader_list); ptmp; ptmp = to_abs(ptmp->next)){
		tmpobj.set(ptmp);
		tmpobj.dump(out, level + 2);
	}
	out << spacer2 << "}" << std::endl;

	out << spacer2 << "writer_list={" << std::endl;
	for(PFLLOCKER ptmp = to_abs(pcurrent->writer_list); ptmp; ptmp = to_abs(ptmp->next)){
		tmpobj.set(ptmp);
		tmpobj.dump(out, level + 2);
	}
	out << spacer2 << "}" << std::endl;

	out << spacer1 << "}" << std::endl;
}

int FlListOffLock::rawlock(FLCKLOCKTYPE LockType, dev_t devid, ino_t inoid, flckpid_t flckpid, int fd, time_t timeout_usec)
{
	if(!pcurrent){
		ERR_FLCKPRN("Object is not initialized.");
		return EINVAL;						// EINVAL
	}
	if(FLCK_INVALID_ID == flckpid){
		flckpid = get_flckpid();
	}
	int result = 0;

	if(FLCK_UNLOCK == LockType){
		// UNLOCK
		bool			is_writer = true;
		FlListLocker	tglistobj;

		// first: check writer list
		if(!tglistobj.find(flckpid, fd, true, pcurrent->writer_list)){
			// second: check reader list
			if(!tglistobj.find(flckpid, fd, true, pcurrent->reader_list)){
				// not found target.
				ERR_FLCKPRN("Could not locking rwlock object for pid(%d), tid(%d), fd(%d) in reader_list(%p).", decompose_pid(flckpid), decompose_tid(flckpid), fd, pcurrent->reader_list);
				return EINVAL;				// EINVAL
			}
			is_writer = false;
		}

		// do unlock
		if(0 != (result = fl_unlock_rwlock(&(pcurrent->lockval)))){
			ERR_FLCKPRN("Could not unlock rwlock object(error code=%d) for pid(%d), tid(%d), fd(%d).", result, decompose_pid(flckpid), decompose_tid(flckpid), fd);
			return result;
		}

		// unset lock flag
		tglistobj.set_unlock();

		// retrive target list
		if(tglistobj.cutoff_list((is_writer ? pcurrent->writer_list : pcurrent->reader_list))){
			// return object to free list
			if(!tglistobj.insert_list(FlShm::pFlHead->locker_free)){
				ERR_FLCKPRN("Failed to insert locker to free list, but continue...");
			}
		}

	}else{
		// LOCK
		FlListLocker	tglistobj;

		// Always get new locker object.
		if(!tglistobj.retreive_list(FlShm::pFlHead->locker_free)){
			ERR_FLCKPRN("Could not get free locker structure.");
			fl_unlock_lockid(&FlShm::pFlHead->file_lock_lockid, flckpid);		// unlock lockid
			return ENOLCK;					// ENOLCK
		}
		// initialize
		tglistobj.initialize(flckpid, fd, false);

		// insert locker into list
		if(!tglistobj.insert_list((FLCK_READ_LOCK == LockType ? pcurrent->reader_list : pcurrent->writer_list))){
			ERR_FLCKPRN("Failed to insert locker to top list.");
			// for recover
			if(!tglistobj.insert_list(FlShm::pFlHead->locker_free)){
				ERR_FLCKPRN("Failed to insert locker to free list, but continue...");
			}
			fl_unlock_lockid(&FlShm::pFlHead->file_lock_lockid, flckpid);		// unlock lockid
			return ENOLCK;					// ENOLCK
		}

		// clear protect flag(because locker list is existed now)
		pcurrent->protect = false;

		fl_unlock_lockid(&FlShm::pFlHead->file_lock_lockid, flckpid);			// unlock lockid

		// lock
		if(0 != (result = dolock(LockType, devid, inoid, flckpid, fd, timeout_usec))){
			ERR_FLCKPRN("Could not lock rwlock object(error code=%d) for pid(%d), tid(%d), fd(%d), devid(%lu), inode(%lu).", result, decompose_pid(flckpid), decompose_tid(flckpid), fd, devid, inoid);

			// check remove file lock for recover...
			//
			fl_lock_lockid(&FlShm::pFlHead->file_lock_lockid, flckpid);		// relock lockid

			if(tglistobj.find(flckpid, fd, false, (FLCK_READ_LOCK == LockType ? pcurrent->reader_list : pcurrent->writer_list))){
				if(tglistobj.cutoff_list((FLCK_READ_LOCK == LockType ? pcurrent->reader_list : pcurrent->writer_list))){
					// return object to free list
					if(!tglistobj.insert_list(FlShm::pFlHead->locker_free)){
						ERR_FLCKPRN("Failed to insert locker to free list, but continue...");
					}
				}
			}
			fl_unlock_lockid(&FlShm::pFlHead->file_lock_lockid, flckpid);		// unlock lockid

			return result;
		}

		// set lock flag
		tglistobj.set_lock();
	}
	return result;
}

int FlListOffLock::dolock(FLCKLOCKTYPE LockType, dev_t devid, ino_t inoid, flckpid_t flckpid, int fd, time_t timeout_usec)
{
	// Do lock
	int	result;
	for(result = 0; 0 == result; ){
		if(FLCK_READ_LOCK == LockType){
			if(FLCK_TRY_TIMEOUT == timeout_usec){
				result = fl_tryrdlock_rwlock(&(pcurrent->lockval));
			}else if(FLCK_NO_TIMEOUT == timeout_usec){
				result = fl_rdlock_rwlock(&(pcurrent->lockval), (FlShm::IsHighRobust() ? FlShm::GetRobustLoopCnt() : FLCK_ROBUST_CHKCNT_NOLIMIT));
			}else{
				struct timespec	timeout = {(timeout_usec / (1000 * 1000)), ((timeout_usec % (1000 * 1000)) * 1000)};
				result = fl_timedrdlock_rwlock(&(pcurrent->lockval), &timeout);
			}
		}else{
			if(FLCK_TRY_TIMEOUT == timeout_usec){
				result = fl_trywrlock_rwlock(&(pcurrent->lockval));
			}else if(FLCK_NO_TIMEOUT == timeout_usec){
				result = fl_wrlock_rwlock(&(pcurrent->lockval), (FlShm::IsHighRobust() ? FlShm::GetRobustLoopCnt() : FLCK_ROBUST_CHKCNT_NOLIMIT));
			}else{
				struct timespec	timeout = {(timeout_usec / (1000 * 1000)), ((timeout_usec % (1000 * 1000)) * 1000)};
				result = fl_timedwrlock_rwlock(&(pcurrent->lockval), &timeout);
			}
		}

		if(0 == result){
			// rwlock succeed
			break;
		}

		// error
		if(EINVAL == result){
			ERR_FLCKPRN("rwlock object does not initialize yet, error code=%d", result);

		}else if(EBUSY == result){
			MSG_FLCKPRN("Could not get rwlock(error code=%d), try lock is failed.", result);

		}else if(EDEADLK == result){
			ERR_FLCKPRN("Could not get rwlock(error code=%d), why come here...", result);

		}else if(ETIMEDOUT == result){
			// timeouted
			if(FLCK_NO_TIMEOUT == timeout_usec){
				// recover
				if(FlShm::IsHighRobust()){
					fl_lock_lockid(&FlShm::pFlHead->file_lock_lockid, flckpid);	// lock lockid for top manually.(keep to lock)
					// set protect flag (for not removing pcurrent)
					set_protect();

					// check dead lock to rwlock
					fl_pid_cache_map_t	cache_map;
					check_dead_lock(devid, inoid, &cache_map, flckpid, fd);		// always success.

					pcurrent->protect = false;
					fl_unlock_lockid(&FlShm::pFlHead->file_lock_lockid, flckpid);	// unlock lockid

					result = 0;					// retry to lock
				}else{
					result = 0;					// retry to lock
				}
			}
		}else{
			ERR_FLCKPRN("Something error occurred during getting rwlock(error code=%d).", result);
		}
	}
	return result;
}

// Returns	false	: does not need to remove this object, it means locking now or null.
//			true	: should remove this object, because this object does not lock any now.
//
bool FlListOffLock::check_dead_lock(dev_t devid, ino_t inoid, fl_pid_cache_map_t* pcache, flckpid_t except_flckpid, int except_fd)
{
	if(!pcurrent){
		ERR_FLCKPRN("Object is not initialized.");
		return false;
	}

	FlListLocker	tmpobj;
	int				result = 0;

	// check reader list
	for(PFLLOCKER pabsparent = NULL, pabscur = to_abs(pcurrent->reader_list); pabscur; ){
		tmpobj.set(pabscur);
		if(tmpobj.check_dead_lock(devid, inoid, pcache, except_flckpid, except_fd)){

			// retrive target list
			if(tmpobj.cutoff_list(pcurrent->reader_list)){
				if(tmpobj.is_locked()){
					// do unlock
					if(0 != (result = fl_unlock_rwlock(&(pcurrent->lockval)))){
						ERR_FLCKPRN("Could not unlock rwlock object(error code=%d), but continue....", result);
					}
				}
				// return object to free list
				if(!tmpobj.insert_list(FlShm::pFlHead->locker_free)){
					ERR_FLCKPRN("Failed to insert locker to free list, but continue...");
				}
			}
			// set next
			if(pabsparent){
				pabscur	= to_abs(pabsparent->next);
			}else{
				pabscur = to_abs(pcurrent->reader_list);
			}
		}else{
			// set next
			pabsparent	= pabscur;
			pabscur		= to_abs(pabscur->next);
		}
	}

	// check writer list
	for(PFLLOCKER pabsparent = NULL, pabscur = to_abs(pcurrent->writer_list); pabscur; ){
		tmpobj.set(pabscur);
		if(tmpobj.check_dead_lock(devid, inoid, pcache, except_flckpid, except_fd)){

			// retrive target list
			if(tmpobj.cutoff_list(pcurrent->writer_list)){
				if(tmpobj.is_locked()){
					// do unlock
					if(0 != (result = fl_unlock_rwlock(&(pcurrent->lockval)))){
						ERR_FLCKPRN("Could not unlock rwlock object(error code=%d), but continue....", result);
					}
				}
				// return object to free list
				if(!tmpobj.insert_list(FlShm::pFlHead->locker_free)){
					ERR_FLCKPRN("Failed to insert locker to free list, but continue...");
				}
			}
			// set next
			if(pabsparent){
				pabscur	= to_abs(pabsparent->next);
			}else{
				pabscur = to_abs(pcurrent->writer_list);
			}
		}else{
			// set next
			pabsparent	= pabscur;
			pabscur		= to_abs(pabscur->next);
		}
	}
	return !is_locked();
}

bool FlListOffLock::free_locker_list(void)
{
	if(!pcurrent){
		ERR_FLCKPRN("Object is not initialized.");
		return false;
	}

	FlListLocker	tmpobj;

	// check reader list
	for(PFLLOCKER pabsnext = NULL, pabscur = to_abs(pcurrent->reader_list); pabscur; pabscur = pabsnext){
		pabsnext = to_abs(pabscur->next);

		// return object to free list
		tmpobj.set(pabscur);
		if(!tmpobj.insert_list(FlShm::pFlHead->locker_free)){
			ERR_FLCKPRN("Failed to insert locker to free list, but continue...");
		}
	}
	pcurrent->reader_list = NULL;

	// check writer list
	for(PFLLOCKER pabsnext = NULL, pabscur = to_abs(pcurrent->writer_list); pabscur; pabscur = pabsnext){
		pabsnext = to_abs(pabscur->next);

		// return object to free list
		tmpobj.set(pabscur);
		if(!tmpobj.insert_list(FlShm::pFlHead->locker_free)){
			ERR_FLCKPRN("Failed to insert locker to free list, but continue...");
		}
	}
	pcurrent->writer_list = NULL;

	return true;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
