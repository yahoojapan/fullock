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

#include <assert.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "flckcommon.h"
#include "flckshm.h"
#include "flcklistlocker.h"
#include "flcklistofflock.h"
#include "flcklistfilelock.h"
#include "flcklistnmtx.h"
#include "flcklistncond.h"
#include "flckutil.h"
#include "flckdbg.h"

using namespace std;
using namespace fullock;

//---------------------------------------------------------
// Symbols
//---------------------------------------------------------
#define	FLCK_INITIAL_LOADED_VERSION				1

#define	DEFAULT_SHM_ANTPICKAX_DIRPATH			"/var/lib/antpickax"
#define	DEFAULT_SHM_SUB_DIRPATH					"/tmp"
#define	DEFAULT_SHM_DIRNAME						".fullock"
#define	DEFAULT_SHM_FILENAME					"fullock.shm"

#define	FLCK_FLCKAUTOINIT_YES_STR				"YES"
#define	FLCK_FLCKAUTOINIT_NO_STR				"NO"

#define	FLCK_ROBUSTMODE_NO_STR					"NO"
#define	FLCK_ROBUSTMODE_LOW_STR					"LOW"
#define	FLCK_ROBUSTMODE_HIGH_STR				"HIGH"

#define	FLCK_NOMAPMODE_ALLOW_NORETRY_STR		"ALLOW_NORETRY"
#define	FLCK_NOMAPMODE_DENY_NORETRY_STR			"DENY_NORETRY"
#define	FLCK_NOMAPMODE_ALLOW_RETRY_STR			"ALLOW_RETRY"
#define	FLCK_NOMAPMODE_DENY_RETRY_STR			"DENY_RETRY"
#define	FLCK_NOMAPMODE_ALLOW_STR				"ALLOW"
#define	FLCK_NOMAPMODE_DENY_STR					"DENY"

#define	FLCK_FREEUNITMODE_NO_STR				"NO"
#define	FLCK_FREEUNITMODE_FD_STR				"FD"
#define	FLCK_FREEUNITMODE_OFFSET_STR			"OFFSET"
#define	FLCK_FREEUNITMODE_ALWAYS_STR			"ALWAYS"

#define	FLCK_FLCKFILECNT_DEFAULT				128					// default area count for file lock structure
#define	FLCK_FLCKFILECNT_MIN					1
#define	FLCK_FLCKFILECNT_MAX					2048
#define	FLCK_FLCKOFFETCNT_DEFAULT				8192				// default area count for offset lock structure
#define	FLCK_FLCKOFFETCNT_MIN					4
#define	FLCK_FLCKOFFETCNT_MAX					16384
#define	FLCK_FLCKLOCKERCNT_DEFAULT				8192				// default area count for locker structure
#define	FLCK_FLCKLOCKERCNT_MIN					4
#define	FLCK_FLCKLOCKERCNT_MAX					16384
#define	FLCK_FLCKNMTXCNT_DEFAULT				256					// default area count for named mutex structure
#define	FLCK_FLCKNMTXCNT_MIN					1
#define	FLCK_FLCKNMTXCNT_MAX					8192
#define	FLCK_FLCKNCONDCNT_DEFAULT				256					// default area count for named cond structure
#define	FLCK_FLCKNCONDCNT_MIN					1
#define	FLCK_FLCKNCONDCNT_MAX					8192
#define	FLCK_FLCKWAITERCNT_DEFAULT				8192				// default area count for waiter structure
#define	FLCK_FLCKWAITERCNT_MIN					4
#define	FLCK_FLCKWAITERCNT_MAX					16384

//---------------------------------------------------------
// Helper class
//---------------------------------------------------------
// [NOTE]
// To avoid static object initialization order problem(SIOF)
//
class FlShmHelper
{
	protected:
		FlShm			flckshm;
		string			ShmDirPath;					// flck shm Directory path
		string			ShmFileName;				// flck shm file name
		string			ShmPath;					// flck shm file path

	protected:
		FlShmHelper(void) : flckshm(this), ShmDirPath(""), ShmFileName(""), ShmPath("")
		{
			FlShm::pShmDirPath	= &ShmDirPath;
			FlShm::pShmFileName	= &ShmFileName;
			FlShm::pShmPath		= &ShmPath;
			FlShm::InitializeObject(true);
		}
		virtual ~FlShmHelper(void)
		{
			FlShm::Destroy();
			FlShm::pShmDirPath	= NULL;
			FlShm::pShmFileName	= NULL;
			FlShm::pShmPath		= NULL;
		}

		static FlShmHelper& GetFlShmHelper(void)
		{
			static FlShmHelper	helper;				// singleton
			return helper;
		}

	public:
		static bool Initialize(void)
		{
			(void)GetFlShmHelper();
			return true;
		}

		static string& GetShmDirPath(void)
		{
			if(!FlShm::pShmDirPath){
				Initialize();
			}
			return *(FlShm::pShmDirPath);
		}

		static string& GetShmFileName(void)
		{
			if(!FlShm::pShmFileName){
				Initialize();
			}
			return *(FlShm::pShmFileName);
		}

		static string& GetShmPath(void)
		{
			if(!FlShm::pShmPath){
				Initialize();
			}
			return *(FlShm::pShmPath);
		}
};

//---------------------------------------------------------
// FlShm : Class Variable
//---------------------------------------------------------
const char*			FlShm::FLCKAUTOINIT			= "FLCKAUTOINIT";
const char*			FlShm::FLCKROBUSTMODE		= "FLCKROBUSTMODE";
const char*			FlShm::FLCKNOMAPMODE		= "FLCKNOMAPMODE";
const char*			FlShm::FLCKFREEUNITMODE		= "FLCKFREEUNITMODE";
const char*			FlShm::FLCKROBUSTCHKCNT		= "FLCKROBUSTCHKCNT";
const char*			FlShm::FLCKUMASK			= "FLCKUMASK";
const char*			FlShm::FLCKDIRPATH			= "FLCKDIRPATH";
const char*			FlShm::FLCKFILENAME			= "FLCKFILENAME";
const char*			FlShm::FLCKFILECNT			= "FLCKFILECNT";
const char*			FlShm::FLCKOFFETCNT			= "FLCKOFFETCNT";
const char*			FlShm::FLCKLOCKERCNT		= "FLCKLOCKERCNT";
const char*			FlShm::FLCKNMTXCNT			= "FLCKNMTXCNT";
const char*			FlShm::FLCKNCONDCNT			= "FLCKNCONDCNT";
const char*			FlShm::FLCKWAITERCNT		= "FLCKWAITERCNT";

bool				FlShm::IsAutoInitialize		= true;
FlShm::ROBUSTMODE	FlShm::RobustMode			= FlShm::ROBUST_DEFAULT;
FlShm::NOMAPMODE	FlShm::NomapMode			= FlShm::NOMAP_ALLOW_NORETRY;
FlShm::FREEUNITMODE	FlShm::FreeUnitMode			= FlShm::FREE_FD;
mode_t				FlShm::ShmFileUmask			= 0;
int					FlShm::RobustLoopCnt		= FLCK_ROBUST_CHKCNT_DEFAULT;
size_t				FlShm::FileLockAreaCount	= FLCK_FLCKFILECNT_DEFAULT;
size_t				FlShm::OffLockAreaCount		= FLCK_FLCKOFFETCNT_DEFAULT;
size_t				FlShm::LockerAreaCount		= FLCK_FLCKLOCKERCNT_DEFAULT;
size_t				FlShm::NMtxAreaCount		= FLCK_FLCKNMTXCNT_DEFAULT;
size_t				FlShm::NCondAreaCount		= FLCK_FLCKNCONDCNT_DEFAULT;
size_t				FlShm::WaiterAreaCount		= FLCK_FLCKWAITERCNT_DEFAULT;

