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
 * CREATE:   Mon 15 Jun 2015
 * REVISION:
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <libgen.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <map>

#include "flckshm.h"
#include "flckutil.h"
#include "fullock.h"

using namespace std;

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
	PRN("Usage: %s -f filename [file options]", progname ? programname(progname) : "program");
	PRN("       %s -mutex(m) mutexname", progname ? programname(progname) : "program");
	PRN("       %s -cond(c) condname", progname ? programname(progname) : "program");
	PRN("       %s -help(h)", progname ? programname(progname) : "program");
	PRN(NULL);
	PRN("file options:");
	PRN("       -read(r)             test for reader lock(exclusive -write)");
	PRN("       -write(w)            test for writer lock(exclusive -read)");
	PRN(NULL);
	PRN("*** Test of cond(condition) is not perfect in this program.");
	PRN("    You should test another program for testing cond.");
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

static bool InitTestFile(const char* filepath)
{
	int	fd;
	if(-1 == (fd = open(filepath, O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH))){
		ERR("Could not open file(%s).", filepath);
		return false;
	}

	if(!flck_fill_zero(fd, 32, 0)){
		ERR("Could not write to file(%s).", filepath);
		close(fd);
		return false;
	}
	close(fd);
	return true;
}

//---------------------------------------------------------
// Functions
//---------------------------------------------------------
static bool SimpleReadWriteLock(const char* filepath, bool is_read)
{
	int	fd;
	if(-1 == (fd = open(filepath, O_RDWR))){
		ERR("Could not open file(%s).", filepath);
		return false;
	}

	// C++ I/F
	int	result;
	{
		FlShm	flobj;

		PRN("[Start] - C++ I/F test.");

		// Lock
		if(is_read){
			result = flobj.ReadLock(fd, 0, 1);
		}else{
			result = flobj.WriteLock(fd, 0, 1);
		}
		PRN("[Phase 1] - Lock: result(errno=%d)", result);
		if(0 != result){
			close(fd);
			return false;
		}
		result = flobj.Unlock(fd, 0, 1);
		PRN("[Phase 2] - Unlock: result(errno=%d)", result);
		if(0 != result){
			close(fd);
			return false;
		}

		// TryLock
		if(is_read){
			result = flobj.TryReadLock(fd, 0, 1);
		}else{
			result = flobj.TryWriteLock(fd, 0, 1);
		}
		PRN("[Phase 3] - TryLock: result(errno=%d)", result);
		if(0 != result){
			close(fd);
			return false;
		}
		result = flobj.Unlock(fd, 0, 1);
		PRN("[Phase 4] - Unlock: result(errno=%d)", result);
		if(0 != result){
			close(fd);
			return false;
		}

		// TimedLock
		if(is_read){
			result = flobj.TimeoutReadLock(fd, 0, 1, 1000 * 1000);		// 1s
		}else{
			result = flobj.TimeoutWriteLock(fd, 0, 1, 1000 * 1000);		// 1s
		}
		PRN("[Phase 5] - Timeout Lock: result(errno=%d)", result);
		if(0 != result){
			close(fd);
			return false;
		}
		result = flobj.Unlock(fd, 0, 1);
		PRN("[Phase 6] - Unlock: result(errno=%d)", result);
		if(0 != result){
			close(fd);
			return false;
		}

		// Is Locked
		bool	is_locked;
		if(is_read){
			result = flobj.ReadLock(fd, 0, 1);
		}else{
			result = flobj.WriteLock(fd, 0, 1);
		}
		is_locked = flobj.IsLocked(fd, 0, 1);

		PRN("[Phase 7] - Check Lock status: result(errno=%d), and Lock Status(%s)", result, is_locked ? "OK" : "NG");
		if(0 != result){
			close(fd);
			return false;
		}
		result = flobj.Unlock(fd, 0, 1);
		PRN("[Phase 8] - Unlock: result(errno=%d)", result);
		if(0 != result){
			close(fd);
			return false;
		}

		PRN("[Finish] - All C++ I/F tests were succeed.\n");
	}

	// C I/F
	{
		PRN("[Start] - C I/F test.");

		// Lock
		if(is_read){
			result = fullock_rwlock_rdlock(fd, 0, 1);
		}else{
			result = fullock_rwlock_wrlock(fd, 0, 1);
		}
		PRN("[Phase 1] - Lock: result(errno=%d)", result);
		if(0 != result){
			close(fd);
			return false;
		}
		result = fullock_rwlock_unlock(fd, 0, 1);
		PRN("[Phase 2] - Unlock: result(errno=%d)", result);
		if(0 != result){
			close(fd);
			return false;
		}

		// TryLock
		if(is_read){
			result = fullock_rwlock_tryrdlock(fd, 0, 1);
		}else{
			result = fullock_rwlock_trywrlock(fd, 0, 1);
		}
		PRN("[Phase 3] - TryLock: result(errno=%d)", result);
		if(0 != result){
			close(fd);
			return false;
		}
		result = fullock_rwlock_unlock(fd, 0, 1);
		PRN("[Phase 4] - Unlock: result(errno=%d)", result);
		if(0 != result){
			close(fd);
			return false;
		}

		// TimedLock
		if(is_read){
			result = fullock_rwlock_timedrdlock(fd, 0, 1, 1000 * 1000);		// 1s
		}else{
			result = fullock_rwlock_timedwrlock(fd, 0, 1, 1000 * 1000);		// 1s
		}
		PRN("[Phase 5] - Timeout Lock: result(errno=%d)", result);
		if(0 != result){
			close(fd);
			return false;
		}
		result = fullock_rwlock_unlock(fd, 0, 1);
		PRN("[Phase 6] - Unlock: result(errno=%d)", result);
		if(0 != result){
			close(fd);
			return false;
		}

		// Is Locked
		bool	is_locked;
		if(is_read){
			result = fullock_rwlock_rdlock(fd, 0, 1);
		}else{
			result = fullock_rwlock_wrlock(fd, 0, 1);
		}
		is_locked = fullock_rwlock_islocked(fd, 0, 1);

		PRN("[Phase 7] - Check Lock status: result(errno=%d), and Lock Status(%s)", result, is_locked ? "OK" : "NG");
		if(0 != result){
			close(fd);
			return false;
		}
		result = fullock_rwlock_unlock(fd, 0, 1);
		PRN("[Phase 8] - Unlock: result(errno=%d)", result);
		if(0 != result){
			close(fd);
			return false;
		}

		PRN("[Finish] - All C I/F tests were succeed.\n");
	}
	close(fd);
	return true;
}

