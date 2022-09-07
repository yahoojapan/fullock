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
 * CREATE:   Fri 29 May 2015
 * REVISION:
 *
 */

#ifndef	FLCKSHM_H
#define	FLCKSHM_H

#include "flckstructure.h"
#include "flckthread.h"
#include "flcklocktype.h"
#include "flckutil.h"

//---------------------------------------------------------
// Class FlShm
//---------------------------------------------------------
class FlShmHelper;

class FlShm
{
	friend class FlShmHelper;

	public:
		typedef enum robust_mode{								// ROBUST mode
			ROBUST_NO			= 0,							// no checking process dead, closed file without unlock
			ROBUST_LOW,											// check only process dead
			ROBUST_HIGH,										// check process dead and closed file without unlock
			ROBUST_DEFAULT		= ROBUST_HIGH					//
		}ROBUSTMODE;

		typedef enum nomap_mode{								// Behavior for request locking when could not mmap.
			NOMAP_ALLOW_NORETRY	= 0,							//	- Always allow
			NOMAP_DENY_NORETRY,									//	- Always deny
			NOMAP_ALLOW_RETRY,									//	- Always allow after retrying mmap
			NOMAP_DENY_RETRY,									//	- Always deny after retrying mmap
			NOMAP_ALLOW			= NOMAP_ALLOW_NORETRY,			//
			NOMAP_DENY			= NOMAP_DENY_NORETRY			//
		}NOMAPMODE;

		typedef enum free_unit_mode{							// Release(free) unit mode
			FREE_NO				=0,								// No release
			FREE_FD,											// Release file lock unit when there is no locking in it
			FREE_OFFSET,										// Release offset lock unit when there is no locking in it
			FREE_ALWAYS			= FREE_FD						//
		}FREEUNITMODE;

	protected:
		static const char*		FLCKAUTOINIT;					// Env name for AUTOINIT
		static const char*		FLCKROBUSTMODE;					// Env name for ROBUSTMODE
		static const char*		FLCKNOMAPMODE;					// Env name for NOMAPMODE
		static const char*		FLCKFREEUNITMODE;				// Env name for FREEUNITMODE
		static const char*		FLCKROBUSTCHKCNT;				// Env name for ROBUSTCHKCNT(checking limit for robust mode)
		static const char*		FLCKUMASK;						// Env name for FLCKUMASK
		static const char*		FLCKDIRPATH;					// Env name for flck shmfile path
		static const char*		FLCKFILENAME;					// Env name for flck shmfile name
		static const char*		FLCKFILECNT;					// Env name : area count for file lock structure
		static const char*		FLCKOFFETCNT;					// Env name : area count for offset lock structure
		static const char*		FLCKLOCKERCNT;					// Env name : area count for locker structure
		static const char*		FLCKNMTXCNT;					// Env name : for named mutex structure
		static const char*		FLCKNCONDCNT;					// Env name : for named cond structure
		static const char*		FLCKWAITERCNT;					// Env name : area count for waiter structure

		// Parameters for management
		static bool				IsAutoInitialize;				// Which initializing or not at constructor for singleton.
		static ROBUSTMODE		RobustMode;						// ROBUST mode
		static NOMAPMODE		NomapMode;						// mode for no mmapping
		static FREEUNITMODE		FreeUnitMode;					// Free Unit mode
		static mode_t			ShmFileUmask;					// Umask for shm file
		static int				RobustLoopCnt;					// limit lock loop count for checking robust mode
		static size_t			FileLockAreaCount;				// area count for file lock structure
		static size_t			OffLockAreaCount;				// area count for offset lock structure
		static size_t			LockerAreaCount;				// area count for locker structure
		static size_t			NMtxAreaCount;					// area count for named mutex structure
		static size_t			NCondAreaCount;					// area count for named cond structure
		static size_t			WaiterAreaCount;				// area count for waiter structure

		// Shared memory
		static int				ShmFd;							// shm file descriptor
		static std::string*		pShmDirPath;					// flck shm Directory path
		static std::string*		pShmFileName;					// flck shm file name
		static std::string*		pShmPath;						// flck shm file path

		// Worker thread
		static FlckThread*		pCheckPidThread;				// thread for checking process dead
		static int				InotifyFd;						// inotify fd for other process dead
		static int				WatchFd;						// watch fd for other process dead
		static int				EventFd;						// epoll fd for other process dead