int					FlShm::ShmFd				= FLCK_INVALID_HANDLE;
std::string*		FlShm::pShmDirPath			= NULL;
std::string*		FlShm::pShmFileName			= NULL;
std::string*		FlShm::pShmPath				= NULL;
void*				FlShm::pShmBase				= NULL;
PFLHEAD				FlShm::pFlHead				= NULL;
FlckThread*			FlShm::pCheckPidThread		= NULL;
int					FlShm::InotifyFd			= FLCK_INVALID_HANDLE;
int					FlShm::WatchFd				= FLCK_INVALID_HANDLE;
int					FlShm::EventFd				= FLCK_INVALID_HANDLE;

//---------------------------------------------------------
// FlShm : Class Method
//---------------------------------------------------------
// [NOTE]
// To avoid static object initialization order problem(SIOF)
//
bool FlShm::InitializeSingleton(const void* phelper)
{
	if(phelper){
		return true;
	}
	return FlShmHelper::Initialize();
}

string& FlShm::ShmDirPath(void)
{
	return FlShmHelper::GetShmDirPath();
}

string& FlShm::ShmFileName(void)
{
	return FlShmHelper::GetShmFileName();
}

string& FlShm::ShmPath(void)
{
	return FlShmHelper::GetShmPath();
}

//---------------------------------------------------------
// FlShm : Initialize variables
//---------------------------------------------------------
bool FlShm::InitializeObject(bool is_load_env)
{
	FlShm::ShmPath().erase();

	// Load debug environment
	if(is_load_env && !LoadFlckDbgEnv()){
		// continue...
	}
	if(is_load_env && !FlShm::LoadEnv()){
		return false;
	}
	if(is_load_env && !FlShm::IsAutoInitialize){
		// not initializing
		return true;
	}

	if(FlShm::ShmPath().empty()){
		// Check working directory path
		if(FlShm::ShmDirPath().empty()){
			string	toppath;
			struct stat	st;
			if(0 != stat(DEFAULT_SHM_ANTPICKAX_DIRPATH, &st)){
				MSG_FLCKPRN("Not found %s directory, then use %s directory as default.", DEFAULT_SHM_ANTPICKAX_DIRPATH, DEFAULT_SHM_SUB_DIRPATH);
				toppath = DEFAULT_SHM_SUB_DIRPATH;
			}else{
				if(0 == (st.st_mode & S_IFDIR)){
					MSG_FLCKPRN("%s is not directory, then use %s directory as default.", DEFAULT_SHM_ANTPICKAX_DIRPATH, DEFAULT_SHM_SUB_DIRPATH);
					toppath = DEFAULT_SHM_SUB_DIRPATH;
				}else{
					MSG_FLCKPRN("Found %s directory, then use it as default.", DEFAULT_SHM_ANTPICKAX_DIRPATH);
					toppath = DEFAULT_SHM_ANTPICKAX_DIRPATH;
				}
			}
			toppath				+= "/" DEFAULT_SHM_DIRNAME;
			FlShm::ShmDirPath()	 = toppath;
		}

		// Set & Check working directory
		if(!MakeWorkDirectory(FlShm::ShmDirPath().c_str())){
			ERR_FLCKPRN("Failed to make working directory(%s).", FlShm::ShmPath().c_str());
			return false;
		}
		FlShm::ShmPath()  = FlShm::ShmDirPath();
		FlShm::ShmPath() += "/";
		FlShm::ShmPath() += FlShm::ShmFileName();
	}

	// Initialize
	if(!FlShm::InitializeShm()){
		ERR_FLCKPRN("Failed to initialize.");
		return false;
	}
	return true;
}

bool FlShm::ReInitializeObject(const char* dirname, const char* filename, size_t filelockcnt, size_t offlockcnt, size_t lockercnt, size_t nmtxcnt, size_t ncondcnt, size_t waitercnt)
{
	// check parameters
	if(	(FLCK_INITCNT_DEFAULT != filelockcnt	&& (filelockcnt < FLCK_FLCKFILECNT_MIN	|| FLCK_FLCKFILECNT_MAX < filelockcnt))	||
		(FLCK_INITCNT_DEFAULT != offlockcnt		&& (offlockcnt < FLCK_FLCKOFFETCNT_MIN	|| FLCK_FLCKOFFETCNT_MAX < offlockcnt))	||
		(FLCK_INITCNT_DEFAULT != lockercnt		&& (lockercnt < FLCK_FLCKLOCKERCNT_MIN	|| FLCK_FLCKLOCKERCNT_MAX < lockercnt))	||
		(FLCK_INITCNT_DEFAULT != nmtxcnt		&& (nmtxcnt < FLCK_FLCKNMTXCNT_MIN		|| FLCK_FLCKNMTXCNT_MAX < nmtxcnt))		||
		(FLCK_INITCNT_DEFAULT != ncondcnt		&& (ncondcnt < FLCK_FLCKNCONDCNT_MIN	|| FLCK_FLCKNCONDCNT_MAX < ncondcnt))	||
		(FLCK_INITCNT_DEFAULT != lockercnt		&& (waitercnt < FLCK_FLCKWAITERCNT_MIN	|| FLCK_FLCKWAITERCNT_MAX < waitercnt))	)
	{
		ERR_FLCKPRN("Parameters are wrong: filelock cnt(%zu), offsetlock cnt(%zu), locker cnt(%zu), named mutex cnt(%zu), named cond cnt(%zu), waiter cnt(%zu)", filelockcnt, offlockcnt, lockercnt, nmtxcnt, ncondcnt, waitercnt);
		return false;
	}
	// check dirpath
	if(!FLCKEMPTYSTR(dirname)){
		string	toppath;
		if(!GetRealPath(dirname, toppath)){
			ERR_FLCKPRN("Parameter directory path(%s) is invalid.", dirname);
			return false;
		}
		// check & make dir
		if(!MakeWorkDirectory(toppath.c_str())){
			ERR_FLCKPRN("Directory path(%s) is invalid or no such directory(could not make it).", dirname);
			return false;
		}
	}
	// check filename
	if(!FLCKEMPTYSTR(filename)){
		string	filepath = filename;
		filepath = trim(filepath);
		if(string::npos != filepath.find('/')){
			ERR_FLCKPRN("File name(%s) is invalid.", filename);
			return false;
		}
	}

	// Destroy this
	if(!FlShm::Destroy()){
		ERR_FLCKPRN("Failed to destroy now fullock object.");
		return false;
	}

	// set dirpath as default
	{
		string	toppath;
		struct stat	st;
		if(0 != stat(DEFAULT_SHM_ANTPICKAX_DIRPATH, &st)){
			MSG_FLCKPRN("Not found %s directory, then use %s directory as default.", DEFAULT_SHM_ANTPICKAX_DIRPATH, DEFAULT_SHM_SUB_DIRPATH);
			toppath = DEFAULT_SHM_SUB_DIRPATH;
		}else{
			if(0 == (st.st_mode & S_IFDIR)){
				MSG_FLCKPRN("%s is not directory, then use %s directory as default.", DEFAULT_SHM_ANTPICKAX_DIRPATH, DEFAULT_SHM_SUB_DIRPATH);
				toppath = DEFAULT_SHM_SUB_DIRPATH;
			}else{
				MSG_FLCKPRN("Found %s directory, then use it as default.", DEFAULT_SHM_ANTPICKAX_DIRPATH);
				toppath = DEFAULT_SHM_ANTPICKAX_DIRPATH;
			}
		}
		toppath				+= "/" DEFAULT_SHM_DIRNAME;
		FlShm::ShmDirPath()	 = toppath;
	}

	// set count values as default
	FlShm::ShmFileName()		= DEFAULT_SHM_FILENAME;
	FlShm::FileLockAreaCount	= FLCK_FLCKFILECNT_DEFAULT;
	FlShm::OffLockAreaCount		= FLCK_FLCKOFFETCNT_DEFAULT;
	FlShm::LockerAreaCount		= FLCK_FLCKLOCKERCNT_DEFAULT;
	FlShm::NMtxAreaCount		= FLCK_FLCKNMTXCNT_DEFAULT;

	// Load environment manually
	if(!FlShm::LoadEnv()){
		ERR_FLCKPRN("Failed to load environment.");
		return false;
	}
	// force to set true.
	FlShm::IsAutoInitialize = true;

	// overwrite parameters
	if(!FLCKEMPTYSTR(dirname)){
		GetRealPath(dirname, FlShm::ShmDirPath());
	}
	if(!FLCKEMPTYSTR(filename)){
		FlShm::ShmFileName() = filename;
		FlShm::ShmFileName() = trim(FlShm::ShmFileName());
	}
	if(FLCK_INITCNT_DEFAULT != filelockcnt){
		FlShm::FileLockAreaCount = filelockcnt;
	}
	if(FLCK_INITCNT_DEFAULT != offlockcnt){
		FlShm::OffLockAreaCount = offlockcnt;
	}
	if(FLCK_INITCNT_DEFAULT != lockercnt){
		FlShm::LockerAreaCount = lockercnt;
	}
	if(FLCK_INITCNT_DEFAULT != nmtxcnt){
		FlShm::NMtxAreaCount = nmtxcnt;
	}
	if(FLCK_INITCNT_DEFAULT != ncondcnt){
		FlShm::NCondAreaCount = ncondcnt;
	}
	if(FLCK_INITCNT_DEFAULT != waitercnt){
		FlShm::WaiterAreaCount = waitercnt;
	}

	if(!FlShm::InitializeObject(false)){
		ERR_FLCKPRN("Failed to Initialize fullock object.");
		return false;
	}
	return true;
}

