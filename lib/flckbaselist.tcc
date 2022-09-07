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
 * CREATE:   Fri 3 Jul 2015
 * REVISION:
 *
 */

#ifndef	FLCKBASELIST_TCC
#define FLCKBASELIST_TCC

#include <string.h>
#include <sched.h>
#include <errno.h>
#include <string>
#include <iostream>

#include "flckutil.h"

//---------------------------------------------------------
// Symbols
//---------------------------------------------------------
#define	FLCK_FLCKPID_TRYLOCK_LIMIT			1000

#define	FLCK_ROBUST_CHKCNT_NOLIMIT			-1
#define	FLCK_ROBUST_CHKCNT_DEFAULT			5000		// == 10-20ns * 5000 = 50-100us
#define	FLCK_ROBUST_CHKCNT_MIN				50			//

#define	FLCK_NOSHARED_MUTEX_VAL_LOCKED		1
#define	FLCK_NOSHARED_MUTEX_VAL_UNLOCKED	0

//---------------------------------------------------------
// Description
//---------------------------------------------------------
// This templates are template classes for single chain list.
// 
// [USAGE]
// You can use this templates as following:
// 1) Shortening template classes
//		fl_list_base<int>
//
// 2) Implement following functions
//		int fl_compare_list_base(const int* psrc1, const int* psrc2)
//
namespace fullock
{
	//---------------------------------------------------------
	// Utility
	//---------------------------------------------------------
	inline void fl_lock_lockid(flckpid_t* pflckpid, flckpid_t newid)
	{
		flckpid_t	oldval1;
		flckpid_t	oldval2 = FLCK_INVALID_ID;
		for(int cnt = 0; FLCK_INVALID_ID != (oldval1 = __sync_val_compare_and_swap(pflckpid, FLCK_INVALID_ID, newid)); ++cnt){
			if(oldval1 == newid){
				// already lockid is newid.
				break;
			}else if(oldval1 != oldval2){
				cnt = 0;		// reset
			}else if(FLCK_FLCKPID_TRYLOCK_LIMIT < cnt){
				// check thread(process) dead.
				pid_t	pid = decompose_pid(oldval1);
				tid_t	tid = decompose_tid(oldval1);

				if(!FindThreadProcess(pid, tid)){
					// thread(process) does not run, so force to set this tid.
					oldval2 = oldval1;
					if(oldval2 == (oldval1 = __sync_val_compare_and_swap(pflckpid, oldval2, newid))){
						// success to switch newid.
						break;
					}
					// retry...
				}
			}
			oldval2 = oldval1;

			// [NOTE]
			// we do not call sched_yield but spinning loop
			//sched_yield();
		}
	}

	inline void fl_unlock_lockid(flckpid_t* pflckpid, flckpid_t oldid)
	{
		flckpid_t	oldval1;
		flckpid_t	oldval2 = FLCK_INVALID_ID;
		for(int cnt = 0; oldid != (oldval1 = __sync_val_compare_and_swap(pflckpid, oldid, FLCK_INVALID_ID)); ++cnt){
			if(oldval1 == FLCK_INVALID_ID){
				// already lockid is FLCK_INVALID_ID.
				break;
			}else if(oldval1 != oldval2){
				cnt = 0;		// reset
			}else if(FLCK_FLCKPID_TRYLOCK_LIMIT < cnt){
				// check thread(process) dead.
				pid_t	pid = decompose_pid(oldval1);
				tid_t	tid = decompose_tid(oldval1);

				if(!FindThreadProcess(pid, tid)){
					// thread(process) does not run, so force to set this tid.
					oldval2 = oldval1;
					if(oldval2 == (oldval1 = __sync_val_compare_and_swap(pflckpid, oldval2, FLCK_INVALID_ID))){
						// success to switch newid.
						break;
					}
					// retry...
				}
			}
			oldval2 = oldval1;

			// [NOTE]
			// we do not call sched_yield but spinning loop
			//sched_yield();
		}
	}

	inline bool fl_try_unlock_lockid(flckpid_t* pflckpid, flckpid_t oldid)
	{
		if(oldid != __sync_val_compare_and_swap(pflckpid, oldid, FLCK_INVALID_ID)){
			return false;
		}
		return true;
	}

