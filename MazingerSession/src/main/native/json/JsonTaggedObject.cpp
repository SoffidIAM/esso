/*
 * TaggedJsonObject.cpp
 *
 *  Created on: 12/03/2014
 *      Author: bubu
 */

#include "JsonTaggedObject.h"
#include <stdio.h>

namespace json {

JsonTaggedObject::JsonTaggedObject() {
	left = right = NULL;
}

JsonTaggedObject::~JsonTaggedObject() {
}

const char* JsonTaggedObject::read(const char* str) {
	str = skipSpaces(str);
	if (*str)
	{
		str = skipSpaces(str);
		left = JsonAbstractObject::readObject (str);
		str = skipSpaces(str);
		while (*str && *str != ':')
			str ++;
		if (*str) str ++;
		str = skipSpaces(str);
		right = JsonAbstractObject::readObject(str);
	}
	return str;
}

void JsonTaggedObject::write(std::string& str, int indent) {
	if (left != NULL)
		left->write(str, indent);
	str.append (": ");
	if (right != NULL)
		right->write(str,indent);
}

} /* namespace json */
