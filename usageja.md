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

## 利用環境構築
FULLOCKライブラリをご利用の環境にインストールするには、2つの方法があります。  
ひとつは、[packagecloud.io](https://packagecloud.io/)からFULLOCKライブラリのパッケージをダウンロードし、インストールする方法です。  
もうひとつは、ご自身でFULLOCKライブラリをソースコードからビルドし、インストールする方法です。  
これらの方法について、以下に説明します。

### パッケージのインストール
FULLOCKライブラリは、誰でも利用できるように[packagecloud.io - AntPickax stable repository](https://packagecloud.io/antpickax/stable/)で[パッケージ](https://packagecloud.io/app/antpickax/stable/search?q=fullock)を公開しています。  
FULLOCKライブラリのパッケージは、Debianパッケージ、RPMパッケージの形式で公開しています。  
お使いのOSによりインストール方法が異なりますので、以下の手順を確認してインストールしてください。  

#### Debian(Stretch) / Ubuntu(Bionic Beaver)
```
$ sudo apt-get update -y
$ sudo apt-get install curl -y
$ curl -s https://packagecloud.io/install/repositories/antpickax/stable/script.deb.sh | sudo bash
$ sudo apt-get install libfullock
```
開発者向けパッケージをインストールする場合は、以下のパッケージをインストールしてください。
```
$ sudo apt-get install libfullock-dev
```

#### Fedora28 / CentOS7.x(6.x)
```
$ sudo yum makecache
$ sudo yum install curl -y
$ curl -s https://packagecloud.io/install/repositories/antpickax/stable/script.rpm.sh | sudo bash
$ sudo yum install libfullock
```
開発者向けパッケージをインストールする場合は、以下のパッケージをインストールしてください。
```
$ sudo apt-get install libfullock-devel
```

#### 上記以外のOS
上述したOS以外をお使いの場合は、パッケージが準備されていないため、直接インストールすることはできません。  
この場合には、後述の[ソースコード](https://github.com/yahoojapan/fullock)からビルドし、インストールするようにしてください。

### ソースコードからビルド・インストール
FULLOCKライブラリを[ソースコード](https://github.com/yahoojapan/fullock)からビルドし、インストールする方法は、[ビルド](https://fullock.antpick.ax/buildja.html)を参照してください。

## 利用方法
FULLOCKライブラリをプログラムにリンクするだけで利用できます。  

FULLOCKライブラリをリンクしたプログラムを起動すると、自動的に共有メモリが作成され、利用できる準備がなされます。  
プログラムでFULLOCKを利用する時には、初期化などは不要であり、そのままFULLOCKのI/Fを呼び出し利用できます。

初期設定（共有メモリへのファイルパス、他）を変更する場合には、変更用のI/F、環境変数が準備されていますので、利用を開始する前に設定することでプログラム固有でカスタマイズできます。
