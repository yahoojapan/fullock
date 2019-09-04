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

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <limits.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <pthread.h>

#include <climits>
#include <vector>

#include "flckcommon.h"
#include "flckutil.h"
#include "flckdbg.h"
#include "fullock.h"

using namespace	std;

//---------------------------------------------------------
// Types
//---------------------------------------------------------
typedef std::vector<std::string>				strlist_t;

//---------------------------------------------------------
// Symbols
//---------------------------------------------------------
#define	FLCK_WORK_DIRECTORY_PERMS				(S_IXUSR | S_IRUSR | S_IWUSR | S_IXGRP | S_IRGRP | S_IWGRP | S_IXOTH | S_IROTH | S_IWOTH)
#define	FLCK_PROC_FD_PATH_FORM					"/proc/%d/fd/%d"
#define	FLCK_PROC_PATH_FORM						"/proc/%d/task/%d"

//---------------------------------------------------------
// Macros
//---------------------------------------------------------
#define	LOCKTYPE_STR(type)						(F_UNLCK == type ? "unlock" : F_RDLCK == type ? "read" : F_WRLCK == type ? "write" : "unknown")

//---------------------------------------------------------
// MMap
//---------------------------------------------------------
void* RawMap(int fd, size_t size, off_t offset)
{
	if(FLCK_INVALID_HANDLE == fd){
		ERR_FLCKPRN("Parameter is wrong.");
		return NULL;
	}
	void*	pBase;
	if(MAP_FAILED == (pBase = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset))){
		ERR_FLCKPRN("Could not mmap fd(%d), size(%zu), offset(%zd), errno = %d", fd, size, offset, errno);
		return NULL;
	}
	return pBase;
}

bool RawUnmap(void* pmap, size_t size)
{
	if(!pmap){
		ERR_FLCKPRN("Parameter is wrong.");
		return false;
	}
	if(0 != munmap(pmap, size)){
		ERR_FLCKPRN("Could not munmap pmap(%p), size(%zu), errno = %d", pmap, size, errno);
		return false;
	}
	return true;
}

//---------------------------------------------------------
// Utilities for mode
//---------------------------------------------------------
bool CvtNumberStringToLong(const char* str, long* presult)
{
	if(!presult){
		ERR_FLCKPRN("Parameter is wrong.");
		return false;
	}
	if(FLCKEMPTYSTR(str)){
		ERR_FLCKPRN("String number is wrong.");
		return false;
	}

	char*	errptr	= NULL;
	errno			= 0;
	*presult		= strtol(str, &errptr, 0);

	if((ERANGE == errno && (LONG_MAX == *presult || LONG_MIN == *presult)) || (0 != errno && 0L == *presult)){
		MSG_FLCKPRN("could not convert %s to long, because under(or over) flow", str);
		return false;
	}else if(!errptr){
		MSG_FLCKPRN("could not convert %s to long, because wrong character(%s) in it.", str, errptr);
		return false;
	}
	return true;
}

//---------------------------------------------------------
// File path etc Utilities
//---------------------------------------------------------
bool GetRealPath(const char* pPath, string& strreal)
{
	if(FLCKEMPTYSTR(pPath)){
		WAN_FLCKPRN("Parameter is wrong.");
		return false;
	}

	// temporary full path
	string	tmppath;
	if('/' != pPath[0]){
		char*	cur = get_current_dir_name();
		tmppath	= cur;
		tmppath	+= "/";
		tmppath	+= pPath;
		FLCK_Free(cur);
	}else{
		tmppath = pPath;
	}

	// parse leaf
	bool		is_last_slash = false;
	strlist_t	rawpathleaf;
	for(string::size_type pos = tmppath.find('/'); string::npos != pos; pos = tmppath.find('/')){
		if(0 != pos){
			string	strleaf = tmppath.substr(0, pos);
			rawpathleaf.push_back(strleaf);
		}
		tmppath = tmppath.substr(pos + 1);
	}
	if(tmppath.empty()){
		is_last_slash = true;
	}else{
		rawpathleaf.push_back(tmppath);
	}

	// normalization
	strlist_t	pathleaf;
	for(strlist_t::iterator iter = rawpathleaf.begin(); iter != rawpathleaf.end(); ++iter){
		if((*iter) == "."){
			// nothing to do
		}else if((*iter) == ".."){
			if(rawpathleaf.empty()){
				WAN_FLCKPRN("Path is wrong(over the root).");
				return false;
			}
			pathleaf.pop_back();
		}else{
			pathleaf.push_back(*iter);
		}
	}

	// construction
	strreal.erase();
	for(strlist_t::iterator iter = pathleaf.begin(); iter != pathleaf.end(); ++iter){
		strreal += "/";
		strreal += *iter;
	}
	if(is_last_slash){
		strreal += "/";
	}
	return true;
}

