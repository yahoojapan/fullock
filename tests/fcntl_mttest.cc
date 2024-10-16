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
 * CREATE:   Mon 22 Jun 2015
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
#include <limits.h>
#include <errno.h>
#include <string>
#include <map>

#include "flckutil.h"
#include "fullock.h"

using namespace std;

//---------------------------------------------------------
// Symbol
//---------------------------------------------------------
#define	KEEP_US_RANDOM_VAL				(-1L)
#define	KEEP_US_RANDOM_STR				"random"

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

//
// Process parameter
//
typedef struct proc_common{
	bool			is_rwlock;
	bool			is_run;
}COMPROCPARAM, *PCOMPROCPARAM;

typedef struct proc_rwlock{
	// cppcheck-suppress unusedStructMember
	COMPROCPARAM	common;
	char			filename[PATH_MAX];
	int				loop_count;
	long			keep_us;
	long			wait_ns;
	int				reader_count;
	int				writer_count;
	bool			is_reader;
}RWLPROCPARAM, *PRWLPROCPARAM;

typedef struct proc_mutex{
	// cppcheck-suppress unusedStructMember
	COMPROCPARAM	common;
	char			mutexname[FLCK_NAMED_MUTEX_MAXLENGTH + 1];
	int				loop_count;
	long			keep_us;
	long			wait_ns;
	int				thread_count;
}MTXPROCPARAM, *PMTXPROCPARAM;

typedef union proc_thread{
	COMPROCPARAM	common;
	RWLPROCPARAM	rwl_params;
	MTXPROCPARAM	mtx_params;
}PROCPARAM, *PPROCPARAM;

//
// Thread parameter
//
typedef struct thread_param{
	pthread_t		threadid;
	bool			is_fin_init;
	bool			is_rwlock;
	bool			is_reader;
	long			keep_us;
	long			wait_ns;
	int				succeed_count;
	int				failed_count;
	struct timespec	start_ts;
	struct timespec	end_ts;
	struct timespec	sleep_ts;
}THPARAM, *PTHPARAM;

//---------------------------------------------------------
// Global Variables
//---------------------------------------------------------
static PROCPARAM	procparam;
static bool			is_print = false;

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

