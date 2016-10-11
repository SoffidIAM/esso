/*
 * JsonVector.h
 *
 *  Created on: 12/03/2014
 *      Author: bubu
 */

#ifndef JSONVECTOR_H_
#define JSONVECTOR_H_

#include "JsonAbstractObject.h"
#include <vector>

namespace json {

class JsonVector: public json::JsonAbstractObject {
public:
	JsonVector();
	virtual ~JsonVector();
	virtual const char* read (const char* str) ;
	virtual void write (std::string &str, int indent) ;
	std::vector<JsonAbstractObject*> objects;
};

} /* namespace json */
#endif /* JSONVECTOR_H_ */
