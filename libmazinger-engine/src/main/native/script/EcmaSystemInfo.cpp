/* Gabriel Buades 2011. This file released into the public domain. */

/*
 * This module handle windows registry
 *
 *      Registry                - constructor/opener object
 *      Registry.HKEY_LOCAL_MACHINE  -
 *      Registry.HKEY_USERS
 *      Registry.HKEY_CURRENT_USER
 *      Registry.HKEY_CLASSES_ROOT
 *      Registry.prototype      - container object for common methods
 *      Registry.prototype.open      - opens a subkey
 *      Registry.prototype.getValue  - reads data from the registry key
 *      Registry.prototype.setValue  - writes data to the registry
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <MazingerInternal.h>
#include <vector>
#include <see/see.h>
#include <wchar.h>
#include "ecma.h"
#include <stdio.h>
#include <sstream>
#include <strstream>
#include <fstream>
#include <iosfwd>
#include <unistd.h>

#ifdef WIN32
#include <wtsapi32.h>
#endif


/* Prototypes */
static int SystemInfo_mod_init(void);
static void SystemInfo_alloc(struct SEE_interpreter *);
static void SystemInfo_init(struct SEE_interpreter *);

struct SEE_module SystemInfo_module = { SEE_MODULE_MAGIC, /* magic */
"SystemInfo", /* name */
"1.0", /* version */
0, /* index (set by SEE) */
SystemInfo_mod_init, /* mod_init */
SystemInfo_alloc, /* alloc */
SystemInfo_init /* init */
};

/*
 * We use a private structure to hold per-interpeter data private to this
 * module. It can be accessed through the SEE_MODULE_PRIVATE() macro, or
 * through the simpler PRIVATE macro that we define below.
 * The private data we hold is simply original pointers to the objects
 * that we make during alloc/init. This is because (a) a script is able to
 * change the objects locations at runtime, and (b) this is slightly more
 * efficient than performing a runtime lookup with SEE_OBJECT_GET.
 */
struct module_private {
};

#define PRIVATE(interp)  \
        ((struct module_private *)SEE_MODULE_PRIVATE(interp, &Registry_module))

/*
 * To make string usage more efficient, we globally intern some common
 * strings and provide a STR() macro to access them.
 * Internalised strings are guaranteed to have unique pointers,
 * which means you can use '==' instead of 'strcmp()' to compare names.
 * The pointers must be allocated during mod_init() because the global
 * intern table is locked as soon as an interpreter instance is created.
 */
#define STR(name) s_##name
static struct SEE_string *STR(os);
static struct SEE_string *STR(osVersion);
static struct SEE_string *STR(osDistribution);
static struct SEE_string *STR(hostName);
static struct SEE_string *STR(clientHostName);
static struct SEE_string *STR(systemInfo);
static struct SEE_string *STR(fileSeparator);
static struct SEE_string *STR(username);

/*
 * The 'systemInfo_inst_class' class structure describes how to carry out
 * all object operations on a file_object instance. You can see that
 * many of the function slots point directly to SEE_native_* functions.
 * This is because file_object wraps a SEE_native structure, and we can
 * get all the standard ('native') ECMAScript object behaviour for free.
 * If we didn't want this behaviour, and instead used struct SEE_object as
 * the basis of the file_object structure, then we should use the SEE_no_*
 * functions, or wrappers around them.
 *
 * In this class, there is no need for a [[Construct]] nor [[Call]]
 * property, so those are left at NULL.
 */
static struct SEE_objectclass systemInfo_inst_class = { "SystemInfo", /* Class */
SEE_native_get, /* Get */
SEE_native_put, /* Put */
SEE_native_canput, /* CanPut */
SEE_native_hasproperty, /* HasProperty */
SEE_native_delete, /* Delete */
SEE_native_defaultvalue, /* DefaultValue */
SEE_native_enumerator, /* DefaultValue */
NULL, /* Construct */
NULL, /* Call */
NULL /* HasInstance */
};




/**
 * systemInfo.getOs
 */

static void systemInfo_setOs(struct SEE_interpreter *interp,
		struct SEE_object *obj) {

#ifdef WIN32
	const char* os = "Windows";
#else
	const char* os = "Linux";
#endif
	const int len = strlen(os);
	SEE_string *s = SEE_intern_global(os);
	SEE_value v;
	SEE_SET_STRING(&v, s);
	SEE_OBJECT_PUT(interp, obj, STR(os), &v, SEE_ATTR_READONLY);
}

/**
 * systemInfo.getOs
 */

static void systemInfo_setUser(struct SEE_interpreter *interp,
		struct SEE_object *obj) {

	SEE_string *s = SEE_intern_global(MZNC_getUserName());
	SEE_value v;
	SEE_SET_STRING(&v, s);
	SEE_OBJECT_PUT(interp, obj, STR(username), &v, SEE_ATTR_READONLY);
}



/**
 * systemInfo.getOs
 */

