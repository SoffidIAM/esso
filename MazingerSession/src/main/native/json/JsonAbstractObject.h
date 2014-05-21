/*
 * AbstractObject.h
 *
 *  Created on: 12/03/2014
 *      Author: bubu
 */

#ifndef ABSTRACTOBJECT_H_
#define ABSTRACTOBJECT_H_

#include <string>

namespace json {

class JsonAbstractObject {
public:
	JsonAbstractObject();
	virtual ~JsonAbstractObject();
	virtual const char* read (const char* str) = 0;
	virtual void write (std::string &str, int indent) = 0;
	static const char *skipSpaces (const char *str);
	static JsonAbstractObject *readObject (const char * &str);
	static void ConfigureChromePreferences ();
	void writeIndent (std::string &str, int indent);
};

} /* namespace json */
#endif /* ABSTRACTOBJECT_H_ */
