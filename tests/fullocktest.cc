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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <libgen.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/wait.h>
#include <errno.h>

#include <string>
#include <map>
#include <iostream>

#include "flckshm.h"
#include "fullock.h"
#include "flckutil.h"

using namespace std;

//---------------------------------------------------------
// Symbols
//---------------------------------------------------------
#define	MYTEST_FILE					"/tmp/fullocktest_testfile"

//---------------------------------------------------------
// Structure
//---------------------------------------------------------
//
// For option parser
//
typedef struct opt_param{
	std::string		rawstring;
	bool			is_number;
	int				num_value;
}OPTPARAM, *POPTPARAM;

typedef std::map<std::string, OPTPARAM>		optparams_t;

typedef enum worker_type{
	WORKER_TYPE_PARENT	= 0,
	WORKER_TYPE_WAITER,
	WORKER_TYPE_EXITER
}FLWORKERTYPE;

//---------------------------------------------------------
// Utility Functions
//---------------------------------------------------------
static inline void PRN(const char* format, ...)
{
	if(format){
		va_list ap;
		va_start(ap, format);
		vfprintf(stdout, format, ap); 
		va_end(ap);
	}
	fprintf(stdout, "\n");
}

static inline void ERR(const char* format, ...)
{
	fprintf(stderr, "[ERR] ");
	if(format){
		va_list ap;
		va_start(ap, format);
		vfprintf(stderr, format, ap); 
		va_end(ap);
	}
	fprintf(stderr, "\n");
}

static inline char* programname(char* prgpath)
{
	if(!prgpath){
		return NULL;
	}
	char*	pprgname = basename(prgpath);
	if(0 == strncmp(pprgname, "lt-", strlen("lt-"))){
		pprgname = &pprgname[strlen("lt-")];
	}
	return pprgname;
}

static void Help(char* progname)
{
	PRN(NULL);
	PRN("Usage: %s -help(h)",											progname ? programname(progname) : "program");
	PRN("       %s -env [child]",										progname ? programname(progname) : "program");
	PRN("       %s -overareacnt(oac) -unit {no|fd|offset}",				progname ? programname(progname) : "program");
	PRN("       %s -deadlock(dl)",										progname ? programname(progname) : "program");
	PRN("       %s -autorecover(ar) -thread -robust {no|low|high}",		progname ? programname(progname) : "program");
	PRN("       %s -autorecover(ar) -process -robust {no|low|high}",	progname ? programname(progname) : "program");
	PRN("       %s -moverareacnt(moac)",								progname ? programname(progname) : "program");
	PRN("       %s -mdeadlock(mdl)",									progname ? programname(progname) : "program");
	PRN("       %s -mautorecover(mar) -thread -robust {no|low|high}",	progname ? programname(progname) : "program");
	PRN("       %s -mautorecover(mar) -process -robust {no|low|high}",	progname ? programname(progname) : "program");
	PRN("       %s -coverareacnt(coac)",								progname ? programname(progname) : "program");
	PRN(NULL);
	PRN("test type:");
	PRN("       -env                     environment and reinitialize test.");
	PRN("       -overareacnt(oac)        over area count limit test.");
	PRN("       -deadlock(dl)            deadlock test.");
	PRN("       -autorecover(ar)         automatic recover at dead lock test.");
	PRN(NULL);
	PRN("       -moverareacnt(moac)      mutex over area count limit test.");
	PRN("       -mdeadlock(mdl)          mutex deadlock test.");
	PRN("       -mautorecover(mar)       mutex automatic recover at dead lock test.");
	PRN(NULL);
	PRN("       -coverareacnt(coac)      cond over area count limit test.");
	PRN("                                does not need to check deadlock for cond.");
	PRN("other parameter:");
	PRN("       -unit                    free unit mode(\"no\" or \"fd\" or \"offset\").");
	PRN("       -thread                  use thread for mutex test.");
	PRN("       -process                 use process for mutex test.");
	PRN("       -robust                  rwlock robust mode(no or low or high).");
	PRN(NULL);
}

static void OptionParser(int argc, char** argv, optparams_t& optparams)
{
	optparams.clear();
	for(int cnt = 1; cnt < argc && argv && argv[cnt]; cnt++){
		OPTPARAM	param;
		param.rawstring = "";
		param.is_number = false;
		param.num_value = 0;

		// get option name
		char*	popt = argv[cnt];
		if(FLCKEMPTYSTR(popt)){
			continue;		// skip
		}
		if('-' != *popt){
			ERR("%s option is not started with \"-\".", popt);
			continue;
		}

		// check option parameter
		if((cnt + 1) < argc && argv[cnt + 1]){
			char*	pparam = argv[cnt + 1];
			if(!FLCKEMPTYSTR(pparam) && '-' != *pparam){
				// found param
				param.rawstring = pparam;

				// check number
				param.is_number = true;
				for(char* ptmp = pparam; *ptmp; ++ptmp){
					if(0 == isdigit(*ptmp)){
						param.is_number = false;
						break;
					}
				}
				if(param.is_number){
					param.num_value = atoi(pparam);
				}
				++cnt;
			}
		}
		optparams[string(popt)] = param;
	}
}

static bool MakeTestFile(void)
{
	int	fd;
	if(-1 == (fd = open(MYTEST_FILE, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH))){
		ERR("Could not open file(%s)", MYTEST_FILE);
		return false;
	}

	unsigned char	byBuff[1024];
	memset(byBuff, 0, sizeof(byBuff));

	if(-1 == flck_pwrite(fd, byBuff, sizeof(byBuff), 0)){
		ERR("Failed to write file(%s).", MYTEST_FILE);
		close(fd);
		return false;
	}
	close(fd);
	return true;
}

