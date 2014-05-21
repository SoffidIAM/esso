#ifndef __WAIT_DIALOG_H

#define __WAIT_DIALOG_H

#include <string>
class WaitDialog {
public:
	WaitDialog (HWND hwndParent);
	~WaitDialog ();
	void displayMessage (const wchar_t *lpszMessage);
	void cancelMessage ();
	const wchar_t* getMessage() { return m_szMessage.c_str(); }
	HWND getParentWindow() { return m_hwndParent; }

private:
	std::wstring m_szMessage;
	HWND m_hwndParent;
};

#endif
