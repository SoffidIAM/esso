/*
 * JsonMap.cpp
 *
 *  Created on: 12/03/2014
 *      Author: bubu
 */

#include "JsonMap.h"
#include "JsonTaggedObject.h"
#include "JsonValue.h"

#include <stdio.h>

namespace json {

JsonMap::JsonMap() {
	// TODO Auto-generated constructor stub

}

JsonMap::~JsonMap() {
	// TODO Auto-generated destructor stub
}

const char* JsonMap::read(const char* str) {
	str = skipSpaces(str);
	while (*str && *str != '{')
		str ++;
	if (*str)
	{
		str ++;
		str = skipSpaces(str);
		while (*str && *str != '}')
		{
			JsonTaggedObject *tag = new JsonTaggedObject();
			str = tag->read(str);
			taggedObjects.push_back(tag);


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

void JsonMap::write(std::string& str, int indent) {
	str.append ("{\n");
	if (! taggedObjects.empty())
	{
		std::vector<JsonTaggedObject*>::iterator it = taggedObjects.begin();
		int i = 0;
		while (true)
		{
			writeIndent(str, indent+3);
			if (*it != NULL)
				(*it)->write (str, indent + 3);
			it ++;
			if (it == taggedObjects.end())
				break;
			else
			{
				str.append (",\n");
			}
		}
	}
	str.append("\n");
	writeIndent(str, indent);
	str.append("}");
}

JsonAbstractObject* JsonMap::getObject(const char* tagName) {
	for (std::vector<JsonTaggedObject*>::iterator it = taggedObjects.begin();
			it != taggedObjects.end();
			it ++)
	{
		JsonTaggedObject *tag = *it;
		JsonValue *v = dynamic_cast<JsonValue*> (tag->left);
		if (v != NULL)
		{
			if (v->value == tagName)
				return tag->right;
		}
	}
	return NULL;
}

void JsonMap::setObject(const char* tag, JsonAbstractObject* object) {
	json::JsonTaggedObject *tagValue = new json::JsonTaggedObject();
	json::JsonValue *tagName = new json::JsonValue ();
	tagName->value = tag;
	tagValue->left = tagName;
	tagValue->right = object;
	taggedObjects.push_back(tagValue);
}

} /* namespace json */