	public:
		// Shared memory(direct access for performance)
		static void*			pShmBase;						// shared memory base address
		static PFLHEAD			pFlHead;						// header pointer

	protected:
		static bool InitializeSingleton(const void* phelper);	// MAIN SINGLETON OBJECT(for class variables initializing/destroying)
		static std::string&	ShmDirPath(void);
		static std::string&	ShmFileName(void);
		static std::string&	ShmPath(void);
		static bool LoadEnv(void);

		static void PreforkHandler(void);						// for forking
		static bool CheckAttach(void);
		static bool Attach(void);
		static bool Detach(void);
		static bool InitializeObject(bool is_load_env);
		static bool InitializeShm(void);
		static bool InitializeShmFile(void);
		static bool Destroy(void);

		static PFLFILELOCK MakeListFileLock(PFLFILELOCK ptr, size_t count);
		static PFLOFFLOCK MakeListOffLock(PFLOFFLOCK ptr, size_t count);
		static PFLLOCKER MakeListLocker(PFLLOCKER ptr, size_t count);
		static PFLNAMEDMUTEX MakeListNMtxLock(PFLNAMEDMUTEX ptr, size_t count);
		static PFLNAMEDCOND MakeListNCondLock(PFLNAMEDCOND ptr, size_t count);
		static PFLWAITER MakeListWaiter(PFLWAITER ptr, size_t count);

		static int RawLock(FLCKLOCKTYPE LockType, const char* pname, time_t timeout_usec);													// named mutex
		static int RawLock(FLCKLOCKTYPE LockType, int fd, off_t offset, size_t length, time_t timeout_usec);								// file lock(rwlock)
		static int RawLock(FLCKLOCKTYPE LockType, const char* pcondname, const char* pmutexname, bool is_broadcast, time_t timeout_usec);	// named cond

	public:
		static bool ReInitializeObject(const char* dirname = NULL, const char* filename = NULL, size_t filelockcnt = FLCK_INITCNT_DEFAULT, size_t offlockcnt = FLCK_INITCNT_DEFAULT, size_t lockercnt = FLCK_INITCNT_DEFAULT, size_t nmtxcnt = FLCK_INITCNT_DEFAULT, size_t ncondcnt = FLCK_INITCNT_DEFAULT, size_t waitercnt = FLCK_INITCNT_DEFAULT);

		static ROBUSTMODE SetRobustMode(ROBUSTMODE newval);
		static NOMAPMODE SetNomapMode(NOMAPMODE newval);
		static FREEUNITMODE SetFreeUnitMode(FREEUNITMODE newval);
		static int SetRobustLoopCnt(int newval);
		static size_t SetFileLockAreaCount(size_t newval);
		static size_t SetOffLockAreaCount(size_t newval);
		static size_t SetLockerAreaCount(size_t newval);
		static size_t SetNMtxAreaCount(size_t newval);
		static size_t SetNCondAreaCount(size_t newval);
		static size_t SetWaiterAreaCount(size_t newval);

		static bool IsNoRobust(void) { return (ROBUST_NO == FlShm::RobustMode); }
		static bool IsRobust(void) { return (ROBUST_NO != FlShm::RobustMode); }
		static bool IsHighRobust(void) { return (ROBUST_HIGH == FlShm::RobustMode); }
		static NOMAPMODE GetNomapMode(void) { return FlShm::NomapMode; }
		static bool IsFreeUnitFd(void) { return (FREE_FD == FlShm::FreeUnitMode); }
		static bool IsFreeUnitOffset(void) { return (FREE_FD == FlShm::FreeUnitMode || FREE_OFFSET == FlShm::FreeUnitMode); }
		static int GetRobustLoopCnt(void) { return FlShm::RobustLoopCnt; }
		static size_t GetFileLockAreaCount(void) { return FlShm::FileLockAreaCount; }
		static size_t GetOffLockAreaCount(void) { return FlShm::OffLockAreaCount; }
		static size_t GetLockerAreaCount(void) { return FlShm::LockerAreaCount; }
		static size_t GetNMtxAreaCount(void) { return FlShm::NMtxAreaCount; }
		static size_t GetNCondAreaCount(void) { return FlShm::NCondAreaCount; }
		static size_t GetWaiterAreaCount(void) { return FlShm::WaiterAreaCount; }

