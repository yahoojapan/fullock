---
layout: contents
language: en-us
title: Overview
short_desc: Fast User Level LOCK library
lang_opp_file: homeja.html
lang_opp_word: To Japanese
prev_url: 
prev_string: 
top_url: index.html
top_string: TOP
next_url: feature.html
next_string: Feature
---

# FULLOCK
**FULLOCK** (**F**ast **U**ser **L**evel **LOCK** library) is a low-level lock library that provides secure, fast locking from multi-process, multi-threading.

## Reasons why FULLOCK is necessary
We can use fcntl, flock, Posix mutex/rwlock/cond etc for file lock/mutex/reader-writer lock/condition variable as standard.  
It is also possible to use these with our products such as K2HASH and CHMPX etc.  
However, in the following points these are inadequate for us and we have created our own library.

- fcntl
 - It does not match the required performance
- posix mutex
 - It has a robust mode(robust non-deadlock mode), but deadlock occurs when the process exits normally without releasing the lock terminates.
- posix rwlock
 - It does not have a robust mode, thus deadlock occurs when the process exits without releasing the lock terminates.
- posix cond
 - It does not have a robust mode, thus deadlock occurs when the process exits without releasing the lock terminates.

And, we need to realize a recursive lock by the same thread(When we do a recursive lock, we will not deadlock if it is the same thread), thus we make a library supporting this recursive locking.
