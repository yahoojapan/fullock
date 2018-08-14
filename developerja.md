---
layout: contents
language: ja
title: Developer
short_desc: Fast User Level LOCK library
lang_opp_file: developer.html
lang_opp_word: To English
prev_url: buildja.html
prev_string: Build
top_url: indexja.html
top_string: TOP
next_url: environmentsja.html
next_string: Environments
---

<!-- -----------------------------------------------------------　-->
# 開発者向け

#### [C言語インタフェース](#CAPI)
[バージョン番号](#VER)  
[設定関連](#CONFIG)  
[リーダーライターロック](#RWLOCK)  
[名前付ミューテックス](#NAMEDMUTEX)  
[名前付条件変数](#NAMEDCOND)  

#### [C++言語インタフェース](#CPPAPI)
[FlShmクラス](#FLSHM)  
[FLRwlRcsvクラス](#FLRWLRCSV)  

<!-- -----------------------------------------------------------　-->
***

## <a name="CAPI"> C言語インタフェース
C言語用のインタフェースです。　　
開発時には、以下をインクルードしてください。

```
#include <fullock/fullock.h>
```

リンク時には以下をオプションとして指定してください。
```
-lfullock
```

<!-- -----------------------------------------------------------　-->
***

### <a name="VER"> バージョン番号

#### 書式
- void fullock_print_version(FILE* stream)

#### 説明
- fullock_print_version  
  バージョン番号をストリームに出力します。

#### 引数
- stream  
  FILE*ストリームを指定してください。

#### 返り値

なし。

<!-- -----------------------------------------------------------　-->
***

### <a name="CONFIG"> 設定関連

#### 書式
- bool fullock_set_no_robust(void)
- bool fullock_set_low_robust(void)
- bool fullock_set_high_robust(void)
- bool fullock_set_noretry_allow_nomap(void)
- bool fullock_set_noretry_deny_nomap(void)
- bool fullock_set_retry_allow_nomap(void)
- bool fullock_set_retry_deny_nomap(void)
- bool fullock_set_no_freeunit(void)
- bool fullock_set_fd_freeunit(void)
- bool fullock_set_offset_freeunit(void)
- bool fullock_set_robust_check_count(int val)
- bool fullock_reinitialize(const char* dirpath, const char* filename)
- bool fullock_reinitialize_ex(const char* dirpath, const char* filename, size_t filelockcnt, size_t offlockcnt, size_t lockercnt, size_t nmtxcnt)

#### 説明
- fullock_set_no_robust  
  ROBUSTモードを"NO"に設定します。（環境変数 FLCKROBUSTMODE を参照）
- fullock_set_low_robust  
  ROBUSTモードを"LOW"に設定します。（環境変数 FLCKROBUSTMODE を参照）
- fullock_set_high_robust  
  ROBUSTモードを"HIGH"に設定します。（環境変数 FLCKROBUSTMODE を参照）
- fullock_set_noretry_allow_nomap  
  FULLOCKが共有メモリファイルにアタッチ（作成）していないときの動作を "ALLOW_NORETRY" に設定します。（環境変数 FLCKNOMAPMODE を参照）
- fullock_set_noretry_deny_nomap  
  FULLOCKが共有メモリファイルにアタッチ（作成）していないときの動作を "DENY_NORETRY" に設定します。（環境変数 FLCKNOMAPMODE を参照）
- fullock_set_retry_allow_nomap  
  FULLOCKが共有メモリファイルにアタッチ（作成）していないときの動作を "ALLOW_RETRY" に設定します。（環境変数 FLCKNOMAPMODE を参照）
- fullock_set_retry_deny_nomap  
  FULLOCKが共有メモリファイルにアタッチ（作成）していないときの動作を "DENY_RETRY" に設定します。（環境変数 FLCKNOMAPMODE を参照）
- fullock_set_no_freeunit  
  FULLOCKが持つリソースの開放タイミングを "NO" に設定します。（環境変数 FLCKFREEUNITMODE を参照）
- fullock_set_fd_freeunit  
  FULLOCKが持つリソースの開放タイミングを "FD" に設定します。（環境変数 FLCKFREEUNITMODE を参照）
- fullock_set_offset_freeunit  
  FULLOCKが持つリソースの開放タイミングを "OFFSET" に設定します。（環境変数 FLCKFREEUNITMODE を参照）
- fullock_set_robust_check_count
  FULLOCKが高ROBUSTモードで動作している場合のデッドロック検出のための処理頻度を設定します。（環境変数 FLCKROBUSTCHKCNT を参照）
- fullock_reinitialize  
  FULLOCKの共有メモリファイルへ再アタッチします。再アタッチするときに、古いものはデタッチされ、新たにアタッチするファイルが存在しないか、他のプロセスにより利用されていない場合には初期化されたあとでアタッチします。もし他プロセスにより利用されている場合には、初期化せずアタッチのみ行います。共有メモリファイルへのディレクトリ名、ファイル名にNULLを指定することができます。NULLを指定した場合には、それぞれの項目に応じた環境変数（FLCKDIRPATH、FLCKFILENAME）が指定されていれば、その値が使用されます。環境変数も指定されていない場合にはデフォルトの値が使用されます。
- fullock_reinitialize_ex  
  FULLOCKの共有メモリファイルへ再アタッチします。再アタッチするときに、古いものはデタッチされ、新たにアタッチするファイルが存在しないか、他のプロセスにより利用されていない場合には初期化されたあとでアタッチします。もし他プロセスにより利用されている場合には、初期化せずアタッチのみ行います。 fullock_reinitializeとの違いは、引数が多いだけであり、各リソースを指定できるようになっています。各リソースの引数が0（FLCK_INITCNT_DEFAULT）である場合には、fullock_reinitialize と同様に環境変数（FLCKFILECNT、FLCKOFFETCNT、FLCKLOCKERCNT、FLCKNMTXCNT）を参照します。

#### 返り値
成功すれば true を返します。失敗した場合には false を返します。

#### 注意

環境変数 FLCKAUTOINIT に NO を設定し、再アタッチする場合と同じですが、fullock_reinitialize および fullock_reinitialize_ex 以外の関数により設定された値は、その時点で反映されるわけではありません。各関数を呼び出し、値を設定した後で、fullock_reinitialize もしくは fullock_reinitialize_ex を呼び出すことで、共有メモリファイルへ再アタッチされます。
もし、再アタッチの時点で、ファイルが存在し、他プロセスがアタッチ中であれば、設定された値を無視して再アタッチのみが行われます。
ファイルが存在しない場合にはファイルが新たに作成されます。またファイルが存在しているが、アタッチしているプロセスが存在しない場合には、ファイルは再初期化されます。
fullock_reinitialize および fullock_reinitialize_ex を呼び出した時点でアタッチしていたファイルはデタッチのみされます。

<!-- -----------------------------------------------------------　-->
***

### <a name="RWLOCK"> リーダライターロック

#### 書式
- int fullock_rwlock_rdlock(int fd, off_t offset, size_t length)
- int fullock_rwlock_tryrdlock(int fd, off_t offset, size_t length)
- int fullock_rwlock_timedrdlock(int fd, off_t offset, size_t length, time_t timeout_usec)
- int fullock_rwlock_wrlock(int fd, off_t offset, size_t length)
- int fullock_rwlock_trywrlock(int fd, off_t offset, size_t length)
- int fullock_rwlock_timedwrlock(int fd, off_t offset, size_t length, time_t timeout_usec)
- int fullock_rwlock_unlock(int fd, off_t offset, size_t length)
- bool fullock_rwlock_islocked(int fd, off_t offset, size_t length)
<br />
<br />
- FLCK_RWLOCK_NO_FD(intval)

#### 説明

- fullock_rwlock_rdlock  
  リーダーライターロックのリーダロックを取得します。ロックが取得できるまで呼び出しはブロックされます。
- fullock_rwlock_tryrdlock  
  リーダーライターロックのリーダロック取得を試行します。取得の結果に関わらずブロックされずにすぐ戻ります。
- fullock_rwlock_timedrdlock  
  リーダーライターロックのリーダロック取得をタイムアウト付で試行します。指定時間内にロックが取得できない場合には失敗します。
- fullock_rwlock_wrlock  
  リーダーライターロックのライターロックを取得します。ロックが取得できるまで呼び出しはブロックされます。
- fullock_rwlock_trywrlock  
  リーダーライターロックのライターロック取得を試行します。取得の結果に関わらずブロックされずにすぐ戻ります。
- fullock_rwlock_timedwrlock  
  リーダーライターロックのライターロック取得をタイムアウト付で試行します。指定時間内にロックが取得できない場合には失敗します。
- fullock_rwlock_unlock  
  リーダーライターロックの保持しているロックを開放します。
- fullock_rwlock_islocked  
  指定したファイル、オフセット、領域がロックされているか確認します。
- FLCK_RWLOCK_NO_FD  
  サポートマクロ  
  fdに関連付けしない状態で、fullock_rwlock を利用する場合に各 fullock_rwlock のfd引数にこのマクロで出力する値を指定できます。マクロにはint値の任意の数値を使用でき、利用する側はこの数値にてロック対象を区別することができます。  
  例： fullock_rwlock_rdlock(FLCK_RWLOCK_NO_FD(mydata), 0, 0)

#### 引数
- fd  
  リーダライターロックでロックする対象ファイルのファイルディスクリプタを指定します。
- offset  
  リーダライターロックでロックする対象ファイルのロック開始位置をファイルの先頭からのオフセットで指定します。
- length  
  リーダライターロックでロックする対象ファイルのオフセットからの領域長を指定します。
- timeout_usec  
  リーダライターロックでロック取得する場合のタイムアウトをマイクロ秒単位で指定します。
- intval  
  サポートマクロへの引数であり、int値を指定します。このint値により非fdの場合の fullock_rwlock のロック対象が認識できるようになります。

#### 返り値
- fullock_isrwlocked  
  成功すればtrueを返します。失敗した場合にはfalseを返します。
- 上記以外  
  ロックの取得、開放に成功した場合には0を返します。取得できない、開放できない場合にはエラー番号を返します。

#### 注意
fullock_rwlock は、pthread_rwlockとは異なり、ファイルディスクリプタを必須の引数として要求し、ファイルに対してリーダライターロックの機能を提供しています。これは、fcntlの代替としての機能と考えてください。
<br />
<br />
もし、ファイルディスクリプタに関連しないリーダーライターロックとしてfullock_rwlockを利用する場合には、fd引数には FLCK_RWLOCK_NO_FDマクロを使って擬似fdを作成し、関数を呼び出してください。ファイルディスクリプタの代わりに、FLCK_RWLOCK_NO_FDマクロに指定するint値により、ロック対象が区別されます。

<!-- -----------------------------------------------------------　-->
***

### <a name="NAMEDMUTEX"> 名前付ミューテックス

#### 書式
- int fullock_mutex_lock(const char* pname)
- int fullock_mutex_trylock(const char* pname)
- int fullock_mutex_timedlock(const char* pname, time_t timeout_usec)
- int fullock_mutex_unlock(const char* pname)

#### 説明
- fullock_mutex_lock  
  名前付ミューテックスをロックします。ロックが取得できるまで呼び出しはブロックされます。
- fullock_mutex_trylock  
  名前付ミューテックスをロックを試行します。取得の結果に関わらずブロックされずにすぐ戻ります。
- fullock_mutex_timedlock  
  名前付ミューテックスをロックをタイムアウト付で試行します。指定時間内にロックが取得できない場合には失敗します。
- fullock_mutex_unlock  
  名前付ミューテックスの保持しているロックを開放します。

#### 引数
- pname  
  名前付ミューテックスの名前を指定します。
- timeout_usec  
  名前付ミューテックスをロックする場合のタイムアウトをマイクロ秒単位で指定します。

#### 返り値

ロックの取得、開放に成功した場合には0を返します。取得できない、開放できない場合にはエラー番号を返します。

#### 注意

特になし。

<!-- -----------------------------------------------------------　-->
***

### <a name="NAMEDCOND"> 名前付条件変数

#### 書式
- int fullock_cond_timedwait(const char* pcondname, const char* pmutexname, time_t timeout_usec)
- int fullock_cond_wait(const char* pcondname, const char* pmutexname)
- int fullock_cond_signal(const char* pcondname)
- int fullock_cond_broadcast(const char* pcondname)

#### 説明
- fullock_cond_timedwait  
  名前付条件変数の条件成立までタイムアウト付でウエイトします。指定時間内に条件が成立しない場合にはブロックは解除されます。
- fullock_cond_wait  
  名前付条件変数の条件成立までウエイトします。この関数はブロックされます。
- fullock_cond_signal  
  名前付条件変数をウエイトしているスレッド（プロセス）を1つブロック解除します。
- fullock_cond_broadcast  
  名前付条件変数をウエイトしているすべてのスレッド（プロセス）をブロック解除します。

#### 引数
- pcondname  
  名前付条件変数の名前を指定します。
- pmutexname  
  名前付条件変数と一緒に利用する名前付ミューテックスの名前を指定します。
- timeout_usec  
  名前付条件変数の条件成立までのウエイトをする場合のタイムアウトをマイクロ秒単位で指定します。

#### 返り値
処理が成功した場合には0を返します。それ以外の場合には、エラー番号を返します。

#### 注意
特になし。

<!-- -----------------------------------------------------------　-->
<!-- -----------------------------------------------------------　-->
<!-- -----------------------------------------------------------　-->
<!-- -----------------------------------------------------------　-->
<!-- -----------------------------------------------------------　-->
***

## <a name="CPPAPI"> C++言語インタフェース
C++言語用のインタフェースです。　　
開発時には、必要に応じて以下をインクルードしてください。

```
#include <fullock/flckshm.h>
#include <fullock/rwlockrcsv.h>
```

リンク時には以下をオプションとして指定してください。
```
-lfullock
```

<!-- -----------------------------------------------------------　-->
***

### <a name="FLSHM"> FlShmクラス

リーダライターロックおよび名前付ミューテックスの操作クラスです。

以下は、利用できるメソッドの一覧です。
#### メソッド
- int ReadLock(int fd, off_t offset, size_t length)
- int TryReadLock(int fd, off_t offset, size_t length)
- int TimeoutReadLock(int fd, off_t offset, size_t length, time_t timeout_usec)
- int WriteLock(int fd, off_t offset, size_t length)
- int TryWriteLock(int fd, off_t offset, size_t length)
- int TimeoutWriteLock(int fd, off_t offset, size_t length, time_t timeout_usec)
- int Unlock(int fd, off_t offset, size_t length)
- bool IsLocked(int fd, off_t offset, size_t length);
<br />
<br />
- int Lock(const char* pname)
- int TryLock(const char* pname)
- int TimeoutLock(const char* pname, time_t timeout_usec)
- int Unlock(const char* pname)
<br />
<br />
- int TimeoutWait(const char* pcondname, const char* pmutexname, time_t timeout_usec)
- int Wait(const char* pcondname, const char* pmutexname)
- int Signal(const char* pcondname)
- int Broadcast(const char* pcondname)
<br />
<br />
- static bool Dump(std::ostream& out, bool is_free_list = true)

#### 説明
- ReadLock  
  リーダーライターロックのリーダロックを取得します。ロックが取得できるまで呼び出しはブロックされます。
- TryReadLock  
  リーダーライターロックのリーダロック取得を試行します。取得の結果に関わらずブロックされずにすぐ戻ります。
- TimeoutReadLock  
  リーダーライターロックのリーダロック取得をタイムアウト付で試行します。指定時間内にロックが取得できない場合には失敗します。
- WriteLock  
  リーダーライターロックのライターロックを取得します。ロックが取得できるまで呼び出しはブロックされます。
- TryWriteLock  
  リーダーライターロックのライターロック取得を試行します。取得の結果に関わらずブロックされずにすぐ戻ります。
- TimeoutWriteLock  
  リーダーライターロックのライターロック取得をタイムアウト付で試行します。指定時間内にロックが取得できない場合には失敗します。
- Unlock  
  リーダーライターロックの保持しているロックを開放します。
- IsLocked  
  指定したファイル、オフセット、領域がロックされているか確認します。
- Lock  
  名前付ミューテックスをロックします。ロックが取得できるまで呼び出しはブロックされます。
- TryLock  
  名前付ミューテックスをロックを試行します。取得の結果に関わらずブロックされずにすぐ戻ります。
- TimeoutLock  
  名前付ミューテックスをロックをタイムアウト付で試行します。指定時間内にロックが取得できない場合には失敗します。
- Unlock  
  名前付ミューテックスの保持しているロックを開放します。
- TimeoutWait  
  名前付条件変数の条件成立までタイムアウト付でウエイトします。指定時間内に条件が成立しない場合にはブロックは解除されます。
- Wait  
  名前付条件変数の条件成立までウエイトします。この関数はブロックされます。
- Signal  
  名前付条件変数をウエイトしているスレッド（プロセス）を1つブロック解除します。
- Broadcast  
  名前付条件変数をウエイトしているすべてのスレッド（プロセス）をブロック解除します。
- Dump  
  デバッグ用に共有メモリファイル内部をダンプするメソッドです。通常は利用することはありません。

#### 引数
- fd  
  リーダライターロックでロックする対象ファイルのファイルディスクリプタを指定します。
- offset  
  リーダライターロックでロックする対象ファイルのロック開始位置をファイルの先頭からのオフセットで指定します。
- length  
  リーダライターロックでロックする対象ファイルのオフセットからの領域長を指定します。
- pname, pmutexname  
  名前付ミューテックスの名前を指定します。
- pcondname  
  名前付条件変数の名前を指定します。
- timeout_usec  
  リーダライターロックでロック取得する場合、名前付ミューテックスのロックをする場合のタイムアウトをマイクロ秒単位で指定します。

#### 返り値

- IsLocked  
  成功すればtrueを返します。失敗した場合にはfalseを返します。
- Dump  
  成功すればtrueを返します。失敗した場合にはfalseを返します。
- 上記以外  
  ロックの取得、開放に成功した場合には0を返します。取得できない、開放できない場合にはエラー番号を返します。

#### 注意

C言語インターフェースに対応しています。

リーダライタロックをファイルディスクリプタに関連せず利用する場合には、FLCK_RWLOCK_NO_FD マクロを利用して擬似fdを生成し、指定してください。（C言語インターフェースの fullock_rwlock の説明を参照してください。）

<!-- -----------------------------------------------------------　-->
***

### <a name="FLRWLRCSV"> FLRwlRcsvクラス

FULLOCKの提供するリーダライターロックをラップし、同一スレッドによる再帰ロックを可能とする操作クラスです。
<br />
以下は、利用できるメソッドの一覧です。
#### メソッド
- FLRwlRcsv(int fd = FLCK_INVALID_HANDLE, off_t offset = 0, size_t length = 0)
- FLRwlRcsv(int fd, off_t offset, size_t length, bool is_read)
- ~FLRwlRcsv(void)
- bool Lock(int fd, off_t offset, size_t length, bool is_read = true)
- bool Lock(bool is_read)
- bool Lock(void)
- bool ReadLock(void)
- bool WriteLock(void)
- bool Unlock(void)

#### 説明
- コンストラクタ  
  コンストラクタです。  
  コンストラクタにてファイルディスクリプタ、オフセット、領域長、リーダーロックもしくはライターロックを渡し、コンストラクターが実行された時点でロック取得することもできます。
- デストラクタ  
  デストラクタです。  
  ロックを取得している場合にはロックを開放します。
- Lock  
  リーダーライターロックのロックを取得するメソッドです。3種類あります。  
  一つ目は、ファイルディスクリプタ、オフセット、領域長、リーダーロックもしくはライターロックを指定して、ロックを取得します。  
  二つ目は、ファイルディスクリプタ、オフセット、領域長が指定されていることが前提で、リーダーロックもしくはライターロックのみを指定して、ロックを取得します。  
  三つ目は、ファイルディスクリプタ、オフセット、領域長、リーダーロックもしくはライターロックすべてが設定されていることが前提で、ロックを取得します。  
- ReadLock  
  Lockの二つ目と同じで、ファイルディスクリプタ、オフセット、領域長が指定されていることが前提で、リーダロックを取得します。
- WriteLock  
  Lockの二つ目と同じで、ファイルディスクリプタ、オフセット、領域長が指定されていることが前提で、ライターロックを取得します。
- Unlock  
  リーダーライターロックの保持しているロックを開放します。

#### 引数
- fd  
  リーダライターロックでロックする対象ファイルのファイルディスクリプタを指定します。
- offset  
  リーダライターロックでロックする対象ファイルのロック開始位置をファイルの先頭からのオフセットで指定します。
- length  
  リーダライターロックでロックする対象ファイルのオフセットからの領域長を指定します。
- is_read  
  リーダライターロックのリーダーロックの場合trueを指定し、ライターロックの場合にはfalseを指定します。

#### 返り値
ロックの取得、開放に成功した場合にはtrueを返します。取得できない、開放できない場合にはfalseを返します。

#### 注意

このクラスは、FULLOCKのリーダライターロックをラップし、同一スレッドによる再帰ロックを可能とする操作クラスであり、同一ファイルディスクリプタ、オフセット、領域長に対して、同一スレッドからロックを取得してもブロックしません。  
例えば、リーダロックを取得したスレッドで、本クラスの別インスタンスを作り、同領域に対してライターロックを取得することが可能です。  
また、本クラスのオブジェクトは自動ロック解除を行いますので、Unlockを呼び出さずオブジェクトを破棄しても自動的にロックを解除します。  
この利用方法としては、C/C++言語におけるスコープ内でオート変数として本クラスオブジェクトを定義するだけで、スコープ内で安全にロック/ロック開放ができることを意味しています。

<!-- -----------------------------------------------------------　-->
<!-- -----------------------------------------------------------　-->
<!-- -----------------------------------------------------------　-->
<!-- -----------------------------------------------------------　-->
<!-- -----------------------------------------------------------　-->