bool MakeWorkDirectory(const char* pDirPath)
{
	if(FLCKEMPTYSTR(pDirPath)){
		WAN_FLCKPRN("Parameter is wrong.");
		return false;
	}

	string	fullpath;
	if(!GetRealPath(pDirPath, fullpath)){
		return false;
	}

	// parse leaf
	strlist_t	pathleaf;
	for(string::size_type pos = fullpath.find('/'); string::npos != pos; pos = fullpath.find('/')){
		if(0 != pos){
			string	strleaf = fullpath.substr(0, pos);
			pathleaf.push_back(strleaf);
		}
		fullpath = fullpath.substr(pos + 1);
	}
	if(!fullpath.empty()){
		pathleaf.push_back(fullpath);
	}

	// check & make directory
	fullpath = "";
	for(strlist_t::iterator iter = pathleaf.begin(); iter != pathleaf.end(); ++iter){
		fullpath += "/";
		fullpath += *iter;

		struct stat	st;
		if(0 != stat(fullpath.c_str(), &st)){
			if(ENOENT != errno){
				WAN_FLCKPRN("Could not access %s, errno=%d", fullpath.c_str(), errno);
				return false;
			}
			// Make directory
			mode_t	old_umask = umask(0);
			if(0 != mkdir(fullpath.c_str(), FLCK_WORK_DIRECTORY_PERMS)){
				WAN_FLCKPRN("Could not make directory(%s), errno=%d", fullpath.c_str(), errno);
				umask(old_umask);
				return false;
			}
			umask(old_umask);

		}else{
			if(0 == (st.st_mode & S_IFDIR)){
				WAN_FLCKPRN("%s is not directory.", fullpath.c_str());
				return false;
			}
		}
	}
	return true;
}

// [NOTE]
// stat(fstat) function call to /proc system takes average 100-150ns.
// FindThreadProcess and GetFileDevNode functions for the load reduction
// have the argument fl_pid_cache_map_t for cache.
//
bool FindThreadProcess(pid_t pid, tid_t tid, fl_pid_cache_map_t* pcache)
{
	bool	is_run = false;
	if(pcache){
		if(get_execution_cache(pcache, pid, tid, is_run)){
			return is_run;
		}
	}

	char		szPath[PATH_MAX];
	struct stat	st;
	sprintf(szPath, FLCK_PROC_PATH_FORM, pid, tid);
	if(-1 == stat(szPath, &st)){
		is_run = false;
	}else{
		is_run = true;
	}

	if(pcache){
		set_execution_cache(pcache, pid, tid, is_run);
	}
	return is_run;
}

static inline int GetFileDevNode(const char* file, dev_t& devid, ino_t& inodeid)
{
	struct stat	st;
	if(-1 == stat(file, &st)){
		if(EACCES == errno){
			//MSG_FLCKPRN("Could not access stat for %s(errno:%d)", file, errno);
			devid	= FLCK_EACCESS_ID;
			inodeid	= FLCK_EACCESS_ID;
		}else{	// ENOENT == errno
			//MSG_FLCKPRN("Could not get stat for %s(errno:%d)", file, errno);
			devid	= FLCK_INVALID_ID;
			inodeid	= FLCK_INVALID_ID;
		}
		return errno;
	}
	devid	= st.st_dev;
	inodeid	= st.st_ino;
	return 0;
}