	inline bool fl_islock_lockid(const flckpid_t* pflckpid)
	{
		if(pflckpid && FLCK_INVALID_ID != *pflckpid){
			return true;
		}
		return false;
	}

	inline int fl_rdlock_rwlock(flck_rwlock_t* plockval, int max_count = FLCK_ROBUST_CHKCNT_NOLIMIT)
	{
		flck_rwlock_t	newval;
		flck_rwlock_t	beforeval;
		int				cnt = 0;
		do{
			if(FLCK_ROBUST_CHKCNT_NOLIMIT != max_count && max_count < cnt){
				return ETIMEDOUT;
			}else{
				cnt++;
			}
			beforeval	= *plockval;
			newval		= beforeval + 1;
		}while((FLCK_RWLOCK_UNLOCK > beforeval || beforeval != __sync_val_compare_and_swap(plockval, beforeval, newval)) && -1 <= sched_yield());
		return 0;
	}

	inline int fl_tryrdlock_rwlock(flck_rwlock_t* plockval)
	{
		flck_rwlock_t	beforeval	= *plockval;
		flck_rwlock_t	newval		= beforeval + 1;
		if(FLCK_RWLOCK_UNLOCK > beforeval){
			return EBUSY;
		}
		if(beforeval != __sync_val_compare_and_swap(plockval, beforeval, newval)){
			return EBUSY;
		}
		return 0;
	}

	inline int fl_timedrdlock_rwlock(flck_rwlock_t* plockval, const struct timespec* limittime)
	{
		struct timespec	starttime;
		if(-1 == clock_gettime(CLOCK_MONOTONIC, &starttime)){
			return EBUSY;
		}

		while(true){
			flck_rwlock_t	newval;
			flck_rwlock_t	beforeval;

			beforeval	= *plockval;
			newval		= beforeval + 1;
			if(FLCK_RWLOCK_UNLOCK <= beforeval){
				if(beforeval == __sync_val_compare_and_swap(plockval, beforeval, newval)){
					break;
				}
			}

			struct timespec	endtime;
			if(-1 == clock_gettime(CLOCK_MONOTONIC, &endtime)){
				return EBUSY;
			}
			if(IS_OVER_TIMESPEC(&starttime, &endtime, limittime)){
				return ETIMEDOUT;
			}
			sched_yield();
		}
		return 0;
	}

	inline int fl_wrlock_rwlock(flck_rwlock_t* plockval, int max_count = FLCK_ROBUST_CHKCNT_NOLIMIT)
	{
		int			cnt = 0;
		do{
			if(FLCK_ROBUST_CHKCNT_NOLIMIT != max_count && max_count < cnt){
				return ETIMEDOUT;
			}else{
				cnt++;
			}
		}while(FLCK_RWLOCK_UNLOCK != __sync_val_compare_and_swap(plockval, FLCK_RWLOCK_UNLOCK, FLCK_RWLOCK_WLOCK) && -1 <= sched_yield());
		return 0;
	}

	inline int fl_trywrlock_rwlock(flck_rwlock_t* plockval)
	{
		if(FLCK_RWLOCK_UNLOCK != __sync_val_compare_and_swap(plockval, FLCK_RWLOCK_UNLOCK, FLCK_RWLOCK_WLOCK)){
			return EBUSY;
		}
		return 0;
	}

	inline int fl_timedwrlock_rwlock(flck_rwlock_t* plockval, const struct timespec* limittime)
	{
		struct timespec	starttime;
		if(-1 == clock_gettime(CLOCK_MONOTONIC, &starttime)){
			return EBUSY;
		}
		while(FLCK_RWLOCK_UNLOCK != __sync_val_compare_and_swap(plockval, FLCK_RWLOCK_UNLOCK, FLCK_RWLOCK_WLOCK)){
			struct timespec	endtime;
			if(-1 == clock_gettime(CLOCK_MONOTONIC, &endtime)){
				return EBUSY;
			}
			if(IS_OVER_TIMESPEC(&starttime, &endtime, limittime)){
				return ETIMEDOUT;
			}
			sched_yield();
		}
		return 0;
	}

