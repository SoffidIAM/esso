/*
 * PluginObject.h
 *
 *  Created on: 26/02/2014
 *      Author: bubu
 */

#ifndef PLUGINOBJECT_H_
#define PLUGINOBJECT_H_

#include "AfroditaC.h"
#include <MazingerInternal.h>
#include "AbstractWebElement.h"

namespace mazinger_chrome {

class PluginObject: public NPObject {
public:
	PluginObject();
	virtual ~PluginObject();

	NPP instance;
	bool getProperty (NPObject *object, const char *name, NPVariant &v);
	bool invoke0 (NPObject *object, const char *name, NPVariant &v);
	bool invoke1 (NPObject *object, const char *name, NPVariant &param, NPVariant &v);
	bool invoke2 (NPObject *object, const char *name, NPVariant &param1, NPVariant &param2, NPVariant &v);
	bool nodeListToVector (NPObject *nodeeList, std::vector<AbstractWebElement*> &elements);

};

} /* namespace arale */
#endif /* PLUGINOBJECT_H_ */
