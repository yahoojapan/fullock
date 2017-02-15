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

#ifndef	FLCKTHREAD_H
#define	FLCKTHREAD_H

#include <pthread.h>
#include <string>

//---------------------------------------------------------
// FlckThread Class
//---------------------------------------------------------
// This class creates one thread for checking process dead.
// But inotify event can not retun the process id, so if
// the inotify(CLOSE) event occurred, this needs to check
// all process id.
// Do not care for it, performance degradation caused by
// this checking pid process is hardly generated. When the
// termination processing by the abnormal termination of
// the process is not performed, but only it is required
// processing time to be released when it has been deadlock.
//
class FlckThread
{
	public:
		typedef enum thcntl_flag{
			FLCK_THCNTL_RUN	= 0,
			FLCK_THCNTL_STOP,
			FLCK_THCNTL_EXIT,
			FLCK_THCNTL_FIN
		}THCNTLFLAG;

		static const int	DEFAULT_INTERVALMS	= 10;		// default interval ms

	protected:
		static const int	FLCK_WAIT_EVENT_MAX	= 32;		// wait event max count
		static int			InotifyFd;
		static int			WatchFd;
		static int			EventFd;
		static void*		pThreadParam;					// free point using in worker thread

		volatile THCNTLFLAG	thflag;							// thread control flags
		int					bup_intervalms;					// backup for forking
		std::string			bup_filepath;					// backup for forking
		bool				is_run_worker;
		pthread_t			pthreadid;						// worker thread id

	protected:
		static void CleanupHandler(void* arg);				// cleanup handler by canceling thread
		static void* WorkerProc(void* param);
		static bool CheckEvent(int InotifyFd, int WatchFd);

		bool IsInitWorker(void) const { return is_run_worker; }

	public:
		FlckThread();
		virtual ~FlckThread();

		bool InitializeThread(const char* pfile, int intervalms = FlckThread::DEFAULT_INTERVALMS);
		bool ReInitializeThread(void);
		bool Run(void);
		bool Stop(void);
		bool Exit(void);
};

#endif	// FLCKTHREAD_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
