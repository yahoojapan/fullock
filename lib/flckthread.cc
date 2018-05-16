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
 * CREATE:   Fri 29 May 2015
 * REVISION:
 *
 */

#include <pthread.h>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <assert.h>

#include "flckcommon.h"
#include "flckshm.h"
#include "flckstructure.h"
#include "flckutil.h"
#include "flckdbg.h"

using namespace std;

//---------------------------------------------------------
// Structure
//---------------------------------------------------------
typedef struct flck_th_param{
	volatile FlckThread::THCNTLFLAG*	pThFlag;
	int									intervalms;				// for epoll_wait
	char*								pfilepath;
}FLCKTHPARAM, *PFLCKTHPARAM;

//---------------------------------------------------------
// Utility for thread control flag
//---------------------------------------------------------
inline void set_val_thread_cntrl_flag(volatile FlckThread::THCNTLFLAG* pflag, FlckThread::THCNTLFLAG oldval, FlckThread::THCNTLFLAG newval)
{
	// oldval is used only first.
	FlckThread::THCNTLFLAG	resval;
	while(oldval != (resval = __sync_val_compare_and_swap(pflag, oldval, newval))){
		if(resval == newval){
			break;
		}
		oldval = resval;
	}
}

inline void set_stop_thread_cntrl_flag(volatile FlckThread::THCNTLFLAG* pflag)
{
	set_val_thread_cntrl_flag(pflag, FlckThread::FLCK_THCNTL_RUN, FlckThread::FLCK_THCNTL_STOP);
}

inline void set_run_thread_cntrl_flag(volatile FlckThread::THCNTLFLAG* pflag)
{
	set_val_thread_cntrl_flag(pflag, FlckThread::FLCK_THCNTL_STOP, FlckThread::FLCK_THCNTL_RUN);
}

inline void set_fin_thread_cntrl_flag(volatile FlckThread::THCNTLFLAG* pflag)
{
	set_val_thread_cntrl_flag(pflag, FlckThread::FLCK_THCNTL_EXIT, FlckThread::FLCK_THCNTL_FIN);
}

inline void set_exit_thread_cntrl_flag(volatile FlckThread::THCNTLFLAG* pflag)
{
	FlckThread::THCNTLFLAG	oldval = FlckThread::FLCK_THCNTL_RUN;	// temporary
	FlckThread::THCNTLFLAG	newval = FlckThread::FLCK_THCNTL_EXIT;
	FlckThread::THCNTLFLAG	resval;
	while(oldval != (resval = __sync_val_compare_and_swap(pflag, oldval, newval))){
		if(FlckThread::FLCK_THCNTL_EXIT <= resval){
			// already set exit or fin
			break;
		}
		oldval = resval;
	}
}

//---------------------------------------------------------
// Class variable
//---------------------------------------------------------
const int	FlckThread::DEFAULT_INTERVALMS;
const int	FlckThread::FLCK_WAIT_EVENT_MAX;
int			FlckThread::InotifyFd			= FLCK_INVALID_HANDLE;
int			FlckThread::WatchFd				= FLCK_INVALID_HANDLE;
int			FlckThread::EventFd				= FLCK_INVALID_HANDLE;
void*		FlckThread::pThreadParam		= NULL;

//---------------------------------------------------------
// Class Method : Thread Cancel Handler
//---------------------------------------------------------
void FlckThread::CleanupHandler(void* arg)
{
	PFLCKTHPARAM			pparam = reinterpret_cast<PFLCKTHPARAM>(arg);
	volatile THCNTLFLAG*	pThFlag= (pparam ? pparam->pThFlag : NULL);

	MSG_FLCKPRN("Thread canceled and call handler with pflag=%p, watchid=%d, inotifyfd=%d, eventfd=%d", pThFlag, FlckThread::WatchFd, FlckThread::InotifyFd, FlckThread::EventFd);

	// free allocated data
	FlckThread::pThreadParam = NULL;		// pparam == FlckThread::pThreadParam
	FLCK_Free(pparam->pfilepath);
	FLCK_Delete(pparam);

	// close handles
	if(FLCK_INVALID_HANDLE != FlckThread::WatchFd){
		inotify_rm_watch(FlckThread::InotifyFd, FlckThread::WatchFd);
	}
	FLCK_CLOSE(FlckThread::InotifyFd);
	FLCK_CLOSE(FlckThread::EventFd);

	// set finish flag
	if(pThFlag){
		set_fin_thread_cntrl_flag(pThFlag);
	}
}

