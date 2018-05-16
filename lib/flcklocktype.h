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

#ifndef	FLCKLOCKTYPE_H
#define	FLCKLOCKTYPE_H

//---------------------------------------------------------
// Symbols
//---------------------------------------------------------
#define	FLCK_NO_TIMEOUT				-1L
#define	FLCK_TRY_TIMEOUT			0L

//---------------------------------------------------------
// Lock type
//---------------------------------------------------------
typedef enum flck_lock_type{
	FLCK_UNLOCK		= 0,
	FLCK_READ_LOCK,
	FLCK_WRITE_LOCK,
	FLCK_NMTX_LOCK	= FLCK_WRITE_LOCK,
	FLCK_NCOND_WAIT	= FLCK_WRITE_LOCK,
	FLCK_NCOND_UP	= FLCK_UNLOCK
}FLCKLOCKTYPE;

//---------------------------------------------------------
// Utility Macros
//---------------------------------------------------------
#define STR_FLCKLOCKTYPE(LockType)			(FLCK_READ_LOCK == LockType ? "reader lock" : FLCK_WRITE_LOCK == LockType ? "writer lock" : FLCK_UNLOCK == LockType ? "unlock" : "unknown type")
#define STR_FLCKFILELOCKTYPE(LockType)		STR_FLCKLOCKTYPE(LockType)
#define STR_FLCKMUTEXTYPE(LockType)			(FLCK_NMTX_LOCK == LockType ? "mutex locked" : FLCK_UNLOCK == LockType ? "mutex unlocked" : "unknown type")
#define STR_FLCKCONDTYPE(LockType)			(FLCK_NCOND_WAIT == LockType ? "waiting cond" : FLCK_NCOND_UP == LockType ? "signaled cond" : "unknown type")

#endif	// FLCKLOCKTYPE_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
