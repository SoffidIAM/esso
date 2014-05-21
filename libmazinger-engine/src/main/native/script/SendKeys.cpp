#include <MazingerInternal.h>

#include <MZNcompat.h>
#include <unistd.h>

#include "SendKeys.h"

#define _TXCHAR(x) x
#ifndef WIN32
#define _T(x) x
#endif

#ifndef WIN32
#include <string.h>
#include <stdlib.h>
#include <X11/Xutil.h>
#define INVALIDKEY XK_VoidSymbol
#endif

#ifdef WIN32


/* 
* ----------------------------------------------------------------------------- 
* Copyright (c) 2004 lallous <lallousx86@yahoo.com>
* All rights reserved.
* 
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
* ----------------------------------------------------------------------------- 

SEE http://www.codeproject.com/KB/cpp/sendkeys_cpp_Article.aspx

The Original SendKeys copyright info
---------------------------------------
SendKeys (sndkeys32.pas) routine for 32-bit Delphi.
Written by Ken Henderson
Copyright (c) 1995 Ken Henderson <khen@compuserve.com>

History
----------
04/19/2004
  * Initial version development
04/21/2004
  * Added number of times specifier to special keys
  * Added {BEEP X Y}
  * Added {APPACTIVATE WindowTitle}
  * Added CarryDelay() and now delay works properly with all keys
  * Added SetDelay() method
  * Fixed code in AppActivate that allowed to pass both NULL windowTitle/windowClass

05/21/2004
  * Fixed a bug in StringToVKey() that caused the search for RIGHTPAREN to be matched as RIGHT
  * Adjusted code so it compiles w/ VC6
05/24/2004
  * Added unicode support

Todo
-------
* perhaps add mousecontrol: mousemove+mouse clicks
* allow sending of normal keys multiple times as: {a 10}

*/

const WORD CSendKeys::VKKEYSCANSHIFTON = 0x01;
const WORD CSendKeys::VKKEYSCANCTRLON  = 0x02;
const WORD CSendKeys::VKKEYSCANALTON   = 0x04;
const WORD CSendKeys::INVALIDKEY       = 0xFFFF;

const BYTE CSendKeys::ExtendedVKeys[MaxExtendedVKeys] =
{
    VK_UP, 
    VK_DOWN,
    VK_LEFT,
    VK_RIGHT,
    VK_HOME,
    VK_END,
    VK_PRIOR, // PgUp
    VK_NEXT,  //  PgDn
    VK_INSERT,
    VK_DELETE
};


