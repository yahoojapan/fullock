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

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <errno.h>

#include "flckcommon.h"
#include "flckutil.h"
#include "flckdbg.h"

using namespace std;

//---------------------------------------------------------
// Global variable
//---------------------------------------------------------
FlckDbgMode	flckdbg_mode	= FLCKDBG_SILENT;
FILE*		flckdbg_fp		= NULL;

//---------------------------------------------------------
// Class FlckDbgCntrl
//---------------------------------------------------------
// [NOTE]
// To avoid static object initialization order problem(SIOF)
//
class FlckDbgCntrl
{
	protected:
		static const char*		DBGENVNAME;
		static const char*		DBGENVFILE;

		FlckDbgMode*			pflckdbg_mode;		// pointer to global variable
		FILE**					pflckdbg_fp;		// pointer to global variable

	protected:
		FlckDbgCntrl() : pflckdbg_mode(&flckdbg_mode), pflckdbg_fp(&flckdbg_fp)
		{
			*pflckdbg_mode	= FLCKDBG_SILENT;
			*pflckdbg_fp	= NULL;
			FlckDbgCntrlLoadEnv();
		}

		virtual ~FlckDbgCntrl()
		{
			UnsetFlckDbgCntrlFile();
		}

		bool FlckDbgCntrlLoadEnvName(void);
		bool FlckDbgCntrlLoadEnvFile(void);

	public:
		bool FlckDbgCntrlLoadEnv(void);

		FlckDbgMode SetFlckDbgCntrlMode(FlckDbgMode mode);
		FlckDbgMode BumpupFlckDbgCntrlMode(void);
		FlckDbgMode GetFlckDbgCntrlMode(void);

		bool SetFlckDbgCntrlFile(const char* filepath);
		bool UnsetFlckDbgCntrlFile(void);

		static FlckDbgCntrl& GetFlckDbgCntrl(void)
		{
			static FlckDbgCntrl	singleton;			// singleton
			return singleton;
		}
};

//
// Class variables
//
const char*		FlckDbgCntrl::DBGENVNAME = "FLCKDBGMODE";
const char*		FlckDbgCntrl::DBGENVFILE = "FLCKDBGFILE";

//
// Methods
//
bool FlckDbgCntrl::FlckDbgCntrlLoadEnv(void)
{
	if(!FlckDbgCntrlLoadEnvName() || !FlckDbgCntrlLoadEnvFile()){
		return false;
	}
	return true;
}

bool FlckDbgCntrl::FlckDbgCntrlLoadEnvName(void)
{
	char*	pEnvVal;
	if(NULL == (pEnvVal = getenv(FlckDbgCntrl::DBGENVNAME))){
		MSG_FLCKPRN("%s ENV is not set.", FlckDbgCntrl::DBGENVNAME);
		return true;
	}
	if(0 == strcasecmp(pEnvVal, "SILENT")){
		SetFlckDbgCntrlMode(FLCKDBG_SILENT);
	}else if(0 == strcasecmp(pEnvVal, "ERR")){
		SetFlckDbgCntrlMode(FLCKDBG_ERR);
	}else if(0 == strcasecmp(pEnvVal, "WAN")){
		SetFlckDbgCntrlMode(FLCKDBG_WARN);
	}else if(0 == strcasecmp(pEnvVal, "INFO")){
		SetFlckDbgCntrlMode(FLCKDBG_MSG);
	}else{
		MSG_FLCKPRN("%s ENV is not unknown string(%s).", FlckDbgCntrl::DBGENVNAME, pEnvVal);
		return false;
	}
	return true;
}

bool FlckDbgCntrl::FlckDbgCntrlLoadEnvFile(void)
{
	const char*	pEnvVal;
	if(NULL == (pEnvVal = getenv(FlckDbgCntrl::DBGENVFILE))){
		MSG_FLCKPRN("%s ENV is not set.", FlckDbgCntrl::DBGENVFILE);
		return true;
	}
	if(!SetFlckDbgCntrlFile(pEnvVal)){
		MSG_FLCKPRN("%s ENV is unsafe string(%s).", FlckDbgCntrl::DBGENVFILE, pEnvVal);
		return false;
	}
	return true;
}

FlckDbgMode FlckDbgCntrl::SetFlckDbgCntrlMode(FlckDbgMode mode)
{
	FlckDbgMode oldmode = *pflckdbg_mode;
	*pflckdbg_mode = mode;
	return oldmode;
}

FlckDbgMode FlckDbgCntrl::BumpupFlckDbgCntrlMode(void)
{
	FlckDbgMode	mode = GetFlckDbgCntrlMode();

	if(FLCKDBG_SILENT == mode){
		mode = FLCKDBG_ERR;
	}else if(FLCKDBG_ERR == mode){
		mode = FLCKDBG_WARN;
	}else if(FLCKDBG_WARN == mode){
		mode = FLCKDBG_MSG;
	}else{	// FLCKDBG_MSG == mode
		mode = FLCKDBG_SILENT;
	}
	return SetFlckDbgCntrlMode(mode);
}

FlckDbgMode FlckDbgCntrl::GetFlckDbgCntrlMode(void)
{
	return *pflckdbg_mode;
}

bool FlckDbgCntrl::SetFlckDbgCntrlFile(const char* filepath)
{
	if(FLCKEMPTYSTR(filepath)){
		ERR_FLCKPRN("Parameter is wrong.");
		return false;
	}
	if(!UnsetFlckDbgCntrlFile()){
		return false;
	}
	FILE*	newfp;
	if(NULL == (newfp = fopen(filepath, "a+"))){
		ERR_FLCKPRN("Could not open debug file(%s). errno = %d", filepath, errno);
		// cppcheck-suppress resourceLeak
		return false;
	}
	*pflckdbg_fp = newfp;

	return true;
}

bool FlckDbgCntrl::UnsetFlckDbgCntrlFile(void)
{
	if(*pflckdbg_fp){
		if(0 != fclose(*pflckdbg_fp)){
			ERR_FLCKPRN("Could not close debug file. errno = %d", errno);
			*pflckdbg_fp = NULL;		// On this case, flckdbg_fp is not correct pointer after error...
			return false;
		}
		*pflckdbg_fp = NULL;
	}
	return true;
}

//---------------------------------------------------------
// Global Functions
//---------------------------------------------------------
FlckDbgMode SetFlckDbgMode(FlckDbgMode mode)
{
	return FlckDbgCntrl::GetFlckDbgCntrl().SetFlckDbgCntrlMode(mode);
}

FlckDbgMode BumpupFlckDbgMode(void)
{
	return FlckDbgCntrl::GetFlckDbgCntrl().BumpupFlckDbgCntrlMode();
}

FlckDbgMode GetFlckDbgMode(void)
{
	return FlckDbgCntrl::GetFlckDbgCntrl().GetFlckDbgCntrlMode();
}

bool LoadFlckDbgEnv(void)
{
	return FlckDbgCntrl::GetFlckDbgCntrl().FlckDbgCntrlLoadEnv();
}

bool SetFlckDbgFile(const char* filepath)
{
	return FlckDbgCntrl::GetFlckDbgCntrl().SetFlckDbgCntrlFile(filepath);
}

bool UnsetFlckDbgFile(void)
{
	return FlckDbgCntrl::GetFlckDbgCntrl().UnsetFlckDbgCntrlFile();
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
