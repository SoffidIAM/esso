/*
 * Log.cpp
 *
 *  Created on: 02/02/2011
 *      Author: u07286
 */

#include "Log.h"
#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <ssoclient.h>
#include <string>
#include "Utils.h"
#include <MZNcompat.h>

static FILE *logFile = NULL;


std::string Log::syslogServer;

Log::Log(const char* nameArg) {
	name.assign(nameArg);
	if (logFile == NULL)
	{
		std::string ginaDebug;
		if (SeyconCommon::readProperty("ginaLogFile",  ginaDebug) ) {
			fclose (stdout);
			logFile = fopen(ginaDebug.c_str(), "a");
			if (logFile != NULL) {
				setbuf(logFile, NULL);
				SeyconCommon::setDebugLevel(3);
			}
		}
	}
}

Log::~Log() {
}

void Log::enableSysLog() {
	SeyconCommon::readProperty("GinaSyslog", syslogServer);
}

static void initWSocket () {
	static bool initialized = false;
	if (! initialized) {
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);
		initialized = true;
	}
}


void Log::sendLine (const char *classifier, const char *message) {
	if (logFile != NULL) {
		time_t t;
		time(&t);
		struct tm *tm = localtime (&t);
		fprintf (logFile, "%d-%02d-%04d %02d:%02d:%02d %s %s %s\n",
				tm->tm_mday, tm->tm_mon+1, tm->tm_year+1900, tm->tm_hour, tm->tm_min, tm->tm_sec,
				classifier, name.c_str(), message);
	}

	if (syslogServer.length() > 0  ) {
		initWSocket ();
		SOCKET s = -1;
		if (s == -1) {
			s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		}
		if (true) {
			sockaddr_in RecvAddr;
			RecvAddr.sin_family = AF_INET;
			RecvAddr.sin_port = htons(514);
			RecvAddr.sin_addr.s_addr = inet_addr(syslogServer.c_str());

			time_t t;
			time(&t);
			struct tm *tm = localtime (&t);
			std::string line = name;
			line += " - ";
			line += MZNC_getHostName();
			line += " ";
			line += classifier;
			line += ":";
			line += message;

			line = MZNC_strtoutf8(line.c_str());

			sendto(s, line.c_str(), line.size(), 0, (SOCKADDR *) & RecvAddr, sizeof (RecvAddr));
		}
	}
}

void Log::sendLine (const char *classifier, const char *szFormat, va_list va) {
	char achMessage[4096];
	_vsnprintf (achMessage, 4095, szFormat, va);
	sendLine (classifier, achMessage);
}

void Log::sendLine (const wchar_t *classifier, const wchar_t *szFormat, va_list va) {
	wchar_t achMessage[4096];
	_vsnwprintf (achMessage, 4095, szFormat, va);
	sendLine (MZNC_wstrtostr(classifier).c_str(), MZNC_wstrtostr(achMessage).c_str());
}


void Log::info (const char *szFormat, ...) {
	va_list va;
	va_start(va, szFormat);
	sendLine ("INFO", szFormat, va);
	va_end(va);
}

void Log::info (const wchar_t *szFormat, ...) {
	va_list va;
	va_start(va, szFormat);
	sendLine (L"INFO", szFormat, va);
	va_end(va);
}

void Log::warn (const char *szFormat, ...) {
	va_list va;
	va_start(va, szFormat);
	sendLine ("WARN", szFormat, va);
	va_end(va);
}
void Log::warn (const wchar_t *szFormat, ...) {
	va_list va;
	va_start(va, szFormat);
	sendLine (L"WARN", szFormat, va);
	va_end(va);
}


void Log::dumpError() {
	std::string msg;

	if (Utils::getLastError(msg)) {
		warn ("ERROR %x: %s\n", (int) GetLastError(), msg.c_str());
	} else {
		warn ("UNKNOWN ERROR %x\n", (int) GetLastError());
	}
}
