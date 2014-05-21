/*
 * AfroditaC.cpp
 *
 *  Created on: 22/11/2010
 *      Author: u07286
 */

#include "AfroditaC.h"
#include <MazingerInternal.h>
#include <stdio.h>
#include <string.h>
#include "PluginObject.h"
#include "ChromeWebApplication.h"

#define MIME_TYPE_DESCRIPTION "application/soffid-sso-plugin:sso:Soffid SSO Plugin"

NPNetscapeFuncs npGlobalFuncs;

FILE *f = stdout;

#ifdef WIN32

extern "C" BOOL __stdcall DllMain(HINSTANCE hinstDLL, DWORD dwReason,
		LPVOID lpvReserved) {

	if (dwReason == DLL_PROCESS_ATTACH) {
		hMazingerInstance = hinstDLL;
//		MessageBox (NULL, "HELLO", "I AM AFRODITA-C", MB_OK);
	}
	return TRUE;
}
#define WINAPI __stdcall

#else
extern "C" void __attribute__((constructor)) startup() {
	printf("Hola caracola\n");
	f = fopen("/tmp/google-chrome.log", "a+");
	setbuf(f, NULL);
	fprintf(f, "DllMain\n");
	if (f == NULL)
		printf("Cannot create log file");
}
#define WINAPI
#endif

bool SetReturnValue(const bool value, NPVariant &result) {
	BOOLEAN_TO_NPVARIANT(value, result);
	return true;
}


static void StubInvalidate(NPObject *npobj) {
}

static bool StubInvokeDefault(NPObject *npobj, const NPVariant *args,
		uint32_t argCount, NPVariant *result) {
	fprintf (f, "StubInvokeDefault\n");
	return false;
}

static bool StubHasProperty(NPObject * npobj, NPIdentifier name) {
	return false;
}

static bool StubGetProperty(NPObject *npobj, NPIdentifier name,
		NPVariant *result) {
	return false;
}

static bool StubSetProperty(NPObject *npobj, NPIdentifier name,
		const NPVariant *value) {
	return false;
}

static bool StubRemoveProperty(NPObject *npobj, NPIdentifier name) {
	return false;
}

static bool StubEnumerate(NPObject *npobj, NPIdentifier **identifier,
		uint32_t *count) {
	return false;
}

static bool StubConstruct(NPObject *npobj, const NPVariant *args,
		uint32_t argCount, NPVariant *result) {
	return false;
}

NPObject *Allocate(NPP instance, NPClass *clazz) {
	fprintf(f, "Allocate\n");
	NPObject *obj = new mazinger_chrome::PluginObject ();
	obj->_class = clazz;
	obj->referenceCount = 0;
	return obj;
}

void Deallocate(NPObject *obj) {
	fprintf(f, "Deallocate\n");
    delete (mazinger_chrome::PluginObject *)obj;
}

void SetInstance(NPP instance, NPObject *passedObj) {
	fprintf(f, "SetInstance\n");
//	NPClassWithNPP *obj = (NPClassWithNPP *)passedObj;
//	obj->npp = instance;
}

bool HasJavascriptMethod(NPObject *npobj, NPIdentifier name) {
	//const char *method = browser_funcs_->utf8fromidentifier(name);
	fprintf(f, "HasJavascriptmethod");
	const char *methodName = npGlobalFuncs.utf8fromidentifier(name);
	fprintf(f, " method=%s\n", methodName);

	return true;
}

bool InvokeJavascript(NPObject *npobj, NPIdentifier name, const NPVariant *args,
		uint32_t argCount, NPVariant *result) {
	const char *methodName = npGlobalFuncs.utf8fromidentifier(name);
	bool success = false;
	mazinger_chrome::PluginObject *object = static_cast<mazinger_chrome::PluginObject*> (npobj);

	fprintf(f, "Calling %s\n", methodName);

	if (!strcmp(methodName, "test")) {
		char *sz = (char*) npGlobalFuncs.memalloc(100);
		strcpy (sz, "HOLA CARACOLA");
		STRINGZ_TO_NPVARIANT(sz, *result);
		success = true;
	}
	else if (!strcmp(methodName, "run")) {
		return SetReturnValue (true, *result);
	}
//	npGlobalFuncs.memfree((void *) methodName);

	fprintf(f, "Finished %s\n", methodName);
	return success;
}

static NPClass JavascriptListener_NPClass = { NP_CLASS_STRUCT_VERSION_CTOR,
		Allocate, Deallocate, StubInvalidate, HasJavascriptMethod,
		InvokeJavascript, StubInvokeDefault, StubHasProperty, StubGetProperty,
		StubSetProperty, StubRemoveProperty, StubEnumerate, StubConstruct }; //NPClass JavascriptListener_NPClass

static void run (NPP instance)
{
	// Get the Dom window object.
	NPObject *sWindowObj;
	npGlobalFuncs.getvalue( instance, NPNVWindowNPObject, &sWindowObj );
	mazinger_chrome::PluginObject plugin;
	plugin.instance = instance;
	mazinger_chrome::ChromeWebApplication app (&plugin, sWindowObj);

	npGlobalFuncs.releaseobject(sWindowObj);

	MZNWebMatch (&app);

}


NPError NPP_New(NPMIMEType pluginType, NPP instance, uint16_t mode,
		int16_t argc, char *argn[], char *argv[], NPSavedData *saved) {
	fprintf(f, "NPP_New\n");

	npGlobalFuncs.setvalue(instance, NPPVpluginWindowBool, (void*) false);

	NPObject* object;

	if (npGlobalFuncs.getvalue(instance, NPNVPluginElementNPObject,
			&object) != NPERR_NO_ERROR)
	{
		return NPERR_MODULE_LOAD_FAILED_ERROR;
	}
	run (instance);

//	npGlobalFuncs.releaseobject(object);

	return NPERR_NO_ERROR;
}