//---------------------------------------------------------
// Test environment functions
//---------------------------------------------------------
static bool env_test(string& strtesttype, const char* procname, bool is_parent)
{
	if(is_parent){
		// parent
		strtesttype = "Test Environment(parent)";

		setenv("FLCKAUTOINIT",		"NO",					1);
		setenv("FLCKROBUSTMODE",	"LOW",					1);
		setenv("FLCKNOMAPMODE",		"DENY_RETRY",			1);
		setenv("FLCKFREEUNITMODE",	"OFFSET",				1);
		setenv("FLCKROBUSTCHKCNT",	"1000",					1);
		setenv("FLCKDIRPATH",		"/tmp/.fullocktest",	1);
		setenv("FLCKFILENAME",		"fullocktest.shm",		1);
		setenv("FLCKFILECNT",		"10",					1);
		setenv("FLCKOFFETCNT",		"10000",				1);
		setenv("FLCKLOCKERCNT",		"1000",					1);
		setenv("FLCKNMTXCNT",		"10",					1);
		setenv("FLCKNCONDCNT",		"10",					1);
		setenv("FLCKWAITERCNT",		"1000",					1);

		// run child
		string	childcmd	= procname;
		childcmd			+= " -env child";
		if(0 != system(childcmd.c_str())){
			ERR("Failed to run child.");
			return false;
		}

	}else{
		// child
		strtesttype = "Test Environment(child)";

		if(	0 != strcmp(getenv("FLCKAUTOINIT"),		"NO")				||
			0 != strcmp(getenv("FLCKROBUSTMODE"),	"LOW")				||
			0 != strcmp(getenv("FLCKNOMAPMODE"),	"DENY_RETRY")		||
			0 != strcmp(getenv("FLCKFREEUNITMODE"),	"OFFSET")			||
			0 != strcmp(getenv("FLCKROBUSTCHKCNT"),	"1000")				||
			0 != strcmp(getenv("FLCKDIRPATH"),		"/tmp/.fullocktest")||
			0 != strcmp(getenv("FLCKFILENAME"),		"fullocktest.shm")	||
			0 != strcmp(getenv("FLCKFILECNT"),		"10")				||
			0 != strcmp(getenv("FLCKOFFETCNT"),		"10000")			||
			0 != strcmp(getenv("FLCKLOCKERCNT"),	"1000")				||
			0 != strcmp(getenv("FLCKNMTXCNT"),		"10")				||
			0 != strcmp(getenv("FLCKNCONDCNT"),		"10")				||
			0 != strcmp(getenv("FLCKWAITERCNT"),	"1000")				)
		{
			ERR("Some environment is not set.");
			return false;
		}

		// FLCKAUTOINIT
		if(FlShm::pShmBase){
			ERR("Fullock SHM is initialized, it should be not initialized(FLCKAUTOINIT=NO).");
			return false;
		}
		// FLCKROBUSTMODE
		if(!FlShm::IsLowRobust()){
			ERR("FLCKROBUSTMODE does not LOW.");
			return false;
		}
		// FLCKNOMAPMODE
		if(FlShm::NOMAP_DENY_RETRY != FlShm::GetNomapMode()){
			ERR("FLCKNOMAPMODE does not DENY_RETRY.");
			return false;
		}
		// FLCKFREEUNITMODE
		if(!(!FlShm::IsFreeUnitFd() && FlShm::IsFreeUnitOffset())){
			ERR("FLCKFREEUNITMODE does not OFFSET.");
			return false;
		}
		// FLCKROBUSTCHKCNT
		if(1000 != FlShm::GetRobustLoopCnt()){
			ERR("FLCKROBUSTCHKCNT does not 1000.");
			return false;
		}
		// FLCKFILECNT
		if(10 != FlShm::GetFileLockAreaCount()){
			ERR("FLCKFILECNT does not 10.");
			return false;
		}
		// FLCKOFFETCNT
		if(10000 != FlShm::GetOffLockAreaCount()){
			ERR("FLCKOFFETCNT does not 10000.");
			return false;
		}
		// FLCKLOCKERCNT
		if(1000 != FlShm::GetLockerAreaCount()){
			ERR("FLCKLOCKERCNT does not 1000.");
			return false;
		}
		// FLCKNMTXCNT
		if(10 != FlShm::GetNMtxAreaCount()){
			ERR("FLCKNMTXCNT does not 10.");
			return false;
		}
		// FLCKNCONDCNT
		if(10 != FlShm::GetNCondAreaCount()){
			ERR("FLCKNCONDCNT does not 10.");
			return false;
		}
		// FLCKLOCKERCNT
		if(1000 != FlShm::GetWaiterAreaCount()){
			ERR("FLCKWAITERCNT does not 1000.");
			return false;
		}
		// unlink shm file
		if(0 != unlink("/tmp/.fullocktest/fullocktest.shm") && ENOENT != errno){
			ERR("Failed to remove shm file(/tmp/.fullocktest/fullocktest.shm).");
			return false;
		}
		// Re-initialize
		if(!FlShm::ReInitializeObject(NULL, NULL, FlShm::GetFileLockAreaCount(), FlShm::GetOffLockAreaCount(), FlShm::GetLockerAreaCount(), FlShm::GetNMtxAreaCount(), FlShm::GetNCondAreaCount(), FlShm::GetWaiterAreaCount())){
			ERR("Failed reinitializing fullock SHM.");
			return false;
		}
		struct stat	st;
		if(0 != stat("/tmp/.fullocktest/fullocktest.shm", &st)){
			ERR("Fullock SHM file(/tmp/.fullocktest/fullocktest.shm) does not exist.");
			return false;
		}
	}
	return true;
}

