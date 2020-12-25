/*
 * ProcessDetector.h
 *
 *  Created on: Dec 17, 2020
 *      Author: gbuades
 */

#ifndef WINDOWS_PROCESSDETECTOR_H_
#define WINDOWS_PROCESSDETECTOR_H_

#include <windows.h>
#include <string>

class ProcessDetector {
public:
	ProcessDetector();
	virtual ~ProcessDetector();
	std::string getProcessList();
	static DWORD WINAPI mainLoop(LPVOID param) ;
};

#endif /* WINDOWS_PROCESSDETECTOR_H_ */
