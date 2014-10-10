/*
 * WebInputSpec.cpp
 *
 *  Created on: 15/11/2010
 *      Author: u07286
 */

#include "WebInputSpec.h"
#include <MazingerInternal.h>

WebInputSpec::WebInputSpec() {
	reType = NULL;
	reValue = NULL;
	szType = NULL;
	szValue = NULL;
}

WebInputSpec::~WebInputSpec() {
}

bool WebInputSpec::matches (AbstractWebElement *element) {
	if ( ! matchAttribute (reType, element, "type") ||
			! matchAttribute (reValue, element, "value") )
	{
		return false;
	}
	else
	{
		return WebComponentSpec::matches(element);
	}
}



const char* WebInputSpec::getTag() {
	static const char* achTag = "input";
	return achTag;
}

void WebInputSpec::dump() {
	MZNSendDebugMessageA("Input name='%s' type='%s' value='%s' id='%s' ref-as='%s'",
			szName == NULL? "": szName,
			szType == NULL? "": szType,
			szValue == NULL? "": szValue,
			szId == NULL? "": szId,
			szRefAs == NULL? "": szRefAs);
}