//---------------------------------------------------------
// Test over area count limit functions
//---------------------------------------------------------
static bool overareacnt_test(string& strtesttype, const char* procname, bool is_parent, FlShm::FREEUNITMODE mode)
{
	if(is_parent){
		// parent
		strtesttype = "Test Over area count limit(parent)";

		if(!MakeTestFile()){
			ERR("Failed to create test file.");
			return false;
		}

		setenv("FLCKAUTOINIT",		"YES",					1);
		setenv("FLCKROBUSTMODE",	"LOW",					1);
		setenv("FLCKNOMAPMODE",		"DENY",					1);
		setenv("FLCKFREEUNITMODE",	(FlShm::FREE_NO == mode ? "NO" : FlShm::FREE_FD == mode ? "FD" : "OFFSET"), 1);
		setenv("FLCKROBUSTCHKCNT",	"1000",					1);
		setenv("FLCKDIRPATH",		"/tmp/.fullocktest",	1);
		setenv("FLCKFILENAME",		"fullocktest.shm",		1);
		setenv("FLCKFILECNT",		"1",					1);
		setenv("FLCKOFFETCNT",		"4",					1);
		setenv("FLCKLOCKERCNT",		"4",					1);
		setenv("FLCKNMTXCNT",		"1",					1);

		// run child
		string	childcmd	= procname;
		childcmd			+= " -oac child -unit ";
		childcmd			+= (FlShm::FREE_NO == mode ? "no" : FlShm::FREE_FD == mode ? "fd" : "offset");
		if(0 != system(childcmd.c_str())){
			ERR("Failed to run child.");
			return false;
		}

	}else{
		// child
		strtesttype = "Test Over area count limit(child)";

		int	fd;
		if(-1 == (fd = open(MYTEST_FILE, O_RDONLY))){
			ERR("Could not open file(%s), errno = %d", MYTEST_FILE, errno);
			return -1;
		}

		int	lockcnt;
		for(lockcnt = 0; lockcnt < 100; ++lockcnt){
			FlShm	shm;
			int		result;
			if(0 != (result = shm.ReadLock(fd, static_cast<off_t>(lockcnt), 1))){
				//PRN("FAILED READ LOCK - No.%d, error=%d", lockcnt, result);
				break;
			}
		}
		bool	bresult = true;
		if(4 != lockcnt){
			ERR("LOCKING LIMIT(%d) is not current limit(4).", lockcnt + 1);
			bresult = false;
		}

		// free
		for(int cnt = 0; cnt < lockcnt; ++cnt){
			FlShm	shm;
			int		result;
			if(0 != (result = shm.Unlock(fd, static_cast<off_t>(cnt), 1))){
				PRN("FAILED UNLOCK - No.%d, error=%d\n", cnt, result);
				break;
			}
		}
		close(fd);

		if(!bresult){
			return false;
		}
	}
	return true;
}

//---------------------------------------------------------
// Test deadlock functions
//---------------------------------------------------------
static bool deadlock_test(string& strtesttype, const char* procname, bool is_parent)
{
	if(is_parent){
		// parent
		strtesttype = "Test Deadlock(parent)";

		if(!MakeTestFile()){
			ERR("Failed to create test file.");
			return false;
		}

		// open
		int	fd;
		if(-1 == (fd = open(MYTEST_FILE, O_RDWR))){
			ERR("Could not open file(%s), errno = %d", MYTEST_FILE, errno);
			return false;
		}

		// write lock
		FlShm	shm;
		int		result;
		if(0 != (result = shm.WriteLock(fd, 0, 1))){
			ERR("Failed write lock, error=%d", result);
			close(fd);
			return false;
		}

		// run child
		string	childcmd	= procname;
		childcmd			+= " -dl child";
		if(0 != system(childcmd.c_str())){
			ERR("Failed to run child.");
			shm.Unlock(fd, 0, 1);
			close(fd);
			return false;
		}
		shm.Unlock(fd, 0, 1);
		close(fd);

	}else{
		// child
		strtesttype = "Test Deadlock(child)";

		int	fd;
		if(-1 == (fd = open(MYTEST_FILE, O_RDWR))){
			ERR("Could not open file(%s), errno = %d", MYTEST_FILE, errno);
			return false;
		}

		FlShm	shm;
		int		result;

		// try read lock
		if(0 == (result = shm.TryReadLock(fd, 0, 1))){
			ERR("Succeed to get read lock, it should block...");
			shm.Unlock(fd, 0, 1);
			close(fd);
			return false;
		}
		// timeouted lock
		if(0 == (result = shm.TimeoutReadLock(fd, 0, 1, 1000))){		// 1ms timeout.
			ERR("Succeed to get read lock(timeout), it should block...");
			shm.Unlock(fd, 0, 1);
			close(fd);
			return false;
		}
		close(fd);
	}
	return true;
}

//---------------------------------------------------------
// Test automatic recover by thread functions
//---------------------------------------------------------
static volatile bool	writer_exiter_thread_result = false;
static volatile bool	reader_waiter_thread_result = false;

