# simple-netstat

At first I wanted little program, that shutdown my pc when all night downloads have been completed. However, it grow to simple netstat.


# Options
-----------
* __Interface:__ network interface to monitor. `cat /sys/class/net/` to find out what interfaces available.
* __Interval:__ [seconds] how often program will read network data from sys files.
* __Idle speed:__ [kilobytes per interval] how many kilobytes is transffered in given interval. 
* __Positive idle speed passes:__ how many times in a row program find out that notwork traffic is below Idle Speed, and then shutdown pc. This option is ignored in Monitor mode.
* __Verbose:__ [0-1] be verbose or only log file?
* __Monitor mode:__ [0-1] only watch network traffic, disables shutdown feature.


# Log file
-----------
Log file is something like this (ignore second line):

	Interface: wlan0  interval: 5 idleSpeed: 250
	12:47:20;18884;2813;0;0
	12:47:25;0;0;0;0
	12:47:30;0;0;0;0
	12:47:35;4;0;0;0
	12:47:40;0;0;0;0
	12:47:45;0;0;0;0
	12:47:50;0;1;0;0
	12:47:55;4;0;0;0

format:

	hour;downloaded kB/interval;uploaded kB/interval;download speed below Idle Speed;upload speed below Idle Speed

You can paste log file to Calc or Excel to make charts or stuff like that.


# Build
-----------
Type: `cmake src/ && make`