NPError NPP_Destroy(NPP instance, NPSavedData **save) {
	fprintf(f, "NPP_Destroy\n");
	return NPERR_NO_ERROR;
}

int16_t NPP_HandleEvent(NPP instance, void *event) {
	fprintf(f, "NPP_HandleEvent\n");
	return 0;
}

NPError NPP_DestroyStream(NPP instance, NPStream* stream, NPReason reason) {
	fprintf(f, "NPP_DestroyStream\n");
	return NPERR_NO_ERROR;
}

NPError NPP_NewStream(NPP instance, NPMIMEType type, NPStream* stream,
		NPBool seekable, uint16_t* stype) {
	fprintf(f, "NPP_NewStream\n");
	return NPERR_NO_ERROR;
}

void NPP_Print(NPP instance, NPPrint* PrintInfo) {
	fprintf(f, "NPP_Print\n");
}

NPError NPP_SetValue(NPP instance, NPNVariable variable, void *value) {
	fprintf(f, "NPP_SetValue\n");
	return NPERR_NO_ERROR;
}

NPError NPP_SetWindow(NPP instance, NPWindow *window) {
	fprintf(f, "NPP_SetWindow\n");
	return NPERR_NO_ERROR;
}

void NPP_StreamAsFile(NPP instance, NPStream* stream, const char* fname) {
	fprintf(f, "NPP_StreamAsFile\n");
}

void NPP_URLNotify(NPP instance, const char* url, NPReason reason,
		void* notifyData) {
	fprintf(f, "NPP_URLNotifyS\n");

}

int32_t NPP_Write(NPP instance, NPStream* stream, int32_t offset, int32_t len,
		void* buf) {
	fprintf(f, "NPP_Write\n");
	return 0;
}

int32_t NPP_WriteReady(NPP instance, NPStream* stream) {
	fprintf(f, "NPP_WriteReady\n");
	return 0;
}

//For Javascript listener

NPError NPP_GetValue(NPP instance, NPPVariable variable, void *value) {
	switch (variable) {
	case NPPVpluginScriptableNPObject: {
		fprintf(f, "NPP_GetValue SCRIPTABLE OBJECT\n");
		mazinger_chrome::PluginObject * javascriptListener = (mazinger_chrome::PluginObject *) npGlobalFuncs.createobject(instance,
				(NPClass *) &JavascriptListener_NPClass);
		*((NPObject **) value) = javascriptListener;
		javascriptListener->instance = instance;
		break;
	}
	case NPPVpluginNeedsXEmbed: {
		*((bool *) value) = true;
		break;
	}
	default: {
		return NPERR_INVALID_PARAM;
	}
	}
	return NPERR_NO_ERROR;
}

extern "C" {

NPError WINAPI NP_GetEntryPoints(NPPluginFuncs *pluginFuncs) {
	fprintf(f, "NP_GetEntryPoints\n");

	pluginFuncs->newp = NPP_New;
	pluginFuncs->destroy = NPP_Destroy;
	pluginFuncs->setwindow = NPP_SetWindow;
	pluginFuncs->newstream = NPP_NewStream;
	pluginFuncs->destroystream = NPP_DestroyStream;
	pluginFuncs->asfile = NPP_StreamAsFile;
	pluginFuncs->writeready = NPP_WriteReady;
	pluginFuncs->write = NPP_Write;
	pluginFuncs->print = NPP_Print;
	pluginFuncs->event = NPP_HandleEvent;
	pluginFuncs->urlnotify = NPP_URLNotify;
	pluginFuncs->getvalue = NPP_GetValue;
	pluginFuncs->setvalue = NPP_SetValue;
	pluginFuncs->javaClass = NULL;
	pluginFuncs->size = sizeof(*pluginFuncs);
	pluginFuncs->version = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;

	return NPERR_NO_ERROR;

}

#if defined(WIN32)
NPError WINAPI NP_Initialize(NPNetscapeFuncs *npFuncs) {
	fprintf(f, "NP_Initialize\n");
	npGlobalFuncs = *npFuncs;
	return NPERR_NO_ERROR;
}

#else
NPError WINAPI NP_Initialize(NPNetscapeFuncs *npFuncs,
		NPPluginFuncs *plugin_funcs) {
	fprintf(f, "NP_Initialize\n");
	npGlobalFuncs = *npFuncs;

	NP_GetEntryPoints(plugin_funcs);

	return NPERR_NO_ERROR;
}
#endif

const char* WINAPI NP_GetMIMEDescription() {
	fprintf(f, "NPP_GetMIMEDescription\n");
	return MIME_TYPE_DESCRIPTION;
}

NPError NP_GetValue(NPP instance, NPPVariable variable, void *value) {
	fprintf(f, "NP_GetValue %d\n", (int) variable);
	switch (variable) {
	case NPPVpluginNameString:
		*((char **) value) = (char *) "Soffid ESSO Plugin";
		return NPERR_NO_ERROR;
	case NPPVpluginDescriptionString:
		*((char **) value) =
				(char *) "Soffid Enterprise Single Sign on version 1.0-beta";
		return NPERR_NO_ERROR;
	default:
		return NPERR_INVALID_PARAM;
	}
	return NPERR_NO_ERROR;
}

NPError WINAPI NP_Shutdown() {
	fprintf(f, "NP_Shutdown\n");
	return NPERR_NO_ERROR;
}

}