	inline int fl_unlock_rwlock(flck_rwlock_t* plockval)
	{
		flck_rwlock_t	newval;
		flck_rwlock_t	beforeval;
		do{
			beforeval		= *plockval;
			if(FLCK_RWLOCK_UNLOCK == beforeval){
				// already unlocked
				break;
			}else if(FLCK_RWLOCK_WLOCK >= beforeval){
				beforeval	= FLCK_RWLOCK_WLOCK;
				newval		= FLCK_RWLOCK_UNLOCK;
			}else{
				newval		= beforeval - 1;
			}
		}while(beforeval != __sync_val_compare_and_swap(plockval, beforeval, newval) && -1 <= sched_yield());
		return 0;
	}

	inline int fl_lock_mutex(flck_mutex_t* plockval, int* plockcnt, flckpid_t lockid, int max_count = FLCK_ROBUST_CHKCNT_NOLIMIT)
	{
		int	cnt = 0;
		do{
			if(FLCK_ROBUST_CHKCNT_NOLIMIT != max_count && max_count < cnt){
				return EWOULDBLOCK;			// EWOULDBLOCK
			}else{
				cnt++;
			}

			// do lock
			if(lockid == *plockval){
				// already locked --> count up lockcnt
				int	oldcnt = __sync_fetch_and_add(plockcnt, 1);
				if(0 <= oldcnt){
					// success to count up --> check lockval(this is failed but get old value)
					if(lockid == __sync_val_compare_and_swap(plockval, lockid, lockid)){
						// success to lock
						break;
					}
				}
				// this case is unlocked before count up. So need to count down and retry.
				__sync_fetch_and_sub(plockcnt, 1);

			}else{
				// try lock
				if(FLCK_MUTEX_UNLOCK == __sync_val_compare_and_swap(plockval, FLCK_MUTEX_UNLOCK, lockid)){
					// get lock --> count up lockcnt
					__sync_fetch_and_add(plockcnt, 1);		// not need to check return value
					break;
				}
			}
		}while(-1 <= sched_yield());
		return 0;
	}

	inline int fl_trylock_mutex(flck_mutex_t* plockval, int* plockcnt, flckpid_t lockid)
	{
		// do lock
		if(lockid == *plockval){
			// already locked --> count up lockcnt
			int	oldcnt = __sync_fetch_and_add(plockcnt, 1);
			if(0 <= oldcnt){
				// success to count up --> check lockval(this is failed but get old value)
				if(lockid == __sync_val_compare_and_swap(plockval, lockid, lockid)){
					// success to lock
					return 0;
				}
			}
			// this case is unlocked before count up. So need to count down.
			__sync_fetch_and_sub(plockcnt, 1);

		}else{
			// try lock
			if(FLCK_MUTEX_UNLOCK == __sync_val_compare_and_swap(plockval, FLCK_MUTEX_UNLOCK, lockid)){
				// get lock --> count up lockcnt
				__sync_fetch_and_add(plockcnt, 1);		// not need to check return value
				return 0;
			}
		}
		return EBUSY;
	}

	inline int fl_timedlock_mutex(flck_mutex_t* plockval, int* plockcnt, flckpid_t lockid, const struct timespec* limittime)
	{
		struct timespec	starttime;
		if(-1 == clock_gettime(CLOCK_MONOTONIC, &starttime)){
			return EBUSY;
		}

		do{
			// do lock
			if(lockid == *plockval){
				// already locked --> count up lockcnt
				int	oldcnt = __sync_fetch_and_add(plockcnt, 1);
				if(0 <= oldcnt){
					// success to count up --> check lockval(this is failed but get old value)
					if(lockid == __sync_val_compare_and_swap(plockval, lockid, lockid)){
						// success to lock
						break;
					}
				}
				// this case is unlocked before count up. So need to count down and retry.
				__sync_fetch_and_sub(plockcnt, 1);

			}else{
				// try lock
				if(FLCK_MUTEX_UNLOCK == __sync_val_compare_and_swap(plockval, FLCK_MUTEX_UNLOCK, lockid)){
					// get lock --> count up lockcnt
					__sync_fetch_and_add(plockcnt, 1);		// not need to check return value
					break;
				}
			}

			struct timespec	endtime;
			if(-1 == clock_gettime(CLOCK_MONOTONIC, &endtime)){
				return EBUSY;
			}
			if(IS_OVER_TIMESPEC(&starttime, &endtime, limittime)){
				return ETIMEDOUT;
			}
		}while(-1 <= sched_yield());
		return 0;
	}

