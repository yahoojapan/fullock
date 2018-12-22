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
 * CREATE:   Wed 13 May 2015
 * REVISION:
 *
 */

#ifndef	FLCKUTIL_H
#define	FLCKUTIL_H

#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <sstream>

#include "flckcommon.h"
#include "flckpidcache.h"

//---------------------------------------------------------
// Symbols
//---------------------------------------------------------
#define	FLCK_DEFAULT_SYSTEM_PAGESIZE			4096

//---------------------------------------------------------
// Templates & macros
//---------------------------------------------------------
// For empty string
template<typename T> inline bool FLCKEMPTYSTR(const T& pstr)
{
	return (NULL == (pstr) || '\0' == *(pstr)) ? true : false;
}

// For alignment
template<typename T1, typename T2> inline T1 ALIGNMENT(const T1& value, const T2& size)
{
	return ((((value) / (size)) + (0 == (value) % (size) ? 0 : 1)) * (size));
}

// Addtional/Subtruction Pointer
template <typename T> inline T* SUBPTR(T* pointer, off_t offset)
{
	return reinterpret_cast<T*>(reinterpret_cast<off_t>(pointer) - offset);
}

template <typename T> inline T* ADDPTR(T* pointer, off_t offset)
{
	return reinterpret_cast<T*>(reinterpret_cast<off_t>(pointer) + offset);
}

#define	CVT_POINTER(ptr, type)	reinterpret_cast<type*>(ptr)

#define	FLCK_Free(ptr)			if(ptr){ \
									free(ptr); \
									ptr = NULL; \
								}

#define	FLCK_Delete(ptr)		if(ptr){ \
									delete ptr; \
									ptr = NULL; \
								}

#define	FLCK_CLOSE(fd)			if(FLCK_INVALID_HANDLE != fd){ \
									close(fd); \
									fd = FLCK_INVALID_HANDLE; \
								}

//---------------------------------------------------------
// String Utilities
//---------------------------------------------------------
#define	SPACECAHRS						" \t\r\n"

// Trim
inline std::string ltrim(const std::string &base, const std::string &trims = SPACECAHRS)
{
	std::string	newstr(base);
	return newstr.erase(0, base.find_first_not_of(trims));
}

inline std::string rtrim(const std::string &base, const std::string &trims = SPACECAHRS)
{
	std::string::size_type	pos = base.find_last_not_of(trims);
	if(std::string::npos == pos){
		return "";
	}
	std::string	newstr(base);
	return newstr.erase(pos + 1);
}

inline std::string trim(const std::string &base, const std::string &trims = SPACECAHRS)
{
	return ltrim(rtrim(base, trims), trims);
}

// Convert
inline std::string lower(std::string base)
{
	for(std::string::size_type pos = 0; pos < base.length(); pos++){
		base[pos] = tolower(base[pos]);
	}
	return base;
}

inline std::string upper(std::string base)
{
	for(std::string::size_type pos = 0; pos < base.length(); pos++){
		base[pos] = toupper(base[pos]);
	}
	return base;
}

template<typename T> inline std::string to_string(T data)
{
	std::stringstream	strstream;
	strstream << data;
	return strstream.str();
}

template<typename T> inline std::string to_hexstring(T data)
{
	std::stringstream	strstream;
	strstream << std::hex << data;
	return strstream.str();
}

inline std::string spaces_string(int count)
{
	std::string result(count, ' ');

	return result;
}

//---------------------------------------------------------
// Hash Utility
//---------------------------------------------------------
//
// Build-in hash function k2h_Fnv_hash (FNV-1a)
// http://www.isthe.com/chongo/tech/comp/fnv/index.html
//
inline flck_hash_t flck_fnv_hash(const void* ptr, size_t len)
{
	flck_hash_t	hash = 14695981039346656037ULL;
	const char*	cptr = static_cast<const char*>(ptr);

	for(; len; --len){
		hash ^= static_cast<flck_hash_t>(*cptr++);
		hash *= static_cast<flck_hash_t>(1099511628211ULL);
	}
	return hash;
}