//---------------------------------------------------------
// Set Variables
//---------------------------------------------------------
// [NOTE]
// These variable is not locked when modifying. Be careful.
//
FlShm::ROBUSTMODE FlShm::SetRobustMode(FlShm::ROBUSTMODE newval)
{
	ROBUSTMODE	oldval	= FlShm::RobustMode;
	FlShm::RobustMode	= newval;

	if(FlShm::ROBUST_HIGH == FlShm::RobustMode){
		if(FlShm::RobustLoopCnt < FLCK_ROBUST_CHKCNT_MIN){
			FlShm::RobustLoopCnt = FLCK_ROBUST_CHKCNT_DEFAULT;
		}
	}
	return oldval;
}

FlShm::NOMAPMODE FlShm::SetNomapMode(FlShm::NOMAPMODE newval)
{
	NOMAPMODE	oldval	= FlShm::NomapMode;
	FlShm::NomapMode	= newval;
	return oldval;
}

FlShm::FREEUNITMODE FlShm::SetFreeUnitMode(FlShm::FREEUNITMODE newval)
{
	FREEUNITMODE	oldval	= FlShm::FreeUnitMode;
	FlShm::FreeUnitMode		= newval;
	return oldval;
}

int FlShm::SetRobustLoopCnt(int newval)
{
	if(FlShm::ROBUST_HIGH != FlShm::RobustMode){
		return -1;
	}
	if(newval < FLCK_ROBUST_CHKCNT_MIN){
		return -1;
	}
	int	oldval			= FlShm::RobustLoopCnt;
	FlShm::RobustLoopCnt= newval;
	return oldval;
}

size_t FlShm::SetFileLockAreaCount(size_t newval)
{
	size_t	oldval			= FlShm::FileLockAreaCount;
	FlShm::FileLockAreaCount= newval;
	return oldval;
}

size_t FlShm::SetOffLockAreaCount(size_t newval)
{
	size_t	oldval			= FlShm::OffLockAreaCount;
	FlShm::OffLockAreaCount	= newval;
	return oldval;
}

size_t FlShm::SetLockerAreaCount(size_t newval)
{
	size_t	oldval			= FlShm::LockerAreaCount;
	FlShm::LockerAreaCount	= newval;
	return oldval;
}

size_t FlShm::SetNMtxAreaCount(size_t newval)
{
	size_t	oldval			= FlShm::NMtxAreaCount;
	FlShm::NMtxAreaCount	= newval;
	return oldval;
}

size_t FlShm::SetNCondAreaCount(size_t newval)
{
	size_t	oldval			= FlShm::NCondAreaCount;
	FlShm::NCondAreaCount	= newval;
	return oldval;
}

size_t FlShm::SetWaiterAreaCount(size_t newval)
{
	size_t	oldval			= FlShm::WaiterAreaCount;
	FlShm::WaiterAreaCount	= newval;
	return oldval;
}

//---------------------------------------------------------
// FlShm : Processes Methods
//---------------------------------------------------------
bool FlShm::CheckProcessDead(void)
{
	if(FLCK_INVALID_HANDLE == FlShm::ShmFd){
		return false;
	}
	flckpid_t			flckpid	= get_flckpid();
	fl_pid_cache_map_t	cache_map;

	// check file lock list
	CheckFileLockDeadLock(&cache_map, flckpid);

	// check mutex list
	CheckMutexDeadLock(&cache_map, flckpid);

	// check cond list
	CheckCondDeadLock(&cache_map, flckpid);

	return true;
}

// Returns	false	: does not dead lock
//			true	: this object is dead lock and force unlock this.
//
bool FlShm::CheckFileLockDeadLock(fl_pid_cache_map_t* pcache_map, flckpid_t flckpid, flckpid_t except_flckpid)
{
	if(FLCK_INVALID_HANDLE == FlShm::ShmFd){
		return false;
	}
	if(!FlShm::pFlHead->file_lock_list){
		return false;
	}

	if(FLCK_INVALID_ID == flckpid){
		flckpid	= get_flckpid();
	}
	fl_lock_lockid(&FlShm::pFlHead->file_lock_lockid, flckpid);		// lock lockid for top manually.(keep to lock)

	FlListFileLock		tmpobj;
	bool				result = false;		// true means that found deadlock and force unlock it.
	for(PFLFILELOCK pParent = NULL, ptmp = to_abs(FlShm::pFlHead->file_lock_list); ptmp; ){
		tmpobj.set(ptmp);
		if(tmpobj.check_dead_lock(pcache_map, except_flckpid)){
			// retrieve target list
			if(tmpobj.cutoff_list(FlShm::pFlHead->file_lock_list)){
				// return object to free list
				if(!tmpobj.insert_list(FlShm::pFlHead->file_lock_free)){
					ERR_FLCKPRN("Failed to insert file lock to free list, but continue...");
				}
			}
			// set next
			if(pParent){
				ptmp = to_abs(pParent->next);
			}else{
				ptmp = to_abs(FlShm::pFlHead->file_lock_list);
			}
			result = true;
		}else{
			// set next
			pParent	= ptmp;
			ptmp	= to_abs(ptmp->next);
		}
	}
	fl_unlock_lockid(&FlShm::pFlHead->file_lock_lockid, flckpid);	// unlock lockid

	return result;
}