	inline int fl_unlock_mutex(flck_mutex_t* plockval, int* plockcnt, flckpid_t lockid)
	{
		do{
			// do unlock
			flck_mutex_t	oldval;
			if(FLCK_MUTEX_UNLOCK == *plockval){
				// already unlocked
				break;
			}else if(lockid != *plockval){
				// another process(thread) is owner.
				return EPERM;
			}else{
				//  locked --> count down lockcnt
				int	oldcnt = __sync_fetch_and_sub(plockcnt, 1);
				if(oldcnt <= 1){
					if(oldcnt < 1){
						// something wrong, but do recover...
						__sync_val_compare_and_swap(plockcnt, oldcnt, 0);	// not check result
					}
					// try unlock
					if(lockid == (oldval = __sync_val_compare_and_swap(plockval, lockid, FLCK_MUTEX_UNLOCK)) || FLCK_MUTEX_UNLOCK == oldval){
						// succeed to unlock
						break;
					}
					// why? but do recover
					__sync_fetch_and_add(plockcnt, 1);						// not check result
				}
			}
		}while(-1 <= sched_yield());
		return 0;
	}

	inline int fl_force_unlock_mutex(flck_mutex_t* plockval, int* plockcnt)
	{
		do{
			int	oldcnt = *plockcnt;
			if(0 == oldcnt || oldcnt == __sync_val_compare_and_swap(plockcnt, oldcnt, 0)){
				flck_mutex_t	oldval = *plockval;
				if(FLCK_MUTEX_UNLOCK == oldval || oldval == __sync_val_compare_and_swap(plockval, oldval, FLCK_MUTEX_UNLOCK)){
					break;
				}
			}
			// retry
		}while(-1 <= sched_yield());
		return 0;
	}

	inline int fl_wait_cond(FLCKLOCKTYPE* plockstatus, FLCKLOCKTYPE waitstatus)
	{
		while(waitstatus != __sync_val_compare_and_swap(plockstatus, waitstatus, waitstatus)){
			sched_yield();
		}
		return 0;
	}

	inline int fl_timedwait_cond(FLCKLOCKTYPE* plockstatus, FLCKLOCKTYPE waitstatus, const struct timespec* limittime)
	{
		struct timespec	starttime;
		if(-1 == clock_gettime(CLOCK_MONOTONIC, &starttime)){
			return EBUSY;
		}
		while(waitstatus != __sync_val_compare_and_swap(plockstatus, waitstatus, waitstatus)){
			struct timespec	endtime;
			if(-1 == clock_gettime(CLOCK_MONOTONIC, &endtime)){
				return EBUSY;
			}
			if(IS_OVER_TIMESPEC(&starttime, &endtime, limittime)){
				return ETIMEDOUT;
			}
			sched_yield();
		}
		return 0;
	}

	inline int fl_force_set_cond(FLCKLOCKTYPE* plockstatus, FLCKLOCKTYPE newstatus)
	{
		FLCKLOCKTYPE	oldstatus1;
		FLCKLOCKTYPE	oldstatus2;
		for(oldstatus1 = *plockstatus; oldstatus1 != (oldstatus2 = __sync_val_compare_and_swap(plockstatus, oldstatus1, newstatus)); oldstatus1 = oldstatus2){
			sched_yield();
		}
		return 0;
	}

