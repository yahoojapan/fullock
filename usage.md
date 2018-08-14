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

# How to linking
You can use it by linking the FULLOCK library to the program.

When you start the program linking the FULLOCK library, shared memory is automatically created and ready for use.  
When the program uses the function of FULLOCK, you do not need to initialize about FULLOCK, and you can immediately call and use FULLOCK I/F.

If you want to change the default setting(the file path to shared memory, etc.), you can change those by calling I/F or FULLOCK environment.  
It can be customized program-specific by setting before using FULLOCK function.
