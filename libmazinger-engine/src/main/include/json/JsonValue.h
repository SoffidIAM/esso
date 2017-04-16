/*
 * JsonValue.h
 *
 *  Created on: 12/03/2014
 *      Author: bubu
 */

#ifndef JSONVALUE_H_
#define JSONVALUE_H_

#include "JsonAbstractObject.h"
#include <string>

namespace json {

class JsonValue: public json::JsonAbstractObject {
public:
	JsonValue();
	JsonValue(const char *value);
	virtual ~JsonValue();
	virtual const char* read (const char* str) ;
	virtual void write (std::string &str, int indent) ;
	std::string value;
};

} /* namespace json */
#endif /* JSONVALUE_H_ */
