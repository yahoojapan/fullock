---
layout: contents
language: ja
title: Overview
short_desc: Fast User Level LOCK library
lang_opp_file: home.html
lang_opp_word: To English
prev_url: 
prev_string: 
top_url: indexja.html
top_string: TOP
next_url: featureja.html
next_string: Feature
---

# FULLOCK
**FULLOCK**（**F**ast **U**ser **L**evel **LOCK** library）は、マルチプロセス、マルチスレッドから安全で、高速なロック機能を提供する低レベルのロックライブラリです。

## 背景
ファイルのロック、ミューテックス、リーダーライターロック、条件変数に関して、fcntl、flock、Posix mutex/rwlock/cond等々あります。  
K2HASH、CHMPXなどのプロダクトで標準的なこれらを利用することも可能ですが、以下の点において不十分であるということで独自で実装したライブラリが必要となりました。

- fcntl
 - 要求されるパフォーマンスにあわない
- posix mutex
 - robustモードによる非デッドロックでもロックを開放し忘れたプロセス終了が存在するとデッドロックしてしまう
- posix rwlock
 - robustモードがなく、デッドロックする可能性がある
- posix cond
 - robustモードがなく、デッドロックする可能性がある

また、同一スレッドによる再帰的ロック（再帰的なロックを実行した場合に同一のスレッドであればデッドロックしない）をサポートしていないため、この再帰的ロックをサポートしたライブラリが必要であった。

上記の理由により低レベルでのロックライブラリを作成し、利用することになりました。
