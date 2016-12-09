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
 * CREATE:   Tue 7 Jul 2015
 * REVISION:
 *
 */

#include <errno.h>
#include <time.h>

#include "flcklistnmtx.h"
#include "flckutil.h"
#include "flckdbg.h"

using namespace std;
using namespace fullock;

//---------------------------------------------------------
// FlListNMtx class
//---------------------------------------------------------
void FlListNMtx::dump(std::ostream &out, int level) const
{
	if(!pcurrent){
		return;
	}
	string	spacer1 = spaces_string(level);
	string	spacer2 = spaces_string(level + 1);

	out << spacer1 << "FLNAMEDMUTEX(" << to_hexstring(pcurrent) << ")={" << std::endl;

	fllistbasenmtx::dump(out, level + 1);

	out << spacer2 << "hash              = "	<< to_hexstring(pcurrent->hash)	<< std::endl;
	out << spacer2 << "name              = \""	<< pcurrent->name				<< "\""<< std::endl;

	out << spacer1 << "}" << std::endl;
}

int FlListNMtx::rawlock(FLCKLOCKTYPE LockType, time_t timeout_usec)
{
	if(!pcurrent){
		ERR_FLCKPRN("Object is not initalized.");
		return EINVAL;						// EINVAL
	}

	flckpid_t	flckpid	= get_flckpid();
	int			result	= 0;
	if(FLCK_UNLOCK == LockType){
		// UNLOCK
		if(0 != (result = fl_unlock_mutex(&(pcurrent->lockval), &(pcurrent->lockcnt), flckpid))){
			ERR_FLCKPRN("Could not unlock mutex(error code=%d), but continue...", result);
		}

	}else{
		// LOCK
		do{
			if(FLCK_NO_TIMEOUT == timeout_usec){
				result = fl_lock_mutex(&(pcurrent->lockval), &(pcurrent->lockcnt), flckpid, (FlShm::IsHighRobust() ? FlShm::GetRobustLoopCnt() : FLCK_ROBUST_CHKCNT_NOLIMIT));
			}else if(FLCK_TRY_TIMEOUT == timeout_usec){
				result = fl_trylock_mutex(&(pcurrent->lockval), &(pcurrent->lockcnt), flckpid);
			}else{
				struct timespec	timeout = {(timeout_usec / (1000 * 1000)), ((timeout_usec % (1000 * 1000)) * 1000)};
				result = fl_timedlock_mutex(&(pcurrent->lockval), &(pcurrent->lockcnt), flckpid, &timeout);
			}
			if(0 != result){
				if(EBUSY == result){
					ERR_FLCKPRN("Could not get mutex(by trylock) or clock_gettime error(timeoutlock), error code=%d.", result);

				}else if(ETIMEDOUT == result){
					//MSG_FLCKPRN("Could not get mutex by timeouted, error code=%d", result);

				}else if(EWOULDBLOCK == result){
					// On robust mode, need to check deadlock
					FlShm::CheckMutexDeadLock(NULL, flckpid, flckpid);
				}else{
					ERR_FLCKPRN("Could not get mutex by unkown error, error code=%d", result);
				}
			}
		}while(EWOULDBLOCK == result);
	}
	return result;
}

// Returns	false	: does not dead lock
//			true	: this object is dead lock and force unlock this.
//
bool FlListNMtx::check_dead_lock(fl_pid_cache_map_t* pcache, flckpid_t except_flckpid)
{
	if(!pcurrent){
		ERR_FLCKPRN("Object is not initialized.");
		return false;
	}

	flck_mutex_t	lockval = pcurrent->lockval;
	if(lockval == except_flckpid || FLCK_MUTEX_UNLOCK == lockval){
		return false;
	}

	// check thread(process) dead.
	pid_t	pid = decompose_pid(lockval);
	tid_t	tid = decompose_tid(lockval);
	if(FindThreadProcess(pid, tid, pcache)){
		return false;
	}

	// thread(process) does not run, so mutex is dead lock
	// do force unlock
	fl_force_unlock_mutex(&(pcurrent->lockval), &(pcurrent->lockcnt));

	return true;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
