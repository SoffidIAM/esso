/*
 * TaggedJsonObject.h
 *
 *  Created on: 12/03/2014
 *      Author: bubu
 */

#ifndef TAGGEDJSONOBJECT_H_
#define TAGGEDJSONOBJECT_H_

#include "JsonAbstractObject.h"

namespace json {

class JsonTaggedObject: public JsonAbstractObject{
public:
	JsonTaggedObject();
	virtual ~JsonTaggedObject();
	virtual const char* read (const char* str);
	virtual void write (std::string &str, int indent);

	JsonAbstractObject *left ;
	JsonAbstractObject *right ;
};

} /* namespace json */
#endif /* TAGGEDJSONOBJECT_H_ */