static bool SimpleMutexLock(const char* mutexname)
{
	int	result;

	// C++ I/F
	{
		FlShm	flobj;

		PRN("[Start] - C++ I/F test.");

		// Lock
		result = flobj.Lock(mutexname);
		PRN("[Phase 1] - Lock(%s): result(errno=%d)", mutexname, result);
		if(0 != result){
			return false;
		}
		result = flobj.Unlock(mutexname);
		PRN("[Phase 2] - Unlock(%s): result(errno=%d)", mutexname, result);
		if(0 != result){
			return false;
		}

		// TryLock
		result = flobj.TryLock(mutexname);
		PRN("[Phase 3] - TryLock(%s): result(errno=%d)", mutexname, result);
		if(0 != result){
			return false;
		}
		result = flobj.Unlock(mutexname);
		PRN("[Phase 4] - Unlock(%s): result(errno=%d)", mutexname, result);
		if(0 != result){
			return false;
		}

		// Timeout Lock
		result = flobj.TimeoutLock(mutexname, 1000 * 1000);			// 1s
		PRN("[Phase 5] - Timeout Lock(%s): result(errno=%d)", mutexname, result);
		if(0 != result){
			return false;
		}
		result = flobj.Unlock(mutexname);
		PRN("[Phase 6] - Unlock(%s): result(errno=%d)", mutexname, result);
		if(0 != result){
			return false;
		}

		PRN("[Finish] - All C++ I/F tests were succeed.");
	}

	// C I/F
	{
		PRN("[Start] - C I/F test.");

		// Lock
		result = fullock_mutex_lock(mutexname);
		PRN("[Phase 1] - Lock(%s): result(errno=%d)", mutexname, result);
		if(0 != result){
			return false;
		}
		result = fullock_mutex_unlock(mutexname);
		PRN("[Phase 2] - Unlock(%s): result(errno=%d)", mutexname, result);
		if(0 != result){
			return false;
		}

		// TryLock
		result = fullock_mutex_trylock(mutexname);
		PRN("[Phase 3] - TryLock(%s): result(errno=%d)", mutexname, result);
		if(0 != result){
			return false;
		}
		result = fullock_mutex_unlock(mutexname);
		PRN("[Phase 4] - Unlock(%s): result(errno=%d)", mutexname, result);
		if(0 != result){
			return false;
		}

		// Timeout Lock
		result = fullock_mutex_timedlock(mutexname, 1000 * 1000);		// 1s
		PRN("[Phase 5] - Timeout Lock(%s): result(errno=%d)", mutexname, result);
		if(0 != result){
			return false;
		}
		result = fullock_mutex_unlock(mutexname);
		PRN("[Phase 6] - Unlock(%s): result(errno=%d)", mutexname, result);
		if(0 != result){
			return false;
		}

		PRN("[Finish] - All C I/F tests were succeed.");
	}
	return true;
}

