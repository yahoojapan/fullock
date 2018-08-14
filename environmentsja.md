---
layout: contents
language: ja
title: Environments
short_desc: Fast User Level LOCK library
lang_opp_file: environments.html
lang_opp_word: To English
prev_url: developerja.html
prev_string: Developer
top_url: indexja.html
top_string: TOP
next_url: 
next_string: 
---

# 環境変数

FULLOCKをリンクしたプログラムは起動時に自動的にFULLOCKの必要とする初期化処理が行われます。
この初期化処理に対して、外部から処理内容、設定を変更するために環境変数を指定できます。  
環境変数の一覧および説明を以下に示します。  

- FLCKAUTOINIT	
 - NO  
   FULLOCKが起動時の初期化処理をしないように指定します。値には、"NO" を指定します。  
   この設定を行った場合には、リーダライターロックおよび名前付ミューテックスを利用する前に fullock_reinitialize() もしくは、fullock_reinitialize_ex() を呼び出し、共有メモリの初期化（もしくはアタッチ）をしてください。  
   この環境変数を指定しなくても、共有メモリファイルを切り替えることはできます。  
   この環境変数を指定した場合には、FULLOCKがデフォルトでアタッチ（作成）する共有メモリファイルを使わない（作成しない）というだけになります。
- FLCKROBUSTMODE	
 - NO / LOW / HIGH  
   FULLOCKが提供するROBUSTモードを指定します。値には、"NO"、"LOW"、もしくは" HIGH" を指定してください。  
   モードの詳細は、「詳細説明」を参照してください。
- FLCKNOMAPMODE	
 - ALLOW_NORETRY / DENY_NORETRY / ALLOW_RETRY / DENY_RETRY / ALLOW / DENY  
   FULLOCKが共有メモリファイルにアタッチ（作成）していないときの動作を指定します。値には、"ALLOW_NORETRY"、"DENY_NORETRY"、"ALLOW_RETRY"、"DENY_RETRY"、"ALLOW"（"ALLOW_NORETRY"と同等）、もしくは"DENY"（"DENY_NORETRY"と同等）を指定してください。  
   モードの詳細は、「詳細説明」を参照してください。
- FLCKFREEUNITMODE	
  - NO / FD / OFFSET
	FULLOCKが持つリソース（リーダライターロックにおけるファイル数、（オフセット＋領域長）数）の管理方法、つまりリソースの開放タイミングを指定します。  
   値には、"NO"、"FD"、"OFFSET"、もしくは"ALWAYS"（"FD"と同等）を指定してください。  
   "NO"の場合には、一度利用したリソースは開放されません（同一の値は再利用されます）。  
   "FD"の場合には利用されなくなった（対象にロック保持/待ちが存在しない）場合には開放され、再利用されます。  
   "OFFSET"の場合には利用されなくなった（対象にロック保持/待ちが存在しない）オフセット＋領域長のみ開放され、再利用されます（ファイルは開放されません）。  
   モードの詳細は、「詳細説明」を参照してください。
- FLCKROBUSTCHKCNT
  - 数値(10進)  
   FULLOCKが高ROBUSTモードで動作している場合のデッドロック検出のための処理頻度を設定します。
   値は、回数（ロック取得を何回試行しても取得できない場合にデッドロック検出をするか）を指定します。ある程度は大きい数字（デフォルト5000）を指定しなければ、不要な負荷となりパフォーマンスに影響します。  
   この環境変数により値を変更するべきではありません。
   モードの詳細は、「詳細説明」を参照してください。
- FLCKUMASK
 - 数値（10進、16進、8進）  
   FULLOCKが作成する共有メモリファイルのパーミッションに影響を与えるUMASK値を指定できます。（デフォルトでは0となっています）  
   1.0.7以前では、最初に共有メモリファイルを作成するプロセスのumaskに影響を受けますが、1.0.8以降ではデフォルト0となっています。  
- FLCKDIRPATH
 - ディレクトリパス  
   FULLOCKが使用する共有メモリファイルのディレクトリパスを指定します。  
   デフォルトは、"/tmp/.fullock"です。
- FLCKFILENAME
 - ファイル名  
   FULLOCKが使用する共有メモリファイルのファイル名を指定します。
   デフォルトは、"fullock.shm" です。
- FLCKFILECNT
 - 数値(10進)  
   FULLOCKが持つリソースのうち、リーダライターロックにおけるファイル数を指定します。  
   デフォルトは、128個です。
- FLCKOFFETCNT
 - 数値(10進)  
   FULLOCKが持つリソースのうち、リーダライターロックにおける（オフセット＋領域長）数を指定します。  
   デフォルトは、8192個です。
- FLCKLOCKERCNT
 - 数値(10進)  
   FULLOCKが持つリソースのうち、リーダライターロックおよび名前付ミューテックで使われる同時にロック保持/待ちをするプロセス（スレッド）の数を指定します。  
   デフォルトは、8192個です。
- FLCKNMTXCNT
 - 数値(10進)  
   FULLOCKが持つリソースのうち、名前付ミューテック数を指定します。  
   デフォルトは、256個です。
- FLCKNCONDCNT
 - 数値(10進)  
FULLOCKが持つリソースのうち、名前付条件変数の数を指定します。  
デフォルトは、256個です。
- FLCKWAITERCNT
 - 数値(10進)  
   FULLOCKが持つリソースのうち、名前付条件変数で使われる同時にウェイトできる総プロセス（スレッド）の数を指定します。  
  デフォルトは、8192個です。
- FLCKDBGMODE
 - SILENT / ERR / WAN / INFO  
   デバッグ出力のレベルを指定します。値は、"SILENT"、"ERR"、"WAN"、"INFO"を指定してください。  
   デフォルトは "SILENT" です。
- FLCKDBGFILE
 - ファイルパス  
   デバッグ出力を行う際の出力ファイルを指定します。  
   デフォルトは未指定となっており、標準エラー出力です。