// [NOTE]
// If could not access to /proc/pid/fd directory because EACCESS,
// devid and inodeid are set FLCK_EACCESS_ID.
//
bool GetFileDevNode(pid_t pid, tid_t tid, int fd, dev_t& devid, ino_t& inodeid, fl_pid_cache_map_t* pcache)
{
	if(pcache){
		if(get_device_cache(pcache, pid, tid, fd, devid, inodeid)){
			if(FLCK_INVALID_ID == devid && FLCK_INVALID_ID == inodeid){
				return false;
			}else{
				return true;
			}
		}
	}

	int		result = 0;
	if(IS_FLCK_RWLOCK_NO_FD(fd)){
		// [NOTE]
		// When fd is FLCK_RWLOCK_NO_FD, it means rwlock with no-fd.
		// On no fd mode, check only pid/tid.
		//
		char		szPath[PATH_MAX];
		struct stat	st;
		sprintf(szPath, FLCK_PROC_PATH_FORM, pid, tid);
		if(-1 == stat(szPath, &st)){
			devid	= FLCK_INVALID_ID;
			inodeid	= FLCK_INVALID_ID;
			result	= errno;
		}else{
			devid	= FLCK_EACCESS_ID;
			inodeid	= FLCK_EACCESS_ID;
		}
	}else{
		char	szPath[PATH_MAX];
		sprintf(szPath, FLCK_PROC_FD_PATH_FORM, pid, fd);
		if(EACCES == (result = GetFileDevNode(szPath, devid, inodeid))){
			// Need to check thread(process) running.
			//
			struct stat	st;
			sprintf(szPath, FLCK_PROC_PATH_FORM, pid, tid);
			if(-1 == stat(szPath, &st)){
				devid	= FLCK_INVALID_ID;
				inodeid	= FLCK_INVALID_ID;
				result	= errno;
			}
		}
	}
	if(pcache){
		if(0 == result || EACCES == result){
			set_device_cache(pcache, pid, tid, fd, devid, inodeid);
		}else{
			set_no_device_cache(pcache, pid, tid, fd);
		}
	}
	return (0 == result || EACCES == result);
}

bool GetFileDevNode(int fd, dev_t& devid, ino_t& inodeid)
{
	if(IS_FLCK_RWLOCK_NO_FD(fd)){
		// [NOTE]
		// fd is FLCK_RWLOCK_NO_FD, it means rwlock with no-fd 
		//
		devid	= FLCK_EACCESS_ID;
		inodeid	= FLCK_EACCESS_ID;
	}else{
		struct stat	st;
		if(-1 == fstat(fd, &st)){
			//MSG_FLCKPRN("Could not get stat for fd:%d(errno:%d)", fd, errno);
			//devid		= FLCK_INVALID_ID;
			//inodeid	= FLCK_INVALID_ID;
			return false;
		}
		devid	= st.st_dev;
		inodeid	= st.st_ino;
	}
	return true;
}

//---------------------------------------------------------
// File Lock Utilities
//---------------------------------------------------------
static bool RawFileLock(int fd, short type, off_t offset, bool block)
{
	if(FLCK_INVALID_HANDLE == fd){
		WAN_FLCKPRN("Parameter is wrong.");
		return false;
	}

	struct flock	fl;
	fl.l_type	= type;
	fl.l_whence	= SEEK_SET;
	fl.l_start	= offset;
	fl.l_len	= 1L;

	while(true){
		if(0 == fcntl(fd, block ? F_SETLKW : F_SETLK, &fl)){
			//MSG_FLCKPRN("Monitor file is Locked %s mode: offset=%jd, %s", LOCKTYPE_STR(type), static_cast<intmax_t>(offset), block ? "blocked" : "non blocked");
			return true;
		}
		if(EINTR == errno){
			//MSG_FLCKPRN("Signal occurred during \"%s\" lock, so retrying.", LOCKTYPE_STR(type));
		}else{
			if(!block){
				if(EACCES != errno && EAGAIN != errno){
					//MSG_FLCKPRN("Could not \"%s\" lock because something unnormal wrong occurred. errno=%d", LOCKTYPE_STR(type), errno);
				}
				break;
			}else{
				//MSG_FLCKPRN("Could not \"%s\" lock because something unnormal wrong occurred. errno=%d", LOCKTYPE_STR(type), errno);
				break;
			}
		}
	}
    return false;
}

