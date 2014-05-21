/*
 * Log.h
 *
 *  Created on: 02/02/2011
 *      Author: u07286
 */

#ifndef LOG_H_
#define LOG_H_

#include <string>
class PamHandler;

class Log {
public:
	Log();
	virtual ~Log();
	void setName (const char *name) {
		this->name.assign(name);
	}
	void info (const char *szFormat, ...);
	void warn (const char *szFormat, ...);
	void init (PamHandler *handler);
private:
	std::string name;
	PamHandler *handler;
};

#endif /* LOG_H_ */
