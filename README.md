fullock
-------
[![Build Status](https://travis-ci.org/yahoojapan/fullock.svg?branch=master)](https://travis-ci.org/yahoojapan/fullock)

FULLOCK - Fast User Level LOCK library

### Overview

FULLOCK is a lock library provided by Yahoo! JAPAN, that is very fast and runs on user level.  
This library provides two lock type.  
One is shared reader/writer lock like that provided by fcntl, another is shared mutex which is specified by name.  
  
These provide the functionality of more than pthread_rwlock and pthread_mutex, and is faster than fcntl.  

### Feature
  - Support multi-threading
  - Support multi-processing
  - Automatically unlock when the process is terminated while holding the lock
  - Automatically unlock when the thread is terminated while holding the lock
  - Automatically unlock when the file handle is closed while holding the lock
  - Not dead lock by same thread locking
  - Provide programming interface like posix mutex and rwlock

### Doccuments
  - [WIKI](https://github.com/yahoojapan/fullock/wiki)

### License
This software is released under the MIT License, see the license file.

Copyright(C) 2015 Yahoo Japan Corporation.
