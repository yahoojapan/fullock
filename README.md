fullock
-------
[![C/C++ AntPickax CI](https://github.com/yahoojapan/fullock/workflows/C/C++%20AntPickax%20CI/badge.svg)](https://github.com/yahoojapan/fullock/actions)
[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/yahoojapan/fullock/master/COPYING)
[![GitHub forks](https://img.shields.io/github/forks/yahoojapan/fullock.svg)](https://github.com/yahoojapan/fullock/network)
[![GitHub stars](https://img.shields.io/github/stars/yahoojapan/fullock.svg)](https://github.com/yahoojapan/fullock/stargazers)
[![GitHub issues](https://img.shields.io/github/issues/yahoojapan/fullock.svg)](https://github.com/yahoojapan/fullock/issues)
[![RPM packages](https://img.shields.io/badge/rpm-packagecloud.io-844fec.svg)](https://packagecloud.io/antpickax/stable)
[![debian packages](https://img.shields.io/badge/deb-packagecloud.io-844fec.svg)](https://packagecloud.io/antpickax/stable)
[![ALPINE packages](https://img.shields.io/badge/apk-packagecloud.io-844fec.svg)](https://packagecloud.io/antpickax/stable)
[![Docker image](https://img.shields.io/docker/pulls/antpickax/libfullock.svg)](https://hub.docker.com/r/antpickax/libfullock)
[![Docker dev image](https://img.shields.io/docker/pulls/antpickax/libfullock-dev.svg)](https://hub.docker.com/r/antpickax/libfullock-dev)

FULLOCK - Fast User Level LOCK library

### Overview

FULLOCK is a lock library provided by Yahoo! JAPAN, that is very fast and runs on user level.  
This library provides two lock type.  
One is shared reader/writer lock like that provided by fcntl, another is shared mutex which is specified by name.  
  
These provide the functionality of more than pthread_rwlock and pthread_mutex, and is faster than fcntl.  

![FULLOCK](https://fullock.antpick.ax/images/top_fullock.png)

### Feature
  - Support multi-threading
  - Support multi-processing
  - Automatically unlock when the process is terminated while holding the lock
  - Automatically unlock when the thread is terminated while holding the lock
  - Automatically unlock when the file handle is closed while holding the lock
  - Not dead lock by same thread locking
  - Provide programming interface like posix mutex and rwlock

### Documents
  - [Document top page](https://fullock.antpick.ax/)
  - [Github wiki page](https://github.com/yahoojapan/fullock/wiki)
  - [About AntPickax](https://antpick.ax/)

### Packages
  - [RPM packages(packagecloud.io)](https://packagecloud.io/antpickax/stable)
  - [Debian packages(packagecloud.io)](https://packagecloud.io/antpickax/stable)
  - [ALPINE packages(packagecloud.io)](https://packagecloud.io/antpickax/stable)

### Docker images
  - [libfullock(Docker Hub)](https://hub.docker.com/r/antpickax/libfullock)
  - [libfullock-dev(Docker Hub)](https://hub.docker.com/r/antpickax/libfullock-dev)

### License
This software is released under the MIT License, see the license file.

### AntPickax
fullock is one of [AntPickax](https://antpick.ax/) products.

Copyright(C) 2015 Yahoo Japan Corporation.