static void* writer_exiter_thread(void* param)
{
	writer_exiter_thread_result = false;

	// open
	int	fd;
	if(-1 == (fd = open(MYTEST_FILE, O_RDWR))){
		ERR("Could not open file(%s), errno = %d", MYTEST_FILE, errno);
		pthread_exit(NULL);
	}

	// write lock
	FlShm	shm;
	int		result;
	if(0 != (result = shm.WriteLock(fd, 0, 1))){
		ERR("Failed write lock, error=%d", result);
		close(fd);
		pthread_exit(NULL);
	}
	sleep(3);

	// close fd without unlock.
	close(fd);

	sleep(5);

	writer_exiter_thread_result = true;

	pthread_exit(NULL);
	return NULL;
}

static void* reader_waiter_thread(void* param)
{
	reader_waiter_thread_result = false;

	// open
	int	fd;
	if(-1 == (fd = open(MYTEST_FILE, O_RDWR))){
		ERR("Could not open file(%s), errno = %d", MYTEST_FILE, errno);
		pthread_exit(NULL);
	}

	// try read lock
	FlShm	shm;
	int		result;
	if(0 == (result = shm.TryReadLock(fd, 0, 1))){
		ERR("Succeed read lock during writer locking...");
		shm.Unlock(fd, 0, 1);
		close(fd);
		pthread_exit(NULL);
	}

	// read lock(dead lock)
	if(0 != (result = shm.ReadLock(fd, 0, 1))){
		ERR("Failed read lock, error code=%d.", result);
		close(fd);
		pthread_exit(NULL);
	}

	// after exiting writer(without unlocking), automatical recover and reader gets lock.
	shm.Unlock(fd, 0, 1);
	close(fd);

	reader_waiter_thread_result = true;

	pthread_exit(NULL);
	return NULL;
}

static bool autorecover_thread_test(string& strtesttype, const char* procname, FlShm::ROBUSTMODE mode)
{
	strtesttype = "automatic recover by thread";

	if(!MakeTestFile()){
		ERR("Failed to create test file.");
		return false;
	}

	// robust mode
	setenv("FLCKROBUSTMODE",	(FlShm::ROBUST_NO == mode ? "NO" : FlShm::ROBUST_LOW == mode ? "LOW" : "HIGH"), 1);
	setenv("FLCKDIRPATH",		"/tmp/.fullocktest",	1);
	setenv("FLCKFILENAME",		"fullocktest.shm",		1);

	// remake shm
	if(!FlShm::ReInitializeObject()){
		ERR("Failed reinitializing fullock SHM.");
		return false;
	}

	// run thread - writer(exiter)
	pthread_t	writer_tid;
	if(0 != pthread_create(&writer_tid, NULL, writer_exiter_thread, NULL)){
		ERR("Could not create writer(exiter) thread.");
		return false;
	}
	// wait(writer sleep 3sec after locking...)
	sleep(1);

	// run thread - reader(waiter)
	pthread_t	reader_tid;
	if(0 != pthread_create(&reader_tid, NULL, reader_waiter_thread, NULL)){
		ERR("Could not create reader(waiter) thread.");
		// do not wait writer exiting..., because this is test program.
		return false;
	}

	// wait for exiting children
	int		result;
	void*	pretval = NULL;
	if(0 != (result = pthread_join(writer_tid, &pretval))){
		ERR("Failed to wait writer thread exit. return code(error) = %d", result);
	}
	if(0 != (result = pthread_join(reader_tid, &pretval))){
		ERR("Failed to wait reader thread exit. return code(error) = %d", result);
	}

	if(!writer_exiter_thread_result || !reader_waiter_thread_result){
		return false;
	}
	return true;
}

//---------------------------------------------------------
// Test automatic recover by process functions
//---------------------------------------------------------
static bool writer_exiter_proc(void)
{
	// open
	int	fd;
	if(-1 == (fd = open(MYTEST_FILE, O_RDWR))){
		ERR("Could not open file(%s), errno = %d", MYTEST_FILE, errno);
		return false;
	}

	// write lock
	FlShm	shm;
	int		result;
	if(0 != (result = shm.WriteLock(fd, 0, 1))){
		ERR("Failed write lock, error=%d", result);
		close(fd);
		return false;
	}
	sleep(3);

	// exit without closing and unlock.
	return true;
}

static bool reader_waiter_proc(void)
{
	// open
	int	fd;
	if(-1 == (fd = open(MYTEST_FILE, O_RDWR))){
		ERR("Could not open file(%s), errno = %d", MYTEST_FILE, errno);
		return false;
	}

	// try read lock
	FlShm	shm;
	int		result;
	if(0 == (result = shm.TryReadLock(fd, 0, 1))){
		ERR("Succeed read lock during writer locking...");
		shm.Unlock(fd, 0, 1);
		close(fd);
		return false;
	}

	// read lock(dead lock)
	if(0 != (result = shm.ReadLock(fd, 0, 1))){
		ERR("Failed read lock, error code=%d.", result);
		close(fd);
		return false;
	}

	// after exiting writer(without unlocking), automatical recover and reader gets lock.
	shm.Unlock(fd, 0, 1);
	close(fd);

	return true;
}

