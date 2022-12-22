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
 * CREATE:   Wed 29 Jul 2015
 * REVISION:
 *
 */

#include "flcklistwaiter.h"
#include "flcklistnmtx.h"
#include "flckutil.h"
#include "flckdbg.h"

using namespace std;
using namespace fullock;

//---------------------------------------------------------
// FlListWaiter class
//---------------------------------------------------------
void FlListWaiter::dump(std::ostream &out, int level) const
{
	if(!pcurrent){
		return;
	}
	string	spacer1 = spaces_string(level);
	string	spacer2 = spaces_string(level + 1);

	out << spacer1 << "FLWAITER(" << to_hexstring(pcurrent) << ")={" << std::endl;

	fllistbasewaiter::dump(out, level + 1);

	out << spacer2 << "flckpid           = " << pcurrent->flckpid	<< std::endl;
	out << spacer2 << "lockstatus        = " << STR_FLCKCONDTYPE(pcurrent->lockstatus) << std::endl;

	out << spacer2 << "named_mutex={" << std::endl;
	FlListNMtx	tmpobj(to_abs(pcurrent->named_mutex));
	tmpobj.dump(out, level + 2);
	out << spacer2 << "}" << std::endl;

	out << spacer1 << "}" << std::endl;
}

int FlListWaiter::rawlock(FLCKLOCKTYPE LockType, time_t timeout_usec)
{
	if(!pcurrent){
		ERR_FLCKPRN("Object is not initialized.");
		return EINVAL;						// EINVAL
	}

	int	result = 0;
	if(FLCK_NCOND_UP == LockType){
		// SIGNAL
		result = fl_force_set_cond(&(pcurrent->lockstatus), FLCK_NCOND_UP);

	}else{
		// FLCK_NCOND_WAIT

		// get named mutex.
		if(!pcurrent->named_mutex){
			ERR_FLCKPRN("Object does not have valid named mutex.");
			return EINVAL;					// EINVAL
		}
		FlListNMtx	tgnmtxobj(to_abs(pcurrent->named_mutex));

		// set lock status to wait
		fl_force_set_cond(&(pcurrent->lockstatus), FLCK_NCOND_WAIT);					// always returns 0

		// do unlock named mutex
		if(0 != (result = tgnmtxobj.unlock())){
			ERR_FLCKPRN("Could not unlock named mutex(error code=%d) for cond.", result);

			// for recover
			fl_force_set_cond(&(pcurrent->lockstatus), FLCK_NCOND_UP);
			return result;
		}

		// do wait
		if(FLCK_NO_TIMEOUT == timeout_usec){
			result = fl_wait_cond(&(pcurrent->lockstatus), FLCK_NCOND_UP);
		}else{
			struct timespec	timeout = {(timeout_usec / (1000 * 1000)), ((timeout_usec % (1000 * 1000)) * 1000)};
			result = fl_timedwait_cond(&(pcurrent->lockstatus), FLCK_NCOND_UP, &timeout);
		}

		// do lock named mutex
		int	subresult;
		if(0 != (subresult = tgnmtxobj.lock(timeout_usec))){
			ERR_FLCKPRN("Could not lock named mutex object(error code=%d) for cond.", subresult);
			// not need to recover on this case.
			if(0 == result){
				result = subresult;
			}
			return result;
		}
	}
	return result;
}

bool FlListWaiter::check_dead_lock(fl_pid_cache_map_t* pcache, flckpid_t except_flckpid)
{
	if(!pcurrent){
		ERR_FLCKPRN("Object is not initialized.");
		return false;
	}

	flckpid_t	flckpid = pcurrent->flckpid;
	if(flckpid == except_flckpid){
		return false;
	}

	// check thread(process) dead.
	pid_t	pid = decompose_pid(flckpid);
	tid_t	tid = decompose_tid(flckpid);
	if(FindThreadProcess(pid, tid, pcache)){
		return false;
	}

	// thread(process) does not run, so waiter is dead.
	return true;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