static void systemInfo_setOsVersion(struct SEE_interpreter *interp,
		struct SEE_object *obj) {
	std::string osVersion;
#ifdef WIN32
	OSVERSIONINFO info;
	info.dwOSVersionInfoSize = sizeof info;
	if (GetVersionEx (&info)) {
		std::stringstream sstr;
		sstr << info.dwMajorVersion << "." << info.dwMinorVersion;
		osVersion = sstr.str();
	} else {
		osVersion = "?";
	}
#else
	FILE *f = fopen ("/etc/lsb-release", "r");
	const char *prefix = "DISTRIB_RELEASE=";
	char achBuffer[1024];
	while (fgets (achBuffer, sizeof achBuffer - 1, f) != NULL) {
		std::string s = achBuffer;
		size_t pos = s.find(prefix);
		if ( pos != std::string::npos) {
			pos += strlen (prefix);
			osVersion = s.substr(pos);
			pos = osVersion.find("\n");
			if ( pos != std::string::npos) {
				osVersion = osVersion.substr(0, pos);
			}
		}
	}
#endif
	SEE_string *s = SEE_CharsToString(interp, osVersion.c_str());
	SEE_value v;
	SEE_SET_STRING(&v, s);
	SEE_OBJECT_PUT(interp, obj, STR(osVersion), &v, SEE_ATTR_READONLY);
}

/**
 * systemInfo.getOsDistribution
 */

static void systemInfo_setOsDistribution(struct SEE_interpreter *interp,
		struct SEE_object *obj) {
	std::string osDistribution;
#ifdef WIN32
	osDistribution = "Microsoft";
#else
	std::ifstream f ("/etc/lsb-release");

	const char *prefix = "DISTRIB_ID=";
	while (!f.eof()) {
		std::string s;
		std::getline(f, s);
		size_t pos = s.find(prefix);
		if ( pos != std::string::npos) {
			pos += strlen (prefix);
			osDistribution = s.substr(pos);
			pos = osDistribution.find("\n");
			if ( pos != std::string::npos) {
				osDistribution = osDistribution.substr(0, pos);
			}
		}
	}
#endif
	SEE_string *s = SEE_CharsToString(interp, osDistribution.c_str());
	SEE_value v;
	SEE_SET_STRING(&v, s);
	SEE_OBJECT_PUT(interp, obj, STR(osDistribution), &v, SEE_ATTR_READONLY);
}



/**
 * systemInfo.fileSeparator
 */

static void systemInfo_setFileSeparator(struct SEE_interpreter *interp,
		struct SEE_object *obj) {
#ifdef WIN32
	SEE_string *s = SEE_intern_global("\\");
#else
	SEE_string *s = SEE_intern_global("/");
#endif
	SEE_value v;
	SEE_SET_STRING(&v, s);
	SEE_OBJECT_PUT(interp, obj, STR(fileSeparator), &v, SEE_ATTR_READONLY);
}

/**
 * systemInfo.getHostName
 */

static void systemInfo_setHostName(struct SEE_interpreter *interp,
		struct SEE_object *obj) {
	SEE_string *s = SEE_CharsToString(interp, MZNC_getHostName());
	SEE_value v;
	SEE_SET_STRING(&v, s);
	SEE_OBJECT_PUT(interp, obj, STR(hostName), &v, SEE_ATTR_READONLY);
}

/**
 * systemInfo.getClientHostName
 */
#ifdef WIN32

static HINSTANCE hWTSAPI;


typedef BOOL (WINAPI *typeWTSQuerySessionInformation)( IN HANDLE hServer,
		IN DWORD SessionId, IN WTS_INFO_CLASS WTSInfoClass,
		OUT char* * ppBuffer, OUT DWORD * pBytesReturned);
static typeWTSQuerySessionInformation pWTSQuerySessionInformation = NULL;
typedef void (WINAPI *typeWTSFreeMemory)(PVOID pMemory);
static typeWTSFreeMemory pWTSFreeMemory = NULL;


static void freeCitrixMemory(PVOID pMemory) {

	/*
	 *  Get handle to WTSAPI.DLL
	 */
	if (hWTSAPI == NULL && (hWTSAPI = LoadLibrary("WTSAPI32")) == NULL) {
		return;
	}

	/*
	 *  Get entry point for WTSEnumerateServers
	 */
	if (pWTSFreeMemory == NULL)
		pWTSFreeMemory = (typeWTSFreeMemory) GetProcAddress(hWTSAPI,
				"WTSFreeMemory");
	if (pWTSFreeMemory == NULL) {
		return;
	}
	(*pWTSFreeMemory)(pMemory);
}

