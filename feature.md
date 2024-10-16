---
layout: contents
language: en-us
title: Feature
short_desc: Fast User Level LOCK library
lang_opp_file: featureja.html
lang_opp_word: To Japanese
prev_url: home.html
prev_string: Overview
top_url: index.html
top_string: TOP
next_url: usage.html
next_string: Usage
---

# Feature

## Flexible installation
We provide suitable FULLOCK installation for your OS. If you use Ubuntu, Fedora, Debian or Alpine you can easily install it from [packagecloud.io](https://packagecloud.io/antpickax/stable). Even if you use none of them, you can use it by [Build](https://fullock.antpick.ax/build.html) by yourself.

## Provided functions
FULLOCK provides the following locking mechanism.

### FULLOCK mutex
Like posix's mutex, this functions as an interprocess shared mutex.  
It has especially powerful **ROBUST** mode.  
This will automatically release the lock even if the process (thread) holding the lock terminates normally.  
In addition, this will automatically manage shared memory etc. to build shared mutex between processes.  
It is easy to understand if you call it a named mutex.

- Overview  
Mutex provided by POSIX (pthread_mutex) can be used as shared lock. In addition, it also provides a ROBUST (robust) mode.  
Then we could also build our products with Mutex provided by POSIX.  
However, this mutex does not deadlock when a process(thread) which locks the mutex exits abnormal, but a deadlock occurs when process exits normally with locking the mutex.


### FULLOCK reader-writer lock(rwlock)
This is a faster locking system than fcntl and provides a process sharing reader writer lock similar to posix 's rwlock.  
There is no ROBUST mode for rwlock shared between processes of posix.  
FULLOCK's rwlock provides the ROBUST mode like a mutex and does not cause a deadlock.  
It provides the same function as F_SETLK of fcntl and it can be used with I / F equivalent to POSIX reader writer lock.  
And we provide a class wrapping the reader writer lock provided by FULLOCK and it can recursively lock.  
This class can recursively lock without doing a deadlock if the thread holding the lock reacquires the same lock.

### FULLOCK condition
Like the mutex, it provides process variable sharing condition variables and operates in ROBUST mode.  
The FULLOCK condition variable is used together with the FULLOCK mutex.  
It is easy to understand if you call it a named condition.

### Others
It provides a simple C ++ template relating to lock as a utility.

## About costs
Linking the FULLOCK library to the program costs a little bit.

- All processes using FULLOCK use one file for shared memory.
- It consumes four file descriptors per process.
- One background thread starts per process. (It does not affect the processing speed of the process)
- It uses 8 bytes per thread.
