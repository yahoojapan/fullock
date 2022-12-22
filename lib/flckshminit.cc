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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "flckcommon.h"
#include "flckshm.h"
#include "flcklistlocker.h"
#include "flcklistofflock.h"
#include "flcklistfilelock.h"
#include "flcklistnmtx.h"
#include "flcklistncond.h"
#include "flcklistwaiter.h"
#include "flckutil.h"
#include "flckdbg.h"

using namespace std;
using namespace fullock;

//---------------------------------------------------------
// Symbols
//---------------------------------------------------------
#define	FLCK_SHM_PERMS				(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

//---------------------------------------------------------
// FlShm : Initialize Methods
//---------------------------------------------------------
bool FlShm::Attach(void)
{
	if(FlShm::pShmBase){
		ERR_FLCKPRN("Already mmap.");
		return false;
	}
	if(FLCK_INVALID_HANDLE == FlShm::ShmFd){
		ERR_FLCKPRN("FlShm::ShmFd is wrong.");
		return false;
	}

	// At first mmap only head
	PFLHEAD	pTmpHead;
	if(NULL == (pTmpHead = reinterpret_cast<PFLHEAD>(RawMap(FlShm::ShmFd, sizeof(FLHEAD), 0)))){
		ERR_FLCKPRN("Failed to mmap FlShm::ShmFd(%d), size(%zu)", FlShm::ShmFd, sizeof(FLHEAD));
		return false;
	}

	// check
	if(FLCK_FILE_VERSION < pTmpHead->version){
		ERR_FLCKPRN("Fullock shm file version(%lu: %s) is newer this library(%ld: %s)", pTmpHead->version, pTmpHead->szver, FLCK_FILE_VERSION, FLCK_FILE_VERSION_STR);
		RawUnmap(pTmpHead, sizeof(FLHEAD));
		return false;
	}
	// get file size.
	size_t	length = pTmpHead->flength;

	// munmap
	RawUnmap(pTmpHead, sizeof(FLHEAD));

	// remmap
	if(NULL == (FlShm::pShmBase = RawMap(FlShm::ShmFd, length, 0))){
		ERR_FLCKPRN("Failed to mmap FlShm::ShmFd(%d), size(%zu)", FlShm::ShmFd, length);
		return false;
	}
	FlShm::pFlHead = reinterpret_cast<PFLHEAD>(FlShm::pShmBase);

	return true;
}

bool FlShm::Detach(void)
{
	// munmap
	if(!FlShm::pShmBase){
		WAN_FLCKPRN("Already munmap.");
	}else{
		if(!FlShm::pFlHead){
			ERR_FLCKPRN("pShmBase(%p) is not NULL, but pFlHead is NULL, but continue...", FlShm::pShmBase);
		}else{
			if(!RawUnmap(FlShm::pShmBase, FlShm::pFlHead->flength)){
				ERR_FLCKPRN("Failed to munmap(%p: %zu), but continue...", FlShm::pShmBase, FlShm::pFlHead->flength);
			}else{
				FlShm::pShmBase	= NULL;
				FlShm::pFlHead	= NULL;
			}
		}
	}

	// close fd & unlock
	if(FLCK_INVALID_HANDLE != FlShm::ShmFd && !FileUnlock(FlShm::ShmFd, 0L)){
		ERR_FLCKPRN("Failed to unlock fd(%d), offset=0, but continue...", FlShm::ShmFd);
	}
	FLCK_CLOSE(FlShm::ShmFd);

	return true;
}

