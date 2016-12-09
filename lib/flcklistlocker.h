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

#ifndef	FLCKLISTLOCKER_H
#define	FLCKLISTLOCKER_H

#include <iostream>
#include <vector>

#include "flckcommon.h"
#include "flckstructure.h"
#include "flckshm.h"
#include "flckpidcache.h"

//---------------------------------------------------------
// List templates
//---------------------------------------------------------
#include "flckbaselist.tcc"

typedef fullock::fl_list_base<FLLOCKER>		fllistbaselocker;

//---------------------------------------------------------
// FlListLocker class
//---------------------------------------------------------
class FlListLocker : public fllistbaselocker
{
	public:
		FlListLocker(PFLLOCKER ptr = NULL) : fllistbaselocker(ptr) {}
		FlListLocker(const FlListLocker& other) : fllistbaselocker(other) {}
		virtual ~FlListLocker() {}

		inline void initialize(flckpid_t flckpid, int fd, bool locked = false, bool is_all = true)
		{
			if(pcurrent){
				if(is_all){
					pcurrent->next		= NULL;
				}
				pcurrent->flckpid		= flckpid;
				pcurrent->fd			= fd;
				pcurrent->locked		= locked;
			}
		}
		inline void initialize(PFLLOCKER ptr, size_t count) { fllistbaselocker::initialize(ptr, count); }
		inline void initialize(bool is_all) { initialize(FLCK_INVALID_ID, FLCK_INVALID_HANDLE, false, is_all); }
		virtual void initialize(void) { initialize(FLCK_INVALID_ID, FLCK_INVALID_HANDLE, false, true); }
		virtual void dump(std::ostream& out, int level) const;

		inline bool is_locked(void) const { return (pcurrent && pcurrent->locked); }
		inline void set_lock(void) { if(pcurrent){ pcurrent->locked = true; } }
		inline void set_unlock(void) { if(pcurrent){ pcurrent->locked = false; } }

		inline bool find(flckpid_t flckpid, int fd, bool locked, PFLLOCKER& preltop)
		{
			FLLOCKER tmp = {NULL, flckpid, fd, locked};
			return fllistbaselocker::find(&tmp, preltop);
		}

		bool check_dead_lock(dev_t devid, ino_t inoid, fl_pid_cache_map_t* pcache = NULL, flckpid_t except_flckpid = FLCK_INVALID_ID, int except_fd = FLCK_INVALID_HANDLE);
};

//---------------------------------------------------------
// Utility functions
//---------------------------------------------------------
// result	0	: same
//			-1	: psrc2 is smaller than psrc1
//			1	: psrc2 is larger than psrc1
inline int fl_compare_list_base(const PFLLOCKER psrc1, const PFLLOCKER psrc2)
{
	if(psrc1->flckpid < psrc2->flckpid){
		return -1;
	}else if(psrc2->flckpid < psrc1->flckpid){
		return 1;
	}else{			// psrc1->flckpid == psrc2->flckpid
		if(psrc1->fd < psrc2->fd){
			return -1;
		}else if(psrc2->fd < psrc1->fd){
			return 1;
		}else{
			if(psrc1->locked){
				if(!psrc2->locked){
					return -1;
				}
			}else{
				if(psrc2->locked){
					return 1;
				}
			}
		}
	}
	return 0;
}

#endif	// FLCKLISTLOCKER_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