static bool autorecover_exit_test(string& strtesttype, const char* procname, FLWORKERTYPE workertype, FlShm::ROBUSTMODE mode)
{
	if(WORKER_TYPE_PARENT == workertype){
		strtesttype = "automatic recover by exit(parent)";

		if(!MakeTestFile()){
			ERR("Failed to create test file.");
			return false;
		}

		// robust mode
		setenv("FLCKROBUSTMODE",	(FlShm::ROBUST_NO == mode ? "NO" : FlShm::ROBUST_LOW == mode ? "LOW" : "HIGH"), 1);
		setenv("FLCKDIRPATH",		"/tmp/.fullocktest",	1);
		setenv("FLCKFILENAME",		"fullocktest.shm",		1);

		// remake shm
		if(!FlShm::ReInitializeObject()){
			ERR("Failed reinitializing fullock SHM.");
			return false;
		}

		// run process - writer(exiter)
		string	childcmd	= procname;
		childcmd			+= " -ar exiter -process -robust ";
		childcmd			+= (FlShm::ROBUST_NO == mode ? "no" : FlShm::ROBUST_LOW == mode ? "low" : "high");
		childcmd			+= " &";
		if(0 != system(childcmd.c_str())){
			ERR("Failed to run child writer(exiter).");
			return false;
		}
		sleep(1);

		// run process - reader(waiter)
		childcmd			= procname;
		childcmd			+= " -ar waiter -process -robust ";
		childcmd			+= (FlShm::ROBUST_NO == mode ? "no" : FlShm::ROBUST_LOW == mode ? "low" : "high");
		childcmd			+= " &";
		if(0 != system(childcmd.c_str())){
			ERR("Failed to run child reader(waiter).");
			return false;
		}
		sleep(10);

		// kill waiter
		if(FlShm::ROBUST_NO == mode){
			childcmd		= "kill -9 `ps ax | grep ";
			childcmd		+= procname;
			childcmd		+= " | grep -v grep | grep waiter | grep process | awk '{print $1}'`";
			if(0 != system(childcmd.c_str())){
				ERR("Failed to kill child waiter.");
			}else{
				ERR("child waiter killed.");
			}
		}

	}else if(WORKER_TYPE_EXITER == workertype){
		strtesttype = "automatic recover by exit(exiter)";
		return writer_exiter_proc();

	}else{
		strtesttype = "automatic recover by exit(waiter)";
		return reader_waiter_proc();
	}
	return true;
}

//---------------------------------------------------------
// Test mutex over area count limit functions
//---------------------------------------------------------
static bool mutex_overareacnt_test(string& strtesttype, const char* procname, bool is_parent)
{
	if(is_parent){
		// parent
		strtesttype = "Test Mutex Over area count limit(parent)";

		setenv("FLCKAUTOINIT",		"YES",					1);
		setenv("FLCKDIRPATH",		"/tmp/.fullocktest",	1);
		setenv("FLCKFILENAME",		"fullocktest.shm",		1);
		setenv("FLCKNMTXCNT",		"1",					1);

		// run child
		string	childcmd	= procname;
		childcmd			+= " -moac child";
		if(0 != system(childcmd.c_str())){
			ERR("Failed to run child.");
			return false;
		}

	}else{
		// child
		strtesttype = "Test Mutex Over area count limit(child)";

		// lock 2 names
		FlShm	shm;
		int		result;
		if(0 != (result = shm.Lock("MUTEX_TEST1"))){
			ERR("Failed to lock named mutex(MUTEX_TEST1), error=%d", result);
			return false;
		}
		if(0 == (result = shm.Lock("MUTEX_TEST2"))){
			ERR("Succeed to lock named mutex(MUTEX_TEST2), it should be failed.");
			return false;
		}
		// unlock
		if(0 != (result = shm.Unlock("MUTEX_TEST1"))){
			ERR("Failed to unlock named mutex(MUTEX_TEST1), error=%d", result);
			return false;
		}
	}
	return true;
}

//---------------------------------------------------------
// Test Mutex deadlock functions
//---------------------------------------------------------
static bool mutex_deadlock_test(string& strtesttype, const char* procname, bool is_parent)
{
	if(is_parent){
		// parent
		strtesttype = "Test Mutex Deadlock(parent)";

		// lock
		FlShm	shm;
		int		result;
		if(0 != (result = shm.Lock("MUTEX_TEST"))){
			ERR("Failed to lock named mutex(MUTEX_TEST), error=%d", result);
			return false;
		}

		// run child
		string	childcmd	= procname;
		childcmd			+= " -mdl child";
		if(0 != system(childcmd.c_str())){
			ERR("Failed to run child.");
			shm.Unlock("MUTEX_TEST");
			return false;
		}
		shm.Unlock("MUTEX_TEST");

	}else{
		// child
		strtesttype = "Test Mutex Deadlock(child)";

		// try lock
		FlShm	shm;
		int		result;
		if(0 == (result = shm.TryLock("MUTEX_TEST"))){
			ERR("Succeed to mutex(MUTEX_TEST) lock, it should block...");
			shm.Unlock("MUTEX_TEST");
			return false;
		}
		// timeouted lock
		if(0 == (result = shm.TimeoutLock("MUTEX_TEST", 1000))){		// 1ms timeout.
			ERR("Succeed to mutex(MUTEX_TEST) lock(timeout), it should block...");
			shm.Unlock("MUTEX_TEST");
			return false;
		}
	}
	return true;
}

//---------------------------------------------------------
// Test Mutex automatic recover by thread functions
//---------------------------------------------------------
static volatile bool	exiter_thread_result = false;
static volatile bool	waiter_thread_result = false;

static void* mutex_exiter_thread(void* param)
{
	exiter_thread_result = false;

	// lock
	FlShm	shm;
	int		result;
	if(0 != (result = shm.Lock("MUTEX_TEST"))){
		ERR("Failed mutex(MUTEX_TEST) lock, error=%d", result);
		pthread_exit(NULL);
	}
	sleep(3);

	exiter_thread_result = true;

	pthread_exit(NULL);
	return NULL;
}

