---
layout: contents
language: en-us
title: Build
short_desc: Fast User Level LOCK library
lang_opp_file: buildja.html
lang_opp_word: To Japanese
prev_url: usage.html
prev_string: Usage
top_url: index.html
top_string: TOP
next_url: developer.html
next_string: Developer
---

# Build

This chapter instructs how to build **FULLOCK** and install it and its header files.

## 1. Install prerequisites

For DebianStretch or Ubuntu(Bionic Beaver) users, follow the steps below.
```
$ sudo apt-get update -y
$ sudo apt install git autoconf automake libtool g++ -y
```

For Fedora28 or CentOS7.x(6.x) users, follow the steps below.
```
$ sudo yum makecache
$ sudo yum install git gcc-c++ make libtool -y
```

## 2. Clone source codes from GitHub

Download the **FULLOCK**'s source code from [GitHub](https://github.com/yahoojapan/fullock).

```
$ git clone https://github.com/yahoojapan/fullock.git
```

## 3. Building and installing FULLOCK

Just follow the steps below to build **FULLOCK** and install it. We use [GNU Automake](https://www.gnu.org/software/automake/) to build **FULLOCK**.


```
$ cd fullock
$ ./autogen.sh
$ ./configure
$ make
$ sudo make install
```

After successfully installing **FULLOCK**, you will see the manual page for **fullock**:
```bash
$ man fullock
```
