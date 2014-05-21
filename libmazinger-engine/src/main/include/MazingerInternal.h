/*
 * MazingerInernal.h
 *
 *  Created on: 03/02/2010
 *      Author: u07286
 */

#ifndef MAZINGERINERNAL_H_
#define MAZINGERINERNAL_H_

#include <MZNcompat.h>
#include <MazingerEnv.h>

class ConfigReader;
class ComponentMatcher;
class WebMatcher;
class NativeComponent;
class AbstractWebApplication;
class DomainPasswordCheck;
class HllMatcher;
class HllApplication;
struct SEE_string;

#define APPLICATION_TYPE_WIN32 0
#define APPLICATION_TYPE_JAVA 1
#define PARSE_DEBUG if (false) MZNSendDebugMessage
//#define PARSE_DEBUG MZNSendDebugMessage



void dumpDiagnostic (NativeComponent *top);
const char*xmlEncode (const char* value);

void MZNWebMatch (AbstractWebApplication *app) ;
void MZNHllMatch (HllApplication *app) ;

void MZNEvaluateJSMatch(ComponentMatcher &match, const char *script) ;
void MZNEvaluateJSMatch(HllMatcher &match, const char *script) ;
void MZNEvaluateJSMatch(WebMatcher &match, const char *script) ;
bool MZNEvaluateJS(const char *script) ; // True is success, False is error
bool MZNEvaluateJS(const char *script, std::string &errorMessage);
void MZNSendSpyMessageA(const char* lpszMessage, ...);
void MZNSendSpyMessageW(const wchar_t* lpszMessage, ...);
void MZNSendDebugMessageA(const char* lpszMessage, ...);
void MZNSendDebugMessageW(const wchar_t* lpszMessage, ...);
void MZNSendTraceMessageA(const char* lpszMessage, ...);
void MZNSendTraceMessageW(const wchar_t* lpszMessage, ...);
#ifdef _UNICODE
#define MZNSendDebugMessage MZNSendDebugMessageW
#define MZNSendTraceMessage MZNSendTraceMessageW
#else
#define MZNSendDebugMessage MZNSendDebugMessageA
#define MZNSendTraceMessage MZNSendTraceMessageA
#endif

#define MZNSendSpyMessage MZNSendSpyMessageA

#ifdef WIN32
HINSTANCE getDllHandler ();
extern HINSTANCE hMazingerInstance;
#endif
//#define TRACE NULL
#define TRACE if(false)	MZNSendTraceMessageA("Trace at %s:%d",__FILE__,__LINE__)
#endif /* MAZINGERINERNAL_H_ */