// Returns	false	: does not dead lock
//			true	: this object is dead lock and force unlock this.
//
bool FlShm::CheckMutexDeadLock(fl_pid_cache_map_t* pcache_map, flckpid_t flckpid, flckpid_t except_flckpid)
{
	if(FLCK_INVALID_HANDLE == FlShm::ShmFd){
		return false;
	}
	if(!FlShm::pFlHead->named_mutex_list){
		return false;
	}

	if(FLCK_INVALID_ID == flckpid){
		flckpid	= get_flckpid();
	}
	fl_lock_lockid(&FlShm::pFlHead->named_mutex_lockid, flckpid);		// lock lockid for top manually.(keep to lock)

	// check mutex list
	FlListNMtx			tmpobj;
	bool				result = false;		// true means that found deadlock and force unlock it.
	for(PFLNAMEDMUTEX ptmp = to_abs(FlShm::pFlHead->named_mutex_list); ptmp; ptmp = to_abs(ptmp->next)){
		tmpobj.set(ptmp);
		if(tmpobj.check_dead_lock(pcache_map, except_flckpid)){
			result = true;
		}
	}
	fl_unlock_lockid(&FlShm::pFlHead->named_mutex_lockid, flckpid);	// unlock lockid

	return result;
}

// Returns	false	: does not dead lock
//			true	: this object is dead lock and force unlock this.
//
bool FlShm::CheckCondDeadLock(fl_pid_cache_map_t* pcache_map, flckpid_t flckpid, flckpid_t except_flckpid)
{
	if(FLCK_INVALID_HANDLE == FlShm::ShmFd){
		return false;
	}
	if(!FlShm::pFlHead->named_cond_list){
		return false;
	}

	if(FLCK_INVALID_ID == flckpid){
		flckpid	= get_flckpid();
	}
	fl_lock_lockid(&FlShm::pFlHead->named_cond_lockid, flckpid);		// lock lockid for top manually.(keep to lock)

	FlListNCond		tmpobj;
	bool			result = false;		// true means that found deadlock and force unlock it.
	for(PFLNAMEDCOND ptmp = to_abs(FlShm::pFlHead->named_cond_list); ptmp; ptmp = to_abs(ptmp->next)){
		tmpobj.set(ptmp);
		if(tmpobj.check_dead_lock(pcache_map, except_flckpid)){
			result = true;
		}
	}
	fl_unlock_lockid(&FlShm::pFlHead->named_cond_lockid, flckpid);	// unlock lockid

	return result;
}

//---------------------------------------------------------
// FlShm : For Forking
//---------------------------------------------------------
// When the program which loads this fullock library is forked, we need to start worker thread
// in child process.
//
void FlShm::PreforkHandler(void)
{
	if(FlShm::pCheckPidThread){
		if(!FlShm::pCheckPidThread->ReInitializeThread()){
			ERR_FLCKPRN("Call Prefork handler and try to reinitialize thread for child process(%d), but FAILED TO RUN THREAD", getpid());
		}
		if(!FlShm::pCheckPidThread->Run()){
			ERR_FLCKPRN("Call Prefork handler and try to run thread for child process(%d), but FAILED TO RUN THREAD", getpid());
		}
	}
}

//---------------------------------------------------------
// FlShm : Lock
//---------------------------------------------------------
bool FlShm::CheckAttach(void)
{
	if(FLCK_INVALID_HANDLE != FlShm::ShmFd){
		return true;
	}
	if(NOMAP_ALLOW_RETRY == FlShm::NomapMode || NOMAP_DENY_RETRY == FlShm::NomapMode){
		if(FlShm::InitializeShm()){
			// Success to initialize
			return true;
		}
		ERR_FLCKPRN("Failed to initialize.");
	}
	return false;
}

int FlShm::RawLock(FLCKLOCKTYPE LockType, const char* pname, time_t timeout_usec)
{
	if(!pname){
		ERR_FLCKPRN("Parameter is wrong.");
		return EINVAL;							// EINVAL
	}
	// check mapping
	if(!FlShm::CheckAttach()){
		ERR_FLCKPRN("Does not attach shm.");
		return ((NOMAP_ALLOW_RETRY == FlShm::NomapMode || NOMAP_ALLOW_NORETRY == FlShm::NomapMode) ? 0 : ENOLCK);		// ENOLCK
	}

	flckpid_t	flckpid	= get_flckpid();
	int			result	= 0;

	fl_lock_lockid(&FlShm::pFlHead->named_mutex_lockid, flckpid);					// lock lockid for top manually.(keep to lock)

	if(FLCK_UNLOCK == LockType){
		// UNLOCK
		FlListNMtx		tglistobj;
		if(!tglistobj.find(pname, FlShm::pFlHead->named_mutex_list)){
			// not found target.
			ERR_FLCKPRN("Could not locking named mutex for name(%s).", pname);
			fl_unlock_lockid(&FlShm::pFlHead->named_mutex_lockid, flckpid);		// unlock lockid
			return EINVAL;						// EINVAL
		}

		// do unlock
		if(0 != (result = tglistobj.unlock())){
			ERR_FLCKPRN("Could not unlock named mutex(error code=%d) for name(%s).", result, pname);
		}
		fl_unlock_lockid(&FlShm::pFlHead->named_mutex_lockid, flckpid);			// unlock lockid

	}else{
		// LOCK
		FlListNMtx		tglistobj;

		if(!tglistobj.find(pname, FlShm::pFlHead->named_mutex_list)){
			// Not found, so get new file lock and insert it.
			if(!tglistobj.retrieve_list(FlShm::pFlHead->named_mutex_free)){
				ERR_FLCKPRN("Could not get free named mutex structure.");
				fl_unlock_lockid(&FlShm::pFlHead->named_mutex_lockid, flckpid);	// unlock lockid
				return ENOLCK;					// ENOLCK
			}
			// initialize
			tglistobj.initialize(pname);

			// insert file lock into list
			if(!tglistobj.insert_list(FlShm::pFlHead->named_mutex_list)){
				ERR_FLCKPRN("Failed to insert named mutex to top list.");
				// for recover
				if(!tglistobj.insert_list(FlShm::pFlHead->named_mutex_free)){
					ERR_FLCKPRN("Failed to insert named mutex to free list, but continue...");
				}
				fl_unlock_lockid(&FlShm::pFlHead->named_mutex_lockid, flckpid);	// unlock lockid
				return ENOLCK;					// ENOLCK
			}
		}
		fl_unlock_lockid(&FlShm::pFlHead->named_mutex_lockid, flckpid);			// unlock lockid

		// do lock
		if(0 != (result = tglistobj.lock(timeout_usec))){
			ERR_FLCKPRN("Could not lock named mutex object(error code=%d) for name(%s).", result, pname);
			// do not remove named mutex
			return result;
		}
	}
	return result;
}

