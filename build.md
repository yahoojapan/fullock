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

# Building
The build method for FULLOCK is explained below.

## 1. Install prerequisites before compiling
- Debian / Ubuntu
```
$ sudo apt update
$ sudo apt install git autoconf automake libtool g++
```

- Fedora / CentOS
```
$ sudo yum install git gcc-c++ make libtool
```

## 2. Clone source codes from Github
```
$ git clone git@github.com:yahoojapan/fullock.git
```

## 3. Building and installing FULLOCK
```
$ ./autogen.sh
$ ./configure
$ make
$ sudo make install
```
