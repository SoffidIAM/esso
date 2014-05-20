/*
 * ecma.h
 *
 *  Created on: 22/10/2010
 *      Author: u07286
 */

#ifndef ECMA_H_
#define ECMA_H_

#include <string>

class AbstractWebApplication;

std::wstring SEE_StringToWChars (SEE_interpreter *i, SEE_string  *string);
std::string SEE_StringToChars (SEE_interpreter *i, SEE_string  *string);
std::string SEE_StringToUTF8 (SEE_interpreter *i, SEE_string  *string);
SEE_string* SEE_WCharsToString (SEE_interpreter *i, const wchar_t* str);
SEE_string* SEE_CharsToString (SEE_interpreter *i, const char* str);
SEE_string* SEE_UTF8ToString (SEE_interpreter *i, const char* str);
void createDocumentObject (AbstractWebApplication *spec, SEE_interpreter *interp);

#endif /* ECMA_H_ */
