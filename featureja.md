---
layout: contents
language: ja
title: Feature
short_desc: Fast User Level LOCK library
lang_opp_file: feature.html
lang_opp_word: To English
prev_url: homeja.html
prev_string: Overview
top_url: indexja.html
top_string: TOP
next_url: usageja.html
next_string: Usage
---

# FULLOCKの特徴

## 柔軟なインストール
FULLOCKは、あなたのOSに応じて、柔軟にインストールが可能です。あなたのOSが、Ubuntu、CentOS、Fedora、Debianなら、[packagecloud.io](https://packagecloud.io/antpickax/stable)からソースコードをビルドすることなく、簡単にインストールできます。それ以外のOSであっても、自身で[ビルド](https://fullock.antpick.ax/buildja.html)して使うことができます。

## 提供機能
FULLOCKは以下の機能を提供します。

### FULLOCKミューテックス
posixのmutexと同様にプロセス間共有ミューテックスとして機能します。  
ただし、ロック保持しているプロセス（スレッド）が正常終了（exitなどで終了）した場合でもロックを自動的に開放できる強力な**ROBUST**モードを備えています。  
また、プロセス間共有ミューテックスを構築するための共有メモリの管理など自動的に行われます。  
名前付ミューテックスと考えると理解しやすいです。

- 背景  
POSIXの提供するミューテックス（pthread_mutex）は、共有ロックとして利用でき、ROBUST（堅牢）モードも存在しているため、POSIXの提供するミューテックスそのものでの提供が可能です。  
しかし、ロック保持しているプロセス（スレッド）が異常終了（KILLなど）した場合にはロック開放されますが、正常終了（exitなどで終了）した場合にはロックの開放が行われません。


### FULLOCKリーダライターロック
fcntlよりも高速なロックシステムであり、posixのrwlockと同様のプロセス間共有のリーダーライターロックを提供します。  
posixのrwlockでは、プロセス間共有したrwlockにはROBUSTモードは存在しませんが、FULLOCKではミューテックスと同様にROBUSTモードを提供し、デッドロックを起こさないロックを提供します。  
fcntlのF_SETLKと同等の機能を提供し、POSIXのリーダライターロックと同等のI/Fで利用できます。  
FULLOCKでは、提供するリーダーライターロックをラップしたクラスを提供し、再帰的なロックにも対応しています。  
ロックを保持するスレッドが同一のロックを再取得する場合において、デッドロックせず再帰的にロックができる機能を提供します。


### FULLOCK条件変数
ミューテックスと同様にROBUSTモードを強固にしたプロセス間共有の条件変数を提供します。  
FULLOCKミューテックスを利用し、提供される条件変数です。  
名前付条件変数と考えると理解しやすいです。

### その他
ロックに関連する簡易的なC++テンプレートを提供します。


## コスト
FULLOCKを利用した場合（リンクした場合）には以下のコストがかかります。

- 全プロセスで共有メモリファイルとして1つのファイルを利用します。
- 1プロセスあたりファイルディスクリプタを4つ消費します。（スレッドではないことに注意）
- 1プロセスあたり1つのバックグラウンドスレッドが起動します。（ほとんど処理速度には影響しません）
- 1スレッドあたり8バイトの領域を利用します。
