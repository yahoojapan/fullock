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

#ifndef	FLCKLISTNMTX_H
#define	FLCKLISTNMTX_H

#include <string.h>
#include <iostream>
#include <vector>

#include "flckcommon.h"
#include "flckstructure.h"
#include "flckshm.h"
#include "flckdbg.h"

//---------------------------------------------------------
// List templates
//---------------------------------------------------------
#include "flckbaselist.tcc"

typedef fullock::fl_list_base<FLNAMEDMUTEX>	fllistbasenmtx;

//---------------------------------------------------------
// FlListNMtx class
//---------------------------------------------------------
class FlListNMtx : public fllistbasenmtx
{
	protected:
		int rawlock(FLCKLOCKTYPE LockType, time_t timeout_usec);

	public:
		FlListNMtx(PFLNAMEDMUTEX ptr = NULL) : fllistbasenmtx(ptr) {}
		FlListNMtx(const FlListNMtx& other) : fllistbasenmtx(other) {}
		virtual ~FlListNMtx() {}

		void initialize(const char* pname, bool is_all = true)
		{
			if(pcurrent){
				if(is_all){
					pcurrent->next		= NULL;
				}
				if(!FLCKEMPTYSTR(pname)){
					pcurrent->name[FLCK_NAMED_MUTEX_MAXLENGTH] = '\0';
					strncpy(pcurrent->name, pname, FLCK_NAMED_MUTEX_MAXLENGTH);
					pcurrent->hash = flck_fnv_hash(&pcurrent->name[0], strlen(pcurrent->name));
				}else{
					pcurrent->name[0]	= '\0';
					pcurrent->hash		= 0;
				}
				// initialize mutex
				if(is_all){
					pcurrent->lockval	= FLCK_INVALID_ID;
					pcurrent->lockcnt	= 0;
				}
			}
		}
		inline void initialize(PFLNAMEDMUTEX ptr, size_t count) { fllistbasenmtx::initialize(ptr, count); }
		inline void initialize(bool is_all) { initialize(NULL, is_all); }
		virtual void initialize(void) { initialize(NULL, true); }
		virtual void dump(std::ostream& out, int level) const;

		inline bool find(const char* pname, PFLNAMEDMUTEX& preltop)
		{
			FLNAMEDMUTEX 	tmp;
			if(!FLCKEMPTYSTR(pname)){
				tmp.name[FLCK_NAMED_MUTEX_MAXLENGTH] = '\0';
				strncpy(tmp.name, pname, FLCK_NAMED_MUTEX_MAXLENGTH);
				tmp.hash = flck_fnv_hash(&tmp.name[0], strlen(tmp.name));
			}else{
				tmp.name[0]	= '\0';
				tmp.hash	= 0;
			}
			return fllistbasenmtx::find(&tmp, preltop);
		}

		inline int lock(time_t timeout_usec = FLCK_NO_TIMEOUT) { return rawlock(FLCK_NMTX_LOCK, timeout_usec); }
		inline int unlock(void) { return rawlock(FLCK_UNLOCK, FLCK_NO_TIMEOUT); }

		bool check_dead_lock(fl_pid_cache_map_t* pcache = NULL, flckpid_t except_flckpid = FLCK_INVALID_ID);
};

//---------------------------------------------------------
// Utility functions
//---------------------------------------------------------
// result	0	: same
//			-1	: psrc2 is smaller than psrc1
//			1	: psrc2 is larger than psrc1
inline int fl_compare_list_base(const PFLNAMEDMUTEX psrc1, const PFLNAMEDMUTEX psrc2)
{
	if(psrc1->hash < psrc2->hash){
		return -1;
	}else if(psrc1->hash > psrc2->hash){
		return 1;
	}
	return strncmp(psrc1->name, psrc2->name, FLCK_NAMED_MUTEX_MAXLENGTH);
}

#endif	// FLCKLISTNMTX_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
