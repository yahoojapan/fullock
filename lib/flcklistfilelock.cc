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
 * CREATE:   Mon 6 Jul 2015
 * REVISION:
 *
 */

#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include "flcklistfilelock.h"
#include "flcklistofflock.h"
#include "flckutil.h"
#include "flckdbg.h"

using namespace std;
using namespace fullock;

//---------------------------------------------------------
// FlListFileLock class
//---------------------------------------------------------
void FlListFileLock::dump(std::ostream &out, int level) const
{
	if(!pcurrent){
		return;
	}
	string	spacer1 = spaces_string(level);
	string	spacer2 = spaces_string(level + 1);

	out << spacer1 << "FLFILELOCK(" << to_hexstring(pcurrent) << ")={" << std::endl;

	fllistbasefilelock::dump(out, level + 1);

	out << spacer2 << "dev_id            = " << pcurrent->dev_id	<< std::endl;
	out << spacer2 << "ino_id            = " << pcurrent->ino_id	<< std::endl;
	out << spacer2 << "protect           = " << (pcurrent->protect ? "true" : "false") << std::endl;

	FlListOffLock	tmpobj;

	out << spacer2 << "offset_lock_list={" << std::endl;
	for(PFLOFFLOCK ptmp = to_abs(pcurrent->offset_lock_list); ptmp; ptmp = to_abs(ptmp->next)){
		tmpobj.set(ptmp);
		tmpobj.dump(out, level + 2);
	}
	out << spacer2 << "}" << std::endl;

	out << spacer1 << "}" << std::endl;
}

int FlListFileLock::rawlock(FLCKLOCKTYPE LockType, flckpid_t flckpid, int fd, off_t offset, size_t length, time_t timeout_usec)
{
	if(!pcurrent){
		ERR_FLCKPRN("Object is not initialized.");
		return ENOLCK;						// ENOLCK
	}
	if(FLCK_INVALID_ID == flckpid){
		flckpid = get_flckpid();
	}

	int	result	= 0;
	if(FLCK_UNLOCK == LockType){
		// UNLOCK
		FlListOffLock	tglistobj;
		if(!tglistobj.find(offset, length, pcurrent->offset_lock_list)){
			// not found target.
			ERR_FLCKPRN("Could not find locking offset object for pid(%d), tid(%d), fd(%d), offset(%zd), length(%zu).", decompose_pid(flckpid), decompose_tid(flckpid), fd, offset, length);
			return EINVAL;						// EINVAL
		}

		// do unlock(unlocked lockid after this)
		if(0 != (result = tglistobj.unlock(flckpid, fd))){
			ERR_FLCKPRN("Could not unlock offset object(error code=%d) for pid(%d), tid(%d), fd(%d), offset(%zd), length(%zu).", result, decompose_pid(flckpid), decompose_tid(flckpid), fd, offset, length);
			return result;
		}

		// check free
		if(FlShm::IsFreeUnitOffset()){
			if(!tglistobj.is_locked()){
				// retrieve target list
				if(tglistobj.cutoff_list(pcurrent->offset_lock_list)){
					// return object to free list
					if(!tglistobj.insert_list(FlShm::pFlHead->offset_lock_free)){
						ERR_FLCKPRN("Failed to insert offset lock to free list, but continue...");
					}
				}
			}
		}

	}else{
		// LOCK
		FlListOffLock	tglistobj;
		if(!tglistobj.find(offset, length, pcurrent->offset_lock_list)){
			// Not found, so get new offset lock and insert it.
			if(!tglistobj.retrieve_list(FlShm::pFlHead->offset_lock_free)){
				ERR_FLCKPRN("Could not get free offset lock structure.");
				fl_unlock_lockid(&FlShm::pFlHead->file_lock_lockid, flckpid);		// unlock lockid
				return ENOLCK;					// ENOLCK
			}
			// initialize
			tglistobj.initialize(offset, length, true, true);

			// insert offset lock into list
			if(!tglistobj.insert_list(pcurrent->offset_lock_list)){
				ERR_FLCKPRN("Failed to insert offset lock to top list.");
				// for recover
				if(!tglistobj.insert_list(FlShm::pFlHead->offset_lock_free)){
					ERR_FLCKPRN("Failed to insert offset lock to free list, but continue...");
				}
				fl_unlock_lockid(&FlShm::pFlHead->file_lock_lockid, flckpid);		// unlock lockid
				return ENOLCK;					// ENOLCK
			}
		}else{
			tglistobj.set_protect();
		}
		// clear protect flag(because offset lock list is existed now)
		pcurrent->protect = false;

		// do lock(unlocked lockid after this )
		if(0 != (result = tglistobj.lock(LockType, pcurrent->dev_id, pcurrent->ino_id, flckpid, fd, timeout_usec))){
			ERR_FLCKPRN("Could not lock offset object(error code=%d) for pid(%d), tid(%d), fd(%d), offset(%zd), length(%zu), devid(%lu), inode(%lu).", result, decompose_pid(flckpid), decompose_tid(flckpid), fd, offset, length, pcurrent->dev_id, pcurrent->ino_id);

			// check remove offset lock for recover...
			//
			if(FlShm::IsFreeUnitOffset()){
				fl_lock_lockid(&FlShm::pFlHead->file_lock_lockid, flckpid);	// lock lockid

				if(tglistobj.find(offset, length, pcurrent->offset_lock_list)){
					if(!tglistobj.is_locked()){
						// retrieve target list
						if(tglistobj.cutoff_list(pcurrent->offset_lock_list)){
							// return object to free list
							if(!tglistobj.insert_list(FlShm::pFlHead->offset_lock_free)){
								ERR_FLCKPRN("Failed to insert offset lock to free list, but continue...");
							}
						}
					}
				}
				fl_unlock_lockid(&FlShm::pFlHead->file_lock_lockid, flckpid);	// unlock lockid
			}
			return result;
		}
	}
	return result;
}

