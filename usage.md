---
layout: contents
language: en-us
title: Usage
short_desc: Fast User Level LOCK library
lang_opp_file: usageja.html
lang_opp_word: To Japanese
prev_url: feature.html
prev_string: Feature
top_url: index.html
top_string: TOP
next_url: build.html
next_string: Build
---

# Usage

## Creating a usage environment
There are two ways to install FULLOCK library in your environment.  
One is to download and install the package of FULLOCK library from [packagecloud.io](https://packagecloud.io/).  
The other way is to build and install FULLOCK library from source code yourself.  
These methods are described below.  

### Installing packages
The FULLOCK library publishes [packages](https://packagecloud.io/app/antpickax/stable/search?q=fullock) in [packagecloud.io - AntPickax stable repository](https://packagecloud.io/app/antpickax/stable) so that anyone can use it.  
The package of the FULLOCK library is released in the form of Debian package, RPM package.  
Since the installation method differs depending on your OS, please check the following procedure and install it.  

#### Debian(Stretch) / Ubuntu(Bionic Beaver)
```
$ sudo apt-get update -y
$ sudo apt-get install curl -y
$ curl -s https://packagecloud.io/install/repositories/antpickax/stable/script.deb.sh | sudo bash
$ sudo apt-get install libfullock
```
To install the developer package, please install the following package.
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
To install the developer package, please install the following package.
```
$ sudo apt-get install libfullock-devel
```

#### Other OS
If you are not using the above OS, packages are not prepared and can not be installed directly.  
In this case, build from the [source code](https://github.com/yahoojapan/fullock) described below and install it.

### Build and install from source code
For details on how to build and install FULLOCK library from [source code](https://github.com/yahoojapan/fullock), please see [Build](https://fullock.antpick.ax/build.html).

## How to linking
You can use it by linking the FULLOCK library to the program.

When you start the program linking the FULLOCK library, shared memory is automatically created and ready for use.  
When the program uses the function of FULLOCK, you do not need to initialize about FULLOCK, and you can immediately call and use FULLOCK I/F.

If you want to change the default setting(the file path to shared memory, etc.), you can change those by calling I/F or FULLOCK environment.  
It can be customized program-specific by setting before using FULLOCK function.