int FlShm::RawLock(FLCKLOCKTYPE LockType, int fd, off_t offset, size_t length, time_t timeout_usec)
{
	// check mapping
	if(!FlShm::CheckAttach()){
		ERR_FLCKPRN("Does not attach shm.");
		return ((NOMAP_ALLOW_RETRY == FlShm::NomapMode || NOMAP_ALLOW_NORETRY == FlShm::NomapMode) ? 0 : ENOLCK);			// ENOLCK
	}

	// device id/inode
	dev_t	devid		= FLCK_INVALID_ID;
	ino_t	inodeid		= FLCK_INVALID_ID;
	if(!GetFileDevNode(fd, devid, inodeid)){
		ERR_FLCKPRN("Failed to get device id/inode from fd(%d) offset(%zd) length(%zu)", fd, offset, length);
		return ((NOMAP_ALLOW_RETRY == FlShm::NomapMode || NOMAP_ALLOW_NORETRY == FlShm::NomapMode) ? 0 : ENOLCK);			// ENOLCK
	}
	flckpid_t	flckpid	= get_flckpid();
	int			result	= 0;

	fl_lock_lockid(&FlShm::pFlHead->file_lock_lockid, flckpid);					// lock lockid for top manually.(keep to lock)

	if(FLCK_UNLOCK == LockType){
		// UNLOCK
		FlListFileLock	tglistobj;
		if(!tglistobj.find(devid, inodeid, FlShm::pFlHead->file_lock_list)){
			// not found target.
			ERR_FLCKPRN("Could not locking file lock for fd(%d), offset(%zd), length(%zu).", fd, offset, length);
			fl_unlock_lockid(&FlShm::pFlHead->file_lock_lockid, flckpid);			// unlock lockid
			return EINVAL;						// EINVAL
		}

		// do unlock(unlocked lockid after this)
		if(0 != (result = tglistobj.unlock(flckpid, fd, offset, length))){
			ERR_FLCKPRN("Could not unlock file lock(error code=%d) for fd(%d), offset(%zd), length(%zu).", result, fd, offset, length);
			fl_unlock_lockid(&FlShm::pFlHead->file_lock_lockid, flckpid);			// unlock lockid
			return result;
		}

		// check free
		if(FlShm::IsFreeUnitFd()){
			if(!tglistobj.is_locked()){
				// retrieve target list
				if(tglistobj.cutoff_list(FlShm::pFlHead->file_lock_list)){
					// return object to free list
					if(!tglistobj.insert_list(FlShm::pFlHead->file_lock_free)){
						ERR_FLCKPRN("Failed to insert file lock to free list, but continue...");
					}
				}
			}
		}
		fl_unlock_lockid(&FlShm::pFlHead->file_lock_lockid, flckpid);				// unlock lockid

	}else{
		// LOCK
		FlListFileLock	tglistobj;
		if(!tglistobj.find(devid, inodeid, FlShm::pFlHead->file_lock_list)){
			// Not found, so get new file lock and insert it.
			if(!tglistobj.retrieve_list(FlShm::pFlHead->file_lock_free)){
				ERR_FLCKPRN("Could not get free file lock structure.");
				fl_unlock_lockid(&FlShm::pFlHead->file_lock_lockid, flckpid);		// unlock lockid
				return ENOLCK;					// ENOLCK
			}
			// initialize
			tglistobj.initialize(devid, inodeid, true, true);

			// insert file lock into list
			if(!tglistobj.insert_list(FlShm::pFlHead->file_lock_list)){
				ERR_FLCKPRN("Failed to insert file lock to top list.");
				// for recover
				if(!tglistobj.insert_list(FlShm::pFlHead->file_lock_free)){
					ERR_FLCKPRN("Failed to insert file lock to free list, but continue...");
				}
				fl_unlock_lockid(&FlShm::pFlHead->file_lock_lockid, flckpid);		// unlock lockid
				return ENOLCK;					// ENOLCK
			}
		}else{
			tglistobj.set_protect();
		}

		// do lock(unlocked lockid after this)
		if(0 != (result = tglistobj.lock(LockType, flckpid, fd, offset, length, timeout_usec))){
			ERR_FLCKPRN("Could not %s lock file lock(error code=%d) for fd(%d), offset(%zd), length(%zu).", (FLCK_READ_LOCK == LockType ? "read" : "write"), result, fd, offset, length);

			// check remove file lock for recover...
			//
			if(FlShm::IsFreeUnitFd()){
				fl_lock_lockid(&FlShm::pFlHead->file_lock_lockid, flckpid);		// lock lockid

				if(tglistobj.find(devid, inodeid, FlShm::pFlHead->file_lock_list)){
					if(!tglistobj.is_locked()){
						// free all
						tglistobj.free_offset_lock_list();

						// retrieve target list
						if(tglistobj.cutoff_list(FlShm::pFlHead->file_lock_list)){
							// return object to free list
							if(!tglistobj.insert_list(FlShm::pFlHead->file_lock_free)){
								ERR_FLCKPRN("Failed to insert file lock to free list, but continue...");
							}
						}
					}
				}
				fl_unlock_lockid(&FlShm::pFlHead->file_lock_lockid, flckpid);		// unlock lockid
			}
			return result;
		}
	}
	return result;
}

bool FlShm::IsLocked(int fd, off_t offset, size_t length)
{
	// check mapping
	if(!FlShm::CheckAttach()){
		ERR_FLCKPRN("Does not attach shm.");
		return (NOMAP_ALLOW_RETRY != FlShm::NomapMode && NOMAP_ALLOW_NORETRY != FlShm::NomapMode);			// always "allow" means not lock now.
	}

	// device id/inode
	dev_t	devid	= FLCK_INVALID_ID;
	ino_t	inodeid	= FLCK_INVALID_ID;
	if(!GetFileDevNode(fd, devid, inodeid)){
		ERR_FLCKPRN("Failed to get device id/inode from fd(%d) offset(%zd) length(%zu)", fd, offset, length);
		// device does not exist, so it means unlock status.
		//
		return false;
	}

	FlListFileLock	tglistobj;
	flckpid_t		flckpid = get_flckpid();
	fl_lock_lockid(&FlShm::pFlHead->file_lock_lockid, flckpid);			// lock lockid for top manually.(keep to lock)

	if(!tglistobj.find(devid, inodeid, FlShm::pFlHead->file_lock_list)){
		fl_unlock_lockid(&FlShm::pFlHead->file_lock_lockid, flckpid);		// unlock lockid
		return false;
	}

	bool	result = tglistobj.is_locked();
	fl_unlock_lockid(&FlShm::pFlHead->file_lock_lockid, flckpid);			// unlock lockid

	return result;
}

