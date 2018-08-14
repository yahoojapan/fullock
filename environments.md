---
layout: contents
language: en-us
title: Environments
short_desc: Fast User Level LOCK library
lang_opp_file: environmentsja.html
lang_opp_word: To Japanese
prev_url: developer.html
prev_string: Developer
top_url: index.html
top_string: TOP
next_url: 
next_string: 
---

# Environments
Programs linked with FULLOCK are automatically initialized to require FULLOCK at startup.  
For this initialization processing, you can specify environment variables to change processing contents and settings from outside.  
A list of environment variables and descriptions are shown below.  
<br />
- FLCKAUTOINIT	
 - NO  
   Specify that FULLOCK should not initialize at startup. For the value, specify "NO".
   When this setting is made, calls fullock_reinitialize () or fullock_reinitialize_ex () before using the reader writer lock and named mutex to initialize (or attach) the shared memory.  
   You can switch shared memory files without specifying this environment variable.
   If this environment variable is specified, it will only use (do not create) the shared memory file that FULLOCK will attach (create) by default.
- FLCKROBUSTMODE	
 - NO / LOW / HIGH  
   Specify the ROBUST mode provided by FULLOCK. For the value, specify "NO", "LOW", or "HIGH".
   For details of the mode, refer to "Detailed explanation".
- FLCKNOMAPMODE	
 - ALLOW_NORETRY / DENY_NORETRY / ALLOW_RETRY / DENY_RETRY / ALLOW / DENY  
   Specify the operation when FULLOCK is not attached (created) to the shared memory file. For the value, specify "ALLOW_NORETRY", "DENY_NORETRY", "ALLOW_RETRY", "DENY_RETRY", "ALLOW" (equivalent to "ALLOW_NORETRY"), or "DENY" (equivalent to "DENY_NORETRY").
   For details of the mode, refer to "Detailed explanation".
- FLCKFREEUNITMODE	
  - NO / FD / OFFSET
   Specify the management method of resources (number of files in reader / writer lock, (offset + area length)) of FULLOCK, that is, resource release timing.  
   For the value, specify "NO", "FD", "OFFSET", or "ALWAYS" (equivalent to "FD").    
   If "NO", resources once used are not released (same values are reused).  
   In the case of "FD" it is released and reused when it is no longer used (there is no lock holding / waiting in the object).  
   In the case of "OFFSET", only the offset + area length which is no longer used (lock hold / wait does not exist) is released and reused (the file is not released).  
   For details of the mode, refer to "Detailed explanation".
- FLCKROBUSTCHKCNT
  - Numerical value (decimal)  
   Sets the processing frequency for deadlock detection when FULLOCK is operating in high ROBUST mode.
   The value specifies the number of times (whether deadlock detection should be performed if it can not acquire lock acquisition number of times).
   If you do not specify a large number (default 5000) to some extent, it becomes unnecessary load and affects performance.  
   You should not change the value with this environment variable.
   For details of the mode, refer to "Detailed explanation".
- FLCKUMASK
 - Number (decimal, hexadecimal, octal)  
   You can specify the UMASK value that affects the permissions of shared memory files created by FULLOCK. (It is 0 by default.)  
   Before 1.0.7, it will be affected by the umask of the process of first creating the shared memory file, but it is default 0 after 1.0.8. 
- FLCKDIRPATH
 - Directory path  
   Specify the directory path of the shared memory file used by FULLOCK.
   The default is "/tmp/.fullock".   
- FLCKFILENAME
 - file name  
   Specify the file name of the shared memory file used by FULLOCK.
   The default is "fullock.shm".   
- FLCKFILECNT
 - Numerical value (decimal)  
   Specify the number of files in the reader writer lock among the resources of FULLOCK.
   The default is 128.
- FLCKOFFETCNT
 - Numerical value (decimal)  
   Specify the number of (offset + area length) in reader writer lock among resources possessed by FULLOCK.
   The default is 8192.   
- FLCKLOCKERCNT
 - Numerical value (decimal)  
   Specify the number of processes (threads) to hold / wait at the same time that are used in the reader writer lock and named mutex among the resources of FULLOCK.
   The default is 8192.
- FLCKNMTXCNT
 - Numerical value (decimal)  
   Specify the number of named mutexes among the resources of FULLOCK.
   The default is 256.
- FLCKNCONDCNT
 - Numerical value (decimal)  
   Specify the number of named condition variables among the resources of FULLOCK.
   The default is 256.
- FLCKWAITERCNT
 - Numerical value (decimal)  
   Specify the number of total processes (threads) that can be waited at the same time that are used in the named condition variable among the resources of FULLOCK.
   The default is 8192.
- FLCKDBGMODE
 - SILENT / ERR / WAN / INFO  
   Specify the level of debug output. For the value, specify "SILENT", "ERR", "WAN", "INFO".
   The default is "SILENT".
- FLCKDBGFILE
 - File Path  
   Specify the output file for debug output.
   The default is unspecified, and it is standard error output.