// Delphi port regexps:
// ---------------------
// search: .+Name:'([^']+)'.+vkey:([^\)]+)\)
// replace: {"\1", \2}
//
// **If you add to this list, you must be sure to keep it sorted alphabetically
// by Name because a binary search routine is used to scan it.**
//
CSendKeys::key_desc_t CSendKeys::KeyNames[CSendKeys::MaxSendKeysRecs] = 
{
  {_T("ADD"), VK_ADD}, 
  {_T("APPS"), VK_APPS},
  {_T("AT"), '@', true},
  {_T("BACKSPACE"), VK_BACK},
  {_T("BKSP"), VK_BACK},
  {_T("BREAK"), VK_CANCEL},
  {_T("BS"), VK_BACK},
  {_T("CAPSLOCK"), VK_CAPITAL},
  {_T("CARET"), '^', true},
  {_T("CLEAR"), VK_CLEAR},
  {_T("DECIMAL"), VK_DECIMAL}, 
  {_T("DEL"), VK_DELETE},
  {_T("DELETE"), VK_DELETE},
  {_T("DIVIDE"), VK_DIVIDE}, 
  {_T("DOWN"), VK_DOWN},
  {_T("END"), VK_END},
  {_T("ENTER"), VK_RETURN},
  {_T("ESC"), VK_ESCAPE},
  {_T("ESCAPE"), VK_ESCAPE},
  {_T("F1"), VK_F1},
  {_T("F10"), VK_F10},
  {_T("F11"), VK_F11},
  {_T("F12"), VK_F12},
  {_T("F13"), VK_F13},
  {_T("F14"), VK_F14},
  {_T("F15"), VK_F15},
  {_T("F16"), VK_F16},
  {_T("F2"), VK_F2},
  {_T("F3"), VK_F3},
  {_T("F4"), VK_F4},
  {_T("F5"), VK_F5},
  {_T("F6"), VK_F6},
  {_T("F7"), VK_F7},
  {_T("F8"), VK_F8},
  {_T("F9"), VK_F9},
  {_T("HELP"), VK_HELP},
  {_T("HOME"), VK_HOME},
  {_T("INS"), VK_INSERT},
  {_T("LEFT"), VK_LEFT},
  {_T("LEFTBRACE"), '{', true},
  {_T("LEFTPAREN"), '(', true},
  {_T("LWIN"), VK_LWIN},
  {_T("MULTIPLY"), VK_MULTIPLY}, 
  {_T("NUMLOCK"), VK_NUMLOCK},
  {_T("NUMPAD0"), VK_NUMPAD0}, 
  {_T("NUMPAD1"), VK_NUMPAD1}, 
  {_T("NUMPAD2"), VK_NUMPAD2}, 
  {_T("NUMPAD3"), VK_NUMPAD3}, 
  {_T("NUMPAD4"), VK_NUMPAD4}, 
  {_T("NUMPAD5"), VK_NUMPAD5}, 
  {_T("NUMPAD6"), VK_NUMPAD6}, 
  {_T("NUMPAD7"), VK_NUMPAD7}, 
  {_T("NUMPAD8"), VK_NUMPAD8}, 
  {_T("NUMPAD9"), VK_NUMPAD9}, 
  {_T("PERCENT"), '%', true},
  {_T("PGDN"), VK_NEXT},
  {_T("PGUP"), VK_PRIOR},
  {_T("PLUS"), '+', true},
  {_T("PRTSC"), VK_PRINT},
  {_T("RIGHT"), VK_RIGHT},
  {_T("RIGHTBRACE"), '}', true},
  {_T("RIGHTPAREN"), ')', true},
  {_T("RWIN"), VK_RWIN},
  {_T("SCROLL"), VK_SCROLL},
  {_T("SEPARATOR"), VK_SEPARATOR}, 
  {_T("SNAPSHOT"), VK_SNAPSHOT},
  {_T("SUBTRACT"), VK_SUBTRACT}, 
  {_T("TAB"), VK_TAB},
  {_T("TILDE"), '~', true}, 
  {_T("UP"), VK_UP},
  {_T("WIN"), VK_LWIN}
};


