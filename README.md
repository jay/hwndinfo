hwndinfo
========

hwndinfo - Show window info and/or monitor the foreground window

### Usage

Usage: `hwndinfo [--fore] [--brief] [[hwndhex] ...]`

Show window info for one or more windows.

~~~
--brief:  Only show brief info.

--fore:   Wait for the user to make each hwnd the foreground.
          When hwnd is the foreground more info may be shown.
          If no hwnd then monitor any change in foreground.
~~~

### Sample output

~~~
C:\Users\Owner>hwndinfo --fore 4b12ea

------------------------ Sun Aug 06 03:31:29.136 PM ------------------------
Waiting for user to bring 004B12EA to the foreground...

------------------------ Sun Aug 06 03:31:31.256 PM ------------------------
HWND: 004B12EA
pid: 10224 (notepad.exe)
tid: 6008
Name: "Untitled - Notepad"
Class name: "Notepad"

IsIconic(hwnd): FALSE
IsZoomed(hwnd): FALSE
IsWindowVisible(hwnd): TRUE

guithreadinfo.flags: GUI_CARETBLINKING
guithreadinfo.hwndActive: 004B12EA
guithreadinfo.hwndFocus: 00591096
guithreadinfo.hwndCapture: 00000000
guithreadinfo.hwndMenuOwner: 00000000
guithreadinfo.hwndMoveSize: 00000000
guithreadinfo.hwndCaret: 00591096
guithreadinfo.rcCaret: (5, 1)-(6, 19)

window_placement.showCmd: SW_SHOWNORMAL
window_placement.ptMaxPosition: (-1, -1)
window_placement.ptMinPosition: (0, 0)
window_placement.rcNormalPosition: (190, 53)-(1226, 652)

window_rect: (190, 53)-(1226, 652)
client_rect: (0, 0)-(1020, 540)

MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL): 00010001

SPI_GETFOREGROUNDLOCKTIMEOUT: 4846544 (4846 seconds)
~~~

Other
-----

### License

hwndinfo is free software and it is licensed under the
[GNU General Public License version 3 (GPLv3)](https://github.com/jay/hwndinfo/blob/master/License_GPLv3.txt),
a license that will keep it free. You may not remove my copyright or the
copyright of any contributors under the terms of the license. The source code
for hwndinfo cannot be used in proprietary software, but you can for example
execute a free software application from a proprietary software application.
**In any case please review the GPLv3 license, which is designed to protect
freedom, not take it away.**

### Source

The source can be found on
[GitHub](https://github.com/jay/hwndinfo).
Since you're reading this maybe you're already there?

### Send me any questions you have

Jay Satiro `<raysatiro$at$yahoo{}com>` and put 'hwndinfo' in the subject.
