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

#ifndef	FLCKSTRUCTURE_H
#define	FLCKSTRUCTURE_H

#include <stdint.h>

#include "flckcommon.h"
#include "fullock.h"
#include "flcklocktype.h"

//---------------------------------------------------------
// Typedefs
//---------------------------------------------------------
typedef	uint64_t					flck_ver_t;				// for fullock shared memory file version

//---------------------------------------------------------
// Symbols
//---------------------------------------------------------
#define	FLCK_FILE_VERSION			1L
#define	FLCK_FILE_VERSION_STR		"FULLOCK FILEVER 1"
#define	FLCK_FILE_VERSION_BUFFSIZE	24

#define	FLCK_MUTEX_UNLOCK			0

#define	FLCK_RWLOCK_UNLOCK			0
#define	FLCK_RWLOCK_WLOCK			-1
#define	FLCK_RWLOCK_RLOCK			1						// over 1

//---------------------------------------------------------
// Structure
//---------------------------------------------------------
//
// RWLocker by one Process/Thread/FileDescriptor
//
typedef struct fl_locker{
	struct fl_locker*		next;							// next list

	flckpid_t				flckpid;						// pid and tid(packed)
	int						fd;
	volatile bool			locked;
}FLLOCKER, *PFLLOCKER;

//
// RWLocker by offset
//
typedef struct fl_offset_lock{
	struct fl_offset_lock*	next;							// next list

	flck_rwlock_t			lockval;						// lock variable
	off_t					offset;							// offset from file top
	size_t					length;							// length for locking area
	PFLLOCKER				reader_list;					// lock readers list
	PFLLOCKER				writer_list;					// lock writers list
	volatile bool			protect;
}FLOFFLOCK, *PFLOFFLOCK;

//
// RWLocker by File
//
typedef struct fl_file_lock{
	struct fl_file_lock*	next;							// next list

	dev_t					dev_id;							// devide id
	ino_t					ino_id;							// inode id
	PFLOFFLOCK				offset_lock_list;
	volatile bool			protect;
}FLFILELOCK, *PFLFILELOCK;

//
// Named Mutex
//
typedef struct fl_named_mutex{
	struct fl_named_mutex*	next;							// next list

	flck_mutex_t			lockval;						// lock variable(=pid/tid)
	int						lockcnt;						// lock count for recursive
	flck_hash_t				hash;							// hash value by flck_fnv_hash()
	char					name[FLCK_NAMED_MUTEX_MAXLENGTH + 1];	// mutex name
}FLNAMEDMUTEX, *PFLNAMEDMUTEX;

//
// Conditional waiter by Process/Thread/NamedMutex
//
typedef struct fl_waiter{
	struct fl_waiter*		next;							// next list

	flckpid_t				flckpid;						// pid and tid(packed)
	FLCKLOCKTYPE			lockstatus;						// lock status
	PFLNAMEDMUTEX			named_mutex;					// named mutex pointer
}FLWAITER, *PFLWAITER;

//
// Named Conditional
//
typedef struct fl_named_cond{
	struct fl_named_cond*	next;							// next list

	PFLWAITER				waiter_list;					// list for waiting condition
	flck_hash_t				hash;							// hash value by flck_fnv_hash()
	char					name[FLCK_NAMED_COND_MAXLENGTH + 1];	// cond name
}FLNAMEDCOND, *PFLNAMEDCOND;

//
// Header(Main structure)
//
typedef struct fl_header{
	flck_ver_t			version;							// * fullock shared memory structure version
                                                            //		this buffer is reading locked by fcntl because initializing.
	char				szver[FLCK_FILE_VERSION_BUFFSIZE];	// * fullock shared memory structure version string
	size_t				flength;							// * shared memory file size

	flckpid_t			file_lock_lockid;					// * lock of shared rwlock
															//		lock pid/tid variable for file_lock_list and free pointers
	PFLFILELOCK			file_lock_list;						// * shared rwlock list by file
															//		This is shared rwlock for reading/modifying.

	flckpid_t			named_mutex_lockid;					// * lock of shared mutex
															//		lock pid/tid variable for named_mutex_list and free pointer
	PFLNAMEDMUTEX		named_mutex_list;					// * shared mutex list for named mutex
															//		This is shared mutex for reading/modifying.

	flckpid_t			named_cond_lockid;					// * lock of shared cond
															//		lock pid/tid variable for named_cond and free pointer
	PFLNAMEDCOND		named_cond_list;					// * shared cond list for named cond
															//		This is shared cond for reading/modifying.

	PFLFILELOCK			file_lock_free;						// * free pointer list for file descriptor
	PFLOFFLOCK			offset_lock_free;					// * free pointer list for offset locker
	PFLLOCKER			locker_free;						// * free pointer list for locker
	PFLNAMEDMUTEX		named_mutex_free;					// * free pointer list for named mutex
	PFLNAMEDCOND		named_cond_free;					// * free pointer list for named cond
	PFLWAITER			waiter_free;						// * free pointer list for waiter
}FLHEAD, *PFLHEAD;

#endif	// FLCKSTRUCTURE_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