	template<typename T> inline void flck_lock_noshared_mutex(T* plockval)
	{
		T	oldval;
		while(static_cast<T>(FLCK_NOSHARED_MUTEX_VAL_UNLOCKED) != (oldval = __sync_val_compare_and_swap(plockval, static_cast<T>(FLCK_NOSHARED_MUTEX_VAL_UNLOCKED), static_cast<T>(FLCK_NOSHARED_MUTEX_VAL_LOCKED)))){
			sched_yield();
		}
	}

	template<typename T>
	inline bool flck_trylock_noshared_mutex(T* plockval)
	{
		if(static_cast<T>(FLCK_NOSHARED_MUTEX_VAL_UNLOCKED) != __sync_val_compare_and_swap(plockval, static_cast<T>(FLCK_NOSHARED_MUTEX_VAL_UNLOCKED), static_cast<T>(FLCK_NOSHARED_MUTEX_VAL_LOCKED))){
			return false;
		}
		return true;
	}

	template<typename T>
	inline void flck_unlock_noshared_mutex(T* plockval)
	{
		T	oldval;
		while(static_cast<T>(FLCK_NOSHARED_MUTEX_VAL_LOCKED) != (oldval = __sync_val_compare_and_swap(plockval, static_cast<T>(FLCK_NOSHARED_MUTEX_VAL_LOCKED), static_cast<T>(FLCK_NOSHARED_MUTEX_VAL_UNLOCKED)))){
			if(static_cast<T>(FLCK_NOSHARED_MUTEX_VAL_UNLOCKED) == oldval){
				break;		// already unlocked
			}
			sched_yield();
		}
	}

	template<typename T>
	inline T flck_counter_up(T* pcounter)
	{
		return __sync_fetch_and_add(pcounter, static_cast<T>(1));
	}

	template<typename T>
	inline T flck_counter_down(T* pcounter)
	{
		if(0 == *pcounter){
			return 0;
		}
		return __sync_fetch_and_sub(pcounter, static_cast<T>(1));
	}

	//---------------------------------------------------------
	// fl_list_base template class
	//---------------------------------------------------------
	// list base class
	// 
	template<typename T>
	class fl_list_base
	{
		public:
			typedef fl_list_base<T>		s_type;
			typedef fl_list_base<T>*	s_ptr_type;
			typedef T					st_type;
			typedef T*					st_ptr_type;

		protected:
			static const st_ptr_type	nullval;

			st_ptr_type					pcurrent;

		public:
			explicit fl_list_base(const st_ptr_type ptr = nullval) : pcurrent(ptr) {}
			explicit fl_list_base(const st_type& other) : pcurrent(other.pcurrent) {}
			virtual ~fl_list_base() {}

			inline bool initialize(st_ptr_type ptr, size_t count);
			virtual void initialize(void);
			inline void set(const st_ptr_type ptr) { pcurrent = ptr; }

			inline st_ptr_type get(void) const { return pcurrent; }
			inline st_ptr_type rel_get(void) const { return to_rel(pcurrent); }
			inline st_ptr_type next(void) { return (pcurrent ? to_abs(pcurrent->next) : nullval); }
			inline st_ptr_type rel_next(void) { return pcurrent->next; }
			inline bool to_next(void) { if(pcurrent){ pcurrent = to_abs(pcurrent->next); return true; }else{ return false; } }
			inline bool insert_list(st_ptr_type& preltop);
			inline bool retrieve_list(st_ptr_type& preltop);
			inline bool cutoff_list(st_ptr_type& preltop) const;

			inline bool find(const st_ptr_type pbase, st_ptr_type& preltop);
			inline bool rfind(const st_ptr_type pbase, st_ptr_type& preltop);

			virtual void dump(std::ostream& out, int level) const;
	};

	//---------------------------------------------------------
	// Methods for fl_list_base
	//---------------------------------------------------------
	template<typename T>
	const typename fl_list_base<T>::st_ptr_type	fl_list_base<T>::nullval = NULL;

	template<typename T>
	inline bool fl_list_base<T>::initialize(st_ptr_type ptr, size_t count)
	{
		if(!ptr || 0 == count){
			return false;
		}
		pcurrent = ptr;

		st_ptr_type	prev= nullval;
		for(size_t cnt = 0; cnt < count; ++cnt){
			if(prev){
				prev->next = to_rel(ptr);
			}
			ptr->next	= nullval;
			prev		= ptr++;
		}
		return true;
	}

