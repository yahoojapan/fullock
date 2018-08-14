---
layout: contents
language: ja
title: Usage
short_desc: Fast User Level LOCK library
lang_opp_file: usage.html
lang_opp_word: To English
prev_url: featureja.html
prev_string: Feature
top_url: indexja.html
top_string: TOP
next_url: buildja.html
next_string: Build
---

# 使い方
FULLOCKライブラリをプログラムにリンクするだけで利用できます。
  
FULLOCKライブラリをリンクしたプログラムを起動すると、自動的に共有メモリが作成され、利用できる準備がなされます。  
プログラムでFULLOCKを利用する時には、初期化などは不要であり、そのままFULLOCKのI/Fを呼び出し利用できます。

初期設定（共有メモリへのファイルパス、他）を変更する場合には、変更用のI/F、環境変数が準備されていますので、利用を開始する前に設定することでプログラム固有でカスタマイズできます。
