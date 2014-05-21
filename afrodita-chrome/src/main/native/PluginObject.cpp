/*
 * PluginObject.cpp
 *
 *  Created on: 26/02/2014
 *      Author: bubu
 */

#include "PluginObject.h"
#include "ChromeElement.h"
#include "stdio.h"

namespace mazinger_chrome {

PluginObject::PluginObject() {

}

PluginObject::~PluginObject() {
}

bool PluginObject::getProperty(NPObject* object, const char* name,
		NPVariant &v) {
	NPIdentifier identifier = npGlobalFuncs.getstringidentifier( name);
	// Declare a local variant value.
	// Get the location property from the window object (which is another object).
	return npGlobalFuncs.getproperty( instance, object, identifier, &v );
}

bool PluginObject::invoke1(NPObject* object, const char* name,
		NPVariant &param, NPVariant &variantValue) {
	NPIdentifier identifier = npGlobalFuncs.getstringidentifier( name);
	// Declare a local variant value.
	// Get the location property from the window object (which is another object).
	return npGlobalFuncs.invoke( instance, object, identifier, &param, 1, &variantValue );
}

bool PluginObject::invoke0(NPObject* object, const char* name, NPVariant &variantValue) {
	NPIdentifier identifier = npGlobalFuncs.getstringidentifier( name);
	// Declare a local variant value.
	NPVariant param;
	// Get the location property from the window object (which is another object).
	return npGlobalFuncs.invoke( instance, object, identifier, &param, 0, &variantValue );
}

bool PluginObject::invoke2(NPObject* object, const char* name,
		NPVariant& param1, NPVariant& param2, NPVariant&variantValue) {
	NPIdentifier identifier = npGlobalFuncs.getstringidentifier( name);
	// Declare a local variant value.
	NPVariant params[2];
	params[0] = param1;
	params[1] = param2;
	// Get the location property from the window object (which is another object).
	return npGlobalFuncs.invoke( instance, object, identifier, &params[0], 2, &variantValue );
}

bool PluginObject::nodeListToVector(NPObject* nodeList,
		std::vector<AbstractWebElement*>& elements) {
	NPIdentifier identifier ;
	NPVariant variantValue;
	NPVariant variantParam;
	identifier = npGlobalFuncs.getstringidentifier("length");
	bool ok = false;
	if (npGlobalFuncs.getproperty( instance, nodeList, identifier, &variantValue ))
	{
		char ch[1000];
		sprintf (ch, "En paso 7: Type %d", variantValue.type);
		int length = 0;
		if (variantValue.type == NPVariantType_Int32)
		{
			length = variantValue.value.intValue;
		}
		else if (variantValue.type == NPVariantType_Double)
		{
			length = variantValue.value.doubleValue;
		}

		identifier = npGlobalFuncs.getstringidentifier("item");
		for (int i = 0; i < length; i++)
		{
			NPVariant variantValue2;
			INT32_TO_NPVARIANT(i, variantParam);
			if (npGlobalFuncs.invoke( instance, nodeList, identifier, &variantParam, 1, &variantValue2 ))
			{
				if (variantValue2.type == NPVariantType_Object)
				{
					elements.push_back(new ChromeElement(this, variantValue2.value.objectValue));
				}
				npGlobalFuncs.releasevariantvalue(&variantValue2);
			}
		}
		ok = true;
		npGlobalFuncs.releasevariantvalue(&variantValue);
	}
	return ok;
}

} /* namespace mazinger::chrome */
