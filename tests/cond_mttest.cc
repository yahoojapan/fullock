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
 * CREATE:   Thu 30 Jun 2015
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

//
// Thread parameter
//
typedef struct thread_param{
	pthread_t		threadid;
	bool			is_fin_init;
	bool			is_start_wait;
	int				result;
	struct timespec	start_ts;
	struct timespec	end_ts;
}THPARAM, *PTHPARAM;

//---------------------------------------------------------
// Global Variables
//---------------------------------------------------------
static bool			is_run		= false;
static bool			is_print	= false;

static const char	szCondName[]= "cond_mttest_condname";
static const char	szMutexName[]= "cond_mttest_mutexname";

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
	PRN("Usage: %s [options]", progname ? programname(progname) : "program");
	PRN("       %s -help(h)", progname ? programname(progname) : "program");
	PRN(NULL);
	PRN("options:");
	PRN("       -signal(s)                   signal type is one shot");
	PRN("       -broadcast(b)                signal type is broad cast for all waiter");
	PRN("       -waiter(w) <waiter count>    waiting thread count for cond");
	PRN("       -print(p)                    print messages by each thread.");
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
// Thread
//---------------------------------------------------------
static void* RunThread(void* param)
{
	PTHPARAM	pParam = reinterpret_cast<PTHPARAM>(param);
	if(!pParam){
		ERR("Parameter for thread is wrong.");
		pthread_exit(NULL);
	}

	FlShm			flobj;
	int				result;
	struct timespec	sleeptime	= {0L, 100};		// 100ns
	tid_t			tid			= get_threadid();
	pParam->is_fin_init			= true;

	// wait to go(global flag)
	while(!is_run){
		nanosleep(&sleeptime, NULL);
	}

	MONO_TIMESPEC(&pParam->start_ts);

	// lock mutex
	if(0 != (result = flobj.Lock(szMutexName))){
		ERR("[Thread %d] Failed to lock named mutex(%s), error=%d", tid, szMutexName, result);
		pParam->is_start_wait	= true;
		pParam->result			= result;
		MONO_TIMESPEC(&pParam->end_ts);
		pthread_exit(NULL);
	}
	pParam->is_start_wait = true;

	// waiting
	if(0 != (result = flobj.Wait(szCondName, szMutexName))){
		ISPRN("[Thread %d] Failed to wait named cond(condname=%s, mutexname=%s), error=%d", tid, szCondName, szMutexName, result);
	}else{
		ISPRN("[Thread %d] Succeed to wait named cond(condname=%s, mutexname=%s)", tid, szCondName, szMutexName);
	}
	pParam->result = result;

	// unlock mutex
	if(0 != (result = flobj.Unlock(szMutexName))){
		ERR("[Thread %d] Failed to unlock named mutex(%s), error=%d", tid, szMutexName, result);
	}
	MONO_TIMESPEC(&pParam->end_ts);

	pthread_exit(NULL);
	return NULL;
}

