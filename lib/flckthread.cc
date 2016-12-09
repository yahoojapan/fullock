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
 * CREATE:   Fri 29 May 2015
 * REVISION:
 *
 */

#include <pthread.h>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <string.h>
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
// Class variable
//---------------------------------------------------------
const int	FlckThread::DEFAULT_INTERVALMS;
const int	FlckThread::FLCK_WAIT_EVENT_MAX;

//---------------------------------------------------------
// Class Method : Worker proc
//---------------------------------------------------------
// The communication between worker thread and main thread uses
// thflag flag. FlckThread does not use mutex for exclusion
// control.
//
void* FlckThread::WorkerProc(void* param)
{
	PFLCKTHPARAM	pparam = reinterpret_cast<PFLCKTHPARAM>(param);
	if(!pparam){
		ERR_FLCKPRN("Parameter is wrong.");
		pthread_exit(NULL);
	}

	// copy & free
	volatile THCNTLFLAG*	pThFlag		= pparam->pThFlag;
	int						intervalms	= pparam->intervalms;
	char*					pfilepath	= pparam->pfilepath;
	FLCK_Delete(pparam);

	// fds
	int	InotifyFd;
	int	WatchFd;
	int	EventFd;

	// create event fd
	if(-1 == (EventFd = epoll_create1(EPOLL_CLOEXEC))){
		ERR_FLCKPRN("Failed to create epoll, error %d", errno);
		pthread_exit(NULL);
	}
	// create inotify
	if(-1 == (InotifyFd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC))){
		ERR_FLCKPRN("Failed to create inotify, error %d", errno);
		FLCK_CLOSE(EventFd);
		pthread_exit(NULL);
	}
	// add file to inotify
	if(FLCK_INVALID_HANDLE == (WatchFd = inotify_add_watch(InotifyFd, pfilepath, IN_CLOSE))){
		ERR_FLCKPRN("Could not add to watch file %s (errno=%d)", pfilepath, errno);
		FLCK_CLOSE(InotifyFd);
		FLCK_CLOSE(EventFd);
		FLCK_Free(pfilepath);
		pthread_exit(NULL);
	}
	FLCK_Free(pfilepath);

	// add event
	struct epoll_event	epoolev;
	memset(&epoolev, 0, sizeof(struct epoll_event));
	epoolev.data.fd	= InotifyFd;
	epoolev.events	= EPOLLIN | EPOLLET;

	if(-1 == epoll_ctl(EventFd, EPOLL_CTL_ADD, InotifyFd, &epoolev)){
		ERR_FLCKPRN("Failed to add inotifyfd(%d)-watchfd(%d) to event fd(%d), error=%d", InotifyFd, WatchFd, EventFd, errno);
		inotify_rm_watch(InotifyFd, WatchFd);
		FLCK_CLOSE(InotifyFd);
		FLCK_CLOSE(EventFd);
		pthread_exit(NULL);
	}

	// do loop
	struct timespec		sleepms = {(intervalms / 1000), (intervalms % 1000) * 1000 * 1000};
	struct epoll_event  events[FLCK_WAIT_EVENT_MAX];
	int					eventcnt;

	while(FlckThread::FLCK_THCNTL_EXIT != *pThFlag){
		if(FlckThread::FLCK_THCNTL_STOP == *pThFlag){
			// stop(sleep)
			nanosleep(&sleepms, NULL);
			continue;
		}

		if(FlckThread::FLCK_THCNTL_RUN == *pThFlag){
			// wait event
			if(0 < (eventcnt = epoll_pwait(EventFd, events, FLCK_WAIT_EVENT_MAX, intervalms, NULL))){
				// catch event
				for(int cnt = 0; cnt < eventcnt; cnt++){
					if(events[cnt].data.fd != InotifyFd){
						WAN_FLCKPRN("Why event fd(%d) is not same inotify fd(%d), but continue...", events[cnt].data.fd, InotifyFd);
						continue;
					}
					// check event
					if(FlckThread::CheckEvent(InotifyFd, WatchFd)){
						// CLOSE event is occurred.
						FlShm	LocalFlShm;
						if(!LocalFlShm.CheckProcessDead()){
							WAN_FLCKPRN("Failed to check process dead in FlShm object, but continue...");
						}
					}
				}

			}else if(-1 >= eventcnt){
				if(EINTR != errno){
					ERR_FLCKPRN("Something error occured in waiting event(errno=%d): inotifyfd(%d) watchfd(%d) event fd(%d)", errno, InotifyFd, WatchFd, EventFd);
					break;
				}
				// signal occurred.

			}else{	// 0 == eventcnt
				// timeouted, nothing to do
			}
		}
	}

	// close all fds
	inotify_rm_watch(InotifyFd, WatchFd);
	FLCK_CLOSE(InotifyFd);
	FLCK_CLOSE(EventFd);

	// [NOTE]
	// Do not call pthread_exit because this thread is dead locked in pthread_exit
	// when dlclose is called immediately after calling dlopen.
	// One of this case, user makes fullock linked apache loadable module which is
	// loaded by LoadModule in configuration. Apache loads that dso module and unloads
	// it, reloads it at apache startup. Then apache internal processing calls dlopen,
	// dlclose, and (re)dlopen.
	// In these series of processes, the destructor of fullock is called by dlclose.
	// The destructor will stop this thread, then this thread will call pthread_exit
	// for exiting. Exactly when that time, pthread_exit is deadlock!
	// The stack is pointed following: 
	//   pthread_exit -> pthread_cancel_init -> do_dlopen -> pthread_mutex_lock -> deadlock
	// 
	// So we do not call pthread_exit here.
	// But we will not need to cogitate, when the thread is exited without calling
	// pthread_exit, it is equivalent to calling pthread_exit with the value supplied
	// in the return statement.(see: man pthrad_create)
	// 
	//pthread_exit(NULL);

	return NULL;
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
FlckThread::FlckThread() : thflag(FlckThread::FLCK_THCNTL_STOP), is_run_worker(false)
{
}