static void* mutex_waiter_thread(void* param)
{
	waiter_thread_result = false;

	// try lock
	FlShm	shm;
	int		result;
	if(0 == (result = shm.TryLock("MUTEX_TEST"))){
		ERR("Succeed mutex(MUTEX_TEST) lock during exiter locking...");
		shm.Unlock("MUTEX_TEST");
		pthread_exit(NULL);
	}

	// lock(dead lock)
	if(0 != (result = shm.Lock("MUTEX_TEST"))){
		ERR("Failed mutex(MUTEX_TEST) lock, error code=%d.", result);
		pthread_exit(NULL);
	}

	// after exiting exiter(without unlocking), automatical recover and gets lock.
	shm.Unlock("MUTEX_TEST");

	waiter_thread_result = true;

	pthread_exit(NULL);
	return NULL;
}

static bool mutex_autorecover_thread_test(string& strtesttype, const char* procname, FlShm::ROBUSTMODE mode)
{
	strtesttype = "mutex automatic recover by thread";

	// robust mode
	setenv("FLCKROBUSTMODE",	(FlShm::ROBUST_NO == mode ? "NO" : FlShm::ROBUST_LOW == mode ? "LOW" : "HIGH"), 1);
	setenv("FLCKDIRPATH",		"/tmp/.fullocktest",	1);
	setenv("FLCKFILENAME",		"fullocktest.shm",		1);

	// remake shm
	if(!FlShm::ReInitializeObject()){
		ERR("Failed reinitializing fullock SHM.");
		return false;
	}

	// run exiting thread
	pthread_t	exiter_tid;
	if(0 != pthread_create(&exiter_tid, NULL, mutex_exiter_thread, NULL)){
		ERR("Could not create mutex exiter thread.");
		return false;
	}
	// wait(exiter sleep 3sec after locking...)
	sleep(1);

	// run waiter thread
	pthread_t	waiter_tid;
	if(0 != pthread_create(&waiter_tid, NULL, mutex_waiter_thread, NULL)){
		ERR("Could not create mutex waiter thread.");
		// do not wait exiter thread exiting..., because this is test program.
		return false;
	}

	// wait for exiting children
	int		result;
	void*	pretval = NULL;
	if(0 != (result = pthread_join(exiter_tid, &pretval))){
		ERR("Failed to wait exiter thread exit. return code(error) = %d", result);
	}
	if(0 != (result = pthread_join(waiter_tid, &pretval))){
		ERR("Failed to wait waiter thread exit. return code(error) = %d", result);
	}

	if(!exiter_thread_result || !waiter_thread_result){
		return false;
	}
	return true;
}

//---------------------------------------------------------
// Test automatic recover by process functions
//---------------------------------------------------------
static bool mutex_exiter_proc(void)
{
	// lock
	FlShm	shm;
	int		result;
	if(0 != (result = shm.Lock("MUTEX_TEST"))){
		ERR("Failed mutex lock, error=%d", result);
		return false;
	}
	sleep(3);

	// exit without unlock.
	return true;
}

static bool mutex_waiter_proc(void)
{
	// try lock
	FlShm	shm;
	int		result;
	if(0 == (result = shm.TryLock("MUTEX_TEST"))){
		ERR("Succeed mutex lock during exiter locking...");
		shm.Unlock("MUTEX_TEST");
		return false;
	}

	// read lock(dead lock)
	if(0 != (result = shm.Lock("MUTEX_TEST"))){
		ERR("Failed mutex lock, error code=%d.", result);
		return false;
	}

	// after exiting exiter(without unlocking), automatical recover and waiter gets lock.
	shm.Unlock("MUTEX_TEST");

	return true;
}

static bool mutex_autorecover_exit_test(string& strtesttype, const char* procname, FLWORKERTYPE workertype, FlShm::ROBUSTMODE mode)
{
	if(WORKER_TYPE_PARENT == workertype){
		strtesttype = "mutex automatic recover by exit(parent)";

		// robust mode
		setenv("FLCKROBUSTMODE",	(FlShm::ROBUST_NO == mode ? "NO" : FlShm::ROBUST_LOW == mode ? "LOW" : "HIGH"), 1);
		setenv("FLCKDIRPATH",		"/tmp/.fullocktest",	1);
		setenv("FLCKFILENAME",		"fullocktest.shm",		1);

		// remake shm
		if(!FlShm::ReInitializeObject()){
			ERR("Failed reinitializing fullock SHM.");
			return false;
		}

		// run process - exiter
		string	childcmd	= procname;
		childcmd			+= " -mar exiter -process -robust ";
		childcmd			+= (FlShm::ROBUST_NO == mode ? "no" : FlShm::ROBUST_LOW == mode ? "low" : "high");
		childcmd			+= " &";
		if(0 != system(childcmd.c_str())){
			ERR("Failed to run child exiter.");
			return false;
		}
		sleep(1);

		// run process - waiter
		childcmd			= procname;
		childcmd			+= " -mar waiter -process -robust ";
		childcmd			+= (FlShm::ROBUST_NO == mode ? "no" : FlShm::ROBUST_LOW == mode ? "low" : "high");
		childcmd			+= " &";
		if(0 != system(childcmd.c_str())){
			ERR("Failed to run child waiter.");
			return false;
		}
		sleep(10);

		// kill waiter
		if(FlShm::ROBUST_NO == mode){
			childcmd		= "kill -9 `ps ax | grep ";
			childcmd		+= procname;
			childcmd		+= " | grep -v grep | grep waiter | grep process | awk '{print $1}'`";
			if(0 != system(childcmd.c_str())){
				ERR("Failed to kill child waiter.");
			}else{
				ERR("child waiter killed.");
			}
		}

	}else if(WORKER_TYPE_EXITER == workertype){
		strtesttype = "mutex automatic recover by exit(exiter)";
		return mutex_exiter_proc();

	}else{
		strtesttype = "mutex automatic recover by exit(waiter)";
		return mutex_waiter_proc();
	}
	return true;
}