bool FlShm::InitializeShm(void)
{
	if(FLCK_INVALID_HANDLE != FlShm::ShmFd){
		ERR_FLCKPRN("Already initialized object.");
		return false;
	}
	// set umask
	mode_t	old_umask = umask(FlShm::ShmFileUmask);

	// test by creating
	if(FLCK_INVALID_HANDLE == (FlShm::ShmFd = open(FlShm::ShmPath().c_str(), O_RDWR | O_CREAT | O_EXCL, FLCK_SHM_PERMS))){
		if(EEXIST != errno){
			ERR_FLCKPRN("Failed to open(create) %s(errno=%d).", FlShm::ShmPath().c_str(), errno);
			umask(old_umask);
			return false;
		}
		// already file exists
		if(FLCK_INVALID_HANDLE == (FlShm::ShmFd = open(FlShm::ShmPath().c_str(), O_RDWR, FLCK_SHM_PERMS))){
			ERR_FLCKPRN("Failed to open %s(errno=%d).", FlShm::ShmPath().c_str(), errno);
			umask(old_umask);
			return false;
		}
		umask(old_umask);

		// check lock & locking
		struct timespec	sleeptime = {0L, 10 * 1000 * 1000};		// 10ms
		bool			isSuccess = false;
		for(int cnt = 0; cnt < 100; cnt++){						// total 1s
			// try to lock write mode
			if(FileWriteLock(FlShm::ShmFd, 0L)){
				// re-initialize file
				if(!FlShm::InitializeShmFile()){
					ERR_FLCKPRN("Failed to initialize shmfile(%s) and mmap(fd=%d).", FlShm::ShmPath().c_str(), FlShm::ShmFd);
					break;
				}
				// Change lock mode to read mode
				if(!FileReadLock(FlShm::ShmFd, 0L)){
					ERR_FLCKPRN("Could not lock read mode to %s, give up...", FlShm::ShmPath().c_str());
					break;
				}
				isSuccess = true;
				break;

			}else{
				// try to lock read mode
				if(FileReadLock(FlShm::ShmFd, 0L)){
					// attach
					if(!FlShm::Attach()){
						ERR_FLCKPRN("Failed to mmap shmfile(%s) and mmap(fd=%d).", FlShm::ShmPath().c_str(), FlShm::ShmFd);
					}else{
						isSuccess = true;
					}
					break;
				}
				MSG_FLCKPRN("Failed to lock read mode to %s, so retry...", FlShm::ShmPath().c_str());
			}
			nanosleep(&sleeptime, NULL);
		}
		if(!isSuccess){
			ERR_FLCKPRN("Could not lock both read and write mode to %s, give up...", FlShm::ShmPath().c_str());
			FlShm::Detach();
			return false;
		}

	}else{
		umask(old_umask);

		// lock write mode ASSAP
		if(!FileWriteLock(FlShm::ShmFd, 0L)){
			ERR_FLCKPRN("Could not lock write mode to %s, give up...", FlShm::ShmPath().c_str());
			FLCK_CLOSE(FlShm::ShmFd);
			return false;
		}
		// initialize file
		if(!FlShm::InitializeShmFile()){
			ERR_FLCKPRN("Failed to initialize shmfile(%s) and mmap(fd=%d).", FlShm::ShmPath().c_str(), FlShm::ShmFd);
			FlShm::Detach();
			return false;
		}
		// Change lock mode to read mode
		if(!FileReadLock(FlShm::ShmFd, 0L)){
			ERR_FLCKPRN("Could not lock read mode to %s, give up...", FlShm::ShmPath().c_str());
			FlShm::Detach();
			return false;
		}
	}

	// run epoll thread
	if(FlShm::IsRobust()){
		FlShm::pCheckPidThread = new FlckThread();
		if(!FlShm::pCheckPidThread->InitializeThread(FlShm::ShmPath().c_str())){
			ERR_FLCKPRN("Failed to create and initialize pid check thread for file(%s).", FlShm::ShmPath().c_str());
			FLCK_Delete(FlShm::pCheckPidThread);
			FlShm::Detach();
			return false;
		}
		if(!FlShm::pCheckPidThread->Run()){
			ERR_FLCKPRN("Failed to run pid check thread for file(%s).", FlShm::ShmPath().c_str());
			FLCK_Delete(FlShm::pCheckPidThread);
			FlShm::Detach();
			return false;
		}

		// [NOTE]
		// When the program which loads this fullock library is forked, we need to start worker thread
		// in child process.
		// Thus we set a handler at forking, and it initializes and runs thread in child process.
		//
		int	result = pthread_atfork(NULL, NULL, PreforkHandler);
		if(0 != result){
			ERR_FLCKPRN("Failed to set handler for forking(errno=%d), but continue...", result);
		}
	}
	return true;
}

