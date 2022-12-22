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

#ifndef	FLCKLISTNCOND_H
#define	FLCKLISTNCOND_H

#include <string.h>
#include <iostream>
#include <vector>

#include "flckcommon.h"
#include "flckstructure.h"
#include "flckshm.h"

//---------------------------------------------------------
// List templates
//---------------------------------------------------------
#include "flckbaselist.tcc"

typedef fullock::fl_list_base<FLNAMEDCOND>	fllistbasencond;

//---------------------------------------------------------
// FlListNCond class
//---------------------------------------------------------
class FlListNCond : public fllistbasencond
{
	protected:
		int rawlock(FLCKLOCKTYPE LockType, bool is_broadcast, PFLNAMEDMUTEX abs_nmtx, time_t timeout_usec);

	public:
		explicit FlListNCond(PFLNAMEDCOND ptr = NULL) : fllistbasencond(ptr) {}
		FlListNCond(const FlListNCond& other) : fllistbasencond(other) {}
		virtual ~FlListNCond() {}

		void initialize(const char* pname, bool is_all = true)
		{
			if(pcurrent){
				if(is_all){
					pcurrent->next			= NULL;
				}
				if(!FLCKEMPTYSTR(pname)){
					pcurrent->name[FLCK_NAMED_COND_MAXLENGTH] = '\0';
					strncpy(pcurrent->name, pname, FLCK_NAMED_COND_MAXLENGTH);
					pcurrent->hash			= flck_fnv_hash(&pcurrent->name[0], strlen(pcurrent->name));
				}else{
					pcurrent->name[0]		= '\0';
					pcurrent->hash			= 0;
				}
				// initialize cond
				if(is_all){
					pcurrent->waiter_list	= NULL;
				}
			}
		}
		inline void initialize(PFLNAMEDCOND ptr, size_t count) { fllistbasencond::initialize(ptr, count); }
		inline void initialize(bool is_all) { initialize(NULL, is_all); }
		virtual void initialize(void) { initialize(NULL, true); }
		virtual void dump(std::ostream& out, int level) const;

		inline bool find(const char* pname, PFLNAMEDCOND& preltop)
		{
			FLNAMEDCOND 	tmp;
			if(!FLCKEMPTYSTR(pname)){
				tmp.name[FLCK_NAMED_COND_MAXLENGTH] = '\0';
				strncpy(tmp.name, pname, FLCK_NAMED_COND_MAXLENGTH);
				tmp.hash	= flck_fnv_hash(&tmp.name[0], strlen(tmp.name));
			}else{
				tmp.name[0]	= '\0';
				tmp.hash	= 0;
			}
			return fllistbasencond::find(&tmp, preltop);
		}

		inline int wait(PFLNAMEDMUTEX abs_nmtx, time_t timeout_usec = FLCK_NO_TIMEOUT) { return rawlock(FLCK_NCOND_WAIT, false, abs_nmtx, timeout_usec); }
		inline int signal(void) { return rawlock(FLCK_NCOND_UP, false, NULL, FLCK_NO_TIMEOUT); }
		inline int broadcast(void) { return rawlock(FLCK_NCOND_UP, true, NULL, FLCK_NO_TIMEOUT); }

		bool check_dead_lock(fl_pid_cache_map_t* pcache = NULL, flckpid_t except_flckpid = FLCK_INVALID_ID);
};

//---------------------------------------------------------
// Utility functions
//---------------------------------------------------------
// result	0	: same
//			-1	: psrc2 is smaller than psrc1
//			1	: psrc2 is larger than psrc1
inline int fl_compare_list_base(const PFLNAMEDCOND psrc1, const PFLNAMEDCOND psrc2)
{
	if(psrc1->hash < psrc2->hash){
		return -1;
	}else if(psrc1->hash > psrc2->hash){
		return 1;
	}
	return strncmp(psrc1->name, psrc2->name, FLCK_NAMED_COND_MAXLENGTH);
}

#endif	// FLCKLISTNCOND_H

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