// Returns	false	: does not need to remove this object, it means locking now or null.
//			true	: should remove this object, because this object does not lock any now.
//
bool FlListFileLock::check_dead_lock(fl_pid_cache_map_t* pcache, flckpid_t except_flckpid, int except_fd)
{
	if(!pcurrent){
		ERR_FLCKPRN("Object is not initialized.");
		return false;
	}
	FlListOffLock	tmpobj;

	// check offset list
	for(PFLOFFLOCK pabsparent = NULL, pabscur = to_abs(pcurrent->offset_lock_list); pabscur; ){
		tmpobj.set(pabscur);
		if(tmpobj.check_dead_lock(pcurrent->dev_id, pcurrent->ino_id, pcache, except_flckpid, except_fd)){
			// retrieve target list
			if(tmpobj.cutoff_list(pcurrent->offset_lock_list)){
				// return object to free list
				if(!tmpobj.insert_list(FlShm::pFlHead->offset_lock_free)){
					ERR_FLCKPRN("Failed to insert offset lock to free list, but continue...");
				}
			}
			// set next
			if(pabsparent){
				pabscur	= to_abs(pabsparent->next);
			}else{
				pabscur = to_abs(pcurrent->offset_lock_list);
			}
		}else{
			// set next
			pabsparent	= pabscur;
			pabscur		= to_abs(pabscur->next);
		}
	}
	return !is_locked();
}

//
// Need to lock list before calling this.
//
bool FlListFileLock::is_locked(void) const
{
	if(!pcurrent){
		return false;
	}
	if(pcurrent->protect){
		return true;
	}
	// check offset list
	FlListOffLock	tmpobj;
	for(PFLOFFLOCK pabscur = to_abs(pcurrent->offset_lock_list); pabscur; pabscur = to_abs(pabscur->next)){
		tmpobj.set(pabscur);
		if(tmpobj.is_locked()){
			return true;
		}
	}
	return false;
}

bool FlListFileLock::free_offset_lock_list(void)
{
	if(!pcurrent){
		ERR_FLCKPRN("Object is not initialized.");
		return false;
	}
	FlListOffLock	tmpobj;

	// check offset list
	for(PFLOFFLOCK pabsnext = NULL, pabscur = to_abs(pcurrent->offset_lock_list); pabscur; pabscur = pabsnext){
		pabsnext = to_abs(pabscur->next);

		tmpobj.set(pabscur);
		tmpobj.free_locker_list();

		// return object to free list
		if(!tmpobj.insert_list(FlShm::pFlHead->offset_lock_free)){
			ERR_FLCKPRN("Failed to insert offset lock to free list, but continue...");
		}
	}
	pcurrent->offset_lock_list = NULL;

	return true;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
