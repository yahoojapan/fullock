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

#ifndef	FLCKLISTWAITER_H
#define	FLCKLISTWAITER_H

#include <iostream>
#include <vector>

#include "flckcommon.h"
#include "flckstructure.h"
#include "flckshm.h"

//---------------------------------------------------------
// List templates
//---------------------------------------------------------
#include "flckbaselist.tcc"

typedef fullock::fl_list_base<FLWAITER>		fllistbasewaiter;

//---------------------------------------------------------
// FlListWaiter class
//---------------------------------------------------------
class FlListWaiter : public fllistbasewaiter
{
	protected:
		int rawlock(FLCKLOCKTYPE LockType, time_t timeout_usec);

	public:
		explicit FlListWaiter(PFLWAITER ptr = NULL) : fllistbasewaiter(ptr) {}
		FlListWaiter(const FlListWaiter& other) : fllistbasewaiter(other) {}
		virtual ~FlListWaiter() {}

		inline void initialize(flckpid_t flckpid, FLCKLOCKTYPE lockstatus = FLCK_NCOND_UP, PFLNAMEDMUTEX abs_nmtx = NULL, bool is_all = true)
		{
			if(pcurrent){
				if(is_all){
					pcurrent->next		= NULL;
				}
				pcurrent->flckpid		= flckpid;
				pcurrent->lockstatus	= lockstatus;
				pcurrent->named_mutex	= to_rel(abs_nmtx);
			}
		}
		virtual bool initialize(PFLWAITER ptr, size_t count) { return fllistbasewaiter::initialize(ptr, count); }
		inline void initialize(bool is_all) { initialize(FLCK_INVALID_ID, FLCK_NCOND_UP, NULL, is_all); }
		virtual void initialize(void) { initialize(FLCK_INVALID_ID, FLCK_NCOND_UP, NULL, true); }
		virtual void dump(std::ostream& out, int level) const;

		inline bool is_wait(void) const { return (pcurrent && FLCK_NCOND_WAIT == pcurrent->lockstatus); }

		// search in waiter list, only check lockstatus and return lastest value.
		inline bool rfind(FLCKLOCKTYPE lockstatus, PFLWAITER& preltop)
		{
			FLWAITER tmp = {NULL, FLCK_INVALID_ID, lockstatus, NULL};
			return fllistbasewaiter::rfind(&tmp, preltop);
		}

		inline int wait(time_t timeout_usec = FLCK_NO_TIMEOUT) { return rawlock(FLCK_NCOND_WAIT, timeout_usec); }
		inline int signal(void) { return rawlock(FLCK_NCOND_UP, FLCK_NO_TIMEOUT); }

		bool check_dead_lock(fl_pid_cache_map_t* pcache = NULL, flckpid_t except_flckpid = FLCK_INVALID_ID);
};

//---------------------------------------------------------
// Utility functions
//---------------------------------------------------------
// result	0	: same
//			-1	: psrc2 is smaller than psrc1
//			1	: psrc2 is larger than psrc1
//
// [NOTE] check only lockstatus
//
inline int fl_compare_list_base(const PFLWAITER psrc1, const PFLWAITER psrc2)
{
	if(psrc1->lockstatus < psrc2->lockstatus){
		return -1;
	}else if(psrc2->lockstatus < psrc1->lockstatus){
		return 1;
	}
	return 0;
}

#endif	// FLCKLISTWAITER_H

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
