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
#include <ssoclient.h>
#include <syslog.h>
#include <security/pam_ext.h>
#include "PamHandler.h"

static FILE *logFile = NULL;

Log::Log() {

}

void Log::init(PamHandler *handler) {
	this->handler = handler;
	name.assign("PamHandler ");
	char ach[15];

	sprintf (ach, "%lx", (long) handler->getPamHandler());
	name.append (ach);
	if (logFile == NULL)
	{
		std::string debugFile;
		if (SeyconCommon::readProperty("pamLogFile", debugFile) ) {
			fclose (stdout);
			logFile = fopen(debugFile.c_str(), "a");
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
	if (handler != NULL) {
		va_list va;
		va_start(va, szFormat);
		pam_vsyslog (handler ->getPamHandler(),  LOG_INFO, szFormat, va);
		va_end (va);
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
	if (handler != NULL) {
		va_list va;
		va_start(va, szFormat);
		pam_vsyslog (handler ->getPamHandler(),  LOG_WARNING, szFormat, va);
		va_end (va);
	}
}