	template<typename T>
	void fl_list_base<T>::initialize(void)
	{
		if(pcurrent){
			pcurrent->next = nullval;
		}
	}

	// Insert one list object to top of list.
	//
	template<typename T>
	inline bool fl_list_base<T>::insert_list(st_ptr_type& preltop)
	{
		if(!pcurrent){
			return false;
		}
		// insert current before top.
		st_ptr_type	newreltop = to_rel(pcurrent);
		st_ptr_type	oldreltop;
		do{
			oldreltop		= preltop;
			pcurrent->next	= oldreltop;
		}while(oldreltop != __sync_val_compare_and_swap(&preltop, oldreltop, newreltop));

		return true;
	}

	// Retrieve one list object from top of list.
	//
	template<typename T>
	inline bool fl_list_base<T>::retrieve_list(st_ptr_type& preltop)
	{
		// retrieve one from top
		st_ptr_type	oldreltop;
		st_ptr_type	newreltop;
		do{
			oldreltop = preltop;
			if(!oldreltop){
				return false;
			}
			newreltop = to_abs(oldreltop)->next;

		}while(oldreltop != __sync_val_compare_and_swap(&preltop, oldreltop, newreltop));

		pcurrent		= to_abs(oldreltop);
		pcurrent->next	= nullval;

		return true;
	}

	template<typename T>
	inline bool fl_list_base<T>::cutoff_list(st_ptr_type& preltop) const
	{
		if(!pcurrent || !preltop){
			return false;
		}
		// search current in list
		st_ptr_type		oldrelnext	= to_rel(pcurrent);
		st_ptr_type		newrelnext;
		st_ptr_type*	ptroldnext	= &preltop;
		bool			is_restart	= false;
		for(st_ptr_type pabstarget = to_abs(preltop), pabsparent = nullval; pabstarget; ){
			if(pabstarget == pcurrent){
				// switch parent's next to current's next(cut current)
				newrelnext = pcurrent->next;

				if(oldrelnext == __sync_val_compare_and_swap(ptroldnext, oldrelnext, newrelnext)){
					pcurrent->next = nullval;
					return true;
				}
				is_restart = true;
			}

			if(is_restart){
				// restart from top
				is_restart	= false;
				pabsparent	= nullval;
				ptroldnext	= &preltop;
				pabstarget	= to_abs(preltop);
			}else{
				// next
				pabsparent	= pabstarget;
				ptroldnext	= &(pabsparent->next);
				pabstarget	= to_abs(pabstarget->next);
			}
		}
		return false;
	}

	template<typename T>
	inline bool fl_list_base<T>::find(const st_ptr_type pbase, st_ptr_type& preltop)
	{
		if(!pbase || !preltop){
			return false;
		}
		for(st_ptr_type pabstarget = to_abs(preltop); pabstarget; pabstarget = to_abs(pabstarget->next)){
			if(0 == fl_compare_list_base(pbase, pabstarget)){
				// set current
				pcurrent = pabstarget;
				return true;
			}
		}
		return false;
	}

	template<typename T>
	inline bool fl_list_base<T>::rfind(const st_ptr_type pbase, st_ptr_type& preltop)
	{
		if(!pbase || !preltop){
			return false;
		}
		st_ptr_type	lastfound = nullval;
		for(st_ptr_type pabstarget = to_abs(preltop); pabstarget; pabstarget = to_abs(pabstarget->next)){
			if(0 == fl_compare_list_base(pbase, pabstarget)){
				lastfound = pabstarget;
			}
		}
		if(nullval != lastfound){
			// set current
			pcurrent = lastfound;
			return true;
		}
		return false;
	}

	template<typename T>
	void fl_list_base<T>::dump(std::ostream& out, int level) const
	{
		if(!pcurrent){
			return;
		}
		std::string	spacer = spaces_string(level);
		out << spacer << "next              = " << to_hexstring(pcurrent->next) << std::endl;
	}
};

#endif	// FLCKBASELIST_TCC

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
