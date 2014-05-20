#include <windows.h>
#include <ctype.h>
#include <stdio.h>


void fatalError ()
{
	   LPWSTR pstr;
	   DWORD dw = GetLastError ();
	   FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER| FORMAT_MESSAGE_FROM_SYSTEM,
			   NULL,
			   dw,
			   0,
			   (LPWSTR)&pstr,
			   0,
			   NULL);
	   if (pstr == NULL)
			wprintf (L"Unknown error: %d\n",
					dw);
	   else
		   wprintf (L"Unable to send message: %s\n",
				pstr);
	   LocalFree(pstr);
	   ExitProcess(1);
}


HHOOK hhk;
LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam) {
    return CallNextHookEx(hhk, nCode, wParam,
       lParam);
}

HINSTANCE hinst;
extern "C" __declspec(dllexport) void KOJITaskMgrDisable() {
	hhk = SetWindowsHookEx(WH_CBT, (HOOKPROC) CBTProc, hinst, NULL);
}

extern "C" __declspec(dllexport) void KOJITaskMgrEnable() {
    UnhookWindowsHookEx(hhk);
}


HANDLE hMutex = NULL;

extern "C" __declspec(dllexport) BOOL __stdcall DllMain(  HINSTANCE hinstDLL,
  DWORD dwReason,
  LPVOID lpvReserved
  ) {

  if (dwReason == DLL_PROCESS_ATTACH)
  {
	  hinst = hinstDLL;
	  char achFileName[2096] = "";
	  GetModuleFileNameA(NULL, achFileName, sizeof achFileName);
	  for (int i = 0; achFileName[i] ; i++)
		  achFileName[i] = tolower(achFileName[i]);
	  if (strstr(achFileName, "taskmgr.exe") != NULL)
		  ExitProcess(0);
  }
  return TRUE;
}