//---------------------------------------------------------
// Class Method : Worker proc
//---------------------------------------------------------
// The communication between worker thread and main thread uses
// thflag flag. FlckThread does not use mutex for exclusion
// control.
//
// [NOTE]
// This fullock library starts the following worker threads when loading
// the library, and ends when the library is unloaded.
// 
// All worker thread is terminated by calling pthrad_cancel function.
// As a result, when the library is unloaded with dlclose and when it
// terminates normally, the worker thread can be terminated by same way.
// The handle and memory used by the worker thread are post-processed by
// CleanupHandler method.
// 
// Take care for that this library uses the nonportable function as
// pthread_tryjoin_np to wait for the worker thread to terminate in the
// main thread.
// In the main thread, not only pthrad_exit is used to terminate a thread,
// but also a control flag is detected.
// 
// If the process loading the library is forked, this library automatically
// starts the worker thread immediately after starting the child process.
// 
void* FlckThread::WorkerProc(void* param)
{
	//
	// Initialize variables and cleanup handler
	//
	int	old_cancel_state = PTHREAD_CANCEL_ENABLE;
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old_cancel_state);		// blocking cancel in initializing

	// initialize variables
	FlckThread::InotifyFd	= FLCK_INVALID_HANDLE;
	FlckThread::WatchFd		= FLCK_INVALID_HANDLE;
	FlckThread::EventFd		= FLCK_INVALID_HANDLE;

	// get parameter
	PFLCKTHPARAM	pparam = reinterpret_cast<PFLCKTHPARAM>(param);			// param is freed in cancel handler
	if(!pparam){
		ERR_FLCKPRN("Parameter is wrong.");
		pthread_setcancelstate(old_cancel_state, NULL);						// rollback cancel state
		pthread_testcancel();												// check cancel
		pthread_exit(NULL);
	}
	FlckThread::pThreadParam			= param;							// for forking
	volatile THCNTLFLAG*	pThFlag		= pparam->pThFlag;
	int						intervalms	= pparam->intervalms;
	char*					pfilepath	= pparam->pfilepath;

	//
	// set cleanup handler
	// Take care for pthread_cleanup_push, it is macro which has a part of "do{ }while()".(see: pthread.h)
	//
	pthread_cleanup_push(FlckThread::CleanupHandler, pparam);
	pthread_setcancelstate(old_cancel_state, NULL);							// set allowing cancel
	pthread_testcancel();													// check cancel

	// create event fd
	if(FLCK_INVALID_HANDLE == (FlckThread::EventFd = epoll_create1(EPOLL_CLOEXEC))){
		ERR_FLCKPRN("Failed to create epoll, error %d", errno);
		pthread_testcancel();
		pthread_exit(NULL);
	}
	// create inotify
	if(FLCK_INVALID_HANDLE == (FlckThread::InotifyFd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC))){
		ERR_FLCKPRN("Failed to create inotify, error %d", errno);
		pthread_testcancel();												// check cancel
		pthread_exit(NULL);
	}
	// add file to inotify
	if(FLCK_INVALID_HANDLE == (FlckThread::WatchFd = inotify_add_watch(FlckThread::InotifyFd, pfilepath, IN_CLOSE))){
		ERR_FLCKPRN("Could not add to watch file %s (errno=%d)", pfilepath, errno);
		pthread_testcancel();												// check cancel
		pthread_exit(NULL);
	}

	// add event
	struct epoll_event	epoolev;
	memset(&epoolev, 0, sizeof(struct epoll_event));
	epoolev.data.fd		= FlckThread::InotifyFd;
	epoolev.events		= EPOLLIN | EPOLLET;
	if(-1 == epoll_ctl(FlckThread::EventFd, EPOLL_CTL_ADD, FlckThread::InotifyFd, &epoolev)){
		ERR_FLCKPRN("Failed to add inotifyfd(%d)-watchfd(%d) to event fd(%d), error=%d", FlckThread::InotifyFd, FlckThread::WatchFd, FlckThread::EventFd, errno);
		pthread_testcancel();												// check cancel
		pthread_exit(NULL);
	}
	pthread_testcancel();													// check cancel

	// do loop
	struct timespec		sleepms = {(intervalms / 1000), (intervalms % 1000) * 1000 * 1000};
	struct epoll_event  events[FLCK_WAIT_EVENT_MAX];
	int					eventcnt;
	while(FlckThread::FLCK_THCNTL_EXIT > *pThFlag){
		pthread_testcancel();												// check cancel

		if(FlckThread::FLCK_THCNTL_STOP == *pThFlag){
			// stop(sleep)
			nanosleep(&sleepms, NULL);

		}else if(FlckThread::FLCK_THCNTL_RUN == *pThFlag){
			// wait event
			if(0 < (eventcnt = epoll_pwait(FlckThread::EventFd, events, FLCK_WAIT_EVENT_MAX, intervalms, NULL))){
				// catch event
				for(int cnt = 0; cnt < eventcnt; cnt++){
					pthread_testcancel();									// check cancel
					// check flag
					if(FlckThread::FLCK_THCNTL_EXIT <= *pThFlag){
						break;
					}
					if(events[cnt].data.fd != FlckThread::InotifyFd){
						WAN_FLCKPRN("Why event fd(%d) is not same inotify fd(%d), but continue...", events[cnt].data.fd, FlckThread::InotifyFd);
						continue;
					}
					// check event
					if(FlckThread::CheckEvent(FlckThread::InotifyFd, FlckThread::WatchFd)){
						// CLOSE event is occurred.
						FlShm	LocalFlShm;
						if(!LocalFlShm.CheckProcessDead()){
							WAN_FLCKPRN("Failed to check process dead in FlShm object, but continue...");
						}
					}
				}

			}else if(-1 >= eventcnt){
				if(EINTR != errno){
					ERR_FLCKPRN("Something error occured in waiting event(errno=%d): inotifyfd(%d) watchfd(%d) event fd(%d)", errno, FlckThread::InotifyFd, FlckThread::WatchFd, FlckThread::EventFd);
					break;
				}
				// signal occurred.

			}else{	// 0 == eventcnt
				// timeouted, nothing to do
			}
		}
	}
	pthread_testcancel();													// check cancel
	pthread_cleanup_pop(1);													// pop and execute cleanup handler
	pthread_exit(NULL);
}