//---------------------------------------------------------
// Test Cond over area count limit functions
//---------------------------------------------------------
static bool cond_overareacnt_test(string& strtesttype, const char* procname, bool is_parent)
{
	if(is_parent){
		// parent
		strtesttype = "Test Cond Over area count limit(parent)";

		setenv("FLCKAUTOINIT",		"YES",					1);
		setenv("FLCKDIRPATH",		"/tmp/.fullocktest",	1);
		setenv("FLCKFILENAME",		"fullocktest.shm",		1);
		setenv("FLCKNCONDCNT",		"1",					1);

		// run child
		string	childcmd	= procname;
		childcmd			+= " -coac child";
		if(0 != system(childcmd.c_str())){
			ERR("Failed to run child.");
			return false;
		}

	}else{
		// child
		strtesttype = "Test Cond Over area count limit(child)";

		// wait 2 names
		FlShm	shm;
		int		result;
		if(0 != (result = shm.Lock("MUTEX_TEST1"))){
			ERR("Failed to lock named mutex(MUTEX_TEST1) for cond(COND_TEST1), error=%d", result);
			return false;
		}
		if(ETIMEDOUT != (result = shm.TimeoutWait("COND_TEST1", "MUTEX_TEST1", 1000))){		// 1ms timeout
			ERR("Failed to wait named cond(COND_TEST1), error=%d", result);
			shm.Unlock("MUTEX_TEST1");
			return false;
		}
		if(ETIMEDOUT == (result = shm.TimeoutWait("COND_TEST2", "MUTEX_TEST1", 1000))){		// 1ms timeout
			ERR("Success to wait named cond(COND_TEST2), it must be not ETIMEOUT(110)");
			shm.Unlock("MUTEX_TEST1");
			return false;
		}
		if(0 != (result = shm.Unlock("MUTEX_TEST1"))){
			ERR("Failed to unlock named mutex(MUTEX_TEST1) for cond(COND_TEST1), error=%d", result);
			return false;
		}
	}
	return true;
}

