/*
 * Log.h
 *
 *  Created on: 02/02/2011
 *      Author: u07286
 */

#ifndef LOG_H_
#define LOG_H_

#include <string>
#include <stdarg.h>

class Log {
public:
	Log(const char* name);
	virtual ~Log();
	void setName (const char *name) {
		this->name.assign(name);
	}
	void info (const char *szFormat, ...);
	void info (const wchar_t* szFormat, ...);
	void warn (const char *szFormat, ...);
	void warn (const wchar_t* szFormat, ...);
	void dumpError ();
	static void enableSysLog ();

private:
	static std::string syslogServer;
	std::string name;
	void sendLine (const char *classifier, const char *message) ;
	void sendLine (const wchar_t *classifier, const wchar_t *szFormat, va_list va) ;
	void sendLine (const char *classifier, const char *szFormat, va_list va) ;
};

#endif /* LOG_H_ */