static void getCitrixClientName(std::string &name) {
	char* lpszBytes = (char*) LocalAlloc (LMEM_FIXED, 100);
	DWORD dwBytes = 99;
	BOOL bOK = FALSE;
	name.clear ();

	/*
	 *  Get handle to WTSAPI.DLL
	 */
	if (hWTSAPI == NULL && (hWTSAPI = LoadLibrary("WTSAPI32")) == NULL) {
		return;
	}

	/*
	 *  Get entry point for WTSEnumerateServers
	 */
	if (pWTSQuerySessionInformation == NULL)
		pWTSQuerySessionInformation
				= (typeWTSQuerySessionInformation) GetProcAddress(hWTSAPI,
						"WTSQuerySessionInformationA");
	if (pWTSQuerySessionInformation == NULL) {
		name.clear ();
		return;
	}
	bOK = (*pWTSQuerySessionInformation)(WTS_CURRENT_SERVER_HANDLE,
			WTS_CURRENT_SESSION, WTSClientName, &lpszBytes, &dwBytes);
	if (bOK) {
		name.assign (lpszBytes);
		freeCitrixMemory(lpszBytes);
	} else
		name.clear ();
}
#endif
static void systemInfo_setClientHostName(struct SEE_interpreter *interp,
		struct SEE_object *obj) {
	std::string name;
#ifdef WIN32
	getCitrixClientName (name);
#endif
	SEE_string *s = SEE_CharsToString(interp, name.c_str());
	SEE_value v;
	SEE_SET_STRING(&v, s);
	SEE_OBJECT_PUT(interp, obj, STR(clientHostName), &v, SEE_ATTR_READONLY);
}


/*
 * mod_init: The module initialisation function.
 * This function is called exactly once, before any interpreters have
 * been created.
 * You can use this to set up global intern strings (like I've done),
 * and/or to initialise global data independent of any single interpreter.
 *
 * It is possible to call SEE_module_add() recursively from this
 * function. However, if a mod_init fails (returns non-zero), SEE
 * will also 'forget' about all the modules that it recursively added,
 * even if they succeeded.
 */
static int SystemInfo_mod_init() {
	STR(systemInfo) = SEE_intern_global("SystemInfo");
	STR(os) = SEE_intern_global("os");
	STR(osVersion) = SEE_intern_global("osVersion");
	STR(osDistribution) = SEE_intern_global("osDistribution");
	STR(hostName) = SEE_intern_global("hostName");
	STR(clientHostName) = SEE_intern_global("clientHostName");
	STR(fileSeparator) = SEE_intern_global("fileSeparator");
	STR(username) = SEE_intern_global("username");
	return 0;
}

/*
 * alloc: Per-interpreter allocation function.
 * This optional function is called early during interpreter initialisation,
 * but before the interpreter is completely initialised. At this stage,
 * the interpreter is not really gety for use; only some storage has been
 * allocated, so you should not invoke any property accessors at this stage.
 * So, why is this function available? It turns out to be useful if you have
 * mutually dependent modules that, during init(), need to find pointers in
 * to the other modules.
 *
 * In this module, we use the alloc function simply to allocate the
 * per-interpreter module-private storage structure, which we access
 * later through the PRIVATE() macro.
 */
static void SystemInfo_alloc(struct SEE_interpreter *interp) {
	SEE_MODULE_PRIVATE(interp, &SystemInfo_module) =
			SEE_NEW(interp, struct module_private);
}

/*
 * init: Per-interpreter initialisation function.
 * This is the real workhorse of the module. Its job is to build up
 * an initial collection of host objects and install them into a fresh
 * interpreter instance. This function is called every time an interpreter
 * is created.
 *
 * Here we create the 'File' container/constructor object, and
 * populate it with 'File.prototype', 'File.In', etc. The 'File.prototype'
 * object is also created, and given its cfunction properties,
 * 'File.prototype.get', 'File.prototype.put', etc.
 * Functions and methods are most easily created using SEE_cfunction_make().
 *
 * Also, a 'File.FileError' exception is also created (for use when
 * throwing get or put errors) and the whole tree of new objects is
 * published by inserting the toplevel 'File' object into the interpreter
 * by making it a property of the Global object.
 */

static void SystemInfo_init(struct SEE_interpreter *interp) {
	struct SEE_native *systemInfo_object;
	struct SEE_value v;

	/* Convenience macro for adding properties to File */
#define PUTOBJ(parent, name, obj)                                       \
        SEE_SET_OBJECT(&v, obj);                                        \
        SEE_OBJECT_PUT(interp, parent, STR(name), &v, SEE_ATTR_DEFAULT);

	/* Convenience macro for adding functions to File.prototype */
#define PUTFUNC(obj, name, len)                                         \
        SEE_SET_OBJECT(&v, SEE_cfunction_make(interp, systemInfo_##name,\
                STR(name), len));                                       \
        SEE_OBJECT_PUT(interp, obj, STR(name), &v, SEE_ATTR_DEFAULT);

	/* Create the File.prototype object (cf. newfile()) */
	systemInfo_object = SEE_NEW(interp,	struct SEE_native);
	SEE_native_init(systemInfo_object, interp,
			&systemInfo_inst_class, interp->Object_prototype);

//	systemInfo_setClientHostName(interp, &systemInfo_object->object);
	systemInfo_setOs(interp, &systemInfo_object->object);
	systemInfo_setOsVersion(interp, &systemInfo_object->object);
	systemInfo_setOsDistribution(interp, &systemInfo_object->object);
	systemInfo_setHostName(interp, &systemInfo_object->object);
	systemInfo_setFileSeparator(interp, &systemInfo_object->object);
	systemInfo_setUser(interp, &systemInfo_object->object);


	PUTOBJ(interp->Global, systemInfo, &systemInfo_object->object);


	return;

}
#undef PUTFUNC
#undef PUTOBJ
