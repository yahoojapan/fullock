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

#include <errno.h>
#include <time.h>

#include "flcklistncond.h"
#include "flcklistwaiter.h"
#include "flckutil.h"
#include "flckdbg.h"

using namespace std;
using namespace fullock;

//---------------------------------------------------------
// FlListNCond class
//---------------------------------------------------------
void FlListNCond::dump(std::ostream &out, int level) const
{
	if(!pcurrent){
		return;
	}
	string	spacer1 = spaces_string(level);
	string	spacer2 = spaces_string(level + 1);

	out << spacer1 << "FLNAMEDCOND(" << to_hexstring(pcurrent) << ")={" << std::endl;

	fllistbasencond::dump(out, level + 1);

	out << spacer2 << "hash              = "	<< to_hexstring(pcurrent->hash)	<< std::endl;
	out << spacer2 << "name              = \""	<< pcurrent->name				<< "\""<< std::endl;

	FlListWaiter	tmpobj;

	out << spacer2 << "waiter_list={" << std::endl;
	for(PFLWAITER ptmp = to_abs(pcurrent->waiter_list); ptmp; ptmp = to_abs(ptmp->next)){
		tmpobj.set(ptmp);
		tmpobj.dump(out, level + 2);
	}
	out << spacer2 << "}" << std::endl;

	out << spacer1 << "}" << std::endl;
}

int FlListNCond::rawlock(FLCKLOCKTYPE LockType, bool is_broadcast, PFLNAMEDMUTEX abs_nmtx, time_t timeout_usec)
{
	if(!pcurrent){
		ERR_FLCKPRN("Object is not initalized.");
		return EINVAL;						// EINVAL
	}

	flckpid_t	flckpid	= get_flckpid();
	int			result	= 0;
	if(FLCK_NCOND_UP == LockType){
		// SIGNAL or BROADCAST

		if(is_broadcast){
			// BROADCAST
			FlListWaiter	tmpobj;
			int				subresult;
			for(PFLWAITER ptmp = to_abs(pcurrent->waiter_list); ptmp; ptmp = to_abs(ptmp->next)){
				tmpobj.set(ptmp);
				if(tmpobj.is_wait()){
					// wake up waiter
					if(0 != (subresult = tmpobj.signal())){
						ERR_FLCKPRN("Failed to send signal to waiter(error code=%d).", subresult);
						if(0 == result){
							result = subresult;
						}
					}
				}
			}

		}else{	// SIGNAL
			// get end of waiter list
			FlListWaiter	tglistobj;
			if(!tglistobj.rfind(FLCK_NCOND_WAIT, pcurrent->waiter_list)){
				WAN_FLCKPRN("Could not get end of waiter list, maybe no waiter.");
				return 0;					// Success
			}

			// wake up waiter
			if(0 != (result = tglistobj.signal())){
				ERR_FLCKPRN("Failed to send signal to waiter.");
			}
		}

	}else{
		// FLCK_NCOND_WAIT
		if(!abs_nmtx){
			ERR_FLCKPRN("Named mutex for cond is NULL.");
			fl_unlock_lockid(&FlShm::pFlHead->named_cond_lockid, flckpid);			// unlock lockid
			return EINVAL;					// EINVAL
		}

		// Always get new waiter object.
		FlListWaiter	tglistobj;
		if(!tglistobj.retreive_list(FlShm::pFlHead->waiter_free)){
			ERR_FLCKPRN("Could not get waiter structure.");
			fl_unlock_lockid(&FlShm::pFlHead->named_cond_lockid, flckpid);			// unlock lockid
			return ENOLCK;					// ENOLCK
		}
		// initialize
		tglistobj.initialize(flckpid, FLCK_NCOND_WAIT, abs_nmtx, false);

		// insert waiter into list
		if(!tglistobj.insert_list(pcurrent->waiter_list)){
			ERR_FLCKPRN("Failed to insert waiter to top list.");

			// for recover
			if(!tglistobj.insert_list(FlShm::pFlHead->waiter_free)){
				ERR_FLCKPRN("Failed to insert waiter to free list, but continue...");
			}
			fl_unlock_lockid(&FlShm::pFlHead->named_cond_lockid, flckpid);			// unlock lockid
			return ENOLCK;					// ENOLCK
		}
		fl_unlock_lockid(&FlShm::pFlHead->named_cond_lockid, flckpid);				// unlock lockid

		// do wait
		result = tglistobj.wait(timeout_usec);

		// retreive waiter from list
		fl_lock_lockid(&FlShm::pFlHead->named_cond_lockid, flckpid);				// relock lockid
		if(tglistobj.cutoff_list(pcurrent->waiter_list)){
			// put back waiter to free
			if(!tglistobj.insert_list(FlShm::pFlHead->waiter_free)){
				ERR_FLCKPRN("Failed to insert waiter to free list, but continue...");
			}
		}else{
			ERR_FLCKPRN("Could not retreive waiter from list, but continue...");
			if(0 == result){
				result = ENOLCK;			// ENOLCK
			}
		}
		fl_unlock_lockid(&FlShm::pFlHead->named_cond_lockid, flckpid);				// unlock lockid
	}
	return result;
}

bool FlListNCond::check_dead_lock(fl_pid_cache_map_t* pcache, flckpid_t except_flckpid)
{
	if(!pcurrent){
		ERR_FLCKPRN("Object is not initialized.");
		return false;
	}

	FlListWaiter	tmpobj;
	bool			is_deadlock_found = false;
	for(PFLWAITER pabsparent = NULL, pabscur = to_abs(pcurrent->waiter_list); pabscur; ){
		tmpobj.set(pabscur);
		if(tmpobj.check_dead_lock(pcache, except_flckpid)){
			// retrive target list
			if(tmpobj.cutoff_list(pcurrent->waiter_list)){
				// return object to free list
				if(!tmpobj.insert_list(FlShm::pFlHead->waiter_free)){
					ERR_FLCKPRN("Failed to insert waiter to free list, but continue...");
				}
			}

			// set next
			if(pabsparent){
				pabscur	= to_abs(pabsparent->next);
			}else{
				pabscur = to_abs(pcurrent->waiter_list);
			}
			is_deadlock_found = true;
		}else{
			// set next
			pabsparent	= pabscur;
			pabscur		= to_abs(pabscur->next);
		}
	}
	return is_deadlock_found;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
