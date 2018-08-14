---
layout: contents
language: ja
title: Build
short_desc: Fast User Level LOCK library
lang_opp_file: build.html
lang_opp_word: To English
prev_url: usageja.html
prev_string: Usage
top_url: indexja.html
top_string: TOP
next_url: developerja.html
next_string: Developer
---

# ビルド方法
FULLOCKをビルドする方法を説明します。

## 1. 事前環境
- Debian / Ubuntu
```
$ sudo apt update
$ sudo apt install git autoconf automake libtool g++
```

- Fedora / CentOS
```
$ sudo yum install git gcc-c++ make libtool
```

## 2. clone
```
$ git clone git@github.com:yahoojapan/fullock.git
```

## 3. ビルド、インストール：FULLOCK
```
$ ./autogen.sh
$ ./configure
$ make
$ sudo make install
```
