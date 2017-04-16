/*
 * AfroditaC.cpp
 *
 *  Created on: 22/11/2010
 *      Author: u07286
 */

#include "AfroditaC.h"
#include <MazingerInternal.h>
#include <stdio.h>
#include <string.h>
#include "ChromeWebApplication.h"
#include "CommunicationManager.h"
#include "json/JsonMap.h"
#include "json/JsonValue.h"
#include <SeyconServer.h>
#include <stdlib.h>

#define MIME_TYPE_DESCRIPTION "application/soffid-sso-plugin:sso:Soffid SSO Plugin"

using namespace mazinger_chrome;
using namespace std;
using namespace json;


#ifdef WIN32

#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <dbghelp.h>
#include <strsafe.h>

DWORD threadId;

DWORD CALLBACK DumpData (
   PEXCEPTION_POINTERS ExceptionInfo
)
{
    BOOL bMiniDumpSuccessful;
    WCHAR szPath[MAX_PATH];
    WCHAR szFileName[MAX_PATH];
    DWORD dwBufferSize = MAX_PATH;
    HANDLE hDumpFile;
    SYSTEMTIME stLocalTime;
    MINIDUMP_EXCEPTION_INFORMATION ExpParam;
    static int i = 0;

    if (i++ == 0)
    	return 0;

    GetLocalTime( &stLocalTime );
    GetTempPathW( dwBufferSize, szPath );

    //StringCchPrintfW( szFileName, MAX_PATH, L"%s%s", szPath, szAppName );
    StringCchPrintfW( szFileName, MAX_PATH, L"%s\\Soffid Dump", szPath );
    CreateDirectoryW( szFileName, NULL );

    StringCchPrintfW( szFileName, MAX_PATH, L"%s\\Soffid Dump\\Afrodita-Chrome-%04d%02d%02d-%02d%02d%02d-%ld-%d.dmp",
               szPath,
               stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
               stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond,
               GetCurrentProcessId(), i);

    hDumpFile = CreateFileW(szFileName, GENERIC_READ|GENERIC_WRITE,
                FILE_SHARE_WRITE|FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

	ExpParam.ThreadId = threadId;
    ExpParam.ExceptionPointers = ExceptionInfo;
    ExpParam.ClientPointers = TRUE;

    bMiniDumpSuccessful = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
                    hDumpFile, (MINIDUMP_TYPE)( MiniDumpNormal //|
//                    MiniDumpWithDataSegs |
//                    MiniDumpWithPrivateReadWriteMemory |
//                    MiniDumpWithHandleData |
//                    MiniDumpWithFullMemoryInfo |
                    | MiniDumpWithThreadInfo
//                    MiniDumpWithUnloadedModules
                    ), &ExpParam, NULL, NULL);

    SeyconCommon::warn ("Generated dump file %ls", szFileName);

	MessageBox(NULL, "Generated dump file", "Soffid Chrome extension", MB_OK);
	return 0;
}

extern "C" LONG CALLBACK VectoredHandler (
   PEXCEPTION_POINTERS ExceptionInfo
)
{
	threadId = GetCurrentThreadId();
	HANDLE hThread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) DumpData, ExceptionInfo, 0, NULL);
	WaitForSingleObject( hThread, INFINITE );
	ExitProcess(0);
	return EXCEPTION_CONTINUE_SEARCH;
}

static DWORD WINAPI mainThreadProc(
  LPVOID arg
)
{
	CommunicationManager* manager = CommunicationManager::getInstance();

	manager->mainLoop();

	ExitProcess(0);

}



#else

#ifdef USEQT

#include <QApplication>
#include "ChromeWidget.h"

#else


GtkWidget *signalWindow = NULL;

#endif

extern "C" void __attribute__((constructor)) startup() {
	setbuf(stdin, NULL);
	setbuf(stdout, NULL );
}

static void* qtThreadProc (void *app)
{
	SeyconCommon::setDebugLevel(0);
	DEBUG ("Started AfroditaC");
	CommunicationManager* manager = CommunicationManager::getInstance();

	manager->mainLoop();
}

#endif


extern "C" int main (int argc, char **argv)
{
#ifdef WIN32
	setmode(fileno(stdout), O_BINARY);
	setmode(fileno(stdin), O_BINARY);
	std::string vh;
	SeyconCommon::readProperty ("crashDump", vh);
	if (! vh.empty())
		AddVectoredExceptionHandler(0, VectoredHandler);

	SeyconCommon::setDebugLevel(0);
	if ( CreateThread (NULL,  0, mainThreadProc, NULL, 0, NULL) == NULL)
		ExitProcess(1);

	MSG msg;
	while (GetMessageA(&msg, (HWND) NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

#else
#ifdef USEQT
 	QApplication *app = new QApplication (argc, argv);

	pthread_t threadId;
	if (pthread_create(&threadId, NULL, qtThreadProc, app) != 0)
		exit(1);

	app->exec();

	exit(0);
#else
	gdk_threads_init();
	gtk_init(&argc, &argv);


	signalWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	pthread_t threadId;
	if (pthread_create(&threadId, NULL, qtThreadProc, NULL) != 0)
		exit(1);

	gtk_main();


#endif
#endif

	return 0;
}
