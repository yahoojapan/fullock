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

# ビルド

この章では**FULLOCK**をビルドしてそれとそのヘッダファイルをインストールする方法を説明します。

## 1.ビルド環境の構築

最近のDebianベースLinuxの利用者は、以下の手順に従ってください。
```
$ sudo apt-get update -y
$ sudo apt install git autoconf automake libtool g++ -y
```

Fedoraの利用者は、以下の手順に従ってください。
```
$ sudo dnf makecache
$ sudo dnf install git gcc-c++ make libtool -y
```

その他最近のRPMベースのLinuxの場合は、以下の手順に従ってください。
```
$ sudo yum makecache
$ sudo yum install git gcc-c++ make libtool -y
```

## 2. GitHubからソースコードを複製する

**FULLOCK**のソースコードを[GitHub](https://github.com/yahoojapan/fullock)からダウンロードしてください。

```
$ git clone https://github.com/yahoojapan/fullock.git
```

## 3. FULLOCKの構築とインストール

以下の手順に従って**FULLOCK**をビルドしてインストールしてください。 **FULLOCK**を構築するために[GNU Automake](https://www.gnu.org/software/automake/)を使用します。


```
$ cd fullock
$ ./autogen.sh
$ ./configure
$ make
$ sudo make install
```

**FULLOCK**のインストールに成功すると、**fullock**のマニュアルページが表示されます。
```bash
$ man fullock
```
