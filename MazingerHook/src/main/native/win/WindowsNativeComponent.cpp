#include <windows.h>
#include <string>
#include <MazingerInternal.h>
#include "MazingerHook.h"
#include "WindowsNativeComponent.h"

WindowsNativeComponent::WindowsNativeComponent(HWND hwnd) {
	m_hwnd = hwnd;
}

const char *WindowsNativeComponent::getClass() {
	return "WindowsNativeComponent";
}

int WindowsNativeComponent::equals (NativeComponent &component) {
	if ( getClass() != component.getClass ())
		return 0;
	return m_hwnd == ((WindowsNativeComponent&)component).m_hwnd;
}

NativeComponent* WindowsNativeComponent::clone() {
	WindowsNativeComponent *result = new WindowsNativeComponent(this->m_hwnd);
	return result;
}

NativeComponent *WindowsNativeComponent::getParent() {
	DWORD dwStyle = GetWindowLong (m_hwnd, GWL_STYLE);
	if (dwStyle & WS_CHILDWINDOW) {
		HWND hwnd = GetParent(m_hwnd);
		if (hwnd == NULL)
		{
			return NULL;
		}
		else
		{
			return new WindowsNativeComponent (hwnd);
		}
	} else {
		return NULL;
	}

}



void WindowsNativeComponent::getAttribute(const char* attributeName, std::string &result) {
	result.clear();
	const int maxsize = 1024;
	if (strcmp (attributeName, "class") == 0)
	{
		wchar_t wch[maxsize+1];
		if ( GetClassNameW (m_hwnd, wch, maxsize-1) > 0)
		{
			wch[maxsize] = '\0';
			result = MZNC_wstrtoutf8(wch);
		}
	}
	if (strcmp (attributeName, "dlgId") == 0)
	{
		DWORD dw = GetWindowLong (m_hwnd, GWL_ID);
		if (dw != 0)
		{
			char ach[20];
			wsprintf (ach, "%x", dw);
			result.assign(ach);
		}

	}
	if (strcmp (attributeName, "text") == 0)
	{
		wchar_t wch[maxsize+1];
		if ( GetWindowTextW (m_hwnd, wch, maxsize) > 0)
		{
			wch[maxsize] = '\0';
			result.assign ( MZNC_wstrtoutf8(wch));
		}
	}
}

struct EnumParams {
	HWND hWndParent;
	std::vector<NativeComponent*> *v;
};

static BOOL CALLBACK MyEnumChildWindows(
    HWND hWnd,
    LPARAM lParam
) {
	struct EnumParams* p = (struct EnumParams*)lParam;
	HWND hwndParent = GetParent(hWnd);
	if (hwndParent == p->hWndParent) {
		WindowsNativeComponent *pc = new WindowsNativeComponent (hWnd);
		p->v->push_back ( pc );
	}
	return true;
}

void WindowsNativeComponent::getChildren(std::vector<NativeComponent*> &children) {
	children.clear();
	struct EnumParams p ;
	p.hWndParent = m_hwnd;
	p.v = &children;
	EnumChildWindows(m_hwnd, MyEnumChildWindows, (LPARAM)&p);
}

void WindowsNativeComponent::setAttribute(const char*attributeName, const char* value) {
	if (strcmp (attributeName, "text") == 0)
	{
		SetWindowText (m_hwnd, value);
	}
	else
	{
		MZNSendDebugMessageA("Warning: Cannot set %s attribute", attributeName);
	}
}

void WindowsNativeComponent::setFocus() {
	SetFocus (m_hwnd);
}

void WindowsNativeComponent::click() {
	RECT r;
	GetWindowRect(m_hwnd, &r);
	DWORD pos = MAKELONG ((r.right-r.left)/2, (r.bottom-r.top)/2);
	SendMessage (m_hwnd, WM_LBUTTONDOWN, MK_LBUTTON, pos);

}