//---------------------------------------------------------
// Main
//---------------------------------------------------------
int main(int argc, char** argv)
{
	// Parse Parameters
	optparams_t	optparams;
	OptionParser(argc, argv, optparams);

	bool					result = false;
	string					strtesttype;
	optparams_t::iterator	iter;
	if(optparams.end() != (iter = optparams.find("-help")) || optparams.end() != (iter = optparams.find("-h"))){
		Help(argv[0]);
		exit(EXIT_SUCCESS);
	}

	if(optparams.end() != (iter = optparams.find("-env"))){
		// environment test
		result = env_test(strtesttype, argv[0], iter->second.rawstring.empty());

	}else if(optparams.end() != (iter = optparams.find("-overareacnt")) || optparams.end() != (iter = optparams.find("-oac"))){
		// over area count limit test
		optparams_t::iterator	iter2;
		if(optparams.end() == (iter2 = optparams.find("-unit"))){
			ERR("\"-overareacnt(oac)\" type needs \"-unit\" parameter.");
			exit(EXIT_FAILURE);
		}
		FlShm::FREEUNITMODE	mode;
		if(0 == strcmp(iter2->second.rawstring.c_str(), "no")){
			mode = FlShm::FREE_NO;
		}else if(0 == strcmp(iter2->second.rawstring.c_str(), "fd")){
			mode = FlShm::FREE_FD;
		}else if(0 == strcmp(iter2->second.rawstring.c_str(), "offset")){
			mode = FlShm::FREE_OFFSET;
		}else{
			ERR("\"-unit\" parameter(%s) is not \"no\" or \"fd\" or \"offset\".", iter2->second.rawstring.c_str());
			exit(EXIT_FAILURE);
		}
		result = overareacnt_test(strtesttype, argv[0], iter->second.rawstring.empty(), mode);

	}else if(optparams.end() != (iter = optparams.find("-deadlock")) || optparams.end() != (iter = optparams.find("-dl"))){
		// dead lock test
		result = deadlock_test(strtesttype, argv[0], iter->second.rawstring.empty());

	}else if(optparams.end() != (iter = optparams.find("-autorecover")) || optparams.end() != (iter = optparams.find("-ar"))){
		// auto recover test
		optparams_t::iterator	iter2;
		if(optparams.end() == (iter2 = optparams.find("-robust"))){
			ERR("\"-autorecover(ar)\" type needs \"-robust\" parameter.");
			exit(EXIT_FAILURE);
		}
		FlShm::ROBUSTMODE	mode;
		if(0 == strcmp(iter2->second.rawstring.c_str(), "no")){
			mode = FlShm::ROBUST_NO;
		}else if(0 == strcmp(iter2->second.rawstring.c_str(), "low")){
			mode = FlShm::ROBUST_LOW;
		}else if(0 == strcmp(iter2->second.rawstring.c_str(), "high")){
			mode = FlShm::ROBUST_HIGH;
		}else{
			ERR("\"-robust\" parameter(%s) is not \"no\" or \"low\" or \"high\".", iter2->second.rawstring.c_str());
			exit(EXIT_FAILURE);
		}

		// do
		if(optparams.end() != (iter2 = optparams.find("-thread"))){
			// target is thread
			if(FlShm::ROBUST_HIGH != mode){
				PRN("NOTICE: Run \"-autorecover\" with \"-thread\" and \"%s\" robust mode, this case is DEADLOCK!", (mode == FlShm::ROBUST_LOW ? "low" : "high"));
			}
			result = autorecover_thread_test(strtesttype, argv[0], mode);

		}else if(optparams.end() != (iter2 = optparams.find("-process"))){
			// target is process
			FLWORKERTYPE	workertype;
			if(iter->second.rawstring.empty()){
				if(FlShm::ROBUST_NO == mode){
					PRN("NOTICE: Run \"-autorecover\" with \"-process\" and \"no\" robust mode, this case is DEADLOCK!");
				}
				workertype = WORKER_TYPE_PARENT;
			}else if(0 == strcasecmp(iter->second.rawstring.c_str(), "exiter")){
				workertype = WORKER_TYPE_EXITER;
			}else if(0 == strcasecmp(iter->second.rawstring.c_str(), "waiter")){
				workertype = WORKER_TYPE_WAITER;
			}else{
				ERR("\"-mar\" parameter(%s) is wrong.", iter->second.rawstring.c_str());
				exit(EXIT_FAILURE);
			}
			result = autorecover_exit_test(strtesttype, argv[0], workertype, mode);

		}else{
			ERR("\"-autorecover(ar)\" type needs parameter.");
			exit(EXIT_FAILURE);
		}

	}else if(optparams.end() != (iter = optparams.find("-moverareacnt")) || optparams.end() != (iter = optparams.find("-moac"))){
		// mutex over area count limit test
		result = mutex_overareacnt_test(strtesttype, argv[0], iter->second.rawstring.empty());

	}else if(optparams.end() != (iter = optparams.find("-mdeadlock")) || optparams.end() != (iter = optparams.find("-mdl"))){
		// mutex dead lock test
		result = mutex_deadlock_test(strtesttype, argv[0], iter->second.rawstring.empty());

	}else if(optparams.end() != (iter = optparams.find("-mautorecover")) || optparams.end() != (iter = optparams.find("-mar"))){
		// mutex auto recover test
		optparams_t::iterator	iter2;
		if(optparams.end() == (iter2 = optparams.find("-robust"))){
			ERR("\"-mautorecover(mar)\" type needs \"-robust\" parameter.");
			exit(EXIT_FAILURE);
		}
		FlShm::ROBUSTMODE	mode;
		if(0 == strcmp(iter2->second.rawstring.c_str(), "no")){
			mode = FlShm::ROBUST_NO;
		}else if(0 == strcmp(iter2->second.rawstring.c_str(), "low")){
			mode = FlShm::ROBUST_LOW;
		}else if(0 == strcmp(iter2->second.rawstring.c_str(), "high")){
			mode = FlShm::ROBUST_HIGH;
		}else{
			ERR("\"-robust\" parameter(%s) is not \"no\" or \"low\" or \"high\".", iter2->second.rawstring.c_str());
			exit(EXIT_FAILURE);
		}

		// do
		if(optparams.end() != (iter2 = optparams.find("-thread"))){
			// thread
			if(FlShm::ROBUST_HIGH != mode){
				PRN("NOTICE: Run \"-mautorecover\" with \"-thread\" and \"%s\" robust mode, this case is DEADLOCK!", (mode == FlShm::ROBUST_LOW ? "low" : "high"));
			}
			result = mutex_autorecover_thread_test(strtesttype, argv[0], mode);

		}else if(optparams.end() != (iter2 = optparams.find("-process"))){
			// process
			FLWORKERTYPE	workertype;
			if(iter->second.rawstring.empty()){
				if(FlShm::ROBUST_NO == mode){
					PRN("NOTICE: Run \"-autorecover\" with \"-process\" and \"no\" robust mode, this case is DEADLOCK!");
				}
				workertype = WORKER_TYPE_PARENT;
			}else if(0 == strcasecmp(iter->second.rawstring.c_str(), "exiter")){
				workertype = WORKER_TYPE_EXITER;
			}else if(0 == strcasecmp(iter->second.rawstring.c_str(), "waiter")){
				workertype = WORKER_TYPE_WAITER;
			}else{
				ERR("\"-mar\" parameter(%s) is wrong.", iter->second.rawstring.c_str());
				exit(EXIT_FAILURE);
			}
			result = mutex_autorecover_exit_test(strtesttype, argv[0], workertype, mode);

		}else{
			ERR("\"-mautorecover(mar)\" type needs \"-thread\" or \"-process\"parameter.");
			exit(EXIT_FAILURE);
		}

	}else if(optparams.end() != (iter = optparams.find("-coverareacnt")) || optparams.end() != (iter = optparams.find("-coac"))){
		// cond over area count limit test
		result = cond_overareacnt_test(strtesttype, argv[0], iter->second.rawstring.empty());

	}else{
		ERR("Does not specify parameters, you can see parameters by \"-help\" parameter.");
		Help(argv[0]);
		exit(EXIT_FAILURE);
	}

	PRN("%s result : %s", strtesttype.c_str(), result ? "SUCCEED" : "FAILED");

	exit(EXIT_SUCCESS);
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
