/* hwndinfo - Show window info and/or monitor the foreground window

Usage: hwndinfo [--fore] [--brief] [[hwndhex] ...]
Show window info for one or more windows.

--brief:  Only show brief info.

--fore:   Wait for the user to make each hwnd the foreground.
          When hwnd is the foreground more info may be shown.
          If no hwnd then monitor any change in foreground.

cl /W4 /wd4127 hwndinfo.cpp user32.lib
g++ -Wall -std=gnu++11 -o hwndinfo hwndinfo.cpp

https://github.com/jay/hwndinfo
*//*
Copyright (C) 2017 Jay Satiro <raysatiro@yahoo.com>
All rights reserved.

This file is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This file is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

<https://www.gnu.org/licenses/#GPL>
*/

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x501
#endif

#ifndef WINVER
#define WINVER _WIN32_WINNT
#endif

#include <Windows.h>
#include <TlHelp32.h>

#include <assert.h>
#include <limits.h>
#include <stdio.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

#ifndef _MSC_VER
#define __pragma(x)
#endif

#ifndef GW_ENABLEDPOPUP
#define GW_ENABLEDPOPUP 6
#endif

#ifndef INVALID_HWND_VALUE
#define INVALID_HWND_VALUE ((HWND)(LONG_PTR)-1)
#endif

#if defined(_MSC_VER) && (_MSC_VER < 1800)
#define strtoll _strtoi64
#define strtoull _strtoui64
#endif

#define WHILE_FALSE \
  __pragma(warning(push)) \
  __pragma(warning(disable:4127)) \
  while(0) \
  __pragma(warning(pop))

#define PRINT(x) \
  cout << #x ": " << x << endl;

// for bool functions as well
#define PRINTBOOL(x) \
  do { \
    BOOL b = x; \
    cout << #x ": " << (b ? "TRUE" : "FALSE") << endl; \
  } WHILE_FALSE

#define PRINTPOINT(point) \
  cout << #point << ": " << "(" << point.x << ", " << point.y << ")" << endl;

#define PRINTRECT(rect) \
  cout << #rect << ": " << "(" << rect.left << ", " << rect.top << ")-" \
       << "(" << rect.right << ", " << rect.bottom << ")" << endl;

#define PRINTBOOLFUNC_IF_FAIL(x) \
  do { \
    BOOL b = x; \
    if(!b) { \
      DWORD gle = GetLastError(); \
      cout << #x ": FALSE (GetLastError: " << gle << ")" << endl; \
    } \
    boolfunc_failed = !b; \
  } WHILE_FALSE