// calls keybd_event() and waits, if needed, till the sent input is processed
void CSendKeys::KeyboardEvent(BYTE VKey, BYTE ScanCode, LONG Flags)
{
  MSG KeyboardMsg;

  keybd_event(VKey, ScanCode, Flags, 0);

  if (m_bWait)
  {
    while (::PeekMessage(&KeyboardMsg, 0, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
    {
      ::TranslateMessage(&KeyboardMsg);
      ::DispatchMessage(&KeyboardMsg);
    }
  }
}

// Checks whether the specified VKey is an extended key or not
bool CSendKeys::IsVkExtended(BYTE VKey)
{
  for (int i=0;i<MaxExtendedVKeys;i++)
  {
    if (ExtendedVKeys[i] == VKey)
      return true;
  }
  return false;
}

// Generates KEYUP
void CSendKeys::SendKeyUp(int VKey)
{
  BYTE ScanCode = LOBYTE(::MapVirtualKey(VKey, 0));

  KeyboardEvent(VKey, 
                ScanCode, 
                KEYEVENTF_KEYUP | (IsVkExtended(VKey) ? KEYEVENTF_EXTENDEDKEY : 0));
}

void CSendKeys::SendKeyDown(int VKey, int NumTimes, bool GenUpMsg, bool bDelay)
{
  WORD Cnt = 0;
  BYTE ScanCode = 0;
  bool NumState = false;

  if (VKey == VK_NUMLOCK)
  {
    DWORD dwVersion = ::GetVersion();

    for (Cnt=1; Cnt<=NumTimes; Cnt++)
    {
      if (bDelay)
        CarryDelay();
      // snippet based on:
      // http://www.codeproject.com/cpp/togglekeys.asp
      if (dwVersion < 0x80000000)
      {
        ::keybd_event(VKey, 0x45, KEYEVENTF_EXTENDEDKEY, 0);
        ::keybd_event(VKey, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
      }
      else
      {
        // Win98 and later
        if ( ((DWORD)(HIBYTE(LOWORD(dwVersion))) >= 10) )
        {
          // Define _WIN32_WINNT > 0x0400
          // to compile
          INPUT input[2];
          input[0].type = input[1].type = INPUT_KEYBOARD;
          input[0].ki.wVk = input[1].ki.wVk = VKey;
          input[1].ki.dwFlags = KEYEVENTF_KEYUP;
          ::SendInput(sizeof(input) / sizeof(INPUT), input, sizeof(INPUT));
        }
        // Win95
        else
        {
          KEYBOARDSTATE_t KeyboardState;
          NumState = GetKeyState(VK_NUMLOCK) & 1 ? true : false;
          GetKeyboardState(&KeyboardState[0]);
          if (NumState)
            KeyboardState[VK_NUMLOCK] &= ~1;
          else
            KeyboardState[VK_NUMLOCK] |= 1;

          SetKeyboardState(&KeyboardState[0]);
        }
      }
    }
    return;
  }

  // Get scancode
  ScanCode = LOBYTE(::MapVirtualKey(VKey, 0));

  // Send keys
  for (Cnt=1; Cnt<=NumTimes; Cnt++)
  {
    // Carry needed delay ?
    if (bDelay)
      CarryDelay();

    KeyboardEvent(VKey, ScanCode, IsVkExtended(VKey) ? KEYEVENTF_EXTENDEDKEY : 0);
    if (GenUpMsg)
      KeyboardEvent(VKey, ScanCode, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP);
  }
}

// Checks whether a bit is set
bool CSendKeys::BitSet(BYTE BitTable, UINT BitMask)
{
  return BitTable & BitMask ? true : false;
}

// Sends a single key
void CSendKeys::SendKey(int MKey, int NumTimes, bool GenDownMsg)
{
  // Send appropriate shift keys associated with the given VKey
  if (BitSet(HIBYTE(MKey), VKKEYSCANSHIFTON))
    SendKeyDown(VK_SHIFT, 1, false);

  if (BitSet(HIBYTE(MKey), VKKEYSCANCTRLON))
    SendKeyDown(VK_CONTROL, 1, false);

  if (BitSet(HIBYTE(MKey), VKKEYSCANALTON))
    SendKeyDown(VK_MENU, 1, false);

  // Send the actual VKey
  SendKeyDown(LOBYTE(MKey), NumTimes, GenDownMsg, true);

  // toggle up shift keys
  if (BitSet(HIBYTE(MKey), VKKEYSCANSHIFTON))
    SendKeyUp(VK_SHIFT);

  if (BitSet(HIBYTE(MKey), VKKEYSCANCTRLON))
    SendKeyUp(VK_CONTROL);

  if (BitSet(HIBYTE(MKey), VKKEYSCANALTON))
    SendKeyUp(VK_MENU);
}


bool CSendKeys::AppActivate(HWND wnd)
{
  if (wnd == NULL)
    return false;

  ::SendMessage(wnd, WM_SYSCOMMAND, SC_HOTKEY, (LPARAM) wnd);
  ::SendMessage(wnd, WM_SYSCOMMAND, SC_RESTORE, (LPARAM) wnd);
  
  ::ShowWindow(wnd, SW_SHOW);
  ::SetForegroundWindow(wnd);
  ::SetFocus(wnd);

  return true;
}

BOOL CALLBACK CSendKeys::enumwindowsProc(HWND hwnd, LPARAM lParam)
{
  enumwindow_t *t = (enumwindow_t *) lParam;

  LPTSTR wtitle = 0, wclass = 0, str = t->str;

  if (!*str)
    str++;
  else
  {
    wtitle = str;
    str += _tcslen(str) + 1;
  }

  if (*str)
    wclass = str;

  bool bMatch(false);

  if (wclass)
  {
    TCHAR szClass[300];
    if (::GetClassName(hwnd, szClass, sizeof(szClass)))
      bMatch |= (_tcsstr(szClass, wclass) != 0);
  }

  if (wtitle)
  {
    TCHAR szTitle[300];
    if (::GetWindowText(hwnd, szTitle, sizeof(szTitle)))
      bMatch |= (_tcsstr(szTitle, wtitle) != 0);
  }

  if (bMatch)
  {
    t->hwnd = hwnd;
    return false;
  }
  return true;
}

// Searchs and activates a window given its title or class name
bool CSendKeys::AppActivate(LPCTSTR WindowTitle, LPCTSTR WindowClass)
{
  HWND w;

  w = ::FindWindow(WindowClass, WindowTitle);
  if (w == NULL)
  {
    // Must specify at least a criteria
    if (WindowTitle == NULL && WindowClass == NULL)
      return false;

    // << Code to serialize the windowtitle/windowclass in order to send to EnumWindowProc()
    size_t l1(0), l2(0);
    if (WindowTitle)
      l1 = _tcslen(WindowTitle);
    if (WindowClass)
      l2 = _tcslen(WindowClass);

    LPTSTR titleclass = new TCHAR [l1 + l2 + 5];

    memset(titleclass, '\0', l1+l2+5);

    if (WindowTitle)
      _tcscpy(titleclass, WindowTitle);

    titleclass[l1] = 0;

    if (WindowClass)
      _tcscpy(titleclass+l1+1, WindowClass);

    // >>

    enumwindow_t t;

    t.hwnd = NULL;
    t.str  = titleclass;
    ::EnumWindows(enumwindowsProc, (LPARAM) & t);
    w = t.hwnd;
    delete [] titleclass;
  }

  if (w == NULL)
    return false;

  return AppActivate(w);
}


/*
Test Binary search
void CSendKeys::test()
{
  WORD miss(0);
  for (int i=0;i<MaxSendKeysRecs;i++)
  {
    char *p = (char *)KeyNames[i].keyName;
    WORD v = StringToVKeyB(p);
    if (v == INVALIDKEY)
    {
      miss++;
    }
  }
}
*/

/*
Search in a linear manner
WORD CSendKeys::StringToVKey(const char *KeyString, int &idx)
{
for (int i=0;i<MaxSendKeysRecs;i++)
{
size_t len = strlen(KeyNames[i].keyName);
if (strnicmp(KeyNames[i].keyName, KeyString, len) == 0)
{
idx = i;
return KeyNames[i].VKey;
}
}
idx = -1;
return INVALIDKEY;
}
*/
int CSendKeys::CharToVKey(int VKey) {
	return ::VkKeyScan(VKey);
}

int CSendKeys::KeyToVKey(int vkey) {
	return ::VkKeyScan(vkey);
}

// Sends a key string
bool CSendKeys::SendLiteral(const wchar_t* KeysString, bool Wait)
{
  LPWSTR pKey = (LPWSTR) KeysString;
  WCHAR  ch;

  m_bWait = Wait;
//  m_nDelayAlways = 500;

  while ((ch = *pKey) != '\0')
  {
    INPUT input[2];
    input[0].type = input[1].type = INPUT_KEYBOARD;
    input[0].ki.wVk = input[1].ki.wVk = NULL;
    input[0].ki.wScan = input[1].ki.wScan = ch;
    input[0].ki.dwFlags = KEYEVENTF_UNICODE;
    input[1].ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_UNICODE;
    ::SendInput(sizeof(input) / sizeof(INPUT), input, sizeof(INPUT));
    if (m_bWait)
    {
      MSG KeyboardMsg;
      while (::PeekMessage(&KeyboardMsg, 0, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
      {
        ::TranslateMessage(&KeyboardMsg);
        ::DispatchMessage(&KeyboardMsg);
      }
    }
//    CarryDelay();
    pKey++;
  }
  return true;
}

#else
Display* CSendKeys::display = NULL;

bool CSendKeys::searchWindow (Window parent, const char *windowTitle, Window &window)
{
	XTextProperty prop;
	if ( XGetWMIconName(getDisplay(), parent, &prop) ) {
		if (strstr ((char*) prop.value, windowTitle) != NULL)
		{
			window = parent;
			XFree(prop.value);
			return true;
		}
	}

	Window parentReturn;
	Window root;
	Window* children = NULL;
	unsigned int nchildren = 0;
	XQueryTree (getDisplay(), parent, &root, &parentReturn, &children, &nchildren);
	for (int i = 0; i < nchildren; i++) {
		if (searchWindow(children[i], windowTitle, window))
			return true;
	}
	if (children != NULL)
		XFree (children);
	return false;

}
// Searchs and activates a window given its title or class name
bool CSendKeys::AppActivate(const char* WindowTitle, const char* WindowClass)
{
	Display *display = getDisplay();
	Window w;
	if (searchWindow (RootWindow(display, DefaultScreen(display)), WindowTitle, w))
	{
		XSetInputFocus(display, w, RevertToParent, CurrentTime);
		return true;
	}
	else
		return false;
}

CSendKeys::key_desc_t CSendKeys::KeyNames[CSendKeys::MaxSendKeysRecs] =
{
  {_T("ADD"), XK_KP_Add},
  {_T("APPS"), XK_Menu},
  {_T("AT"), '@', true},
  {_T("BACKSPACE"), XK_BackSpace},
  {_T("BKSP"), XK_BackSpace},
  {_T("BREAK"), XK_Break},
  {_T("BS"), XK_BackSpace},
  {_T("CAPSLOCK"), XK_Caps_Lock},
  {_T("CARET"), '^', true},
  {_T("CLEAR"), XK_Clear},
  {_T("DECIMAL"), XK_KP_Decimal},
  {_T("DEL"), XK_Delete},
  {_T("DELETE"), XK_Delete},
  {_T("DIVIDE"), XK_KP_Divide},
  {_T("DOWN"), XK_Down},
  {_T("END"), XK_End},
  {_T("ENTER"), XK_Return},
  {_T("ESC"), XK_Escape},
  {_T("ESCAPE"), XK_Escape},
  {_T("F1"), XK_F1},
  {_T("F10"), XK_F10},
  {_T("F11"), XK_F11},
  {_T("F12"), XK_F12},
  {_T("F13"), XK_F13},
  {_T("F14"), XK_F14},
  {_T("F15"), XK_F15},
  {_T("F16"), XK_F16},
  {_T("F2"), XK_F2},
  {_T("F3"), XK_F3},
  {_T("F4"), XK_F4},
  {_T("F5"), XK_F5},
  {_T("F6"), XK_F6},
  {_T("F7"), XK_F7},
  {_T("F8"), XK_F8},
  {_T("F9"), XK_F9},
  {_T("HELP"), XK_Help},
  {_T("HOME"), XK_Home},
  {_T("INS"), XK_Insert},
  {_T("LEFT"), XK_Left},
  {_T("LEFTBRACE"), '{', true},
  {_T("LEFTPAREN"), '(', true},
  {_T("LWIN"), XK_Super_L},
  {_T("MULTIPLY"), XK_KP_Multiply},
  {_T("NUMLOCK"), XK_Num_Lock},
  {_T("NUMPAD0"), XK_KP_0},
  {_T("NUMPAD1"), XK_KP_1},
  {_T("NUMPAD2"), XK_KP_2},
  {_T("NUMPAD3"), XK_KP_3},
  {_T("NUMPAD4"), XK_KP_4},
  {_T("NUMPAD5"), XK_KP_5},
  {_T("NUMPAD6"), XK_KP_6},
  {_T("NUMPAD7"), XK_KP_7},
  {_T("NUMPAD8"), XK_KP_8},
  {_T("NUMPAD9"), XK_KP_9},
  {_T("PERCENT"), '%', true},
  {_T("PGDN"), XK_Next},
  {_T("PGUP"), XK_Prior},
  {_T("PLUS"), '+', true},
  {_T("PRTSC"), XK_Print},
  {_T("RIGHT"), XK_Right},
  {_T("RIGHTBRACE"), '}', true},
  {_T("RIGHTPAREN"), ')', true},
  {_T("RWIN"), XK_Super_R},
  {_T("SCROLL"), XK_Scroll_Lock},
  {_T("SNAPSHOT"), XK_Print},
  {_T("SUBTRACT"), XK_KP_Subtract},
  {_T("TAB"), XK_Tab},
  {_T("TILDE"), '~', true},
  {_T("UP"), XK_Up},
  {_T("WIN"), XK_Super_L}
};


int CSendKeys::CharToVKey(int vkey) {
	if (vkey < 127)
		return vkey;
	else
		return INVALIDKEY;
}

int CSendKeys::KeyToVKey(int vkey) {
	return vkey;
}

// Sends a single key
void CSendKeys::SendKey(int MKey, int NumTimes, bool GenDownMsg)
{
	Window focus = 0;
	int status;
	XGetInputFocus(getDisplay(), &focus, &status);
	XKeyEvent event;



	event.display = getDisplay();
	event.window = focus;
	event.root = RootWindow(event.display, DefaultScreen(event.display));
	event.subwindow = None;
	event.time = CurrentTime;
	event.x = 1;
	event.y = 1;
	event.x_root = 1;
	event.y_root = 1;
	event.same_screen = true;
	event.type = KeyPress;
	event.state = 0;

	event.keycode = XKeysymToKeycode(event.display, MKey);
	if (event.keycode != NoSymbol) {
		int cols = 0;
		KeySym* symtab = XGetKeyboardMapping(event.display, event.keycode, 1, &cols);
		XModifierKeymap *map = XGetModifierMapping(event.display);
		for (int i = 0; i < cols; i++) {
			if (symtab[i] == MKey) {
				event.state = 0;
				if ( i & 1)
					event.state |= ShiftMask;
				if ( i & 2)
					event.state |= ShiftMask;
				if ( i & 4)
					event.state |= Mod5Mask;
				break;
			}
		}
		XFree(symtab);

		while (NumTimes > 0 ) {
			XSendEvent(event.display, focus, true, KeyPressMask, (XEvent*) &event);
			event.type = KeyRelease;
			if (GenDownMsg)
				XSendEvent(event.display, focus, true, KeyReleaseMask, (XEvent*) &event);

			XFlush(event.display);
			NumTimes --;
	        CarryDelay();
		}
	}
}

// Sends a key string
bool CSendKeys::SendLiteral(const wchar_t* KeysString, bool Wait)
{
	Display *display = getDisplay();

	const int cols2 = 8;
	KeyCode code = 255;
	int cols = 0;
	KeySym symtab2[cols2];

	KeySym* symtab = XGetKeyboardMapping(display, code, 1, &cols);

	Window focus = 0;
	int status;
	XGetInputFocus(display, &focus, &status);
	XKeyEvent event;

	event.display = display;
	event.window = focus;
	event.root = RootWindow(event.display, DefaultScreen(event.display));
	event.subwindow = None;
	event.time = CurrentTime;
	event.x = 1;
	event.y = 1;
	event.x_root = 1;
	event.y_root = 1;
	event.same_screen = true;
	event.type = KeyPress;
	event.state = 0;

	XMappingEvent mapEvent;
	mapEvent.type = MappingNotify;
	mapEvent.request = MappingKeyboard;
	mapEvent.display = display;
	mapEvent.first_keycode = code;
	mapEvent.count = 1;
	mapEvent.serial = 0;
	mapEvent.send_event = false;
	mapEvent.window = focus;

	for ( int i = 0; KeysString[i]; i++) {
		wchar_t character = KeysString[i];
		int keySym = (character >= 0 && character < 128 ? character :  0x01000000 + character);
		for (int j = 0 ; i < cols2; j++)
			symtab2[i] =keySym;
		XChangeKeyboardMapping(display, code, cols2, symtab2, 1);
		XSendEvent(display, focus, true, KeymapStateMask, (XEvent*) &mapEvent);
		XSync(display, false);

		event.keycode = code;
		event.type = KeyPress;
		XSendEvent(display, focus, true, KeyPressMask, (XEvent*) &event);
		event.type = KeyRelease;
		XSendEvent(display, focus, true, KeyReleaseMask, (XEvent*) &event);
		XSync(display, false);
	}
	XChangeKeyboardMapping(display, code, cols, symtab, 1);
	XSendEvent(display, focus, true, KeymapStateMask, (XEvent*) &mapEvent);
	XSync(display, false);

	return true;
}

#endif // WIN32

// Carries the required delay and clears the m_nDelaynow value
void CSendKeys::CarryDelay()
{
  // Should we delay once?
  if (!m_nDelayNow)
    // should we delay always?
    m_nDelayNow = m_nDelayAlways;

  // No delay specified?
  if (m_nDelayNow) {
    usleep (m_nDelayNow); //::Beep(100, m_nDelayNow);
  }

  // clear SleepNow
  m_nDelayNow = 0;
}

int CSendKeys::StringToVKey(const char* KeyString, int &idx)
{
  bool Found = false, Collided;
  int  Bottom = 0,
       Top = MaxSendKeysRecs,
       Middle = (Bottom + Top) / 2;
  int retval = INVALIDKEY;

  idx    = -1;

  do
  {
    Collided = (Bottom == Middle) || (Top == Middle);
    int cmp = strnicmp(KeyNames[Middle].keyName, KeyString, strlen(KeyString));
    if (cmp == 0)
    {
      Found = true;
      retval = KeyNames[Middle].VKey;
      idx    = Middle;
      break;
    }
    else
    {
      if (cmp < 0)
        Bottom = Middle;
      else
        Top = Middle;
      Middle = (Bottom + Top) / 2;
    }
  } while (!(Found || Collided));

  return retval;
}


// Releases all shift keys (keys that can be depressed while other keys are being pressed
// If we are in a modifier group this function does nothing
void CSendKeys::PopUpShiftKeys()
{
  if (!m_bUsingParens)
  {
#ifdef WIN32
    if (m_bShiftDown)
      SendKeyUp(VK_SHIFT);
    if (m_bControlDown)
      SendKeyUp(VK_CONTROL);
    if (m_bAltDown)
      SendKeyUp(VK_MENU);
    if (m_bWinDown)
      SendKeyUp(VK_LWIN);
#endif
    m_bWinDown = m_bShiftDown = m_bControlDown = m_bAltDown = false;
  }
}

#include <stdio.h>
// Sends a key string
bool CSendKeys::SendKeys(const char* KeysString, bool Wait)
{
  int MKey, NumTimes;
  char KeyString[300] = {0};
  int  keyIdx;

  char* pKey = (char*) KeysString;
  char ch;

  m_bWait = Wait;

  m_bWinDown = m_bShiftDown = m_bControlDown = m_bAltDown = m_bUsingParens = false;

  while ((ch = *pKey) != 0)
  {
    switch (ch)
    {
    // begin modifier group
    case _TXCHAR('('):
      m_bUsingParens = true;
      break;

    // end modifier group
    case _TXCHAR(')'):
      m_bUsingParens = false;
      PopUpShiftKeys(); // pop all shift keys when we finish a modifier group close
      break;

    // ALT key
    case _TXCHAR('%'):
      m_bAltDown = true;
#ifdef WIN32
      SendKeyDown(VK_MENU, 1, false);
#endif
      break;

    // SHIFT key
    case _TXCHAR('+'):
      m_bShiftDown = true;
#ifdef WIN32
      SendKeyDown(VK_SHIFT, 1, false);
#endif
      break;

    // CTRL key
    case _TXCHAR('^'):
      m_bControlDown = true;
#ifdef WIN32
      SendKeyDown(VK_CONTROL, 1, false);
#endif
      break;

    // WINKEY (Left-WinKey)
    case '@':
      m_bWinDown = true;
#ifdef WIN32
      SendKeyDown(VK_LWIN, 1, false);
#endif
      break;

    // enter
    case _TXCHAR('~'):
#ifdef WIN32
      SendKey(VK_RETURN, 1, true);
#else
	  SendKey(XK_Return, 1, true);
#endif
      PopUpShiftKeys();
      break;

    // begin special keys
    case _TXCHAR('{'):
      {
        char* p = pKey+1; // skip past the beginning '{'
        size_t t;

        // find end of close
        while (*p && *p != _TXCHAR('}'))
          p++;

        t = p - pKey;
        // special key definition too big?
        if (t > sizeof(KeyString))
          return false;

        // Take this KeyString into local buffer
        strncpy (KeyString, pKey+1, t);

        KeyString[t-1] = _TXCHAR('\0');
        keyIdx = -1;

        pKey += t; // skip to next keystring

        // Invalidate key
        MKey = INVALIDKEY;

        // sending arbitrary vkeys?
        if (strnicmp(KeyString, "VKEY", 4) == 0)
        {
          p = KeyString + 4;
          MKey = atoi(p);
        }
        else if (strnicmp(KeyString, "BEEP", 4) == 0)
        {
          p = KeyString + 4 + 1;
          char *p1 ;
          int frequency, delay;

          if ((p1 = strstr(p, " ")) != NULL)
          {
            *p1++ = '\0';
            frequency = atoi(p);
            delay = atoi(p1);
#ifdef WIN32
            ::Beep(frequency, delay);
#endif
          }
        }
        // Should activate a window?
        else if (strnicmp(KeyString, "APPACTIVATE", 11) == 0)
        {
          p = KeyString + 11 + 1;
          AppActivate(p);
        }
        // want to send/set delay?
        else if (strnicmp(KeyString, "DELAY", 5) == 0)
        {
          // Advance to parameters
          p = KeyString + 5;
          // set "sleep factor"
          if (*p == _TXCHAR('='))
            m_nDelayAlways = atoi(p + 1); // Take number after the '=' character
          else
            // set "sleep now"
            m_nDelayNow = atoi(p);
        }
        // not command special keys, then process as keystring to VKey
        else
        {
          MKey = StringToVKey(KeyString, keyIdx);
          // Key found in table
          if (keyIdx != -1)
          {
            NumTimes = 1;

            // Does the key string have also count specifier?
            t = strlen(KeyNames[keyIdx].keyName);
            if (strlen(KeyString) > t)
            {
              p = KeyString + t;
              // Take the specified number of times
              NumTimes = atoi(p);
            }

            if (KeyNames[keyIdx].normalkey)
              MKey = KeyToVKey(KeyNames[keyIdx].VKey);
          }
        }

        // A valid key to send?
        if (MKey != INVALIDKEY)
        {
          SendKey(MKey, NumTimes, true);
          PopUpShiftKeys();
        }
      }
      break;

      // a normal key was pressed
    default:
      // Get the VKey from the key
      MKey = CharToVKey(ch);
      SendKey(MKey, 1, true);
      PopUpShiftKeys();
    }
    pKey++;
  }

  m_bUsingParens = false;
  PopUpShiftKeys();
  return true;
}

CSendKeys::CSendKeys()
{
  m_nDelayNow = m_nDelayAlways = 0;
}
