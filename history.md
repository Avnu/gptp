gptp: Add monotonic raw clock option AVnu/OpenAvnu#795
Fix the dead lock when handling SYNC_RECEIPT_TIMEOUT_EXPIRES event AVnu/OpenAvnu#779
Wrong seqId used in log message AVnu/OpenAvnu#783
gptp : remove unnecessary sequence that try to delete pdelay request object. AVnu/OpenAvnu#780
gptp: fix race condition during signal thread creation AVnu/OpenAvnu#750
Start PDelay calculation as fast as possible in Automotive Profile AVnu/OpenAvnu#748
Add brackets fixing broken if statement AVnu/OpenAvnu#745
gptp: fixing/adding logging AVnu/OpenAvnu#744
Added wireless timestamper and port code AVnu/OpenAvnu#734
Fixing stale Tx timestamp fetch. AVnu/OpenAvnu#728
RFC: Simplify timeval & timespec redefinition fix AVnu/OpenAvnu#707
Fixes for compile breakage AVnu/OpenAvnu#705
Working appveyor AVnu/OpenAvnu#702
listen for announces on init and linkup AVnu/OpenAvnu#691
gPTP Announce interval timer restart fixed AVnu/OpenAvnu#688
PTP Port role restore on Link Up event AVnu/OpenAvnu#683
SyncIntervalTimer lock changed to trylock AVnu/OpenAvnu#678
Rest of fix for gPTP intervals on ARM AVnu/OpenAvnu#646
Fix for gPTP interval init on ARM platforms AVnu/OpenAvnu#641
RFC: Fix Priority1 check in announce timeout AVnu/OpenAvnu#635
gPTP: Windows: MSVC compile fixes AVnu/OpenAvnu#621
gPTP: fix SIOCGIFNAME ioctl failure AVnu/OpenAvnu#605
gPTP: change HWTimestamper_adjclockrate into const function AVnu/OpenAvnu#603
Removed clock dependence on timestamper - necessary for multiport AVnu/OpenAvnu#598
gPTP: add negative time jumps detection AVnu/OpenAvnu#595
Ignore twoStepFlag as per Table 11-4 of 802.1AS-2011 AVnu/OpenAvnu#594
Added AVDECC information to gPTP daemon AVnu/OpenAvnu#593
Windows 10 detection fix AVnu/OpenAvnu#591
Change to the gPTP daemon Windows support so that the Windows 8 SDK i… AVnu/OpenAvnu#586
Separate common port functionality from Ethernet port functionality AVnu/OpenAvnu#585
Systemd watchdog AVnu/OpenAvnu#575
gPTP: add linkUp flag initialization by interface state AVnu/OpenAvnu#566
gPTP: redundant (false) LINKUP events filtering AVnu/OpenAvnu#563
gPTP: use message type in combination with sequence ID to uniquely id… AVnu/OpenAvnu#501
gptp: fix wrong Follow_up Correction Field value in GPTP_LOG_VERBOSE AVnu/OpenAvnu#551
FIxing gptp Makefile - precise time test using CC  AVnu/OpenAvnu#546
Update avbts_osnet.hpp AVnu/OpenAvnu#536
Support 32/64 bits build AVnu/OpenAvnu#528
Fix proposal to the issue 490 AVnu/OpenAvnu#526
Add optional GENIVI DLT (logging) to gPTP AVnu/OpenAvnu#515
Fix gPTP link up/down detection. Check only the interface used by port AVnu/OpenAvnu#512
gPTP: IEEE1588Port::processEvent(), start and stop pDelay on LINKUP/D… AVnu/OpenAvnu#489
gPTP: remove accelerated SYNC send on startup. See discussion in #465. AVnu/OpenAvnu#486
Fixes to Makefiles and compiler warnings and errors AVnu/OpenAvnu#477
gPTP: Log output and Dox comment updates only AVnu/OpenAvnu#483
gPTP: fix MSVC compiler warnings AVnu/OpenAvnu#478
Change link up handling of hardware timestamper AVnu/OpenAvnu#469
gPTP - add some NULL  pointer checking AVnu/OpenAvnu#471
gPTP - windows build update to cmdline parameter parsing. See issue #… AVnu/OpenAvnu#466
Fixes to gPTP signaling message TLV values AVnu/OpenAvnu#464
Fixup .ini file parse and add support for linux kernel HW timestamping AVnu/OpenAvnu#462
Fixing issue #390 - gPTP daemon build failure with ARCH set to I210 AVnu/OpenAvnu#451
Bug fixes for gPTP daemon AVnu/OpenAvnu#447
gPTP Fixes and Improvements AVnu/OpenAvnu#434
Updates to gPTP AVnu/OpenAvnu#431
Memory leak AVnu/OpenAvnu#426
Fix default initialLogPdelayReqInterval AVnu/OpenAvnu#425
Fix logic for starting PDelay sending AVnu/OpenAvnu#424
Change all doxygen-style comments to have a @brief AVnu/OpenAvnu#423
Memory leak in buildPTPMessage() AVnu/OpenAvnu#420
Fix for #396 AVnu/OpenAvnu#408
Fix out-of-bounds read in ClockIdentity::set() caused by malformed an… AVnu/OpenAvnu#415
Windows registry info in gPTP readme AVnu/OpenAvnu#411
Merge a rebased and cleaned up version of the feature-gptp-avnu-automotive-profile branch AVnu/OpenAvnu#405
Fix TLV parse offset AVnu/OpenAvnu#394
Fix broken windows code AVnu/OpenAvnu#377
daemons: gPTP, Windows HAL updates so that CMake based build works a… AVnu/OpenAvnu#371
Continue sending announce when priority1 is 255 AVnu/OpenAvnu#372
Feature gptp next AVnu/OpenAvnu#369
daemon_cl: Fixes PPS out-of-phase issue. AVnu/OpenAvnu#333
Srinath88 gptp enhancements AVnu/OpenAvnu#314
Open avb next AVnu/OpenAvnu#296
gptplocaltime() function added. AVnu/OpenAvnu#291
Fixed memory leak where timer descriptor was not deleted; removed red… AVnu/OpenAvnu#259
Fix bogus PHY latency values in gPTP Linux HAL AVnu/OpenAvnu#252
Gptp doc task AVnu/OpenAvnu#221
gPTP: add method to retrieve asCapable from IEEE1588Port class. AVnu/OpenAvnu#212
Fixed AVnu certification test failure 6.1a and 6.3a AVnu/OpenAvnu#177
Add support for Linux "generic" cross timestamp support AVnu/OpenAvnu#171
Open avb next AVnu/OpenAvnu#91
Asi for upstream avb next AVnu/OpenAvnu#88
gtpt/linux: Fix compilation issue related to type redefinition AVnu/OpenAvnu#85
gptp daemon -  suggested fixes/features AVnu/OpenAvnu#82
Misc Changes AVnu/OpenAvnu#80
Fixed network-system clock offset computation AVnu/OpenAvnu#79
Pulled correction from 'main' branch AVnu/OpenAvnu#61
Add support for additional Linux platforms; Improve Windows IPC communication AVnu/OpenAvnu#55
Updated fast startup features AVnu/OpenAvnu#38
Minor fixes of gptp flags AVnu/OpenAvnu#29
Merge open-avb-next to master AVnu/OpenAvnu#21
