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
 * CREATE:   Mon 13 Feb 2016
 * REVISION:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <libgen.h>
#include <string.h>
#include "flckutil.h"

#include <string>

using namespace	std;

//---------------------------------------------------------
// Macros and Symbols
//---------------------------------------------------------
#define	DEFAULT_CHILDPROC_CNT	10

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

static inline std::string programname(const char* prgpath)
{
	string	strresult("program");
	if(!prgpath){
		return strresult;
	}
	char*	tmpname  = strdup(prgpath);
	char*	pprgname = basename(tmpname);
	if(0 == strncmp(pprgname, "lt-", strlen("lt-"))){
		pprgname = &pprgname[strlen("lt-")];
	}
	strresult = pprgname;
	FLCK_Free(tmpname);
	return strresult;
}

static void Help(const char* progname)
{
	PRN(NULL);
	PRN("Usage: %s [<libfullock path> [run child process count(default 10)]]", programname(progname).c_str());
	PRN(NULL);
}

//---------------------------------------------------------
// Main
//---------------------------------------------------------
int main(int argc, const char** argv)
{
	int		childcnt	= DEFAULT_CHILDPROC_CNT;
	string	fullockpath	= "libfullock.so";
	if(3 < argc){
		ERR("Unknown parameter is %s", argv[3]);
		Help(argv[0]);
		exit(EXIT_FAILURE);
	}else if(1 < argc){
		fullockpath = argv[1];
		if(2 < argc){
			if(0 >= (childcnt = atoi(argv[2]))){
				ERR("Parameter %s is wrong", argv[2]);
				Help(argv[0]);
				exit(EXIT_FAILURE);
			}
		}
	}

	// load library
	void*	dhandle = dlopen(fullockpath.c_str(), RTLD_LAZY);
	if(!dhandle){
		ERR("Could not load libfullock.so(path: %s)", fullockpath.c_str());
		exit(EXIT_FAILURE);
	}
	sleep(1);

	// fork
	pid_t	childpid;
	bool	parent = true;
	for(int cnt = 0; childcnt > cnt; ++cnt){
		childpid = fork();
		if(-1 == childpid){
			ERR("Could not fork no.%d child process", cnt);
			exit(EXIT_FAILURE);
		}else if(0 == childpid){
			//PRN("Succeed to wakeup no.%d child process(pid=%d)", cnt, getpid());
			parent = false;
			sleep(2);
			break;
		}else{
			//PRN("Succeed to run no.%d child process(pid=%d)", cnt, getpid());
		}
	}

	// unload library(parent/child process)
	dlclose(dhandle);

	// wait exiting children in parent process
	if(parent){
		sleep(5);		// 5s = enough for exiting children.
		int	cnt;
		int	limitcnt;
		for(cnt = childcnt, limitcnt = 10; 0 < cnt && 0 < limitcnt; ){
			int	status;
			if(0 < waitpid(-1, &status, WNOHANG)){
				--cnt;
				//PRN("Succeed to exit child process, rest child process count = %d", cnt);
			}else{
				// no children is exiting, wait 1s(total 10s).
				--limitcnt;
				sleep(1);
			}
		}
		if(0 != cnt && 0 == limitcnt){
			ERR("Some children could not exit.");
			exit(EXIT_FAILURE);
		}
	}
	exit(EXIT_SUCCESS);
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