bool FlShm::InitializeShmFile(void)
{
	if(FLCK_INVALID_HANDLE == FlShm::ShmFd){
		ERR_FLCKPRN("FlShm::ShmFd is wrong.");
		return false;
	}

	// file size to zero
	if(0 != ftruncate(FlShm::ShmFd, 0)){
		ERR_FLCKPRN("Could not truncate zero to FlShm::ShmFd(%d), errno=%d", FlShm::ShmFd, errno);
		return false;
	}

	// calc initialize size(align 64bit)
	size_t	sz_head		= sizeof(FLHEAD);
	size_t	sz_filelock	= sizeof(FLFILELOCK)	* FlShm::FileLockAreaCount;
	size_t	sz_offlock	= sizeof(FLOFFLOCK)		* FlShm::OffLockAreaCount;
	size_t	sz_locker	= sizeof(FLLOCKER)		* FlShm::LockerAreaCount;
	size_t	sz_nmtxlock	= sizeof(FLNAMEDMUTEX)	* FlShm::NMtxAreaCount;
	size_t	sz_ncondlock= sizeof(FLNAMEDCOND)	* FlShm::NCondAreaCount;
	size_t	sz_waiter	= sizeof(FLWAITER)		* FlShm::WaiterAreaCount;

	off_t	off_head		= 0;
	off_t	off_filelock	= off_head		+ ALIGNMENT(sz_head,		sizeof(uint64_t));
	off_t	off_offlock		= off_filelock	+ ALIGNMENT(sz_filelock,	sizeof(uint64_t));
	off_t	off_locker		= off_offlock	+ ALIGNMENT(sz_offlock,		sizeof(uint64_t));
	off_t	off_nmtxlock	= off_locker	+ ALIGNMENT(sz_locker,		sizeof(uint64_t));
	off_t	off_ncondlock	= off_nmtxlock	+ ALIGNMENT(sz_nmtxlock,	sizeof(uint64_t));
	off_t	off_waiter		= off_ncondlock	+ ALIGNMENT(sz_ncondlock,	sizeof(uint64_t));
	off_t	off_end			= off_waiter	+ ALIGNMENT(sz_waiter,		sizeof(uint64_t));
	size_t	sz_total	= ALIGNMENT(off_end, GetSystemPageSize());

	// fill zero to hole file
	if(!flck_fill_zero(FlShm::ShmFd, sz_total, 0)){
		ERR_FLCKPRN("Failed to initialize zero %zu byte to FlShm::ShmFd(%d)", sz_total, FlShm::ShmFd);
		return false;
	}

	// mmap
	if(NULL == (FlShm::pShmBase = RawMap(FlShm::ShmFd, sz_total, 0))){
		ERR_FLCKPRN("Failed to mmap FlShm::ShmFd(%d), size(%zu)", FlShm::ShmFd, sz_total);
		return false;
	}

	// initialize parts
	FlShm::pFlHead = reinterpret_cast<PFLHEAD>(FlShm::pShmBase);

	strcpy(FlShm::pFlHead->szver, FLCK_FILE_VERSION_STR);

	FlShm::pFlHead->version					= FLCK_FILE_VERSION;
	FlShm::pFlHead->flength					= sz_total;
	FlShm::pFlHead->file_lock_lockid		= FLCK_INVALID_ID;
	FlShm::pFlHead->file_lock_list			= NULL;
	FlShm::pFlHead->named_mutex_lockid		= FLCK_INVALID_ID;
	FlShm::pFlHead->named_mutex_list		= NULL;
	FlShm::pFlHead->file_lock_free			= FlShm::MakeListFileLock(	ADDPTR(CVT_POINTER(FlShm::pShmBase, FLFILELOCK),	off_filelock),	FlShm::FileLockAreaCount);
	FlShm::pFlHead->offset_lock_free		= FlShm::MakeListOffLock(	ADDPTR(CVT_POINTER(FlShm::pShmBase, FLOFFLOCK),		off_offlock),	FlShm::OffLockAreaCount);
	FlShm::pFlHead->locker_free				= FlShm::MakeListLocker(	ADDPTR(CVT_POINTER(FlShm::pShmBase, FLLOCKER),		off_locker),	FlShm::LockerAreaCount);
	FlShm::pFlHead->named_mutex_free		= FlShm::MakeListNMtxLock(	ADDPTR(CVT_POINTER(FlShm::pShmBase, FLNAMEDMUTEX),	off_nmtxlock),	FlShm::NMtxAreaCount);
	FlShm::pFlHead->named_cond_free			= FlShm::MakeListNCondLock(	ADDPTR(CVT_POINTER(FlShm::pShmBase, FLNAMEDCOND),	off_ncondlock),	FlShm::NCondAreaCount);
	FlShm::pFlHead->waiter_free				= FlShm::MakeListWaiter(	ADDPTR(CVT_POINTER(FlShm::pShmBase, FLWAITER),		off_waiter),	FlShm::WaiterAreaCount);

	// check
	if(!FlShm::pFlHead->file_lock_free || !FlShm::pFlHead->offset_lock_free || !FlShm::pFlHead->locker_free || !FlShm::pFlHead->named_mutex_free || !FlShm::pFlHead->named_cond_free || !FlShm::pFlHead->waiter_free){
		ERR_FLCKPRN("FATAL - Could not initialize some free leaf pointer.");
		RawUnmap(FlShm::pShmBase, sz_total);
		FlShm::pShmBase	= NULL;
		FlShm::pFlHead	= NULL;
		return false;
	}
	return true;
}

