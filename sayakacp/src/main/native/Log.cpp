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

static FILE *logFile = NULL;

Log::Log(const char* nameArg) {
	name.assign(nameArg);
	if (logFile == NULL)
	{
		std::string ginaLogFile;

		if (SeyconCommon::readProperty("ginaLogFile", ginaLogFile)) {
			fclose (stdout);
			logFile = fopen(ginaLogFile.c_str(), "a");
			setbuf(logFile, NULL);
			SeyconCommon::setDebugLevel(3);
		}
	}
}

Log::~Log() {
}

void Log::info (const char *szFormat, ...) {
	if (logFile != NULL) {
		va_list va;
		va_start(va, szFormat);
		time_t t;
		time(&t);
		struct tm *tm = localtime (&t);
		fprintf (logFile, "%d-%02d-%04d %02d:%02d:%02d INFO %s ",
				tm->tm_mday, tm->tm_mon+1, tm->tm_year+1900, tm->tm_hour, tm->tm_min, tm->tm_sec,
				name.c_str());
		vfprintf(logFile, szFormat, va);
		fprintf (logFile, "\n");
		va_end(va);
	}
}
void Log::info (const wchar_t *szFormat, ...) {
	if (logFile != NULL) {
		va_list va;
		va_start(va, szFormat);
		time_t t;
		time(&t);
		struct tm *tm = localtime (&t);
		fprintf (logFile, "%d-%02d-%04d %02d:%02d:%02d INFO %s ",
				tm->tm_mday, tm->tm_mon+1, tm->tm_year+1900, tm->tm_hour, tm->tm_min, tm->tm_sec,
				name.c_str());
		vfwprintf(logFile, szFormat, va);
		fprintf (logFile, "\n");
		va_end(va);
	}
}

void Log::warn (const char *szFormat, ...) {
	if (logFile != NULL) {
		va_list va;
		va_start(va, szFormat);
		time_t t;
		time(&t);
		struct tm *tm = localtime (&t);
		fprintf (logFile, "%d-%02d-%04d %02d:%02d:%02d WARN %s ",
				tm->tm_mday, tm->tm_mon+1, tm->tm_year+1900, tm->tm_hour, tm->tm_min, tm->tm_sec,
				name.c_str());
		vfprintf(logFile, szFormat, va);
		fprintf (logFile, "\n");
		va_end(va);
	}
}
void Log::warn (const wchar_t *szFormat, ...) {
	if (logFile != NULL) {
		va_list va;
		va_start(va, szFormat);
		time_t t;
		time(&t);
		struct tm *tm = localtime (&t);
		fprintf (logFile, "%d-%02d-%04d %02d:%02d:%02d WARN %s ",
				tm->tm_mday, tm->tm_mon+1, tm->tm_year+1900, tm->tm_hour, tm->tm_min, tm->tm_sec,
				name.c_str());
		vfwprintf(logFile, szFormat, va);
		fprintf (logFile, "\n");
		va_end(va);
	}
}