//
// If "CLOSE" event occurred, return true.
//
bool FlckThread::CheckEvent(int InotifyFd, int WatchFd)
{
	if(FLCK_INVALID_HANDLE == InotifyFd || FLCK_INVALID_HANDLE == WatchFd){
		ERR_FLCKPRN("Parameters are wrong.");
		return false;
	}

	// read inotify event
	unsigned char*	pevent = NULL;
	ssize_t			bytes;
	if(-1 == (bytes = flck_read(InotifyFd, &pevent))){
		MSG_FLCKPRN("read no inotify event, no more inotify event data.");
		return false;
	}

	// do for type
	struct inotify_event*	in_event= NULL;
	bool					retval	= false;
	for(unsigned char* ptr = pevent; (ptr + sizeof(struct inotify_event)) <= (pevent + bytes); ptr += sizeof(struct inotify_event) + in_event->len){
		in_event = reinterpret_cast<struct inotify_event*>(ptr);

		if(WatchFd != in_event->wd){
			continue;
		}
		if(in_event->mask & IN_CLOSE || in_event->mask & IN_CLOSE_NOWRITE || in_event->mask & IN_CLOSE_WRITE){
			// found close event
			MSG_FLCKPRN("Get close event(%d)", in_event->mask);
			retval = true;
		}
	}
	FLCK_Free(pevent);

	return retval;
}

//---------------------------------------------------------
// Methods
//---------------------------------------------------------
FlckThread::FlckThread() : thflag(FlckThread::FLCK_THCNTL_STOP), bup_intervalms(FlckThread::DEFAULT_INTERVALMS), bup_filepath(""), is_run_worker(false)
{
}

FlckThread::~FlckThread()
{
	if(IsInitWorker()){
		Exit();
	}
}

bool FlckThread::InitializeThread(const char* pfile, int intervalms)
{
	if(!pfile){
		ERR_FLCKPRN("Parameter is wrong.");
		return false;
	}
	if(IsInitWorker()){
		ERR_FLCKPRN("Already run worker thread, must stop worker.");
		return false;
	}
	// backup for forking
	bup_intervalms			= intervalms;
	bup_filepath			= pfile;

	// init param
	PFLCKTHPARAM	pparam	= new FLCKTHPARAM;
	pparam->pThFlag			= &thflag;
	pparam->intervalms		= intervalms;
	pparam->pfilepath		= strdup(pfile);

	// create thread
	int	result;
	if(0 != (result = pthread_create(&pthreadid, NULL, FlckThread::WorkerProc, pparam))){
		ERR_FLCKPRN("Failed to create thread. return code(error) = %d", result);
		FLCK_Free(pparam->pfilepath);
		FLCK_Delete(pparam);
		return false;
	}
	is_run_worker = true;

	return true;
}