PFLFILELOCK FlShm::MakeListFileLock(PFLFILELOCK ptr, size_t count)
{
	if(!ptr){
		ERR_FLCKPRN("Parameter is wrong.");
		return NULL;
	}
	// initialize as list
	FlListFileLock	list;
	list.initialize(ptr, count);
	return list.rel_get();
}

PFLOFFLOCK FlShm::MakeListOffLock(PFLOFFLOCK ptr, size_t count)
{
	if(!ptr){
		ERR_FLCKPRN("Parameter is wrong.");
		return NULL;
	}
	// initialize as list
	FlListOffLock	list;
	list.initialize(ptr, count);
	return list.rel_get();
}

PFLLOCKER FlShm::MakeListLocker(PFLLOCKER ptr, size_t count)
{
	if(!ptr){
		ERR_FLCKPRN("Parameter is wrong.");
		return NULL;
	}
	// initialize as list
	FlListLocker	list;
	list.initialize(ptr, count);
	return list.rel_get();
}

PFLNAMEDMUTEX FlShm::MakeListNMtxLock(PFLNAMEDMUTEX ptr, size_t count)
{
	if(!ptr){
		ERR_FLCKPRN("Parameter is wrong.");
		return NULL;
	}
	// initialize as list
	FlListNMtx	list;
	list.initialize(ptr, count);
	return list.rel_get();
}

PFLNAMEDCOND FlShm::MakeListNCondLock(PFLNAMEDCOND ptr, size_t count)
{
	if(!ptr){
		ERR_FLCKPRN("Parameter is wrong.");
		return NULL;
	}
	// initialize as list
	FlListNCond	list;
	list.initialize(ptr, count);
	return list.rel_get();
}

PFLWAITER FlShm::MakeListWaiter(PFLWAITER ptr, size_t count)
{
	if(!ptr){
		ERR_FLCKPRN("Parameter is wrong.");
		return NULL;
	}
	// initialize as list
	FlListWaiter	list;
	list.initialize(ptr, count);
	return list.rel_get();
}

bool FlShm::Destroy(void)
{
	if(FLCK_INVALID_HANDLE == FlShm::ShmFd){
		MSG_FLCKPRN("Already destroyed object.");
		return true;
	}

	// stop epoll thread
	if(FlShm::pCheckPidThread){
		FlShm::pCheckPidThread->Exit();
		FLCK_Delete(FlShm::pCheckPidThread);
	}

	if(!FlShm::Detach()){
		ERR_FLCKPRN("Could not detach shm file, but continue...");
	}
	return true;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
