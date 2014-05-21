/*
 * JsonMap.h
 *
 *  Created on: 12/03/2014
 *      Author: bubu
 */

#ifndef JSONMAP_H_
#define JSONMAP_H_

#include "JsonAbstractObject.h"
#include "JsonTaggedObject.h"
#include <vector>

namespace json {

class JsonMap: public json::JsonAbstractObject {
public:
	JsonMap();
	virtual ~JsonMap();
	virtual const char* read (const char* str) ;
	virtual void write (std::string &str, int indent) ;
	std::vector<JsonTaggedObject *> taggedObjects;

	JsonAbstractObject *getObject (const char *tag);
	void setObject (const char *tag, JsonAbstractObject* object);
};

} /* namespace json */
#endif /* JSONMAP_H_ */
