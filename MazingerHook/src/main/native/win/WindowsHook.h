/*
 * WindowsHook.h
 *
 *  Created on: 16/02/2010
 *      Author: u07286
 */

#ifndef WINDOWSHOOK_H_
#define WINDOWSHOOK_H_

LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam);
extern HINSTANCE hinst;
extern HHOOK hhk;

#endif /* WINDOWSHOOK_H_ */
