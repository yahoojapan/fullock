/*
 * FULLOCK - Fast User Level LOCK library by Yahoo! JAPAN
 *
 * Copyright 2015 Yahoo! JAPAN corporation.
 *
 * FULLOCK is fast locking library on user level by Yahoo! JAPAN.
 * FULLOCK is following specifications.
 *
 * For the full copyright and license information, please view
 * the LICENSE file that was distributed with this source code.
 *
 * AUTHOR:   Takeshi Nakatani
 * CREATE:   Wed 13 May 2015
 * REVISION:
 *
 */

#ifndef	FLCKCOMMON_H
#define	FLCKCOMMON_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

//---------------------------------------------------------
// Macros for compiler
//---------------------------------------------------------
#if defined(__cplusplus)
#define	DECL_EXTERN_C_START			extern "C" {
#define	DECL_EXTERN_C_END			}
#else	// __cplusplus
#define	DECL_EXTERN_C_START
#define	DECL_EXTERN_C_END
#endif	// __cplusplus

//---------------------------------------------------------
// Common Symbols
//---------------------------------------------------------
#define	FLCK_INVALID_HANDLE			(-1)
#define	FLCK_INVALID_ID				(0)
#define	FLCK_EACCESS_ID				(-1)

//---------------------------------------------------------
// types
//---------------------------------------------------------
typedef	uint64_t					flck_hash_t;
typedef pid_t						tid_t;
typedef	uint64_t					flckpid_t;
typedef	flckpid_t					flck_mutex_t;
typedef	int							flck_rwlock_t;

#define	__STDC_FORMAT_MACROS
#include <inttypes.h>

#endif	// FLCKCOMMON_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