//---------------------------------------------------------
// System Utilities
//---------------------------------------------------------
#ifndef HAVE_GETTID
tid_t gettid(void);
#endif
tid_t get_threadid(void);					// should use this because caching
size_t GetSystemPageSize(void);

//---------------------------------------------------------
// pid/tid Utilities
//---------------------------------------------------------
inline flckpid_t get_flckpid(void)
{
	return ((static_cast<flckpid_t>(getpid()) << 32) | (static_cast<flckpid_t>(get_threadid()) & 0xFFFFFFFF));
}

inline flckpid_t compose_flckpid(pid_t pid, tid_t tid)
{
	return ((static_cast<flckpid_t>(pid) << 32) | (static_cast<flckpid_t>(tid) & 0xFFFFFFFF));
}

inline void decompose_flckpid(flckpid_t flckpid, pid_t& pid, tid_t& tid)
{
	pid = static_cast<pid_t>((flckpid >> 32) & 0xFFFFFFFF);
	tid = static_cast<tid_t>(flckpid & 0xFFFFFFFF);
}

inline pid_t decompose_pid(flckpid_t flckpid)
{
	return static_cast<pid_t>((flckpid >> 32) & 0xFFFFFFFF);
}

inline tid_t decompose_tid(flckpid_t flckpid)
{
	return static_cast<tid_t>(flckpid & 0xFFFFFFFF);
}

//---------------------------------------------------------
// Utility Functions(timespec)
//---------------------------------------------------------
inline void SUB_TIMESPEC(struct timespec* start, const struct timespec* end, struct timespec* result)
{
	if((end)->tv_nsec < (start)->tv_nsec){
		if(1 > (end)->tv_sec){
			(result)->tv_sec	= 0;
			(result)->tv_nsec	= 0;
			return;
		}
		(result)->tv_sec	= (end)->tv_sec - 1 - (start)->tv_sec - 1;
		(result)->tv_nsec	= (end)->tv_nsec + (1000 * 1000 * 1000) - (start)->tv_nsec;
	}else{
		(result)->tv_sec	= (end)->tv_sec - (start)->tv_sec;
		(result)->tv_nsec	= (end)->tv_nsec - (start)->tv_nsec;
	}
}

inline bool IS_OVER_TIMESPEC(struct timespec* start, const struct timespec* end, const struct timespec* limit)
{
	struct timespec	subtime;
	SUB_TIMESPEC(start, end, &subtime);

	if((limit)->tv_sec > subtime.tv_sec){
		return false;
	}else if((limit)->tv_sec < subtime.tv_sec){
		return true;
	}

	if((limit)->tv_nsec > subtime.tv_nsec){
		return false;
	}
	return true;
}

//---------------------------------------------------------
// Other Utilities
//---------------------------------------------------------
void* RawMap(int fd, size_t size, off_t offset);
bool RawUnmap(void* pmap, size_t size);

bool CvtNumberStringToLong(const char* str, long* presult);
bool GetRealPath(const char* pPath, std::string& strreal);
bool MakeWorkDirectory(const char* pDirPath);
bool FindThreadProcess(pid_t pid, tid_t tid, fl_pid_cache_map_t* pcache = NULL);
bool GetFileDevNode(pid_t pid, tid_t tid, int fd, dev_t& devid, ino_t& inodeid, fl_pid_cache_map_t* pcache = NULL);
bool GetFileDevNode(int fd, dev_t& devid, ino_t& inodeid);
inline bool GetFileDevNode(flckpid_t flckpid, int fd, dev_t& devid, ino_t& inodeid, fl_pid_cache_map_t* pcache)
{
	return GetFileDevNode(decompose_pid(flckpid), decompose_tid(flckpid), fd, devid, inodeid, pcache);
}

bool FileReadLock(int fd, off_t offset, bool block = false);
bool FileWriteLock(int fd, off_t offset, bool block = false);
bool FileUnlock(int fd, off_t offset, bool block = false);

ssize_t flck_read(int fd, unsigned char** ppbuff);
ssize_t flck_pread(int fd, void *buf, size_t count, off_t offset);
ssize_t flck_pwrite(int fd, const void *buf, size_t count, off_t offset);
bool flck_fill_zero(int fd, size_t count, off_t offset);

#endif	// FLCKUTIL_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
