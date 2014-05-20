/*
 * ExplorerWebApplication.cpp
 *
 *  Created on: 19/10/2010
 *      Author: u07286
 */

#include "AfroditaC.h"
#include "ChromeWebApplication.h"
#include "ChromeElement.h"
#include <string.h>
#include <stdio.h>

namespace mazinger_chrome
{


static std::string getAttribute (PluginObject *plugin, NPObject *windowObject, const char *attribute)
{
	std::string value;
	NPVariant variantValue;
	NPVariant variantValue2;
	if (plugin->getProperty(windowObject, "document", variantValue))
	{
		if (variantValue.type == NPVariantType_Object)
		{
			NPObject* documentObject = variantValue.value.objectValue;

			// Gets the "title" identifier.
			if (plugin->getProperty(documentObject, attribute, variantValue2))
			{
				if (variantValue2.type == NPVariantType_String)
				{
					value.assign(variantValue2.value.stringValue.UTF8Characters,
							variantValue2.value.stringValue.UTF8Length);
				}
				npGlobalFuncs.releasevariantvalue(&variantValue2);
			}

		}
		npGlobalFuncs.releasevariantvalue(&variantValue);
	}

	return value;
}

static void getCollection (PluginObject *plugin, NPObject *windowObject, const char *attribute, std::vector<AbstractWebElement*> & elements)
{
	elements.clear ();
	// Declare a local variant value.
	NPVariant variantValue;
	if (plugin->getProperty(windowObject, "document", variantValue))
	{
		if (variantValue.type == NPVariantType_Object)
		{
			// Get a pointer to the "document" object.
			NPObject *documentObject = variantValue.value.objectValue;

			// Get the location property from the location object.
			NPVariant variantValue2;

			if (plugin->getProperty(documentObject, attribute,variantValue2 ))
			{
				if (variantValue2.type == NPVariantType_Object)
				{
					NPObject *nodeList = variantValue2.value.objectValue;
					// Now iterate over returned objects
					plugin->nodeListToVector (nodeList, elements);
				}
				npGlobalFuncs.releasevariantvalue(&variantValue2);
			}
		}
		npGlobalFuncs.releasevariantvalue(&variantValue);
	}
}
ChromeWebApplication::~ChromeWebApplication() {
	npGlobalFuncs.releaseobject(windowObject);
}


void ChromeWebApplication::getUrl(std::string & value)
{
	value = getAttribute(plugin, windowObject, "URL");
}



void ChromeWebApplication::getTitle(std::string & value)
{
	value = getAttribute (plugin, windowObject, "title");
}



void ChromeWebApplication::getContent(std::string & value)
{
	value.assign("<not supported>");
}

ChromeWebApplication::ChromeWebApplication(PluginObject *plugin, NPObject *windowObject)
{
	this->plugin = plugin;
	this->windowObject = windowObject;
	npGlobalFuncs.retainobject(windowObject);
}

AbstractWebElement *ChromeWebApplication::getDocumentElement()
{
	NPVariant variantValue;
	ChromeElement *ce = NULL;
	if (plugin->getProperty(windowObject, "document", variantValue))
	{
		if (variantValue.type == NPVariantType_Object)
		{
			ce = new mazinger_chrome::ChromeElement (plugin, variantValue.value.objectValue);
		}
		npGlobalFuncs.releasevariantvalue(&variantValue);
	}
	return ce;
}



void ChromeWebApplication::getElementsByTagName(const char*tag, std::vector<AbstractWebElement*> & elements)
{
	elements.clear ();
	// Declare a local variant value.
	NPVariant variantValue;
	if (plugin->getProperty(windowObject, "document", variantValue))
	{
		if (variantValue.type == NPVariantType_Object)
		{
			// Get a pointer to the "document" object.
			NPObject *documentObject = variantValue.value.objectValue;

			NPVariant variantParam;
			NPVariant variantValue2;
			STRINGZ_TO_NPVARIANT(tag, variantParam);
			// Get the location property from the location object.
			if (plugin->invoke1(documentObject, "getElementsByTagName",
					variantParam, variantValue2 ))
			{
				if (variantValue2.type == NPVariantType_Object)
				{
					NPObject *nodeList = variantValue2.value.objectValue;
					// Now iterate over returned objects
					plugin->nodeListToVector (nodeList, elements);
				}
				npGlobalFuncs.releasevariantvalue(&variantValue2);
			}
		}
	}
	npGlobalFuncs.releasevariantvalue(&variantValue);
}



void ChromeWebApplication::getImages(std::vector<AbstractWebElement*> & elements)
{
	getCollection(plugin, windowObject, "images", elements);
}




void ChromeWebApplication::getLinks(std::vector<AbstractWebElement*> & elements)
{
	getCollection(plugin, windowObject, "links", elements);
}



void ChromeWebApplication::getDomain(std::string & value)
{
	value = getAttribute (plugin, windowObject, "domain");
}



void ChromeWebApplication::getAnchors(std::vector<AbstractWebElement*> & elements)
{
	getCollection(plugin, windowObject, "anchors", elements);
}



void ChromeWebApplication::getCookie(std::string & value)
{
	value = getAttribute (plugin, windowObject, "cookie");
}



AbstractWebElement *ChromeWebApplication::getElementById(const char *id)
{
	// Declare a local variant value.
	NPVariant variantValue;
	AbstractWebElement *element = NULL;
	if ( plugin->getProperty(windowObject, "document", variantValue))
	{
		if (variantValue.type == NPVariantType_Object)
		{

			// Get a pointer to the "document" object.
			NPObject *documentObject = variantValue.value.objectValue;

			NPVariant variantParam;
			NPVariant variantValue2;
			STRINGZ_TO_NPVARIANT(id, variantParam);
			// Get the location property from the location object.
			if (plugin->invoke1(documentObject, "getElementById",
					variantParam, variantValue2 ))
			{
				if (variantValue2.type == NPVariantType_Object)
					element = new ChromeElement(plugin, variantValue2.value.objectValue);
				npGlobalFuncs.releasevariantvalue(&variantValue2);
			}
		}
		npGlobalFuncs.releasevariantvalue(&variantValue);
	}
	return element;
}



void ChromeWebApplication::getForms(std::vector<AbstractWebElement*> & elements)
{
	getCollection(plugin, windowObject, "forms", elements);
}



void ChromeWebApplication::write(const char *text)
{
	// Declare a local variant value.
	NPVariant variantValue;
	NPVariant variantParam;
	if (! plugin->getProperty(windowObject, "document", variantValue))
		return;
	if (variantValue.type != NPVariantType_Object)
		return;
	// Get a pointer to the "document" object.
	NPObject *documentObject = variantValue.value.objectValue;

	STRINGZ_TO_NPVARIANT(text, variantParam);
	// Get the location property from the location object.
	if (! plugin->invoke1(documentObject, "write", variantParam, variantValue ))
		return;
	npGlobalFuncs.releasevariantvalue(&variantValue);
}

void ChromeWebApplication::writeln(const char *text)
{
	// Declare a local variant value.
	NPVariant variantValue;
	NPVariant variantParam;
	if (! plugin->getProperty(windowObject, "document", variantValue))
		return;
	if (variantValue.type != NPVariantType_Object)
		return;
	// Get a pointer to the "document" object.
	NPObject *documentObject = variantValue.value.objectValue;

	STRINGZ_TO_NPVARIANT(text, variantParam);
	// Get the location property from the location object.
	if (! plugin->invoke1(documentObject, "writeln", variantParam, variantValue ))
		return;
	npGlobalFuncs.releasevariantvalue(&variantValue);
}



}