/* system time in format: Tue May 16 03:24:31.123 PM */
string SystemTimeStr(const SYSTEMTIME *t)
{
  const char *dow[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
  const char *mon[] = { NULL, "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
  stringstream ss;

  unsigned t_12hr = (t->wHour > 12 ? t->wHour - 12 : t->wHour ? t->wHour : 12);
  const char *t_ampm = (t->wHour < 12 ? "AM" : "PM");

  ss.fill('0');
  ss << dow[t->wDayOfWeek] << " "
     << mon[t->wMonth] << " "
     << setw(2) << t->wDay << " "
     << setw(2) << t_12hr << ":"
     << setw(2) << t->wMinute << ":"
     << setw(2) << t->wSecond << "."
     << setw(3) << t->wMilliseconds << " "
     << t_ampm;

  return ss.str();
}

string now()
{
  SYSTEMTIME st;
  GetLocalTime(&st);
  return SystemTimeStr(&st);
}

/* The timestamp style in verbose mode:
--- Sun May 28 07:00:55.999 PM ---
text
*/
#define TIMESTAMPED_HEADER \
  "\n------------------------ " << now() << " ------------------------\n"

/* The timestamp style in default mode: [Sun May 28 07:00:27.999 PM]: text */
#define TIMESTAMPED_PREFIX \
  "[" << now() << "]: "

string GetProcessName(DWORD pid)
{
  string name;
  BOOL b;
  HANDLE snapshot;
  PROCESSENTRY32 pe = { sizeof pe, };

  snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

  if(snapshot == INVALID_HANDLE_VALUE)
    return "";

  for(b = Process32First(snapshot, &pe); b; b = Process32Next(snapshot, &pe)) {
    if(pid == pe.th32ProcessID) {
      name = pe.szExeFile;
      break;
    }
  }

  CloseHandle(snapshot);
  return name;
}

string WindowText(HWND hwnd)
{
  char buffer[255];

  if(!GetWindowText(hwnd, buffer, sizeof buffer))
    return "";

  return buffer;
}

string WindowClassName(HWND hwnd)
{
  char buffer[255];

  if(!GetClassName(hwnd, buffer, sizeof buffer))
    return "";

  return buffer;
}

#define SHOW_WINDOW_NAMES(x) \
  cout << "Name: \"" << WindowText(x) << "\"" << endl; \
  cout << "Class name: \"" << WindowClassName(x) << "\"" << endl;

// show brief info (eg no coordinates) instead of normal
#define INFO_BRIEF                 (1<<0)

// wait for the user to make the specified hwnd the foreground window
#define INFO_WAIT_HWND_FOREGROUND  (1<<1)

// show window info for every GetForegroundWindow change
#define INFO_MONITOR_FOREGROUND    (1<<2)

void hwndinfo(HWND hwnd, DWORD dwFlags = 0)
{
  HWND hFore = INVALID_HWND_VALUE;
  HWND hForePrev = INVALID_HWND_VALUE;

  bool firstrun = true;
  bool lastrun = false;
  for(; !lastrun; firstrun = false) {
    if(!firstrun) {
      hForePrev = hFore;
      Sleep(100);
    }

    hFore = GetForegroundWindow();

    if((dwFlags & INFO_WAIT_HWND_FOREGROUND)) {
      if(firstrun) {
        cout << TIMESTAMPED_HEADER
             << "Waiting for user to bring " << hwnd
             << " to the foreground..." << endl;
      }

      if(hFore != hwnd)
        continue;
    }

    if((dwFlags & INFO_MONITOR_FOREGROUND)) {
      if(firstrun) {
        cout << TIMESTAMPED_HEADER
             << "Monitoring GetForegroundWindow for changes..." << endl;
      }

      if(hFore == hForePrev)
        continue;

      hwnd = hFore;
    }
    else
      lastrun = true;

    cout << TIMESTAMPED_HEADER;

    bool boolfunc_failed = false;

    DWORD pid = 0;
    DWORD tid = GetWindowThreadProcessId(hwnd, &pid);
    if(!tid) {
      cout << "Invalid hwnd " << hwnd << endl;
      continue;
    }

    cout << "HWND: " << hwnd << endl;
    cout << "pid: " << pid << " (" << GetProcessName(pid) << ")" << endl;
    PRINT(tid);

    SHOW_WINDOW_NAMES(hwnd);

    cout << endl;

    HWND hRootOwner = GetAncestor(hwnd, GA_ROOTOWNER);
    if(hRootOwner && hRootOwner != hwnd) {
      cout << "GA_ROOTOWNER: " << hRootOwner << endl;
      SHOW_WINDOW_NAMES(hRootOwner);
      cout << endl;
    }

    HWND hEnabledPopup = GetWindow(hwnd, GW_ENABLEDPOPUP);
    if(hEnabledPopup && hEnabledPopup != hwnd) {
      cout << "GW_ENABLEDPOPUP: " << hEnabledPopup << endl;
      SHOW_WINDOW_NAMES(hEnabledPopup);
      cout << endl;
    }

    PRINTBOOL(IsIconic(hwnd));
    PRINTBOOL(IsZoomed(hwnd));
    PRINTBOOL(IsWindowEnabled(hwnd));
    PRINTBOOL(IsWindowVisible(hwnd));

    if((dwFlags & INFO_BRIEF))
      continue;

    cout << endl;

    GUITHREADINFO guithreadinfo, gti2;
    memset(&guithreadinfo, 0, sizeof guithreadinfo);
    guithreadinfo.cbSize = sizeof guithreadinfo;
    memcpy(&gti2, &guithreadinfo, sizeof guithreadinfo);
    PRINTBOOLFUNC_IF_FAIL(GetGUIThreadInfo(tid, &guithreadinfo));
    /* Usually this is empty unless the thread is the foreground, or the active
       hwnd is stored for a minimized window. It fails with error 87 invalid
       paramter when called on non-admin command windows, regardless of whether
       or not this program as run as admin. */
    if(!boolfunc_failed) {
      if(!memcmp(&gti2, &guithreadinfo, sizeof guithreadinfo)) {
        cout << "guithreadinfo is empty" << endl;
      }
      else {
        stringstream ss;
        DWORD flags = guithreadinfo.flags;

#define EXTRACT_GTI_FLAG(f) \
  if((flags & f)) { \
    if(ss.tellp() > 0) \
      ss << " | "; \
    ss << #f; \
    flags &= ~f; \
  }

        EXTRACT_GTI_FLAG(GUI_CARETBLINKING);
        EXTRACT_GTI_FLAG(GUI_INMENUMODE);
        EXTRACT_GTI_FLAG(GUI_INMOVESIZE);
        EXTRACT_GTI_FLAG(GUI_POPUPMENUMODE);
        EXTRACT_GTI_FLAG(GUI_SYSTEMMENUMODE);
        if(flags) {
          if(ss.tellp() > 0)
            ss << " | ";
          ss << "0x" << hex << flags;
        }
        if(ss.tellp() <= 0)
          ss << "<none>";
        cout << "guithreadinfo.flags: " << ss.str() << endl;

        PRINT(guithreadinfo.hwndActive);
        PRINT(guithreadinfo.hwndFocus);
        PRINT(guithreadinfo.hwndCapture);
        PRINT(guithreadinfo.hwndMenuOwner);
        PRINT(guithreadinfo.hwndMoveSize);
        PRINT(guithreadinfo.hwndCaret);
        PRINTRECT(guithreadinfo.rcCaret);
      }
    }

    cout << endl;

    WINDOWPLACEMENT window_placement = { sizeof(window_placement), };
    PRINTBOOLFUNC_IF_FAIL(GetWindowPlacement(hwnd, &window_placement));
    if(!boolfunc_failed) {
      cout << "window_placement.showCmd: ";
      switch(window_placement.showCmd) {
      case SW_SHOWMAXIMIZED: cout << "SW_SHOWMAXIMIZED"; break;
      case SW_SHOWMINIMIZED: cout << "SW_SHOWMINIMIZED"; break;
      case SW_SHOWNORMAL:    cout << "SW_SHOWNORMAL"; break;
      default:               cout << "<unknown>"; break;
      }
      cout << endl;

      PRINTPOINT(window_placement.ptMaxPosition);
      PRINTPOINT(window_placement.ptMinPosition);
      PRINTRECT(window_placement.rcNormalPosition);
    }

    cout << endl;

    RECT window_rect;
    PRINTBOOLFUNC_IF_FAIL(GetWindowRect(hwnd, &window_rect));
    if(!boolfunc_failed)
      PRINTRECT(window_rect);

    RECT client_rect;
    PRINTBOOLFUNC_IF_FAIL(GetClientRect(hwnd, &client_rect));
    if(!boolfunc_failed)
      PRINTRECT(client_rect);

    cout << endl;

    PRINT(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL));

    cout << endl;

    DWORD lt = (DWORD)-1;
    PRINTBOOLFUNC_IF_FAIL(
      SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &lt, 0));
    if(!boolfunc_failed) {
      cout << "SPI_GETFOREGROUNDLOCKTIMEOUT: " << lt
           << " (" << lt / 1000 << " seconds)" << endl;
    }
  }
}

