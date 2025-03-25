/*
 * FULLOCK - Fast User Level LOCK library by Yahoo! JAPAN
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

#ifndef	FLCKLISTOFFLOCK_H
#define	FLCKLISTOFFLOCK_H

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

typedef fullock::fl_list_base<FLOFFLOCK>	fllistbaseofflock;

//---------------------------------------------------------
// FlListOffLock class
//---------------------------------------------------------
class FlListOffLock : public fllistbaseofflock
{
	protected:
		int rawlock(FLCKLOCKTYPE LockType, dev_t devid, ino_t inoid, flckpid_t flckpid, int fd, time_t timeout_usec);
		int dolock(FLCKLOCKTYPE LockType, dev_t devid, ino_t inoid, flckpid_t flckpid, int fd, time_t timeout_usec);

	public:
		explicit FlListOffLock(PFLOFFLOCK ptr = NULL) : fllistbaseofflock(ptr) {}
		FlListOffLock(const FlListOffLock& other) : fllistbaseofflock(other) {}
		virtual ~FlListOffLock() {}

		void initialize(off_t offset, size_t length, bool protect = true, bool is_all = true)
		{
			if(pcurrent){
				if(is_all){
					pcurrent->next		= NULL;
				}
				pcurrent->offset		= offset;
				pcurrent->length		= length;
				pcurrent->reader_list	= NULL;
				pcurrent->writer_list	= NULL;
				pcurrent->protect		= protect;

				// initialize lock variable
				if(is_all){
					pcurrent->lockval	= FLCK_RWLOCK_UNLOCK;
				}
			}
		}
		virtual bool initialize(PFLOFFLOCK ptr, size_t count) { return fllistbaseofflock::initialize(ptr, count); }
		inline void initialize(bool is_all) { initialize(static_cast<off_t>(0), static_cast<size_t>(0), is_all); }
		virtual void initialize(void) { initialize(static_cast<off_t>(0), static_cast<size_t>(0), true); }
		virtual void dump(std::ostream& out, int level) const;

		inline bool find(off_t offset, size_t length, PFLOFFLOCK& preltop)
		{
			FLOFFLOCK tmp;
			tmp.next		= NULL;
			tmp.offset		= offset;
			tmp.length		= length;
			tmp.reader_list	= NULL;
			tmp.writer_list	= NULL;
			tmp.protect		= false;
			return fllistbaseofflock::find(&tmp, preltop);
		}

		// If pcurrent has reader/writer list, it means locking now.
		// Need to lock list before calling this.
		//
		inline bool is_locked(void) const { return (pcurrent && (pcurrent->protect || pcurrent->reader_list || pcurrent->writer_list)); }
		inline void set_protect(void) { if(pcurrent){ pcurrent->protect = true; } }
		bool free_locker_list(void);

		inline int lock(FLCKLOCKTYPE LockType, dev_t devid, ino_t inoid, flckpid_t flckpid, int fd, time_t timeout_usec = FLCK_NO_TIMEOUT) { return rawlock(LockType, devid, inoid, flckpid, fd, timeout_usec); }
		inline int unlock(flckpid_t flckpid, int fd) { return rawlock(FLCK_UNLOCK, FLCK_INVALID_ID, FLCK_INVALID_ID, flckpid, fd, FLCK_NO_TIMEOUT); }

		bool check_dead_lock(dev_t devid, ino_t inoid, fl_pid_cache_map_t* pcache = NULL, flckpid_t except_flckpid = FLCK_INVALID_ID, int except_fd = FLCK_INVALID_HANDLE);
};

//---------------------------------------------------------
// Utility functions
//---------------------------------------------------------
// result	0	: same
//			-1	: psrc2 is smaller than psrc1
//			1	: psrc2 is larger than psrc1

// cppcheck-suppress unmatchedSuppression
// cppcheck-suppress constParameterPointer
inline int fl_compare_list_base(const PFLOFFLOCK psrc1, const PFLOFFLOCK psrc2)
{
	int	result = 0;
	if(psrc1->offset < psrc2->offset){
		// check overlap
		if((psrc1->offset + psrc1->length) >= (psrc2->offset + psrc2->length)){
			// psrc2 is inside the base
			result = 0;
		}else if((psrc1->offset + psrc1->length) > static_cast<size_t>(psrc2->offset)){
			// psrc2 start is inside the base
			result = 0;
		}else{
			// psrc2 start is outside the base, no overlap
			result = 1;
		}
	}else if(psrc1->offset > psrc2->offset){
		// check overlap
		if((psrc1->offset + psrc1->length) <= (psrc2->offset + psrc2->length)){
			// the base is inside psrc2
			result = 0;
		}else if(static_cast<size_t>(psrc1->offset) < (psrc2->offset + psrc2->length)){
			// the base start is inside psrc2
			result = 0;
		}else{
			// psrc2 start is outside the base, no overlap
			result = -1;
		}
	}else{	// psrc1->offset == psrc2->offset
		// always overlap
		result = 0;
	}
	return result;
}

#endif	// FLCKLISTOFFLOCK_H

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
