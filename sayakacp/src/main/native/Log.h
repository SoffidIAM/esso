/*
 * Log.h
 *
 *  Created on: 02/02/2011
 *      Author: u07286
 */

#ifndef LOG_H_
#define LOG_H_

#include <string>
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

private:
	std::string name;
};

#endif /* LOG_H_ */