FlckThread::~FlckThread()
{
	if(IsInitWorker()){
		Exit();
	}
}

bool FlckThread::InitializeThread(const char* pfile, int intervalms, bool is_stop)
{
	if(!pfile){
		ERR_FLCKPRN("Parameter is wrong.");
		return false;
	}
	if(IsInitWorker()){
		ERR_FLCKPRN("Already run worker thread, must stop worker.");
		return false;
	}

	PFLCKTHPARAM	pparam = new FLCKTHPARAM;

	// init param
	pparam->pThFlag		= &thflag;
	pparam->intervalms	= intervalms;
	pparam->pfilepath	= strdup(pfile);

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
	// set flag second to first.
	thflag = FlckThread::FLCK_THCNTL_RUN;

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
	// set flag second to first.
	thflag = FlckThread::FLCK_THCNTL_STOP;

	return true;
}

bool FlckThread::Exit(void)
{
	if(!IsInitWorker()){
		ERR_FLCKPRN("No worker thread is initialized, must initialize worker.");
		return false;
	}
	if(FlckThread::FLCK_THCNTL_EXIT == thflag){
		MSG_FLCKPRN("Already worker thread exit.");
		return true;
	}

	// set flag second to first.
	thflag = FlckThread::FLCK_THCNTL_EXIT;

	// wait for thread exit
	void*	pretval = NULL;
	int		result;
	if(0 != (result = pthread_join(pthreadid, &pretval))){
		ERR_FLCKPRN("Failed to wait exiting thread. return code(error) = %d", result);
		return false;
	}
	MSG_FLCKPRN("Succeed to wait exiting thread. return value ptr = %p(expect=NULL)", pretval);

	is_run_worker = false;

	return true;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