static bool SimpleCondLock(const char* condname)
{
	int	result;

	// C++ I/F
	{
		FlShm	flobj;

		PRN("[Start] - C++ I/F test.");

		// Lock Mutex(same name cond) for cond
		result = flobj.Lock(condname);
		PRN("[Phase 0] - Mutex Lock(%s): result(errno=%d)", condname, result);
		if(0 != result){
			return false;
		}

		// Timeouted wait(not use wait because this program is single thread)
		result = flobj.TimeoutWait(condname, condname, 10000);					// 10ms
		PRN("[Phase 1] - Cond timeouted wait(condname=%s, mutexname=%s): result(errno=%d, 110(ETIMEDOUT) is normal)", condname, condname, result);
		if(ETIMEDOUT != result){
			return false;
		}

		// Signal(but no waiter)
		result = flobj.Signal(condname);
		PRN("[Phase 2] - Send signal(condname=%s): result(errno=%d)", condname, result);
		if(0 != result){
			return false;
		}

		// Broadcast(but no waiter)
		result = flobj.Broadcast(condname);
		PRN("[Phase 3] - Send signal(condname=%s): result(errno=%d)", condname, result);
		if(0 != result){
			return false;
		}

		// Unlock Mutex(same name cond) for cond
		result = flobj.Unlock(condname);
		PRN("[Phase 4] - Mutex Lock(%s): result(errno=%d)", condname, result);
		if(0 != result){
			return false;
		}

		PRN("[Finish] - All C++ I/F tests were succeed.");
	}

	// C I/F
	{
		PRN("[Start] - C I/F test.");

		// Lock Mutex(same name cond) for cond
		result = fullock_mutex_lock(condname);
		PRN("[Phase 0] - Mutex Lock(%s): result(errno=%d)", condname, result);
		if(0 != result){
			return false;
		}

		// Timeouted wait(not use wait because this program is single thread)
		result = fullock_cond_timedwait(condname, condname, 10000);				// 10ms
		PRN("[Phase 1] - Cond timeouted wait(condname=%s, mutexname=%s): result(errno=%d, 110(ETIMEDOUT) is normal)", condname, condname, result);
		if(ETIMEDOUT != result){
			return false;
		}

		// Signal(but no waiter)
		result = fullock_cond_signal(condname);
		PRN("[Phase 2] - Send signal(condname=%s): result(errno=%d)", condname, result);
		if(0 != result){
			return false;
		}

		// Broadcast(but no waiter)
		result = fullock_cond_broadcast(condname);
		PRN("[Phase 3] - Send signal(condname=%s): result(errno=%d)", condname, result);
		if(0 != result){
			return false;
		}

		// Unlock Mutex(same name cond) for cond
		result = fullock_mutex_unlock(condname);
		PRN("[Phase 4] - Mutex Lock(%s): result(errno=%d)", condname, result);
		if(0 != result){
			return false;
		}

		PRN("[Finish] - All C I/F tests were succeed.");
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

	if(optparams.end() != optparams.find("-h") || optparams.end() != optparams.find("-help")){
		Help(argv[0]);

	}else if(optparams.end() != optparams.find("-f")){
		// reader writer lock
		bool	is_read;
		if(optparams.end() != optparams.find("-read") || optparams.end() != optparams.find("-r")){
			is_read = true;
		}else if(optparams.end() != optparams.find("-write") || optparams.end() != optparams.find("-w")){
			is_read = false;
		}else{
			ERR("-f option needs -read or -write option.");
			exit(EXIT_FAILURE);
		}

		// Init file
		if(!InitTestFile(optparams["-f"].rawstring.c_str())){
			ERR("Could not initialize test file(%s).", optparams["-f"].rawstring.c_str());
			exit(EXIT_FAILURE);
		}

		// test
		if(!SimpleReadWriteLock(optparams["-f"].rawstring.c_str(), is_read)){
			ERR("Failed to test.");
			exit(EXIT_FAILURE);
		}

	}else if(optparams.end() != optparams.find("-m") || optparams.end() != optparams.find("-mutex")){
		// mutex lock
		string	mutexname;
		if(optparams.end() != optparams.find("-m")){
			mutexname = optparams["-m"].rawstring;
		}else{
			mutexname = optparams["-mutex"].rawstring;
		}

		// test
		if(!SimpleMutexLock(mutexname.c_str())){
			ERR("Failed to test.");
			exit(EXIT_FAILURE);
		}
	}else if(optparams.end() != optparams.find("-c") || optparams.end() != optparams.find("-cond")){
		// condition
		string	condname;
		if(optparams.end() != optparams.find("-c")){
			condname = optparams["-c"].rawstring;
		}else{
			condname = optparams["-cond"].rawstring;
		}

		// test
		if(!SimpleCondLock(condname.c_str())){
			ERR("Failed to test.");
			exit(EXIT_FAILURE);
		}
	}else{
		ERR("No parameter is specified.");
		Help(argv[0]);
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