		// Check
		static bool CheckProcessDead(void);
		static bool CheckFileLockDeadLock(fl_pid_cache_map_t* pcache_map = NULL, flckpid_t flckpid = FLCK_INVALID_ID, flckpid_t except_flckpid = FLCK_INVALID_ID);
		static bool CheckMutexDeadLock(fl_pid_cache_map_t* pcache_map = NULL, flckpid_t flckpid = FLCK_INVALID_ID, flckpid_t except_flckpid = FLCK_INVALID_ID);
		static bool CheckCondDeadLock(fl_pid_cache_map_t* pcache_map = NULL, flckpid_t flckpid = FLCK_INVALID_ID, flckpid_t except_flckpid = FLCK_INVALID_ID);

	public:
		// Constructor/Destructor
		explicit FlShm(void* phelper = NULL) { InitializeSingleton(phelper); }
		virtual ~FlShm(void) {}

		//
		// Lock/Unlock for named mutex
		//
		int TimeoutLock(const char* pname, time_t timeout_usec) { return RawLock(FLCK_NMTX_LOCK, pname, timeout_usec); }
		int TryLock(const char* pname) { return RawLock(FLCK_NMTX_LOCK, pname, FLCK_TRY_TIMEOUT); }
		int Lock(const char* pname) { return RawLock(FLCK_NMTX_LOCK, pname, FLCK_NO_TIMEOUT); }
		int Unlock(const char* pname) { return RawLock(FLCK_UNLOCK, pname, FLCK_NO_TIMEOUT); }

		//
		// Lock/Unlock for rwlock
		//
		bool IsLocked(int fd, off_t offset, size_t length);

		int TimeoutReadLock(int fd, off_t offset, size_t length, time_t timeout_usec) { return RawLock(FLCK_READ_LOCK, fd, offset, length, timeout_usec); }
		int TryReadLock(int fd, off_t offset, size_t length) { return RawLock(FLCK_READ_LOCK, fd, offset, length, FLCK_TRY_TIMEOUT); }
		int ReadLock(int fd, off_t offset, size_t length) { return RawLock(FLCK_READ_LOCK, fd, offset, length, FLCK_NO_TIMEOUT); }

		int TimeoutWriteLock(int fd, off_t offset, size_t length, time_t timeout_usec) { return RawLock(FLCK_WRITE_LOCK, fd, offset, length, timeout_usec); }
		int TryWriteLock(int fd, off_t offset, size_t length) { return RawLock(FLCK_WRITE_LOCK, fd, offset, length, FLCK_TRY_TIMEOUT); }
		int WriteLock(int fd, off_t offset, size_t length) { return RawLock(FLCK_WRITE_LOCK, fd, offset, length, FLCK_NO_TIMEOUT); }

		int Unlock(int fd, off_t offset, size_t length) { return RawLock(FLCK_UNLOCK, fd, offset, length, FLCK_NO_TIMEOUT); }

		//
		// Wait/Signal for named cond
		//
		int TimeoutWait(const char* pcondname, const char* pmutexname, time_t timeout_usec) { return RawLock(FLCK_NCOND_WAIT, pcondname, pmutexname, false, timeout_usec); }
		int Wait(const char* pcondname, const char* pmutexname) { return RawLock(FLCK_NCOND_WAIT, pcondname, pmutexname, false, FLCK_NO_TIMEOUT); }
		int Signal(const char* pcondname) { return RawLock(FLCK_NCOND_UP, pcondname, NULL, false, FLCK_NO_TIMEOUT); }
		int Broadcast(const char* pcondname) { return RawLock(FLCK_NCOND_UP, pcondname, NULL, true, FLCK_NO_TIMEOUT); }

		// For debug
		static bool Dump(std::ostream& out, bool is_free_list = true);
};

//---------------------------------------------------------
// Utility Macros
//---------------------------------------------------------
#define	to_rel(abs_addr)	(abs_addr ? SUBPTR(abs_addr, reinterpret_cast<off_t>(FlShm::pShmBase)) : abs_addr)
#define	to_abs(rel_addr)	(rel_addr ? ADDPTR(rel_addr, reinterpret_cast<off_t>(FlShm::pShmBase)) : rel_addr)

#endif	// FLCKSHM_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
