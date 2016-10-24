/*
 * compat.h
 *
 *  Created on: 20/04/2011
 *      Author: u07286
 */

#ifndef COMPAT_H_
#define COMPAT_H_

#ifdef WIN32
#include <windows.h>
#define MZNC_swprintf _snwprintf
#else
#define MAX_PATH 2048
#define wcsicmp wcscasecmp
#define stricmp strcasecmp
#define wcsnicmp wcsncasecmp
#define strnicmp strncasecmp
#define MZNC_swprintf swprintf
#endif

#include <string>

const char *MZNC_getCommandLine();
// Configuration mutex
bool MZNC_waitMutex();
void MZNC_endMutex();
void MZNC_destroyMutex();
// Scripting mutex
bool MZNC_waitMutex2();
void MZNC_endMutex2();
void MZNC_destroyMutex2();
std::string MZNC_utf8tostr(const char *sz);
std::string MZNC_strtoutf8 (const char *pszString) ;
std::wstring MZNC_strtowstr (const char *pszString) ;
std::string MZNC_wstrtostr (const wchar_t *pszString) ;
std::string MZNC_wstrtoutf8 (const wchar_t *pszString) ;
std::wstring MZNC_utf8towstr (const char *pszString) ;


void MZNC_setUserName (const char*username);
const char *MZNC_getUserName ();
const char *MZNC_getHostName ();




#endif /* COMPAT_H_ */
