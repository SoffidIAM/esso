#ifndef __SENDKEYS_04192004__INC__
#define __SENDKEYS_04192004__INC__

#ifdef WIN32
#include <tchar.h>
#else
#define XK_XKB_KEYS
#define XK_LATIN1

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#endif
// Please see SendKeys.cpp for copyright and usage issues.

class CSendKeys
{
private:
  bool m_bWait, m_bUsingParens, m_bShiftDown, m_bAltDown, m_bControlDown, m_bWinDown;
  int  m_nDelayAlways, m_nDelayNow;

  struct key_desc_t
  {
    const char* keyName;
    int VKey;
    bool normalkey; // a normal character or a VKEY ?
  };

  enum
  {
    MaxSendKeysRecs  = 71,
    MaxExtendedVKeys = 10
  };

  static key_desc_t KeyNames[MaxSendKeysRecs];

  void   CarryDelay();

#ifdef WIN32
  static BOOL CALLBACK enumwindowsProc(HWND hwnd, LPARAM lParam);

  typedef BYTE KEYBOARDSTATE_t[256];
  struct enumwindow_t
  {
    LPTSTR str;
    HWND hwnd;
  };


  /*
  Reference: VkKeyScan() / MSDN
  Bit Meaning 
  --- --------
  1   Either SHIFT key is pressed. 
  2   Either CTRL key is pressed. 
  4   Either ALT key is pressed. 
  8   The Hankaku key is pressed 
  16 z Reserved (defined by the keyboard layout driver).
  32  Reserved (defined by the keyboard layout driver). 
  */
  static const WORD VKKEYSCANSHIFTON;
  static const WORD VKKEYSCANCTRLON;
  static const WORD VKKEYSCANALTON;
  static const WORD INVALIDKEY;

  static const BYTE ExtendedVKeys[MaxExtendedVKeys];

  static bool BitSet(BYTE BitTable, UINT BitMask);


  static bool IsVkExtended(BYTE VKey);
  void KeyboardEvent(BYTE VKey, BYTE ScanCode, LONG Flags);
  static bool AppActivate(HWND wnd);
  void SendKeyUp(int VKey);
  void SendKeyDown(int VKey, int NumTimes, bool GenUpMsg, bool bDelay = false);
#else
  static Display* display;
  static Display *getDisplay () {
	  if (display == NULL) {
			display = XOpenDisplay(NULL);
	  }
	  return display;
  }
  static bool searchWindow (Window parent, const char *windowTitle, Window &window);
#endif
  void SendKey(int MKey, int NumTimes, bool GenDownMsg);
  static int StringToVKey(const char* KeyString, int &idx);
  static int CharToVKey(int vkey) ;
  static int KeyToVKey(int vkey) ;
  void PopUpShiftKeys();
public:

  bool SendKeys(const char* KeysString, bool Wait = false);
  bool SendLiteral(const wchar_t* KeysString, bool Wait = false);
  static bool AppActivate(const char* WindowTitle, const char* WindowClass = 0);
  void SetDelay(const int delay) { m_nDelayAlways = delay; }
  CSendKeys();
};

#endif
