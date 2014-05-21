/*
 * JsonVector.cpp
 *
 *  Created on: 12/03/2014
 *      Author: bubu
 */

#include "JsonVector.h"
#include <stdio.h>

namespace json {

JsonVector::JsonVector() {
	// TODO Auto-generated constructor stub

}

JsonVector::~JsonVector() {
	// TODO Auto-generated destructor stub
}

const char* JsonVector::read(const char* str) {
	str = skipSpaces(str);

	while (*str && *str != '[')
		str ++;
	if (*str)
	{
		str ++;
		str = skipSpaces(str);
		while (*str && *str != ']')
		{
			JsonAbstractObject *obj = JsonAbstractObject::readObject(str);
			if (obj != NULL)
				objects.push_back(obj);

			str = skipSpaces(str);
			if (*str == ',')
			{
				str ++;
				str = skipSpaces(str);
			}
		}
		if (*str)
			str ++;
	}
	return str;
}

void JsonVector::write(std::string& str, int indent) {
	str.append ("[ ");
	if (!objects.empty())
	{
		std::vector<JsonAbstractObject*>::iterator it = objects.begin();
		while (true)
		{
			if (*it != NULL)
				(*it)->write (str, indent);
			it ++;
			if (it == objects.end())
				break;
			else
			{
				str.append (", ");
			}
		}
		// str.append("\n");
	}
	str.append(" ]");
}

} /* namespace json */