bool FileReadLock(int fd, off_t offset, bool block)
{
	return RawFileLock(fd, F_RDLCK, offset, block);
}

bool FileWriteLock(int fd, off_t offset, bool block)
{
	return RawFileLock(fd, F_WRLCK, offset, block);
}

bool FileUnlock(int fd, off_t offset, bool block)
{
	return RawFileLock(fd, F_UNLCK, offset, block);
}

//---------------------------------------------------------
// Read/Write File Utilities
//---------------------------------------------------------
ssize_t flck_read(int fd, unsigned char** ppbuff)
{
	if(FLCK_INVALID_HANDLE == fd || !ppbuff){
		ERR_FLCKPRN("Parameters are wrong.");
		return -1;
	}

	ssize_t			pagesize = static_cast<ssize_t>(GetSystemPageSize());
	ssize_t			buffsize = pagesize;
	unsigned char*	pbuff;
	if(NULL == (pbuff = reinterpret_cast<unsigned char*>(malloc(buffsize * sizeof(unsigned char))))){
		ERR_FLCKPRN("Could not allocation memory.");
		return -1;
	}

	for(ssize_t pos = 0, readsize = 0; true; pos += readsize){
		if(-1 == (readsize = read(fd, &pbuff[pos], pagesize))){
			if(EAGAIN == errno){
				//MSG_FLCKPRN("reading fd reached end(EAGAIN)");
				break;
			}else if(EINTR == errno){
				//MSG_FLCKPRN("break reading fd by signal, so retry to read.");
				readsize = 0;
				continue;
			}
			// cppcheck-suppress unmatchedSuppression
			// cppcheck-suppress invalidPrintfArgType_sint
			ERR_FLCKPRN("Failed to read from fd(%d: %zd: %zd). errno=%d", fd, pos, pagesize, errno);
			FLCK_Free(pbuff);
			return -1;
		}
		if(0 <= readsize && readsize < pagesize){
			buffsize = pos + readsize;
			break;
		}

		if(buffsize < (pos + readsize + pagesize)){
			// reallocate
			buffsize = pos + readsize + pagesize;

			unsigned char*	ptmp;
			if(NULL == (ptmp = reinterpret_cast<unsigned char*>(realloc(pbuff, buffsize)))){
				ERR_FLCKPRN("Could not allocation memory.");
				FLCK_Free(pbuff);
				return -1;
			}
			pbuff = ptmp;
		}
	}
	*ppbuff = pbuff;
	return buffsize;
}

ssize_t flck_pread(int fd, void *buf, size_t count, off_t offset)
{
	ssize_t	read_cnt;
	ssize_t	one_read;
	for(read_cnt = 0L, one_read = 0L; static_cast<size_t>(read_cnt) < count; read_cnt += one_read){
		if(-1 == (one_read = pread(fd, &(static_cast<unsigned char*>(buf))[read_cnt], (count - static_cast<size_t>(read_cnt)), (offset + read_cnt)))){
			WAN_FLCKPRN("Failed to read from fd(%d:%jd:%zu), errno = %d", fd, static_cast<intmax_t>(offset + read_cnt), count - static_cast<size_t>(read_cnt), errno);
			return -1;
		}
		if(0 == one_read){
			break;
		}
	}
	return read_cnt;
}

ssize_t flck_pwrite(int fd, const void *buf, size_t count, off_t offset)
{
	ssize_t	write_cnt;
	ssize_t	one_write;
	for(write_cnt = 0L, one_write = 0L; static_cast<size_t>(write_cnt) < count; write_cnt += one_write){
		if(-1 == (one_write = pwrite(fd, &(static_cast<const unsigned char*>(buf))[write_cnt], (count - static_cast<size_t>(write_cnt)), (offset + write_cnt)))){
			WAN_FLCKPRN("Failed to write to fd(%d:%jd:%zu), errno = %d", fd, static_cast<intmax_t>(offset + write_cnt), count - static_cast<size_t>(write_cnt), errno);
			return -1;
		}
	}
	return write_cnt;
}