void ShowUsage()
{
cerr <<
"\nUsage: hwndinfo [--fore] [--brief] [[hwndhex] ...]\n"
"\n"
"Show window info for one or more windows.\n"
"\n"
"--brief:  Only show brief info.\n"
"\n"
"--fore:   Wait for the user to make each hwnd the foreground.\n"
"          When hwnd is the foreground more info may be shown.\n"
"          If no hwnd then monitor any change in foreground.\n"
"\n"
"The hwndinfo source can be found at https://github.com/jay/hwndinfo\n"
;
}

int main(int argc, char *argv[])
{
  if(argc < 2) {
    ShowUsage();
    exit(1);
  }

  bool opt_fore = false;
  bool opt_brief = false;

  vector<HWND> windows;

  for(int i = 1; i < argc; ++i) {
    if(!strcmp(argv[i], "--help")) {
      ShowUsage();
      exit(1);
    }
    else if(!strcmp(argv[i], "--fore")) {
      opt_fore = true;
    }
    else if(!strcmp(argv[i], "--brief")) {
      opt_brief = true;
    }
    else if(argv[i][0] == '-') {
      cerr << "Unrecognized option: " << argv[i] << endl;
      exit(1);
    }
    else {
      __int64 h = strtoll(argv[i], NULL, 16);
      if(!h || h == LLONG_MAX || h == LLONG_MIN) {
        cerr << "Unrecognized hwnd: " << argv[i] << endl;
        exit(1);
      }
      windows.push_back((HWND)h);
    }
  }

  DWORD flags = 0;

  if(opt_brief)
    flags |= INFO_BRIEF;

  if(opt_fore) {
    if(!windows.size()) {
      flags |= INFO_MONITOR_FOREGROUND;
      hwndinfo(INVALID_HWND_VALUE, flags);
      return 0;
    }

    flags |= INFO_WAIT_HWND_FOREGROUND;
  }

  for(size_t i = 0; i < windows.size(); ++i)
    hwndinfo(windows[i], flags);

  return 0;
}
