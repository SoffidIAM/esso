#include <string.h>
#include "../MazingerHookImpl.h"
#include <windows.h>
#include "WindowsHook.h"
#include <stdio.h>
#include <psapi.h>
#include <NativeComponent.h>
#include "WindowsNativeComponent.h"
#include <ComponentMatcher.h>
#include "../java/JavaVirtualMachine.h"
#include <MazingerEnv.h>
#include <time.h>

static HWND previousFocusWnd = NULL;

DWORD CALLBACK ProcessFocus (void * param)
{

	MazingerEnv *pEnv = MazingerEnv::getDefaulEnv ();

	if (pEnv != NULL)
	{
		MAZINGER_DATA *data = pEnv->getDataRW();
		if (data != NULL)
		{
			time_t t;
			time (&t);
			data->lastUpdate = t;
		}
	}

	HWND hwnd = (HWND) param;
	if (hwnd != NULL) {
		ConfigReader *config = MazingerEnv::getDefaulEnv()->getConfigReader();
		if (config != NULL)
		{
			if (previousFocusWnd != NULL)
			{
				ComponentMatcher m;
				std::string className;
				WindowsNativeComponent component(previousFocusWnd);
				component.getAttribute("class", className);
				m.search(*config, component);
				if (m.isFound()) {
					m.triggerBlurEvent();
				}
				previousFocusWnd = hwnd;
			}
			ComponentMatcher m;
			std::string className;
			WindowsNativeComponent component(hwnd);
			component.getAttribute("class", className);
			m.search(*config, component);
			if (m.isFound()) {
				m.triggerFocusEvent();
			}
		}
	}

    return 0;
}

LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam) 
{ 
    char szCode[128]; 
	HWND hwnd = (HWND) wParam;

	if (nCode < 0)  // do not process message
    {
        return CallNextHookEx(hhk, nCode, wParam, 
            lParam); 
    }
 
    switch (nCode) 
    { 
        case HCBT_ACTIVATE:
            break; 
 
        case HCBT_CLICKSKIPPED:
            break; 
 
        case HCBT_CREATEWND:
			installJavaPlugin(hwnd);
            break; 
 
        case HCBT_DESTROYWND:
            break; 
 
        case HCBT_KEYSKIPPED:
            break; 
 
        case HCBT_MINMAX:
            break; 
 
        case HCBT_MOVESIZE:
            break; 
 
        case HCBT_QS:
            break; 
 
        case HCBT_SETFOCUS:
        	CreateThread(NULL, // sECURITY ATTRIBUTES
    				0, // Stack Size,
    				ProcessFocus, hwnd, // Param
    				0, // Options,
    				NULL); // Thread id
			installJavaPlugin(hwnd);
            break;						
 
        case HCBT_SYSCOMMAND:
            break; 
 
        default:
            break; 
    } 

	return CallNextHookEx(hhk, nCode, wParam,
        lParam); 
} 


LRESULT CALLBACK wireKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) 
{ 
    if (nCode < 0)  // do not process message 
        return CallNextHookEx(hhk, nCode, 
            wParam, lParam); 
 

    return CallNextHookEx(hhk, nCode, wParam, 
        lParam); 
} 
 

