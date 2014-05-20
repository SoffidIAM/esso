/*
 * ExplorerElement.cpp
 *
 *  Created on: 11/11/2010
 *      Author: u07286
 */
#define NS_SCRIPTABLE


#include "AfroditaC.h"
#include "ChromeElement.h"
#include <stdio.h>
#include <string.h>

namespace mazinger_chrome
{

ChromeElement::ChromeElement(PluginObject *plugin, NPObject* object)
{
	this -> plugin = plugin;
	this -> elementObject = object;
	npGlobalFuncs.retainobject (elementObject);
}

ChromeElement::~ChromeElement() {
	npGlobalFuncs.releaseobject(elementObject);
}



void ChromeElement::getChildren(std::vector<AbstractWebElement*> &children)
{
	children.clear ();

	NPVariant variantValue;
	if (! plugin->getProperty(elementObject, "children", variantValue ))
		return;
	if (variantValue.type != NPVariantType_Object)
	{
		npGlobalFuncs.releasevariantvalue(&variantValue);
		return;
	}

	NPObject *nodeList = variantValue.value.objectValue;
	// Now iterate over returned objects
	plugin->nodeListToVector (nodeList, children);

	npGlobalFuncs.releasevariantvalue(&variantValue);
}



void ChromeElement::click()
{
	NPVariant variantValue;
	if (! plugin->invoke0(elementObject, "click",variantValue ))
		return;
	npGlobalFuncs.releasevariantvalue(&variantValue);
}



void ChromeElement::getAttribute(const char *attribute, std::string & value)
{
	value.clear();
	NPVariant variantValue;
	NPVariant param;

	STRINGZ_TO_NPVARIANT(attribute, param);
	if (! plugin->invoke1(elementObject, "getAttribute", param, variantValue ))
		return;
	if (variantValue.type == NPVariantType_String)
	{
		value.assign (variantValue.value.stringValue.UTF8Characters, variantValue.value.stringValue.UTF8Length);
	}
	npGlobalFuncs.releasevariantvalue(&variantValue);

}



void ChromeElement::blur()
{
	NPVariant variantValue;
	if (! plugin->invoke0(elementObject, "blur",variantValue ))
		return;
	npGlobalFuncs.releasevariantvalue(&variantValue);
}



AbstractWebElement *ChromeElement::getParent()
{
	NPVariant variantValue;
	if (! plugin->invoke0(elementObject, "blur",variantValue ))
		return NULL;
	if (! variantValue.type == NPVariantType_Object)
	{
		npGlobalFuncs.releasevariantvalue(&variantValue);
		return NULL;
	}

	ChromeElement * ce = new ChromeElement(plugin, variantValue.value.objectValue);
	npGlobalFuncs.releasevariantvalue(&variantValue);
}



void ChromeElement::setAttribute(const char *attribute, const char*value)
{
	NPVariant variantValue;
	NPVariant param1, param2;

	STRINGZ_TO_NPVARIANT(attribute, param1);
	STRINGZ_TO_NPVARIANT(value, param2);
	if (! plugin->invoke2(elementObject, "setAttribute", param1, param2, variantValue ))
		return;
	npGlobalFuncs.releasevariantvalue(&variantValue);
}



void ChromeElement::focus()
{
	NPVariant variantValue;
	if (! plugin->invoke0(elementObject, "focus",variantValue ))
		return;
	npGlobalFuncs.releasevariantvalue(&variantValue);

}



void ChromeElement::getTagName(std::string & value)
{
	value.clear ();
	NPVariant variantValue;
	if (! plugin->getProperty(elementObject, "tagName",variantValue ))
		return;
	if (variantValue.type == NPVariantType_String)
	{
		value.assign (variantValue.value.stringValue.UTF8Characters, variantValue.value.stringValue.UTF8Length);
	}
	npGlobalFuncs.releasevariantvalue(&variantValue);
}



AbstractWebElement *ChromeElement::clone()
{
	return new ChromeElement(plugin, elementObject);
}

}

