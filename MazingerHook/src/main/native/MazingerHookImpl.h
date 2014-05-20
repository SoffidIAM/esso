/*
 * MazingerHookImpl.h
 *
 *  Created on: 10/03/2011
 *      Author: u07286
 */

#ifndef MAZINGERHOOKIMPL_H_
#define MAZINGERHOOKIMPL_H_

#include <MazingerHook.h>
#include <MazingerInternal.h>

const char* getMazingerDir();
extern MAZINGER_STOP_CALLBACK stopCallback;

#ifdef WIN32

#undef MAZINGERAPI
#define MAZINGERAPI extern "C" __declspec(dllexport)
HINSTANCE getDllHandler ();
void uninstallJavaPlugin();
void installJavaPlugin(HWND hwndFocus) ;

#else

#undef MAZINGERAPI
#define MAZINGERAPI extern "C" __attribute__((__visibility__("default")))

#endif
#endif /* MAZINGERHOOKIMPL_H_ */
