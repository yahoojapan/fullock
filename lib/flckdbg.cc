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
 * CREATE:   Thu 28 May 2015
 * REVISION:
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <errno.h>

#include "flckcommon.h"
#include "flckutil.h"
#include "flckdbg.h"

using namespace std;

//---------------------------------------------------------
// Class FlckDbgCntrl
//---------------------------------------------------------
class FlckDbgCntrl
{
	protected:
		static const char*		DBGENVNAME;
		static const char*		DBGENVFILE;
		static FlckDbgCntrl		singleton;

	public:
		static bool LoadEnv(void);
		static bool LoadEnvName(void);
		static bool LoadEnvFile(void);

		FlckDbgCntrl();
		virtual ~FlckDbgCntrl();
};

// Class valiables
const char*		FlckDbgCntrl::DBGENVNAME = "FLCKDBGMODE";
const char*		FlckDbgCntrl::DBGENVFILE = "FLCKDBGFILE";
FlckDbgCntrl	FlckDbgCntrl::singleton;

// Constructor / Destructor
FlckDbgCntrl::FlckDbgCntrl()
{
	FlckDbgCntrl::LoadEnv();
}
FlckDbgCntrl::~FlckDbgCntrl()
{
}

// Class Methods
bool FlckDbgCntrl::LoadEnv(void)
{
	if(!FlckDbgCntrl::LoadEnvName() || !FlckDbgCntrl::LoadEnvFile()){
		return false;
	}
	return true;
}

bool FlckDbgCntrl::LoadEnvName(void)
{
	char*	pEnvVal;
	if(NULL == (pEnvVal = getenv(FlckDbgCntrl::DBGENVNAME))){
		MSG_FLCKPRN("%s ENV is not set.", FlckDbgCntrl::DBGENVNAME);
		return true;
	}
	if(0 == strcasecmp(pEnvVal, "SILENT")){
		SetFlckDbgMode(FLCKDBG_SILENT);
	}else if(0 == strcasecmp(pEnvVal, "ERR")){
		SetFlckDbgMode(FLCKDBG_ERR);
	}else if(0 == strcasecmp(pEnvVal, "WAN")){
		SetFlckDbgMode(FLCKDBG_WARN);
	}else if(0 == strcasecmp(pEnvVal, "INFO")){
		SetFlckDbgMode(FLCKDBG_MSG);
	}else{
		MSG_FLCKPRN("%s ENV is not unknown string(%s).", FlckDbgCntrl::DBGENVNAME, pEnvVal);
		return false;
	}
	return true;
}

bool FlckDbgCntrl::LoadEnvFile(void)
{
	char*	pEnvVal;
	if(NULL == (pEnvVal = getenv(FlckDbgCntrl::DBGENVFILE))){
		MSG_FLCKPRN("%s ENV is not set.", FlckDbgCntrl::DBGENVFILE);
		return true;
	}
	if(!SetFlckDbgFile(pEnvVal)){
		MSG_FLCKPRN("%s ENV is unsafe string(%s).", FlckDbgCntrl::DBGENVFILE, pEnvVal);
		return false;
	}
	return true;
}

//---------------------------------------------------------
// Global variable
//---------------------------------------------------------
FlckDbgMode	flckdbg_mode	= FLCKDBG_SILENT;
FILE*		flckdbg_fp		= NULL;

FlckDbgMode SetFlckDbgMode(FlckDbgMode mode)
{
	FlckDbgMode oldmode = flckdbg_mode;
	flckdbg_mode = mode;
	return oldmode;
}

FlckDbgMode BumpupFlckDbgMode(void)
{
	FlckDbgMode	mode = GetFlckDbgMode();

	if(FLCKDBG_SILENT == mode){
		mode = FLCKDBG_ERR;
	}else if(FLCKDBG_ERR == mode){
		mode = FLCKDBG_WARN;
	}else if(FLCKDBG_WARN == mode){
		mode = FLCKDBG_MSG;
	}else{	// FLCKDBG_MSG == mode
		mode = FLCKDBG_SILENT;
	}
	return ::SetFlckDbgMode(mode);
}

FlckDbgMode GetFlckDbgMode(void)
{
	return flckdbg_mode;
}

bool LoadFlckDbgEnv(void)
{
	return FlckDbgCntrl::LoadEnv();
}

bool SetFlckDbgFile(const char* filepath)
{
	if(FLCKEMPTYSTR(filepath)){
		ERR_FLCKPRN("Parameter is wrong.");
		return false;
	}
	if(!UnsetFlckDbgFile()){
		return false;
	}
	FILE*	newfp;
	if(NULL == (newfp = fopen(filepath, "a+"))){
		ERR_FLCKPRN("Could not open debug file(%s). errno = %d", filepath, errno);
		return false;
	}
	flckdbg_fp = newfp;
	return true;
}

bool UnsetFlckDbgFile(void)
{
	if(flckdbg_fp){
		if(0 != fclose(flckdbg_fp)){
			ERR_FLCKPRN("Could not close debug file. errno = %d", errno);
			flckdbg_fp = NULL;		// On this case, flckdbg_fp is not correct pointer after error...
			return false;
		}
		flckdbg_fp = NULL;
	}
	return true;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
