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
 * CREATE:   Thu 11 Jun 2015
 * REVISION:
 *
 */

#ifndef	FLCKPIDCACHE_H
#define	FLCKPIDCACHE_H

#include <map>

//---------------------------------------------------------
// Structure
//---------------------------------------------------------
// Key structure
//
typedef struct fl_pid_cache_key{
	pid_t	pid;
	tid_t	tid;
	int		fd;

	explicit fl_pid_cache_key(pid_t in_pid = FLCK_INVALID_ID, tid_t in_tid = FLCK_INVALID_ID, int in_fd = FLCK_INVALID_HANDLE) : pid(in_pid), tid(in_tid), fd(in_fd) {}
	fl_pid_cache_key(const fl_pid_cache_key& other) : pid(other.pid), tid(other.tid), fd(other.fd) {}

	fl_pid_cache_key& operator=(const fl_pid_cache_key& other)
	{
		pid	= other.pid;
		tid	= other.tid;
		fd	= other.fd;
		return *this;
	}

	bool operator<(const fl_pid_cache_key& other) const
	{
		if(pid < other.pid){
			return true;
		}else if(pid > other.pid){
			return false;
		}
		// pid == other.pid

		if(tid < other.tid){
			return true;
		}else if(tid > other.tid){
			return false;
		}
		// tid == other.tid

		if(fd < other.fd){
			return true;
		}else if(fd > other.fd){
			return false;
		}else if(FLCK_INVALID_HANDLE == fd){
			return false;
		}
		// fd != FLCK_INVALID_HANDLE && fd == other.fd
		return false;
	}
}FLPIDCACHEKEY, *PFLPIDCACHEKEY;

//
// Value structure
//
typedef struct fl_pid_cache_value{
	dev_t	devid;
	ino_t	inodeid;
	bool	is_run;

	explicit fl_pid_cache_value(dev_t in_devid = FLCK_INVALID_ID, ino_t in_inodeid = FLCK_INVALID_ID, bool in_is_run = true) : devid(in_devid), inodeid(in_inodeid), is_run(in_is_run) {}
	fl_pid_cache_value(const fl_pid_cache_value& other) : devid(other.devid), inodeid(other.inodeid), is_run(other.is_run) {}

	fl_pid_cache_value& operator=(const fl_pid_cache_value& other)
	{
		devid	= other.devid;
		inodeid	= other.inodeid;
		is_run	= other.is_run;
		return *this;
	}
}FLPIDCACHEVAL, *PFLPIDCACHEVAL;

//---------------------------------------------------------
// Typedef
//---------------------------------------------------------
typedef std::map<FLPIDCACHEKEY, FLPIDCACHEVAL>	fl_pid_cache_map_t;

//---------------------------------------------------------
// Utility inline functions
//---------------------------------------------------------
inline bool get_device_cache(fl_pid_cache_map_t* pcache, pid_t pid, tid_t tid, int fd, dev_t& devid, ino_t& inodeid)
{
	if(!pcache){
		return false;
	}
	FLPIDCACHEKEY						key(pid, tid, fd);
	fl_pid_cache_map_t::const_iterator	iter = pcache->find(key);
	if(pcache->end() == iter){
		return false;
	}
	devid	= iter->second.devid;
	inodeid	= iter->second.inodeid;
	return true;
}

inline bool set_device_cache(fl_pid_cache_map_t* pcache, pid_t pid, tid_t tid, int fd, dev_t devid, ino_t inodeid)
{
	if(!pcache){
		return false;
	}
	FLPIDCACHEKEY	key(pid, tid, fd);
	FLPIDCACHEVAL	val(devid, inodeid);
	(*pcache)[key] = val;
	return true;
}

inline bool set_no_device_cache(fl_pid_cache_map_t* pcache, pid_t pid, tid_t tid, int fd)
{
	if(!pcache){
		return false;
	}
	FLPIDCACHEKEY	key(pid, tid, fd);
	FLPIDCACHEVAL	val;				// empty
	(*pcache)[key] = val;
	return true;
}

inline bool get_execution_cache(fl_pid_cache_map_t* pcache, pid_t pid, tid_t tid, bool& is_run)
{
	if(!pcache){
		return false;
	}
	FLPIDCACHEKEY						key(pid, tid);
	fl_pid_cache_map_t::const_iterator	iter = pcache->find(key);
	if(pcache->end() == iter){
		return false;
	}
	is_run = iter->second.is_run;
	return true;
}

inline bool set_execution_cache(fl_pid_cache_map_t* pcache, pid_t pid, tid_t tid, bool is_run)
{
	if(!pcache){
		return false;
	}
	FLPIDCACHEKEY	key(pid, tid);
	FLPIDCACHEVAL	val(FLCK_INVALID_ID, FLCK_INVALID_ID, is_run);
	(*pcache)[key] = val;
	return true;
}

#endif	// FLCKPIDCACHE_H

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