int FlShm::RawLock(FLCKLOCKTYPE LockType, const char* pcondname, const char* pmutexname, bool is_broadcast, time_t timeout_usec)
{
	if(!pcondname){
		ERR_FLCKPRN("Parameter is wrong.");
		return EINVAL;							// EINVAL
	}
	// check mapping
	if(!FlShm::CheckAttach()){
		ERR_FLCKPRN("Does not attach shm.");
		return ((NOMAP_ALLOW_RETRY == FlShm::NomapMode || NOMAP_ALLOW_NORETRY == FlShm::NomapMode) ? 0 : ENOLCK);		// ENOLCK
	}

	flckpid_t	flckpid	= get_flckpid();
	int			result	= 0;

	if(FLCK_NCOND_UP == LockType){
		// SIGNAL or BROADCAST
		fl_lock_lockid(&FlShm::pFlHead->named_cond_lockid, flckpid);			// lock lockid for top manually.(keep to lock)

		FlListNCond		tglistobj;
		if(!tglistobj.find(pcondname, FlShm::pFlHead->named_cond_list)){
			// not found target.
			ERR_FLCKPRN("Could not locking named cond for name(%s).", pcondname);
			fl_unlock_lockid(&FlShm::pFlHead->named_cond_lockid, flckpid);		// unlock lockid
			return EINVAL;						// EINVAL
		}

		// do signal(broadcast)
		if(is_broadcast){
			result = tglistobj.broadcast();
		}else{
			result = tglistobj.signal();
		}
		if(0 != result){
			ERR_FLCKPRN("Could not unlock named cond(error code=%d) for name(%s).", result, pcondname);
		}
		fl_unlock_lockid(&FlShm::pFlHead->named_cond_lockid, flckpid);			// unlock lockid

	}else{
		// FLCK_NCOND_WAIT

		// at first check mutex
		//
		// [NOTE]
		// When calling cond wait, must already make(have) named mutex.
		// So check only existing it here.
		//
		fl_lock_lockid(&FlShm::pFlHead->named_mutex_lockid, flckpid);			// lock mutex lockid for top manually.(keep to lock)

		FlListNMtx		tglistmtxobj;
		if(!tglistmtxobj.find(pmutexname, FlShm::pFlHead->named_mutex_list)){
			// not found target.
			ERR_FLCKPRN("Could not locking named mutex(%s) for named cond(%s).", pmutexname, pcondname);
			fl_unlock_lockid(&FlShm::pFlHead->named_mutex_lockid, flckpid);		// unlock mutex lockid
			return EINVAL;						// EINVAL
		}
		fl_unlock_lockid(&FlShm::pFlHead->named_mutex_lockid, flckpid);			// unlock mutex lockid

		PFLNAMEDMUTEX	abs_nmtx = tglistmtxobj.get();

		// COND
		fl_lock_lockid(&FlShm::pFlHead->named_cond_lockid, flckpid);			// lock lockid for top manually.(keep to lock)

		FlListNCond		tglistobj;
		if(!tglistobj.find(pcondname, FlShm::pFlHead->named_cond_list)){
			// Not found, so get new file lock and insert it.
			if(!tglistobj.retrieve_list(FlShm::pFlHead->named_cond_free)){
				ERR_FLCKPRN("Could not get free named cond structure.");
				fl_unlock_lockid(&FlShm::pFlHead->named_cond_lockid, flckpid);	// unlock lockid
				return ENOLCK;					// ENOLCK
			}
			// initialize
			tglistobj.initialize(pcondname);

			// insert file lock into list
			if(!tglistobj.insert_list(FlShm::pFlHead->named_cond_list)){
				ERR_FLCKPRN("Failed to insert named cond to top list.");
				// for recover
				if(!tglistobj.insert_list(FlShm::pFlHead->named_cond_free)){
					ERR_FLCKPRN("Failed to insert named cond to free list, but continue...");
				}
				fl_unlock_lockid(&FlShm::pFlHead->named_cond_lockid, flckpid);	// unlock lockid
				return ENOLCK;					// ENOLCK
			}
		}
		// do lock(unlock lockid in following method)
		if(0 != (result = tglistobj.wait(abs_nmtx, timeout_usec))){
			ERR_FLCKPRN("Could not lock named cond object(error code=%d) for name(%s).", result, pcondname);
			// do not remove named cond
			return result;
		}
	}
	return result;
}

