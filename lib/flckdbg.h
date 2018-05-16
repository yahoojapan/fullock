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
 * CREATE:   Thu 28 May 2015
 * REVISION:
 *
 */

#ifndef	FLCKDBG_H
#define FLCKDBG_H

#include "flckcommon.h"

DECL_EXTERN_C_START

//---------------------------------------------------------
// Debug
//---------------------------------------------------------
typedef enum flck_dbg_mode{
	FLCKDBG_SILENT	= 0,
	FLCKDBG_ERR		= 1,
	FLCKDBG_WARN	= 3,
	FLCKDBG_MSG		= 7
}FlckDbgMode;

extern FlckDbgMode	flckdbg_mode;		// Do not use directly this variable.
extern FILE*		flckdbg_fp;

FlckDbgMode SetFlckDbgMode(FlckDbgMode mode);
FlckDbgMode BumpupFlckDbgMode(void);
FlckDbgMode GetFlckDbgMode(void);
bool LoadFlckDbgEnv(void);
bool SetFlckDbgFile(const char* filepath);
bool UnsetFlckDbgFile(void);

//---------------------------------------------------------
// Debugging Macros
//---------------------------------------------------------
#define	FlckDbgMode_STR(mode)	\
		FLCKDBG_SILENT	== mode ? "SLT" : \
		FLCKDBG_ERR		== mode ? "ERR" : \
		FLCKDBG_WARN	== mode ? "WAN" : \
		FLCKDBG_MSG		== mode ? "MSG" : ""

#define	LOW_FLCKPRINT(mode, fmt, ...) \
		fprintf((flckdbg_fp ? flckdbg_fp : stderr), "[FLCK-%s] %s(%d) : " fmt "%s\n", FlckDbgMode_STR(mode), __func__, __LINE__, __VA_ARGS__)

#define	FLCKPRINT(mode, fmt, ...) \
		if((flckdbg_mode & mode) == mode){ \
			LOW_FLCKPRINT(mode, fmt, __VA_ARGS__); \
		}

#ifdef	FLCK_DEBUG_MSG_NO
#define	SLT_FLCKPRN(fmt, ...)	
#define	ERR_FLCKPRN(fmt, ...)	
#define	WAN_FLCKPRN(fmt, ...)	
#define	MSG_FLCKPRN(fmt, ...)	
#else
#define	SLT_FLCKPRN(fmt, ...)	FLCKPRINT(FLCKDBG_SILENT,	fmt, ##__VA_ARGS__, "")	// This means nothing...
#define	ERR_FLCKPRN(fmt, ...)	FLCKPRINT(FLCKDBG_ERR,		fmt, ##__VA_ARGS__, "")
#define	WAN_FLCKPRN(fmt, ...)	FLCKPRINT(FLCKDBG_WARN,		fmt, ##__VA_ARGS__, "")
#define	MSG_FLCKPRN(fmt, ...)	FLCKPRINT(FLCKDBG_MSG,		fmt, ##__VA_ARGS__, "")
#endif

DECL_EXTERN_C_END

#endif	// FLCKDBG_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