//---------------------------------------------------------
// Main
//---------------------------------------------------------
int main(int argc, char** argv)
{
	// Parse Parameters
	optparams_t	optparams;
	OptionParser(argc, argv, optparams);

	//--------------------------------------------------
	// Set Parameters
	//--------------------------------------------------
	struct timespec			sleeptime = {0L, 1 * 1000 * 1000};		// 1ms
	optparams_t::iterator	iter;
	bool					is_broadcast = false;
	int						waiter_count = 0;
	if(optparams.end() != (iter = optparams.find("-h")) || optparams.end() != (iter = optparams.find("-help"))){
		Help(argv[0]);
		exit(EXIT_SUCCESS);
	}

	if(optparams.end() != (iter = optparams.find("-signal")) || optparams.end() != (iter = optparams.find("-s"))){
		is_broadcast = false;
	}else if(optparams.end() != (iter = optparams.find("-broadcast")) || optparams.end() != (iter = optparams.find("-b"))){
		is_broadcast = true;
	}else{
		ERR("Parameter \"-signal(s)\" or \"-broadcast(b)\" must be specified.");
		Help(argv[0]);
		exit(EXIT_FAILURE);
	}

	if(optparams.end() != (iter = optparams.find("-waiter")) || optparams.end() != (iter = optparams.find("-w"))){
		if(!iter->second.is_number){
			ERR("-waiter(w) option needs integer value.");
			exit(EXIT_FAILURE);
		}
		waiter_count = iter->second.num_value;

		if(waiter_count <= 0){
			ERR("-waiter(w) option value must be over 0.");
			exit(EXIT_FAILURE);
		}
	}else{
		ERR("Parameter \"-waiter(w) <count>\" must be specified.");
		Help(argv[0]);
		exit(EXIT_FAILURE);
	}

	if(optparams.end() != (iter = optparams.find("-p")) || optparams.end() != (iter = optparams.find("-print"))){
		is_print = true;
	}

	//--------------------------------------------------
	// create threads
	//--------------------------------------------------
	PTHPARAM	pParams		= new THPARAM[waiter_count];
	bool		is_failed	= false;
	for(int cnt = 0; cnt < waiter_count; ++cnt){
		pParams[cnt].is_fin_init	= false;
		pParams[cnt].is_start_wait	= false;
		pParams[cnt].result			= 0;
		INIT_TIMESPEC(&pParams[cnt].start_ts);
		INIT_TIMESPEC(&pParams[cnt].end_ts);

		if(0 != pthread_create(&(pParams[cnt].threadid), NULL, RunThread, &pParams[cnt])){
			ERR("Could not create thread.");
			is_failed = true;
			break;
		}
	}
	if(is_failed){
		delete pParams;
		exit(EXIT_FAILURE);		// no care for threads exiting...
	}

	//--------------------------------------------------
	// wait for finish initializing
	//--------------------------------------------------
	for(bool is_waiting = true; is_waiting; ){
		nanosleep(&sleeptime, NULL);

		is_waiting = false;
		for(int cnt = 0; cnt < waiter_count; ++cnt){
			if(!pParams[cnt].is_fin_init){
				is_waiting = true;
				break;
			}
		}
	}

	//--------------------------------------------------
	// run all child
	//--------------------------------------------------
	// start to run(global flag)
	is_run = true;

	//--------------------------------------------------
	// wait for all children is waiting status
	//--------------------------------------------------
	for(bool is_waiting = true; is_waiting; ){
		nanosleep(&sleeptime, NULL);

		is_waiting = false;
		for(int cnt = 0; cnt < waiter_count; ++cnt){
			if(!pParams[cnt].is_start_wait){
				is_waiting = true;
				break;
			}
		}
	}
	// wait...
	nanosleep(&sleeptime, NULL);

	// set start time
	struct timespec	start_total_ts;
	struct timespec	end_total_ts;
	MONO_TIMESPEC(&start_total_ts);

	//--------------------------------------------------
	// send signal
	//--------------------------------------------------
	FlShm	flobj;
	if(is_broadcast){
		int	result;
		if(0 != (result = flobj.Broadcast(szCondName))){
			ERR("Failed to %s cond(%s). return code(error) = %d", is_broadcast ? "broadcast" : "signal", szCondName, result);
		}
	}else{
		int	result;
		for(int cnt = 0; cnt < waiter_count; ++cnt){
			if(0 != (result = flobj.Signal(szCondName))){
				ERR("Failed to %s cond(%s). return code(error) = %d", is_broadcast ? "broadcast" : "signal", szCondName, result);
			}
		}
	}

	//--------------------------------------------------
	// wait all thread exit
	//--------------------------------------------------
	int				succeed_count	= 0;
	int				failed_count	= 0;
	struct timespec	total_cumulative_ts;
	INIT_TIMESPEC(&total_cumulative_ts);
	for(int cnt = 0; cnt < waiter_count; ++cnt){
		void*			pretval = NULL;
		int				result;
		struct timespec	tmp_ts;

		if(0 != (result = pthread_join(pParams[cnt].threadid, &pretval))){
			ERR("Failed to wait thread exit. return code(error) = %d", result);
			continue;
		}
		if(0 == pParams[cnt].result){
			succeed_count++;
		}else{
			failed_count++;
		}
		ABS_TIMESPEC(&tmp_ts, &pParams[cnt].end_ts, &pParams[cnt].start_ts);
		ADD_TIMESPEC(&total_cumulative_ts, &tmp_ts);
	}
	delete pParams;

	// end time
	MONO_TIMESPEC(&end_total_ts);

	//--------------------------------------------------
	// result
	//--------------------------------------------------
	struct timespec	total_ts;
	struct timespec	avrg_total_ts;
	struct timespec	avrg_cumulative_ts;
	ABS_TIMESPEC(&total_ts, &start_total_ts, &end_total_ts);
	DIV_TIMESPEC(&avrg_total_ts, &total_ts, waiter_count);
	DIV_TIMESPEC(&avrg_cumulative_ts, &total_cumulative_ts, waiter_count);

	// display result
	fprintf(stdout, "=====================================================================\n");
	fprintf(stdout, "test type:                    %s\n", is_broadcast ? "broadcast" : "signal");
	fprintf(stdout, "waiter count:                 %d\n", waiter_count);
	fprintf(stdout, "---------------------------------------------------------------------\n");
	fprintf(stdout, "result success:               %d\n", succeed_count);
	fprintf(stdout, "result failure:               %d\n", failed_count);
	fprintf(stdout, "total time:                   %s ( %ldns )\n", STR_TIMESPEC(&total_ts).c_str(), CVT_NS_TIMESPEC(&total_ts));
	fprintf(stdout, "total cumulative time:        %s ( %ldns )\n", STR_TIMESPEC(&total_cumulative_ts).c_str(), CVT_NS_TIMESPEC(&total_cumulative_ts));
	fprintf(stdout, "average time:                 %s ( %ldns )\n", STR_TIMESPEC(&avrg_total_ts).c_str(), CVT_NS_TIMESPEC(&avrg_total_ts));
	fprintf(stdout, "average cumulative time:      %s ( %ldns )\n", STR_TIMESPEC(&avrg_cumulative_ts).c_str(), CVT_NS_TIMESPEC(&avrg_cumulative_ts));
	fprintf(stdout, "=====================================================================\n");

	exit(EXIT_SUCCESS);
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