// [NOTE]
// This method is called only pthread_prefork handler.
// This method re-initialized variables in this class, and starts thread.
//
bool FlckThread::ReInitializeThread(void)
{
	if(bup_filepath.empty()){
		ERR_FLCKPRN("Backup file path is empty.");
		return false;
	}
	MSG_FLCKPRN("Run worker thread by forking.");

	// clean old thread's parameter data
	PFLCKTHPARAM	oldparam = reinterpret_cast<PFLCKTHPARAM>(FlckThread::pThreadParam);
	if(oldparam){
		FLCK_Free(oldparam->pfilepath);
		FLCK_Delete(oldparam);
		FlckThread::pThreadParam = NULL;
	}

	// initialize inner data
	string	tmppath	= bup_filepath;
	thflag			= FlckThread::FLCK_THCNTL_STOP;
	is_run_worker	= false;

	// start thread
	return InitializeThread(tmppath.c_str(), bup_intervalms);
}

bool FlckThread::Run(void)
{
	if(!IsInitWorker()){
		ERR_FLCKPRN("No worker thread is initialized, must initialize worker.");
		return false;
	}

	if(FlckThread::FLCK_THCNTL_STOP != thflag){
		ERR_FLCKPRN("No worker thread is stopping(already run or exit).");
		return false;
	}
	// set flag run to first.
	set_run_thread_cntrl_flag(&thflag);

	return true;
}

bool FlckThread::Stop(void)
{
	if(!IsInitWorker()){
		ERR_FLCKPRN("No worker thread is initialized, must initialize worker.");
		return false;
	}

	if(FlckThread::FLCK_THCNTL_RUN != thflag){
		ERR_FLCKPRN("No worker thread is running(already stop or exit).");
		return false;
	}
	// set flag stop to first.
	set_stop_thread_cntrl_flag(&thflag);

	return true;
}

bool FlckThread::Exit(void)
{
	if(!IsInitWorker()){
		ERR_FLCKPRN("No worker thread is initialized, must initialize worker.");
		return false;
	}
	if(FlckThread::FLCK_THCNTL_EXIT <= thflag){
		MSG_FLCKPRN("Already worker thread exit.");
		return true;
	}

	// [NOTE]
	// At first, send cancel request, next set flag.
	// Because worker thread blocks cancel event in its loop, and make cancel point after breaking loop.
	//
	int	result;
	if(0 != (result = pthread_cancel(pthreadid))){
		ERR_FLCKPRN("Could not cancel thread by errno(%d)", result);
		return false;
	}
	// set flag for exiting
	set_exit_thread_cntrl_flag(&thflag);

	// loop for joining
	struct timespec	sleeptime	= {0, 1000 * 1000};		// = 1ms
	void*			pretval		= NULL;
	for(result = EBUSY; 0 != result; ){
		// [NOTE]
		// Using "nonportable" pthread_tryjoin_np function to avoid blocking by pthread_join.
		//
		if(0 != (result = pthread_tryjoin_np(pthreadid, &pretval))){
			if(EBUSY == result){
				//MSG_FLCKPRN("Worker thread has not exited yet. join returns code(EBUSY: %d)", result);

				if(FlckThread::FLCK_THCNTL_FIN == thflag){
					// Already started exiting worker thread, but it did not finish, or deadlocked.
					// Then we give up waiting(joining) after try to join one more after sleep.
					//
					sleeptime.tv_nsec = 20 * 1000 * 1000;	// 20ms
					nanosleep(&sleeptime, NULL);
					// last retry
					if(0 != (result = pthread_tryjoin_np(pthreadid, &pretval))){
						ERR_FLCKPRN("Failed re-waiting join thread by %s(%d), so give up wor waiting join", (EBUSY == result ? "EBUSY" : EDEADLK == result ? "EDEADLK" : EINVAL == result ? "EINVAL" : EINVAL == result ? "EINVAL" : ESRCH  == result ? "ESRCH" : "unknown"), result);
					}else{
						ERR_FLCKPRN("Thread has exited but probabry pthread_exit is not locked.");
					}
					break;
				}
			}else{
				// something error is occurred, so we can not wait to join any more.
				ERR_FLCKPRN("Failed waiting join worker thread by error(%s: %d), thus break waiting.", (EDEADLK == result ? "EDEADLK" : EINVAL == result ? "EINVAL" : EINVAL == result ? "EINVAL" : ESRCH  == result ? "ESRCH" : "unknown"), result);
				break;
			}
			nanosleep(&sleeptime, NULL);
		}
	}
	MSG_FLCKPRN("Succeed to wait exiting thread. return value ptr=%p(expect PTHREAD_CANCELED=-1), join result=%d", pretval, result);

	is_run_worker = false;

	return true;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
