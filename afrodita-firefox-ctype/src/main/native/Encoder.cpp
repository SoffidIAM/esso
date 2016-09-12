/*
 * Encoder.cpp
 *
 *  Created on: 14/12/2014
 *      Author: gbuades
 */

#include "Encoder.h"
#include <string.h>

namespace json {

Encoder::Encoder() {

}

Encoder::~Encoder() {
}

std::string Encoder::encode(const char* str) {
	std::string result;
	result += '"';
	int len = strlen(str);
	for (int i = 0; i < len; i++)
	{
		switch ( str[i])
		{
		case '\\': result += '\\\\'; break;
		case '\'': result += "\\'"; break;
		case '\"': result += "\\\""; break;
		case '\t': result += "\\t"; break;
		case '\n': result += "\\n"; break;
		case '\b': result += "\\b"; break;
		default: result += str[i]; break;
		}
	}
	result += '"';
	return result;
}

std::string Encoder::decode(const char* str) {
	std::string result;
	if (strcmp (str, "null") == 0)
	{
		// empty string
	}
	else if (str[0] == '\'' || str[0] == '\"')
	{
		int len = strlen(str) - 1;
		for (int i = 1; i < len; i++)
		{
			if ( str[i] == '\\')
			{
				i++;
				switch (str[i])
				{
				case 'n': result += '\n'; break;
				case 't': result += '\t'; break;
				case 'b': result += '\b'; break;
				default: result += str[i]; break;
				}
			} else {
				result += str[i];
			}
		}
	} else {
		result = str;
	}
	return result;
}

} /* namespace json */