//---------------------------------------------------------
// FlShm : Other Methods
//---------------------------------------------------------
bool FlShm::LoadEnv(void)
{
	char*	pEnvVal;

	// FLCKAUTOINIT
	if(NULL == (pEnvVal = getenv(FlShm::FLCKAUTOINIT))){
		MSG_FLCKPRN("%s ENV is not set.", FlShm::FLCKAUTOINIT);
	}else{
		if(0 == strcasecmp(pEnvVal, FLCK_FLCKAUTOINIT_YES_STR)){
			MSG_FLCKPRN("ENV %s value %s, set to mode: YES.", FlShm::FLCKAUTOINIT, pEnvVal);
			FlShm::IsAutoInitialize = true;
		}else if(0 == strcasecmp(pEnvVal, FLCK_FLCKAUTOINIT_NO_STR)){
			MSG_FLCKPRN("ENV %s value %s, set to mode: NO.", FlShm::FLCKAUTOINIT, pEnvVal);
			FlShm::IsAutoInitialize = false;
		}else{
			ERR_FLCKPRN("ENV %s value %s is unknown.", FlShm::FLCKAUTOINIT, pEnvVal);
		}
	}

	// FLCKNOMAPMODE
	if(NULL == (pEnvVal = getenv(FlShm::FLCKNOMAPMODE))){
		MSG_FLCKPRN("%s ENV is not set.", FlShm::FLCKNOMAPMODE);
	}else{
		if(0 == strcasecmp(pEnvVal, FLCK_NOMAPMODE_ALLOW_NORETRY_STR)){
			MSG_FLCKPRN("ENV %s value %s, set to mode: NOMAP_ALLOW_NORETRY.", FlShm::FLCKNOMAPMODE, pEnvVal);
			FlShm::NomapMode = FlShm::NOMAP_ALLOW_NORETRY;
		}else if(0 == strcasecmp(pEnvVal, FLCK_NOMAPMODE_DENY_NORETRY_STR)){
			MSG_FLCKPRN("ENV %s value %s, set to mode: NOMAP_DENY_NORETRY.", FlShm::FLCKNOMAPMODE, pEnvVal);
			FlShm::NomapMode = FlShm::NOMAP_DENY_NORETRY;
		}else if(0 == strcasecmp(pEnvVal, FLCK_NOMAPMODE_ALLOW_RETRY_STR)){
			MSG_FLCKPRN("ENV %s value %s, set to mode: NOMAP_ALLOW_RETRY.", FlShm::FLCKNOMAPMODE, pEnvVal);
			FlShm::NomapMode = FlShm::NOMAP_ALLOW_RETRY;
		}else if(0 == strcasecmp(pEnvVal, FLCK_NOMAPMODE_DENY_RETRY_STR)){
			MSG_FLCKPRN("ENV %s value %s, set to mode: NOMAP_DENY_RETRY.", FlShm::FLCKNOMAPMODE, pEnvVal);
			FlShm::NomapMode = FlShm::NOMAP_DENY_RETRY;
		}else if(0 == strcasecmp(pEnvVal, FLCK_NOMAPMODE_ALLOW_STR)){
			MSG_FLCKPRN("ENV %s value %s, set to mode: NOMAP_ALLOW.", FlShm::FLCKNOMAPMODE, pEnvVal);
			FlShm::NomapMode = FlShm::NOMAP_ALLOW;
		}else if(0 == strcasecmp(pEnvVal, FLCK_NOMAPMODE_DENY_STR)){
			MSG_FLCKPRN("ENV %s value %s, set to mode: NOMAP_DENY.", FlShm::FLCKNOMAPMODE, pEnvVal);
			FlShm::NomapMode = FlShm::NOMAP_DENY;
		}else{
			ERR_FLCKPRN("ENV %s value %s is unknown.", FlShm::FLCKNOMAPMODE, pEnvVal);
		}
	}

	// FLCKROBUSTMODE
	if(NULL == (pEnvVal = getenv(FlShm::FLCKROBUSTMODE))){
		MSG_FLCKPRN("%s ENV is not set.", FlShm::FLCKROBUSTMODE);
	}else{
		if(0 == strcasecmp(pEnvVal, FLCK_ROBUSTMODE_NO_STR)){
			MSG_FLCKPRN("ENV %s value %s, set to mode: ROBUST_NO.", FlShm::FLCKROBUSTMODE, pEnvVal);
			FlShm::RobustMode = FlShm::ROBUST_NO;
		}else if(0 == strcasecmp(pEnvVal, FLCK_ROBUSTMODE_LOW_STR)){
			MSG_FLCKPRN("ENV %s value %s, set to mode: ROBUST_LOW.", FlShm::FLCKROBUSTMODE, pEnvVal);
			FlShm::RobustMode = FlShm::ROBUST_LOW;
		}else if(0 == strcasecmp(pEnvVal, FLCK_ROBUSTMODE_HIGH_STR)){
			MSG_FLCKPRN("ENV %s value %s, set to mode: ROBUST_HIGH.", FlShm::FLCKROBUSTMODE, pEnvVal);
			FlShm::RobustMode = FlShm::ROBUST_HIGH;
		}else{
			ERR_FLCKPRN("ENV %s value %s is unknown.", FlShm::FLCKROBUSTMODE, pEnvVal);
		}
	}

	// FLCKROBUSTCHKCNT
	if(NULL == (pEnvVal = getenv(FlShm::FLCKROBUSTCHKCNT))){
		MSG_FLCKPRN("%s ENV is not set.", FlShm::FLCKROBUSTCHKCNT);
	}else{
		time_t	tmpenvval = static_cast<time_t>(atoi(pEnvVal));
		if(tmpenvval < FLCK_ROBUST_CHKCNT_MIN){
			ERR_FLCKPRN("%s ENV value(%s) is invalid, it must be over %d.", FlShm::FLCKROBUSTCHKCNT, pEnvVal, FLCK_ROBUST_CHKCNT_MIN);
		}else{
			FlShm::RobustLoopCnt = tmpenvval;
			MSG_FLCKPRN("Set rwlock checking limit count %d by ENV(%s=%s).", FlShm::RobustLoopCnt, FlShm::FLCKROBUSTCHKCNT, pEnvVal);
		}
	}
	// Check FLCKROBUSTCHKCNT & MODE
	if(FlShm::ROBUST_HIGH == FlShm::RobustMode){
		if(FlShm::RobustLoopCnt < FLCK_ROBUST_CHKCNT_MIN){
			FlShm::RobustLoopCnt = FLCK_ROBUST_CHKCNT_DEFAULT;
		}
	}

	// FLCKUMASK
	if(NULL == (pEnvVal = getenv(FlShm::FLCKUMASK))){
		MSG_FLCKPRN("%s ENV is not set.", FlShm::FLCKUMASK);
	}else{
		long	tmpval = 0;
		if(!CvtNumberStringToLong(pEnvVal, &tmpval)){
			ERR_FLCKPRN("%s ENV value(%s) is invalid, so skip this environment.", FlShm::FLCKUMASK, pEnvVal);
		}else if(tmpval < 0L || (S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH) <= tmpval){
			ERR_FLCKPRN("%s ENV value(%s) is invalid, so skip this environment.", FlShm::FLCKUMASK, pEnvVal);
		}else{
			FlShm::ShmFileUmask = static_cast<mode_t>(tmpval);
		}
	}

	// FLCKDIRPATH
	bool	isSetDir = false;
	if(NULL == (pEnvVal = getenv(FlShm::FLCKDIRPATH))){
		MSG_FLCKPRN("%s ENV is not set.", FlShm::FLCKDIRPATH);
	}else{
		string	toppath;
		if(!GetRealPath(pEnvVal, toppath)){
			ERR_FLCKPRN("%s ENV has invalid real path(%s).", FlShm::FLCKDIRPATH, pEnvVal);
		}else{
			// check & make dir
			if(!MakeWorkDirectory(toppath.c_str())){
				ERR_FLCKPRN("%s ENV path(%s) is invalid or no such directory(could not make it).", FlShm::FLCKDIRPATH, pEnvVal);
			}else{
				MSG_FLCKPRN("Set shmfile directory path(%s) by ENV(%s=%s).", FlShm::ShmDirPath().c_str(), FlShm::FLCKDIRPATH, pEnvVal);
				FlShm::ShmDirPath()	= toppath;
				isSetDir			= true;
			}
		}
	}
	if(!isSetDir){
		// Not set dirpath, then check default paths
		string	toppath;
		struct stat	st;
		if(0 != stat(DEFAULT_SHM_ANTPICKAX_DIRPATH, &st)){
			MSG_FLCKPRN("Not found %s directory, then use %s directory as default.", DEFAULT_SHM_ANTPICKAX_DIRPATH, DEFAULT_SHM_SUB_DIRPATH);
			toppath = DEFAULT_SHM_SUB_DIRPATH;
		}else{
			if(0 == (st.st_mode & S_IFDIR)){
				MSG_FLCKPRN("%s is not directory, then use %s directory as default.", DEFAULT_SHM_ANTPICKAX_DIRPATH, DEFAULT_SHM_SUB_DIRPATH);
				toppath = DEFAULT_SHM_SUB_DIRPATH;
			}else{
				MSG_FLCKPRN("Found %s directory, then use it as default.", DEFAULT_SHM_ANTPICKAX_DIRPATH);
				toppath = DEFAULT_SHM_ANTPICKAX_DIRPATH;
			}
		}
		toppath += "/" DEFAULT_SHM_DIRNAME;

		if(!MakeWorkDirectory(toppath.c_str())){
			ERR_FLCKPRN("Could not create %s working directory.", toppath.c_str());
		}else{
			MSG_FLCKPRN("Set working directory: %s", toppath.c_str());
			FlShm::ShmDirPath() = toppath;
		}
	}

	// FLCKFILENAME
	if(NULL == (pEnvVal = getenv(FlShm::FLCKFILENAME))){
		MSG_FLCKPRN("%s ENV is not set.", FlShm::FLCKFILENAME);
		FlShm::ShmFileName() = DEFAULT_SHM_FILENAME;
	}else{
		string	filepath = pEnvVal;
		filepath = trim(filepath);
		if(string::npos != filepath.find('/')){
			ERR_FLCKPRN("%s ENV has invalid file name(%s).", FlShm::FLCKFILENAME, pEnvVal);
		}else{
			FlShm::ShmFileName() = filepath;
			MSG_FLCKPRN("Set shmfile name(%s) by ENV(%s=%s).", FlShm::ShmFileName().c_str(), FlShm::FLCKFILENAME, pEnvVal);
		}
	}

	// FLCKFILECNT
	if(NULL == (pEnvVal = getenv(FlShm::FLCKFILECNT))){
		MSG_FLCKPRN("%s ENV is not set.", FlShm::FLCKFILECNT);
	}else{
		size_t	tmpenvval = static_cast<size_t>(atoi(pEnvVal));
		if(tmpenvval < FLCK_FLCKFILECNT_MIN || FLCK_FLCKFILECNT_MAX < tmpenvval){
			ERR_FLCKPRN("%s ENV value(%s) is invalid, it must be between %d and %d.", FlShm::FLCKFILECNT, pEnvVal, FLCK_FLCKFILECNT_MIN, FLCK_FLCKFILECNT_MAX);
		}else{
			FlShm::FileLockAreaCount = tmpenvval;
			MSG_FLCKPRN("Set fd reserve increment count(%zu) by ENV(%s=%s).", FlShm::FileLockAreaCount, FlShm::FLCKFILECNT, pEnvVal);
		}
	}

	// FLCKOFFETCNT
	if(NULL == (pEnvVal = getenv(FlShm::FLCKOFFETCNT))){
		MSG_FLCKPRN("%s ENV is not set.", FlShm::FLCKOFFETCNT);
	}else{
		size_t	tmpenvval = static_cast<size_t>(atoi(pEnvVal));
		if(tmpenvval < FLCK_FLCKOFFETCNT_MIN || FLCK_FLCKOFFETCNT_MAX < tmpenvval){
			ERR_FLCKPRN("%s ENV value(%s) is invalid, it must be between %d and %d.", FlShm::FLCKOFFETCNT, pEnvVal, FLCK_FLCKOFFETCNT_MIN, FLCK_FLCKOFFETCNT_MAX);
		}else{
			FlShm::OffLockAreaCount = tmpenvval;
			MSG_FLCKPRN("Set offset reserve increment count(%zu) by ENV(%s=%s).", FlShm::OffLockAreaCount, FlShm::FLCKOFFETCNT, pEnvVal);
		}
	}

	// FLCKLOCKERCNT
	if(NULL == (pEnvVal = getenv(FlShm::FLCKLOCKERCNT))){
		MSG_FLCKPRN("%s ENV is not set.", FlShm::FLCKLOCKERCNT);
	}else{
		size_t	tmpenvval = static_cast<size_t>(atoi(pEnvVal));
		if(tmpenvval < FLCK_FLCKLOCKERCNT_MIN || FLCK_FLCKLOCKERCNT_MAX < tmpenvval){
			ERR_FLCKPRN("%s ENV value(%s) is invalid, it must be between %d and %d.", FlShm::FLCKLOCKERCNT, pEnvVal, FLCK_FLCKLOCKERCNT_MIN, FLCK_FLCKLOCKERCNT_MAX);
		}else{
			FlShm::LockerAreaCount = tmpenvval;
			MSG_FLCKPRN("Set process id reserve increment count(%zu) by ENV(%s=%s).", FlShm::LockerAreaCount, FlShm::FLCKLOCKERCNT, pEnvVal);
		}
	}

	// FLCKNMTXCNT
	if(NULL == (pEnvVal = getenv(FlShm::FLCKNMTXCNT))){
		MSG_FLCKPRN("%s ENV is not set.", FlShm::FLCKNMTXCNT);
	}else{
		size_t	tmpenvval = static_cast<size_t>(atoi(pEnvVal));
		if(tmpenvval < FLCK_FLCKNMTXCNT_MIN || FLCK_FLCKNMTXCNT_MAX < tmpenvval){
			ERR_FLCKPRN("%s ENV value(%s) is invalid, it must be between %d and %d.", FlShm::FLCKNMTXCNT, pEnvVal, FLCK_FLCKNMTXCNT_MIN, FLCK_FLCKNMTXCNT_MAX);
		}else{
			FlShm::NMtxAreaCount = tmpenvval;
			MSG_FLCKPRN("Set mutex reserve increment count(%zu) by ENV(%s=%s).", FlShm::NMtxAreaCount, FlShm::FLCKNMTXCNT, pEnvVal);
		}
	}

	// FLCKNCONDCNT
	if(NULL == (pEnvVal = getenv(FlShm::FLCKNCONDCNT))){
		MSG_FLCKPRN("%s ENV is not set.", FlShm::FLCKNCONDCNT);
	}else{
		size_t	tmpenvval = static_cast<size_t>(atoi(pEnvVal));
		if(tmpenvval < FLCK_FLCKNCONDCNT_MIN || FLCK_FLCKNCONDCNT_MAX < tmpenvval){
			ERR_FLCKPRN("%s ENV value(%s) is invalid, it must be between %d and %d.", FlShm::FLCKNCONDCNT, pEnvVal, FLCK_FLCKNCONDCNT_MIN, FLCK_FLCKNCONDCNT_MAX);
		}else{
			FlShm::NCondAreaCount = tmpenvval;
			MSG_FLCKPRN("Set mutex reserve increment count(%zu) by ENV(%s=%s).", FlShm::NMtxAreaCount, FlShm::FLCKNCONDCNT, pEnvVal);
		}
	}

	// FLCKWAITERCNT
	if(NULL == (pEnvVal = getenv(FlShm::FLCKWAITERCNT))){
		MSG_FLCKPRN("%s ENV is not set.", FlShm::FLCKWAITERCNT);
	}else{
		size_t	tmpenvval = static_cast<size_t>(atoi(pEnvVal));
		if(tmpenvval < FLCK_FLCKWAITERCNT_MIN || FLCK_FLCKWAITERCNT_MAX < tmpenvval){
			ERR_FLCKPRN("%s ENV value(%s) is invalid, it must be between %d and %d.", FlShm::FLCKWAITERCNT, pEnvVal, FLCK_FLCKWAITERCNT_MIN, FLCK_FLCKWAITERCNT_MAX);
		}else{
			FlShm::WaiterAreaCount = tmpenvval;
			MSG_FLCKPRN("Set process id reserve increment count(%zu) by ENV(%s=%s).", FlShm::LockerAreaCount, FlShm::FLCKWAITERCNT, pEnvVal);
		}
	}

	// FLCKFREEUNITMODE
	if(NULL == (pEnvVal = getenv(FlShm::FLCKFREEUNITMODE))){
		MSG_FLCKPRN("%s ENV is not set.", FlShm::FLCKFREEUNITMODE);
	}else{
		if(0 == strcasecmp(pEnvVal, FLCK_FREEUNITMODE_NO_STR)){
			MSG_FLCKPRN("ENV %s value %s, set to mode: FREE_NO.", FlShm::FLCKFREEUNITMODE, pEnvVal);
			FlShm::FreeUnitMode = FlShm::FREE_NO;
		}else if(0 == strcasecmp(pEnvVal, FLCK_FREEUNITMODE_FD_STR)){
			MSG_FLCKPRN("ENV %s value %s, set to mode: FREE_FD.", FlShm::FLCKFREEUNITMODE, pEnvVal);
			FlShm::FreeUnitMode = FlShm::FREE_FD;
		}else if(0 == strcasecmp(pEnvVal, FLCK_FREEUNITMODE_OFFSET_STR)){
			MSG_FLCKPRN("ENV %s value %s, set to mode: FREE_OFFSET.", FlShm::FLCKFREEUNITMODE, pEnvVal);
			FlShm::FreeUnitMode = FlShm::FREE_OFFSET;
		}else if(0 == strcasecmp(pEnvVal, FLCK_FREEUNITMODE_ALWAYS_STR)){
			MSG_FLCKPRN("ENV %s value %s, set to mode: FREE_ALWAYS.", FlShm::FLCKFREEUNITMODE, pEnvVal);
			FlShm::FreeUnitMode = FlShm::FREE_ALWAYS;		// = FREE_ALWAYS
		}else{
			ERR_FLCKPRN("ENV %s value %s is unknown.", FlShm::FLCKFREEUNITMODE, pEnvVal);
		}
	}
	return true;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
