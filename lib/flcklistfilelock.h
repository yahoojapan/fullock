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

#ifndef	FLCKLISTFILELOCK_H
#define	FLCKLISTFILELOCK_H

#include <iostream>
#include <vector>

#include "flckcommon.h"
#include "flckstructure.h"
#include "flckshm.h"

//---------------------------------------------------------
// List templates
//---------------------------------------------------------
#include "flckbaselist.tcc"

typedef fullock::fl_list_base<FLFILELOCK>	fllistbasefilelock;

//---------------------------------------------------------
// FlListFileLock class
//---------------------------------------------------------
class FlListFileLock : public fllistbasefilelock
{
	protected:
		int rawlock(FLCKLOCKTYPE LockType, flckpid_t flckpid, int fd, off_t offset, size_t length, time_t timeout_usec);

	public:
		FlListFileLock(PFLFILELOCK ptr = NULL) : fllistbasefilelock(ptr) {}
		FlListFileLock(const FlListFileLock& other) : fllistbasefilelock(other) {}
		virtual ~FlListFileLock() {}

		void initialize(dev_t dev_id, ino_t ino_id, bool protect = true, bool is_all = true)
		{
			if(pcurrent){
				if(is_all){
					pcurrent->next			= NULL;
				}
				pcurrent->dev_id			= dev_id;
				pcurrent->ino_id			= ino_id;
				pcurrent->offset_lock_list	= NULL;
				pcurrent->protect			= protect;
			}
		}
		inline void initialize(PFLFILELOCK ptr, size_t count) { fllistbasefilelock::initialize(ptr, count); }
		inline void initialize(bool is_all) { initialize(static_cast<dev_t>(FLCK_INVALID_ID), static_cast<ino_t>(FLCK_INVALID_ID), is_all); }
		virtual void initialize(void) { initialize(static_cast<dev_t>(FLCK_INVALID_ID), static_cast<ino_t>(FLCK_INVALID_ID), true); }
		virtual void dump(std::ostream& out, int level) const;

		inline bool find(dev_t dev_id, ino_t ino_id, PFLFILELOCK& preltop)
		{
			FLFILELOCK tmp = {NULL, dev_id, ino_id, NULL, false};
			return fllistbasefilelock::find(&tmp, preltop);
		}

		bool is_locked(void) const;
		inline void set_protect(void) { if(pcurrent){ pcurrent->protect = true; } }
		bool free_offset_lock_list(void);

		inline int lock(FLCKLOCKTYPE LockType, flckpid_t flckpid, int fd, off_t offset, size_t length, time_t timeout_usec = FLCK_NO_TIMEOUT) { return rawlock(LockType, flckpid, fd, offset, length, timeout_usec); }
		inline int unlock(flckpid_t flckpid, int fd, off_t offset, size_t length) { return rawlock(FLCK_UNLOCK, flckpid, fd, offset, length, FLCK_NO_TIMEOUT); }

		bool check_dead_lock(fl_pid_cache_map_t* pcache = NULL, flckpid_t except_flckpid = FLCK_INVALID_ID, int except_fd = FLCK_INVALID_HANDLE);
};

//---------------------------------------------------------
// Utility functions
//---------------------------------------------------------
// result	0	: same
//			-1	: psrc2 is smaller than psrc1
//			1	: psrc2 is larger than psrc1
inline int fl_compare_list_base(const PFLFILELOCK psrc1, const PFLFILELOCK psrc2)
{
	int result = 0;
	if(psrc1->dev_id < psrc2->dev_id){
		result = 1;
	}else if(psrc1->dev_id > psrc2->dev_id){
		result = -1;
	}else{	// psrc1->dev_id == psrc2->dev_id
		if(psrc1->ino_id < psrc2->ino_id){
			result = 1;
		}else if(psrc1->ino_id > psrc2->ino_id){
			result = -1;
		}else{	// psrc1->ino_id == psrc2->ino_id
			result = 0;
		}
	}
	return result;
}

#endif	// FLCKLISTFILELOCK_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
