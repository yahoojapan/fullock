---
layout: contents
language: en-us
title: Developer
short_desc: Fast User Level LOCK library
lang_opp_file: developerja.html
lang_opp_word: To Japanese
prev_url: build.html
prev_string: Build
top_url: index.html
top_string: TOP
next_url: environments.html
next_string: Environments
---

<!-- -----------------------------------------------------------　-->
# For developer

#### [C API](#CAPI)
[Version number](#VER)  
[Configuration family](#CONFIG)  
[Reader writer lock](#RWLOCK)  
[Named Mutex](#NAMEDMUTEX)  
[Named condition variable](#NAMEDCOND)  

#### [C++ API](#CPPAPI)
[FlShm class](#FLSHM)  
[FLRwlRcsv class](#FLRWLRCSV)  

<!-- -----------------------------------------------------------　-->
***

## <a name="CAPI"> C API
It is an interface for C language. 
Include the following header file for development.

```
#include <fullock/fullock.h>
```

For the link, specify the following as an option.
```
-lfullock
```

<!-- -----------------------------------------------------------　-->
***

### <a name="VER"> Version number

#### Format
- void fullock_print_version(FILE* stream)

#### Description
- fullock_print_version  
  Output the version number to the stream.

#### Argument
- stream  
  Specify FILE * stream.

#### Return value
None.

<!-- -----------------------------------------------------------　-->
***

### <a name="CONFIG"> Configuration family

#### Format
- bool fullock_set_no_robust(void)
- bool fullock_set_low_robust(void)
- bool fullock_set_high_robust(void)
- bool fullock_set_noretry_allow_nomap(void)
- bool fullock_set_noretry_deny_nomap(void)
- bool fullock_set_retry_allow_nomap(void)
- bool fullock_set_retry_deny_nomap(void)
- bool fullock_set_no_freeunit(void)
- bool fullock_set_fd_freeunit(void)
- bool fullock_set_offset_freeunit(void)
- bool fullock_set_robust_check_count(int val)
- bool fullock_reinitialize(const char* dirpath, const char* filename)
- bool fullock_reinitialize_ex(const char* dirpath, const char* filename, size_t filelockcnt, size_t offlockcnt, size_t lockercnt, size_t nmtxcnt)

#### Description
- fullock_set_no_robust  
  Set the ROBUST mode to "NO". (See environment variable FLCKROBUSTMODE)
- fullock_set_low_robust  
  Set the ROBUST mode to "LOW". (See environment variable FLCKROBUSTMODE)
- fullock_set_high_robust  
  Set the ROBUST mode to "HIGH". (See environment variable FLCKROBUSTMODE)
- fullock_set_noretry_allow_nomap  
  Set "ALLOW_NORETRY" to the behavior when FULLOCK is not attached (created) to the shared memory file. (See environment variable FLCKNOMAPMODE)
- fullock_set_noretry_deny_nomap  
  Set the behavior when FULLOCK is not attached (created) to the shared memory file to "DENY_NORETRY". (See environment variable FLCKNOMAPMODE)
- fullock_set_retry_allow_nomap  
  Set "ALLOW_RETRY" to the behavior when FULLOCK is not attached (created) to the shared memory file. (See environment variable FLCKNOMAPMODE)
- fullock_set_retry_deny_nomap  
  Set the behavior when FULLOCK is not attached (created) to the shared memory file to "DENY_RETRY". (See environment variable FLCKNOMAPMODE)
- fullock_set_no_freeunit  
  Set the resource release timing of FULLOCK to "NO". (See environment variable FLCKFREEUNITMODE)
- fullock_set_fd_freeunit  
  Set the release timing of the resource possessed by FULLOCK to "FD". (See environment variable FLCKFREEUNITMODE)
- fullock_set_offset_freeunit  
  Set the release timing of the resource possessed by FULLOCK to "OFFSET". (See environment variable FLCKFREEUNITMODE)
- fullock_set_robust_check_count  
  Sets the processing frequency for deadlock detection when FULLOCK is operating in high ROBUST mode. (See environment variable FLCKROBUSTCHKCNT)
- fullock_reinitialize  
  Reattach to shared memory file of FULLOCK. When reattaching, the old one is detached, and if there is no newly attached file, or it is not used by another process, it will be attached after it is initialized. If it is used by another process, do not initialize but only attach. You can specify NULL for the directory name and file name to the shared memory file. If NULL is specified, if environment variables (FLCKDIRPATH, FLCKFILENAME) corresponding to each item are specified, that value will be used. If an environment variable is also not specified, the default value is used.
- fullock_reinitialize_ex  
  Reattach to shared memory file of FULLOCK. When reattaching, the old one is detached, and if there is no newly attached file, or it is not used by another process, it will be attached after it is initialized. If it is used by another process, do not initialize but only attach. The difference from fullock_reinitialize is that there are only a lot of arguments and each resource can be specified. If the argument of each resource is 0 (FLCK_INITCNT_DEFAULT), it refers to the environment variables (FLCKFILECNT, FLCKOFFETCNT, FLCKLOCKERCNT, FLCKNMTXCNT) like fullock_reinitialize.

#### Return value
Returns true if it succeeds. If it fails, it returns false.

#### Note
The same as when reattaching "NO" to "environment variable FLCKAUTOINIT", but the value set by functions other than fullock_reinitialize and fullock_reinitialize_ex is not reflected at that point.  
After calling each function and setting the value, it is reattached to the shared memory file by calling fullock_reinitialize or fullock_reinitialize_ex.  
If the file is present at the time of reattachment and another process is attached, ignoring the set value and only reattaching will be done.  
If the file does not exist, a file is newly created.  
If the file exists but the attached process does not exist, the file is reinitialized.  
Files that were attached at the time of calling fullock_reinitialize and fullock_reinitialize_ex are only detached.  

<!-- -----------------------------------------------------------　-->
***

### <a name="RWLOCK"> Reader writer lock

#### Format
- int fullock_rwlock_rdlock(int fd, off_t offset, size_t length)
- int fullock_rwlock_tryrdlock(int fd, off_t offset, size_t length)
- int fullock_rwlock_timedrdlock(int fd, off_t offset, size_t length, time_t timeout_usec)
- int fullock_rwlock_wrlock(int fd, off_t offset, size_t length)
- int fullock_rwlock_trywrlock(int fd, off_t offset, size_t length)
- int fullock_rwlock_timedwrlock(int fd, off_t offset, size_t length, time_t timeout_usec)
- int fullock_rwlock_unlock(int fd, off_t offset, size_t length)
- bool fullock_rwlock_islocked(int fd, off_t offset, size_t length)
<br />
<br />
- FLCK_RWLOCK_NO_FD(intval)

#### Description
- fullock_rwlock_rdlock  
  Get reader lock of reader writer lock. The call is blocked until the lock can be obtained.
- fullock_rwlock_tryrdlock  
  Try to acquire the reader lock of the reader writer lock. It returns immediately without being blocked regardless of the result of the acquisition.
- fullock_rwlock_timedrdlock  
  Try to get reader lock of reader writer lock with timeout. It will fail if the lock can not be acquired within the specified time.
- fullock_rwlock_wrlock  
  Get the writer lock of the reader writer lock. The call is blocked until the lock can be obtained.
- fullock_rwlock_trywrlock  
  Try to acquire the writer lock of the reader writer lock. It returns immediately without being blocked regardless of the result of the acquisition.
- fullock_rwlock_timedwrlock  
  Try to acquire writer lock of reader writer lock with timeout. It will fail if the lock can not be acquired within the specified time.
- fullock_rwlock_unlock  
  Release the lock held by the reader writer lock.
- fullock_rwlock_islocked  
  Make sure that the specified file, offset, and area are locked.
- FLCK_RWLOCK_NO_FD  
  Support macro.  
  When using fullock_rwlock without associating it with fd, you can specify the value to be output by this macro as the fd argument of each fullock_rwlock. An arbitrary numeric value of int value can be used for a macro, and the use side can distinguish the object to be locked with this numerical value.  
  Example: fullock_rwlock_rdlock(FLCK_RWLOCK_NO_FD(mydata), 0, 0)

#### Argument
- fd  
  Specify the file descriptor of the target file to be locked with the reader writer lock.
- offset  
  Specify the lock start position of the target file to be locked with the reader writer lock by the offset from the beginning of the file.
- length  
  Specify the area length from the offset of the target file to be locked by the reader writer lock.
- timeout_usec  
  Specify the timeout in microseconds when acquiring lock with reader writer lock.
- intval  
  It is an argument to the support macro and specifies an integer value. This integer value makes it possible to recognize the lock target of full lock_rwlock in the case of non-fd.

#### Return value
- fullock_isrwlocked  
  Returns true if it succeeds. If it fails, it returns false.
- Other than those above  
  Returns 0 if acquisition and release of lock succeed. If it can not be acquired or can not be released, it returns an error number.

#### Note
Unlike pthread_rwlock, fullock_rwlock requests a file descriptor as a required argument and provides reader writer locking function for the file. Think of this as an alternative to fcntl.
<br />
<br />
If you want to use fullock_rwlock as a reader writer lock not related to the file descriptor, create a pseudo fd with the FLCK_RWLOCK_NO_FD macro for the fd argument and call the function. In place of the file descriptor, the lock value is distinguished by the int value specified in the FLCK_RWLOCK_NO_FD macro.

<!-- -----------------------------------------------------------　-->
***

### <a name="NAMEDMUTEX"> Named Mutex

#### Format
- int fullock_mutex_lock(const char* pname)
- int fullock_mutex_trylock(const char* pname)
- int fullock_mutex_timedlock(const char* pname, time_t timeout_usec)
- int fullock_mutex_unlock(const char* pname)

#### Description
- fullock_mutex_lock  
  Lock the named mutex. The call is blocked until the lock can be obtained.
- fullock_mutex_trylock  
  Attempt to lock the named mutex. It returns immediately without being blocked regardless of the result of the acquisition.
- fullock_mutex_timedlock  
  Try to lock named mutex with lock timeout. It will fail if the lock can not be acquired within the specified time.
- fullock_mutex_unlock  
  Release the lock held by the named mutex.

#### Argument
- pname  
  Specify the name of the named mutex.
- timeout_usec  
  Specify the timeout in microseconds for locking named mutexes.

#### Return value
Returns 0 if acquisition and release of lock succeed. If it can not be acquired or can not be released, it returns an error number.

#### Note
nothing special.

<!-- -----------------------------------------------------------　-->
***

### <a name="NAMEDCOND"> Named condition variable

#### Format
- int fullock_cond_timedwait(const char* pcondname, const char* pmutexname, time_t timeout_usec)
- int fullock_cond_wait(const char* pcondname, const char* pmutexname)
- int fullock_cond_signal(const char* pcondname)
- int fullock_cond_broadcast(const char* pcondname)

#### Description
- fullock_cond_timedwait  
  It waits with timeout until the condition of the named condition variable is satisfied. If the condition is not satisfied within the specified time, the block is canceled.
- fullock_cond_wait  
  It waits until the condition of the named condition variable is satisfied. This function is blocked.
- fullock_cond_signal  
  It unblocks a thread (process) that is waiting for a named condition variable.
- fullock_cond_broadcast  
  Unblock all threads (processes) that are waiting for named condition variables.

#### Argument
- pcondname  
  Specify the name of the named condition variable.
- pmutexname  
  Specify the name of the named mutex to be used together with the named condition variable.
- timeout_usec  
  Specify the timeout in microseconds when waiting until the condition of the named condition variable is satisfied.

#### Return value
It returns 0 if the process succeeded. Otherwise, it returns the error number.

#### Note
nothing special.

<!-- -----------------------------------------------------------　-->
<!-- -----------------------------------------------------------　-->
<!-- -----------------------------------------------------------　-->
<!-- -----------------------------------------------------------　-->
<!-- -----------------------------------------------------------　-->
***

## <a name="CPPAPI"> C++ API
It is an interface for the C ++ language.   
<br />
Include the following header file for development.
```
#include <fullock/flckshm.h>
#include <fullock/rwlockrcsv.h>
```

For the link, specify the following as an option.
```
-lfullock
```

<!-- -----------------------------------------------------------　-->
***

### <a name="FLSHM"> FlShm class

Operation class of reader writer lock and named mutex.  
<br />
Below is a list of available methods.
#### Method
- int ReadLock(int fd, off_t offset, size_t length)
- int TryReadLock(int fd, off_t offset, size_t length)
- int TimeoutReadLock(int fd, off_t offset, size_t length, time_t timeout_usec)
- int WriteLock(int fd, off_t offset, size_t length)
- int TryWriteLock(int fd, off_t offset, size_t length)
- int TimeoutWriteLock(int fd, off_t offset, size_t length, time_t timeout_usec)
- int Unlock(int fd, off_t offset, size_t length)
- bool IsLocked(int fd, off_t offset, size_t length);
<br />
<br />
- int Lock(const char* pname)
- int TryLock(const char* pname)
- int TimeoutLock(const char* pname, time_t timeout_usec)
- int Unlock(const char* pname)
<br />
<br />
- int TimeoutWait(const char* pcondname, const char* pmutexname, time_t timeout_usec)
- int Wait(const char* pcondname, const char* pmutexname)
- int Signal(const char* pcondname)
- int Broadcast(const char* pcondname)
<br />
<br />
- static bool Dump(std::ostream& out, bool is_free_list = true)

#### Description
- ReadLock  
  Get reader lock of reader writer lock. The call is blocked until the lock can be obtained.
- TryReadLock  
  Try to acquire the reader lock of the reader writer lock. It returns immediately without being blocked regardless of the result of the acquisition.
- TimeoutReadLock  
  Try reader lock acquisition of reader writer lock with timeout. It will fail if the lock can not be acquired within the specified time.
- WriteLock  
  Get the writer lock of the reader writer lock. The call is blocked until the lock can be obtained.
- TryWriteLock  
  Try to acquire the writer lock of the reader writer lock. I will return immediately without being blocked regardless of the result of the acquisition.
- TimeoutWriteLock  
  This will try to acquire the writer lock of the reader writer lock with timeout. It will fail if the lock can not be acquired within the specified time.
- Unlock  
  Release the lock held by the reader writer lock.
- IsLocked  
  Make sure that the specified file, offset, and area are locked.
- Lock  
  Lock the named mutex. The call is blocked until the lock can be obtained.
- TryLock  
  Attempt to lock the named mutex. It returns immediately without being blocked regardless of the result of the acquisition.
- TimeoutLock  
  Try to lock named mutex with lock timeout. It will fail if the lock can not be acquired within the specified time.
- Unlock  
  Release the lock held by the named mutex.
- TimeoutWait  
  It waits with timeout until the condition of the named condition variable is satisfied. If the condition is not satisfied within the specified time, the block is canceled.
- Wait  
  This waits until the condition of the named condition variable is satisfied. This function is blocked.
- Signal  
  Unblocks one thread (process) waiting for a named condition variable.
- Broadcast  
  Unblock all threads (processes) that are waiting for named condition variables.
- Dump  
  This method dumps inside the shared memory file for debugging. Normally it will not be used.

#### Argument
- fd  
  Specify the file descriptor of the target file to be locked with the reader writer lock.
- offset  
  Specify the lock start position of the target file to be locked with the reader writer lock by the offset from the beginning of the file.
- length  
  Specify the area length from the offset of the target file to be locked by the reader writer lock.
- pname, pmutexname  
  Specify the name of the named mutex.
- pcondname  
  Specify the name of a named condition variable.
- timeout_usec  
  When acquiring a lock with a reader writer lock, specify the timeout in microseconds when locking a named mutex.

#### Return value

- IsLocked  
  Returns true if it succeeds. If it fails, it returns false.
- Dump  
  Returns true if it succeeds. If it fails, it returns false.
- Other than those above  
  Returns 0 if acquisition and release of lock succeed. If it can not be acquired or can not be released, it returns an error number.

#### Note
- It corresponds to C language interface.
- To use the reader writer lock without relation to the file descriptor, generate and specify the pseudo fd using the FLCK_RWLOCK_NO_FD macro. (See the full lock_rwlock description of the C language interface.)

<!-- -----------------------------------------------------------　-->
***

### <a name="FLRWLRCSV"> FLRwlRcsv class

It is an operation class that wraps the reader writer lock provided by FULLOCK and enables recursive locking by the same thread.
<br />
Below is a list of available methods.

#### Method
- FLRwlRcsv(int fd = FLCK_INVALID_HANDLE, off_t offset = 0, size_t length = 0)
- FLRwlRcsv(int fd, off_t offset, size_t length, bool is_read)
- ~FLRwlRcsv(void)
- bool Lock(int fd, off_t offset, size_t length, bool is_read = true)
- bool Lock(bool is_read)
- bool Lock(void)
- bool ReadLock(void)
- bool WriteLock(void)
- bool Unlock(void)

#### Description
- constructor  
  The constructor can also pass a file descriptor, offset, area length, reader lock or writer lock, and acquire lock at the time the constructor is executed.
- destructor  
  The destructor releases the lock if it gets the lock.
- Lock  
  Method to get reader writer lock lock. There are 3 types.  
  The first is to acquire the lock by specifying file descriptor, offset, area length, reader lock or writer lock.  
  Second, on the premise that the file descriptor, offset, and area length are specified, only reader lock or writer lock is specified and the lock is acquired.  
  Third, we acquire the lock on the premise that the file descriptor, offset, area length, reader lock or writer lock are all set.  
- ReadLock  
  It is the same as Lock's second, and it acquires a reader lock on the premise that the file descriptor, offset, and area length are specified.
- WriteLock  
  It is the same as Lock's second, and it acquires the writer lock on the premise that the file descriptor, offset, and area length are specified.
- Unlock  
  Release the lock held by the reader writer lock.

#### Argument
- fd  
  Specify the file descriptor of the target file to be locked with the reader writer lock.
- offset  
  Specify the lock start position of the target file to be locked with the reader writer lock by the offset from the beginning of the file.
- length  
  Specify the area length from the offset of the target file to be locked by the reader writer lock.
- is_read  
  Specify true for reader lock of reader writer lock and false for writer lock.

#### Return value
Returns true if acquisition and release of lock succeeded. If it can not be acquired or can not be released, false is returned.

#### Note

This class is an operation class that wraps the reader writer lock of FULLOCK and allows recursive locking by the same thread, and even if locks are acquired from the same thread for the same file descriptor, offset, and area length, it is blocked No.  
For example, it is possible to create another instance of this class in the thread that acquired the reader lock and acquire the writer lock on the same area.  
Also, since the object of this class automatically releases the lock, even if the object is discarded without calling Unlock, the lock is automatically released.  
As a usage method, it is possible to safely lock / unlock within the scope by simply defining this class object as an auto variable in scope in the C / C ++ language.

<!-- -----------------------------------------------------------　-->
<!-- -----------------------------------------------------------　-->
<!-- -----------------------------------------------------------　-->
<!-- -----------------------------------------------------------　-->
<!-- -----------------------------------------------------------　-->
