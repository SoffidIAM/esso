/*
 * AbstractObject.cpp
 *
 *  Created on: 12/03/2014
 *      Author: bubu
 */

#include "JsonAbstractObject.h"
#include "JsonMap.h"
#include "JsonVector.h"
#include "JsonValue.h"
#include <stdlib.h>
#include <stdio.h>
#include <MazingerInternal.h>
#include <SeyconServer.h>

namespace json {

JsonAbstractObject::JsonAbstractObject() {
}

JsonAbstractObject::~JsonAbstractObject() {
}

const char *JsonAbstractObject::skipSpaces (const char *str)
{
	while (*str && *str <= ' ')
		str ++;
	return str;
}

JsonAbstractObject* JsonAbstractObject::readObject(const char* &str) {
	JsonAbstractObject *o;
	str = skipSpaces(str);
	if (*str == '[')
	{
		o= new JsonVector();
	}
	else if (*str == '{')
	{
		o = new JsonMap ();

	} else {
		o = new JsonValue ();
	}
	str = o->read(str);
	str = skipSpaces(str);

	return o;
}

void JsonAbstractObject::writeIndent(std::string& str, int indent) {
	while (indent --)
	{
		str.append(" ");
	}
}


} /* namespace json */