bool flck_fill_zero(int fd, size_t count, off_t offset)
{
	// data for init
	static unsigned char	bydata[4096];
	static bool				is_init = false;
	if(!is_init){
		memset(bydata, 0x00, sizeof(bydata));
		is_init = true;
	}

	ssize_t	write_cnt;
	ssize_t	one_write;
	for(write_cnt = 0L, one_write = 0L; static_cast<size_t>(write_cnt) < count; write_cnt += one_write){
		one_write = std::min((count - static_cast<size_t>(write_cnt)), sizeof(bydata));
		if(-1 == (one_write = flck_pwrite(fd, bydata, one_write, (offset + write_cnt)))){
			WAN_FLCKPRN("Failed to initialize zero to fd(%d:%jd), errno = %d", fd, static_cast<intmax_t>(offset + write_cnt), errno);
			return false;
		}
	}
	return true;
}

//---------------------------------------------------------
// Other Utilities
//---------------------------------------------------------
#ifndef HAVE_GETTID
tid_t gettid(void)
{
	return static_cast<tid_t>(syscall(SYS_gettid));
}
#endif

size_t GetSystemPageSize(void)
{
	// Check only once.
	static size_t	s_pagesize	= FLCK_DEFAULT_SYSTEM_PAGESIZE;
	static bool		s_init		= false;

	if(s_init){
		long psize = sysconf(_SC_PAGE_SIZE);
		if(-1 == psize){
			ERR_FLCKPRN("Could not get page size(errno = %d), so return %d byte", errno, FLCK_DEFAULT_SYSTEM_PAGESIZE);
		}else{
			s_pagesize = static_cast<size_t>(psize);
		}
		s_init = true;
	}
	return static_cast<size_t>(s_pagesize);
}

//---------------------------------------------------------
// Thread Id Cache
//---------------------------------------------------------
//
// FlTidCache Class
//
// [NOTE]
// To avoid static object initialization order problem(SIOF)
//
class FlTidCache
{
	protected:
		bool			Initialized;
		pthread_key_t	TidKey;						// == unsigned int

	protected:
		FlTidCache(void) : Initialized(false)
		{
			int	result = pthread_key_create(&TidKey, NULL);
			if(0 != result){
				ERR_FLCKPRN("Could not create key for each thread, error code=%d.", result);
				Initialized = false;
			}else{
				Initialized = true;
			}
		}

		virtual ~FlTidCache(void)
		{
			if(Initialized){
				int	result = pthread_key_delete(TidKey);
				if(0 != result){
					ERR_FLCKPRN("Could not delete key for each thread, error code=%d.", result);
				}
			}
		}

		//
		// Access function to avoid static object initialization order problem
		//
		static FlTidCache& GetObject(void)
		{
			static FlTidCache	Cache;				// singleton
			return Cache;
		}

	public:
		static pthread_key_t& GetTid(void)
		{
			return GetObject().TidKey;
		}

		static bool IsInit(void)
		{
			return GetObject().Initialized;
		}
};

//
// Thread Id Cache: Global function
//
tid_t get_threadid(void)
{
	tid_t	tid;
	if(FlTidCache::IsInit()){
		void*	pData;
		if(NULL == (pData = pthread_getspecific(FlTidCache::GetTid()))){
			tid = gettid();
			int	result = pthread_setspecific(FlTidCache::GetTid(), reinterpret_cast<const void*>(tid));
			if(0 != result){
				ERR_FLCKPRN("Could not set key and value(tid=%d), error code=%d.", tid, result);
			}
		}else{
			tid = static_cast<tid_t>(reinterpret_cast<ssize_t>(pData));
		}
	}else{
		tid = gettid();
	}
	return tid;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