static inline void ISPRN(const char* format, ...)
{
	if(is_print){
		char	szbuff[4096];
		szbuff[0] = '\0';
		if(format){
			va_list ap;
			va_start(ap, format);
			vsprintf(szbuff, format, ap); 
			va_end(ap);
		}
		sprintf(&szbuff[strlen(szbuff)], "\n");
		fputs(szbuff, stdout);
		fflush(stdout);
	}
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
	PRN("       %s -help(h)", progname ? programname(progname) : "program");
	PRN(NULL);
	PRN("options:");
	PRN("       -read(r)     <thread count>    thread count for reader lock");
	PRN("       -write(w)    <thread count>    thread count for writer lock");
	PRN("       -loop(l)     <count>           loop count for each thread");
	PRN("       -keep(k)     <usec | \"random\"> keep time lock usec or random time waiting");
	PRN("       -waittime(a) <nsec>            wait time for next lock after unlocking");
	PRN("       -print(p)                      print messages by each thread.");
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
				// cppcheck-suppress knownConditionTrueFalse
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
// Utility Functions(timespec)
//---------------------------------------------------------
static inline void SET_TIMESPEC(struct timespec* ptr, time_t sec, long nsec)
{
	(ptr)->tv_sec	= sec;
	(ptr)->tv_nsec	= nsec;
}

static inline void COPY_TIMESPEC(struct timespec* dest, const struct timespec* src)
{
	SET_TIMESPEC(dest, (src)->tv_sec, (src)->tv_nsec);
}

static inline void INIT_TIMESPEC(struct timespec* ptr)
{
	SET_TIMESPEC(ptr, 0, 0L);
}

static inline int COMPARE_TIMESPEC(const struct timespec* val1, const struct timespec* val2)
{
	if((val1)->tv_sec > (val2)->tv_sec){
		return 1;
	}else if((val1)->tv_sec < (val2)->tv_sec){
		return -1;
	}else if((val1)->tv_nsec > (val2)->tv_nsec){
		return 1;
	}else if((val1)->tv_nsec < (val2)->tv_nsec){
		return -1;
	}
	return 0;
}

static inline void ADD_TIMESPEC(struct timespec* base, const struct timespec* val)
{
	(base)->tv_sec	+= (val)->tv_sec;
	(base)->tv_nsec	+= (val)->tv_nsec;
	if(1 <= (((base)->tv_nsec) / (1000 * 1000 * 1000))){
		((base)->tv_sec)  += ((base)->tv_nsec) / (1000 * 1000 * 1000);
		((base)->tv_nsec)  = ((base)->tv_nsec) % (1000 * 1000 * 1000);
	}
}

static inline void SUB_TIMESPEC(struct timespec* base, const struct timespec* val)
{
	// Do not care down flow.
	//
	if((base)->tv_nsec < (val)->tv_nsec){
		--((base)->tv_sec);
		(base)->tv_nsec += 1000 * 1000 * 1000;
	}
	(base)->tv_nsec	-= (val)->tv_nsec;
	(base)->tv_sec	-= (val)->tv_sec;
}

static inline void ABS_TIMESPEC(struct timespec* result, const struct timespec* val1, const struct timespec* val2)
{
	if(0 < COMPARE_TIMESPEC(val1, val2)){	// val1 > val2
		COPY_TIMESPEC(result, val1);
		SUB_TIMESPEC(result, val2);
	}else{	// val1 <= val2
		COPY_TIMESPEC(result, val2);
		SUB_TIMESPEC(result, val1);
	}
}

static inline long CVT_NS_TIMESPEC(const struct timespec* val)
{
	// care for over flow
	//
	return ((val)->tv_nsec + ((val)->tv_sec * 1000 * 1000 * 1000));
}

static inline void REV_NS_TIMESPEC(struct timespec* result, long val)
{
	(result)->tv_sec	= val / (1000 * 1000 * 1000);
	(result)->tv_nsec	= val % (1000 * 1000 * 1000);
}

static inline void DIV_TIMESPEC(struct timespec* result, const struct timespec* val, int count)
{
	long	ns  = CVT_NS_TIMESPEC(val) / (0 == count ? 1 : count);
	REV_NS_TIMESPEC(result, ns);
}

static inline bool MONO_TIMESPEC(struct timespec* val)
{
	if(-1 == clock_gettime(CLOCK_MONOTONIC, val)){
		INIT_TIMESPEC(val);
		return false;
	}
	return true;
}

static inline string STR_TIMESPEC(const struct timespec* val)
{
	string	result("");

	result += to_string((val)->tv_sec);
	result += "s ";
	result += to_string((val)->tv_nsec / (1000 * 1000));
	result += "ms ";
	result += to_string(((val)->tv_nsec % (1000 * 1000)) / 1000);
	result += "us ";
	result += to_string((val)->tv_nsec % 1000);
	result += "ns";

	return result;
}

//---------------------------------------------------------
// Functions
//---------------------------------------------------------
static bool MtReadWriteLock(PTHPARAM pParams)
{
	if(!pParams){
		ERR("Parameter is wrong.");
		return false;
	}

	// Initialize
	struct timespec	sleeplock	= {0L, pParams->keep_us * 1000};
	struct timespec	sleepunlock	= {0L, pParams->wait_ns};
	struct timespec	sleeptime	= {0L, 100};		// 100ns
	struct flock	f_lock;
	struct flock	f_unlock;
	{
		f_lock.l_type		= pParams->is_reader ? F_RDLCK : F_WRLCK;
		f_lock.l_whence		= SEEK_SET;
		f_lock.l_start		= 0;
		f_lock.l_len		= 1L;

		f_unlock.l_type		= F_UNLCK;
		f_unlock.l_whence	= SEEK_SET;
		f_unlock.l_start	= 0;
		f_unlock.l_len		= 1L;
	}

	tid_t	tid = get_threadid();
	int		fd;
	if(-1 == (fd = open(procparam.rwl_params.filename, O_RDWR))){
		ERR("Could not open file(%s).", procparam.rwl_params.filename);
		return false;
	}
	INIT_TIMESPEC(&pParams->sleep_ts);
	pParams->is_fin_init = true;

	// wait to go
	while(!procparam.common.is_run){
		nanosleep(&sleeptime, NULL);
	}

	MONO_TIMESPEC(&pParams->start_ts);

	// Loop
	for(int cnt = 0; cnt < procparam.rwl_params.loop_count; ++cnt){
		int	result;

		// Lock
		while(true){
			result = fcntl(fd, F_SETLK, &f_lock);
			if(-1 == result){
				//ERR("Thread(%d): Failed to %s lock by errno(%d).", tid, (pParams->is_reader ? "reader" : "writer"), errno);
				pParams->failed_count++;
			}else{
				break;
			}
		}
		ISPRN("Thread(%d): Locked %s.", tid, (pParams->is_reader ? "reader" : "writer"));

		// Sleep
		if(0 < pParams->keep_us){
			nanosleep(&sleeplock, NULL);
			ADD_TIMESPEC(&pParams->sleep_ts, &sleeplock);
		}

		// Unlock
		while(true){
			result = fcntl(fd, F_SETLK, &f_unlock);
			if(-1 == result){
				//ERR("Thread(%d): Failed to %s unlock by errno(%d).", tid, (pParams->is_reader ? "reader" : "writer"), errno);
				pParams->failed_count++;
			}else{
				break;
			}
		}
		ISPRN("Thread(%d): Unlocked %s.", tid, (pParams->is_reader ? "reader" : "writer"));

		// Sleep
		if(0 < pParams->wait_ns){
			nanosleep(&sleepunlock, NULL);
			ADD_TIMESPEC(&pParams->sleep_ts, &sleepunlock);
		}
		pParams->succeed_count++;
	}

	MONO_TIMESPEC(&pParams->end_ts);

	close(fd);
	return true;
}

static bool MtMutexLock(const PTHPARAM pParams)
{
	ERR("Do not support Mutex Lock for fcntl.");
	return false;
}

//---------------------------------------------------------
// Thread
//---------------------------------------------------------
static void* RunThread(void* param)
{
	PTHPARAM	pThParam = reinterpret_cast<PTHPARAM>(param);
	if(!pThParam){
		ERR("Parameter for thread is wrong.");
		pthread_exit(NULL);
	}

	tid_t	tid = get_threadid();
	bool	result;
	if(pThParam->is_rwlock){
		result = MtReadWriteLock(pThParam);
	}else{
		result = MtMutexLock(pThParam);
	}
	if(!result){
		ERR("thread(%d) exited with error.", tid);
	}

	pthread_exit(NULL);
	return NULL;
}

//---------------------------------------------------------
// Main
//---------------------------------------------------------
int main(int argc, char** argv)
{
	// initialize global
	memset(&procparam, 0, sizeof(PROCPARAM));

	// Parse Parameters
	optparams_t	optparams;
	OptionParser(argc, argv, optparams);

	// Set Parameters
	optparams_t::iterator	iter;
	int						thcount = 0;
	if(optparams.end() != (iter = optparams.find("-p")) || optparams.end() != (iter = optparams.find("-print"))){
		is_print = true;
	}
	if(optparams.end() != (iter = optparams.find("-h")) || optparams.end() != (iter = optparams.find("-help"))){
		Help(argv[0]);
		exit(EXIT_SUCCESS);

	}else if(optparams.end() != (iter = optparams.find("-f"))){
		// RWLOCK TYPE
		//
		procparam.common.is_run			= false;
		procparam.common.is_rwlock		= true;
		procparam.rwl_params.is_reader	= true;				// temporary
		procparam.rwl_params.keep_us	= 0;				// 0us
		procparam.rwl_params.wait_ns	= 0;				// 0ns
		strncpy(procparam.rwl_params.filename, iter->second.rawstring.c_str(), (PATH_MAX - 1));

		if(optparams.end() != (iter = optparams.find("-read")) || optparams.end() != (iter = optparams.find("-r"))){
			if(!iter->second.is_number){
				ERR("-read/-r option needs integer value.");
				exit(EXIT_FAILURE);
			}
			procparam.rwl_params.reader_count = iter->second.num_value;
		}
		if(optparams.end() != (iter = optparams.find("-write")) || optparams.end() != (iter = optparams.find("-w"))){
			if(!iter->second.is_number){
				ERR("-write/-w option needs integer value.");
				exit(EXIT_FAILURE);
			}
			procparam.rwl_params.writer_count = iter->second.num_value;
		}
		if(optparams.end() != (iter = optparams.find("-loop")) || optparams.end() != (iter = optparams.find("-l"))){
			if(!iter->second.is_number){
				ERR("-loop/-l option needs integer value.");
				exit(EXIT_FAILURE);
			}
			procparam.rwl_params.loop_count = iter->second.num_value;
		}
		if(optparams.end() != (iter = optparams.find("-keep")) || optparams.end() != (iter = optparams.find("-k"))){
			if(!iter->second.is_number && 0 == strcasecmp(iter->second.rawstring.c_str(), KEEP_US_RANDOM_STR)){
				procparam.rwl_params.keep_us = KEEP_US_RANDOM_VAL;
			}else if(iter->second.is_number){
				procparam.rwl_params.keep_us = iter->second.num_value;
			}else{
				ERR("-keep/-k option needs integer value.");
				exit(EXIT_FAILURE);
			}
		}
		if(optparams.end() != (iter = optparams.find("-waittime")) || optparams.end() != (iter = optparams.find("-a"))){
			if(iter->second.is_number){
				procparam.rwl_params.wait_ns = iter->second.num_value;
			}else{
				ERR("-waittime/-a option needs integer value.");
				exit(EXIT_FAILURE);
			}
		}
		thcount = procparam.rwl_params.reader_count + procparam.rwl_params.writer_count;

		// Init file
		if(!InitTestFile(procparam.rwl_params.filename)){
			ERR("Could not initialize test file(%s).", procparam.rwl_params.filename);
			exit(EXIT_FAILURE);
		}

	}else if(optparams.end() != (iter = optparams.find("-m")) || optparams.end() != (iter = optparams.find("-mutex"))){
		// MUTEX TYPE
		//
		procparam.common.is_run			= false;
		procparam.common.is_rwlock		= false;
		procparam.mtx_params.keep_us	= 0;				// 0us
		procparam.mtx_params.wait_ns	= 0;				// 0ns
		strncpy(procparam.mtx_params.mutexname, iter->second.rawstring.c_str(), FLCK_NAMED_MUTEX_MAXLENGTH);

		if(optparams.end() != (iter = optparams.find("-thread")) || optparams.end() != (iter = optparams.find("-t"))){
			if(!iter->second.is_number){
				ERR("-thread/-t option needs integer value.");
				exit(EXIT_FAILURE);
			}
			procparam.mtx_params.thread_count = iter->second.num_value;
		}
		if(optparams.end() != (iter = optparams.find("-loop")) || optparams.end() != (iter = optparams.find("-l"))){
			if(!iter->second.is_number){
				ERR("-loop/-l option needs integer value.");
				exit(EXIT_FAILURE);
			}
			procparam.mtx_params.loop_count = iter->second.num_value;
		}
		if(optparams.end() != (iter = optparams.find("-keep")) || optparams.end() != (iter = optparams.find("-k"))){
			if(!iter->second.is_number && 0 == strcasecmp(iter->second.rawstring.c_str(), KEEP_US_RANDOM_STR)){
				procparam.mtx_params.keep_us = KEEP_US_RANDOM_VAL;
			}else if(iter->second.is_number){
				procparam.mtx_params.keep_us = iter->second.num_value;
			}else{
				ERR("-keep/-k option needs integer value.");
				exit(EXIT_FAILURE);
			}
		}
		if(optparams.end() != (iter = optparams.find("-waittime")) || optparams.end() != (iter = optparams.find("-a"))){
			if(iter->second.is_number){
				procparam.mtx_params.wait_ns = iter->second.num_value;
			}else{
				ERR("-waittime/-a option needs integer value.");
				exit(EXIT_FAILURE);
			}
		}
		thcount = procparam.mtx_params.thread_count;

	}else{
		ERR("No parameter is specified.");
		Help(argv[0]);
		exit(EXIT_FAILURE);
	}

	// create threads
	PTHPARAM	pParams		= new THPARAM[thcount];
	bool		is_failed	= false;
	int			reader_cnt	= 0;

	for(int cnt = 0; cnt < thcount; ++cnt){
		pParams[cnt].is_fin_init		= false;
		pParams[cnt].is_rwlock			= procparam.common.is_rwlock;
		pParams[cnt].succeed_count		= 0;
		pParams[cnt].failed_count		= 0;
		INIT_TIMESPEC(&pParams[cnt].start_ts);
		INIT_TIMESPEC(&pParams[cnt].end_ts);
		INIT_TIMESPEC(&pParams[cnt].sleep_ts);

		if(procparam.common.is_rwlock){
			if(reader_cnt < procparam.rwl_params.reader_count){
				pParams[cnt].is_reader = true;
				reader_cnt++;
			}else{
				pParams[cnt].is_reader = false;
			}
			if(KEEP_US_RANDOM_VAL == procparam.rwl_params.keep_us){
				pParams[cnt].keep_us = (random() % (10 * 1000));		// 0 - 10ms
			}else{
				pParams[cnt].keep_us = procparam.rwl_params.keep_us;
			}
			pParams[cnt].wait_ns = procparam.rwl_params.wait_ns;
		}else{
			pParams[cnt].is_reader = false;

			if(KEEP_US_RANDOM_VAL == procparam.mtx_params.keep_us){
				pParams[cnt].keep_us = (random() % (10 * 1000));		// 0 - 10ms
			}else{
				pParams[cnt].keep_us = procparam.mtx_params.keep_us;
			}
			pParams[cnt].wait_ns = procparam.mtx_params.wait_ns;
		}
		if(0 != pthread_create(&(pParams[cnt].threadid), NULL, RunThread, &pParams[cnt])){
			ERR("Could not create thread.");
			is_failed = true;
			break;
		}
	}
	if(is_failed){
		delete[] pParams;
		exit(EXIT_FAILURE);		// no care for threads exiting...
	}

	// wait for finish initializing
	for(bool is_waiting = true; is_waiting; ){
		struct timespec	sleeptime = {0L, 1 * 1000 * 1000};		// 1ms
		nanosleep(&sleeptime, NULL);

		is_waiting = false;
		for(int cnt = 0; cnt < thcount; ++cnt){
			if(!pParams[cnt].is_fin_init){
				is_waiting = true;
				break;
			}
		}
	}

	// start time
	struct timespec	start_total_ts;
	struct timespec	end_total_ts;
	MONO_TIMESPEC(&start_total_ts);

	// start to run
	procparam.common.is_run = true;

	// wait all thread exit
	int				succeed_count	= 0;
	int				failed_count	= 0;
	struct timespec	total_run_ts;
	struct timespec	total_sleep_ts;
	INIT_TIMESPEC(&total_run_ts);
	INIT_TIMESPEC(&total_sleep_ts);

	for(int cnt = 0; cnt < thcount; ++cnt){
		void*			pretval = NULL;
		int				result;
		struct timespec	tmp_ts;

		if(0 != (result = pthread_join(pParams[cnt].threadid, &pretval))){
			ERR("Failed to wait thread exit. return code(error) = %d", result);
			continue;
		}

		// result
		succeed_count	+= pParams[cnt].succeed_count;
		failed_count	+= pParams[cnt].failed_count;

		ABS_TIMESPEC(&tmp_ts, &pParams[cnt].end_ts, &pParams[cnt].start_ts);
		ADD_TIMESPEC(&total_run_ts, &tmp_ts);

		ADD_TIMESPEC(&total_sleep_ts, &pParams[cnt].sleep_ts);
	}
	delete[] pParams;

	// end time
	MONO_TIMESPEC(&end_total_ts);

	struct timespec	total_ts;
	struct timespec	total_real_ts;
	struct timespec	avrg_run_ts;
	struct timespec	avrg_real_ts;
	struct timespec	avrg_total_ts;
	ABS_TIMESPEC(&total_ts, &start_total_ts, &end_total_ts);
	DIV_TIMESPEC(&avrg_total_ts, &total_ts, succeed_count);
	ABS_TIMESPEC(&total_real_ts, &total_run_ts, &total_sleep_ts);
	DIV_TIMESPEC(&avrg_run_ts, &total_run_ts, succeed_count);
	DIV_TIMESPEC(&avrg_real_ts, &total_real_ts, succeed_count);

	// display result
	fprintf(stdout, "=====================================================================\n");
	fprintf(stdout, "test type:                    %s\n", procparam.common.is_rwlock ? "rwlock" : "unsupport mutex");
	if(procparam.common.is_rwlock){
		fprintf(stdout, "fcntl reader thread count:    %d\n", procparam.rwl_params.reader_count);
		fprintf(stdout, "fcntl writer thread count:    %d\n", procparam.rwl_params.writer_count);
		fprintf(stdout, "loop count for each thread:   %d\n", procparam.rwl_params.loop_count);
		fprintf(stdout, "total count:                  %d ( = (%d + %d) * %d)\n", (procparam.rwl_params.reader_count + procparam.rwl_params.writer_count) * procparam.rwl_params.loop_count, procparam.rwl_params.reader_count, procparam.rwl_params.writer_count, procparam.rwl_params.loop_count);
	}else{
		// Do not support mutex mode.
	}
	fprintf(stdout, "---------------------------------------------------------------------\n");
	fprintf(stdout, "result success:               %d\n", succeed_count);
	fprintf(stdout, "result failure:               %d\n", failed_count);
	fprintf(stdout, "total time:                   %s ( %ldns )\n", STR_TIMESPEC(&total_ts).c_str(), CVT_NS_TIMESPEC(&total_ts));
	fprintf(stdout, "total cumulative  time:       %s ( %ldns )\n", STR_TIMESPEC(&total_run_ts).c_str(), CVT_NS_TIMESPEC(&total_run_ts));
	fprintf(stdout, "total cumulative sleep time:  %s ( %ldns )\n", STR_TIMESPEC(&total_sleep_ts).c_str(), CVT_NS_TIMESPEC(&total_sleep_ts));
	fprintf(stdout, "total cumulative real time:   %s ( %ldns )\n", STR_TIMESPEC(&total_real_ts).c_str(), CVT_NS_TIMESPEC(&total_real_ts));
	fprintf(stdout, "average time:                 %s ( %ldns )\n", STR_TIMESPEC(&avrg_total_ts).c_str(), CVT_NS_TIMESPEC(&avrg_total_ts));
	fprintf(stdout, "average cumulative time:      %s ( %ldns )\n", STR_TIMESPEC(&avrg_run_ts).c_str(), CVT_NS_TIMESPEC(&avrg_run_ts));
	fprintf(stdout, "average cumulative real time: %s ( %ldns )\n", STR_TIMESPEC(&avrg_real_ts).c_str(), CVT_NS_TIMESPEC(&avrg_real_ts));
	fprintf(stdout, "=====================================================================\n");

	exit(EXIT_SUCCESS);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
