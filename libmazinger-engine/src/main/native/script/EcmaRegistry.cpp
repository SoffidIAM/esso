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

/* Prototypes */
static int Registry_mod_init(void);
static void Registry_alloc(struct SEE_interpreter *);
static void Registry_init(struct SEE_interpreter *);

static void registry_construct(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);

static void registry_proto_getValue(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void registry_proto_setValue(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void registry_proto_openKey(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void registry_proto_createKey(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void registry_proto_deleteKey(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);

static void registry_finalize(struct SEE_interpreter *, void *, void *);

/*
 * The File_module structure is the only symbol exported here.
 * It contains some simple identification information but, most
 * importantly, pointers to the major initialisation functions in
 * this file.
 * This structure is passed to SEE_module_add() once and early.
 */
struct SEE_module Registry_module = { SEE_MODULE_MAGIC, /* magic */
"Registry", /* name */
"1.0", /* version */
0, /* index (set by SEE) */
Registry_mod_init, /* mod_init */
Registry_alloc, /* alloc */
Registry_init /* init */
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
	struct SEE_object *Registry; /* The File object */
    struct SEE_object *RegistryError;      /* Registry.RegistryError */
	struct SEE_object *Registry_prototype; /* File.prototype */
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
static struct SEE_string *STR(Registry);
static struct SEE_string *STR(deleteKey);
static struct SEE_string *STR(openKey);
static struct SEE_string *STR(createKey);
static struct SEE_string *STR(hkey_LOCAL_MACHINE);
static struct SEE_string *STR(hkey_USERS);
static struct SEE_string *STR(hkey_CURRENT_USER);
static struct SEE_string *STR(hkey_CLASSES_ROOT);
static struct SEE_string *STR(hkey_LOCAL_MACHINE32);
static struct SEE_string *STR(hkey_USERS32);
static struct SEE_string *STR(hkey_CURRENT_USER32);
static struct SEE_string *STR(hkey_CLASSES_ROOT32);
static struct SEE_string *STR(getValue);
static struct SEE_string *STR(setValue);
static struct SEE_string *STR(prototype);
static struct SEE_string *STR(path);
static struct SEE_string *STR(RegistryError);
static struct SEE_string *STR(length);

struct Registry_object {
	struct SEE_native native;
	bool b32bits;
};

static struct SEE_object * newregistry(struct SEE_interpreter *interp, const wchar_t *szRegistry, bool b32bits);

/*
 * The 'file_inst_class' class structure describes how to carry out
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
static struct SEE_objectclass registry_inst_class = { "Registry", /* Class */
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

/*
 * This is the class structure for the toplevel 'Registry' object. The 'Registry'
 * object doubles as both a constructor function and as a container object
 * to hold File.prototype and some other useful properties. 'Registry' has only
 * one important intrinsic property, namely the [[Construct]] method. It
 * is called whenever the expression 'new Registry()' is evaluated.
 */
static struct SEE_objectclass registry_constructor_class = { "Registry", /* Class */
SEE_native_get, /* Get */
SEE_native_put, /* Put */
SEE_native_canput, /* CanPut */
SEE_native_hasproperty, /* HasProperty */
SEE_native_delete, /* Delete */
SEE_native_defaultvalue, /* DefaultValue */
SEE_native_enumerator, /* DefaultValue */
registry_construct, /* Construct */
NULL /* Call */
};

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
static int Registry_mod_init() {
	STR(Registry) = SEE_intern_global("Registry");
	STR(setValue) = SEE_intern_global("setValue");
	STR(getValue) = SEE_intern_global("getValue");
	STR(openKey)  = SEE_intern_global("deleteKey");
	STR(openKey)  = SEE_intern_global("openKey");
	STR(createKey)  = SEE_intern_global("createKey");
	STR(hkey_LOCAL_MACHINE) = SEE_intern_global("HKEY_LOCAL_MACHINE");
	STR(hkey_CLASSES_ROOT)  = SEE_intern_global("HKEY_CLASSES_ROOT");
	STR(hkey_USERS)         = SEE_intern_global("HKEY_USERS");
	STR(hkey_CURRENT_USER)  = SEE_intern_global("HKEY_CURRENT_USER");
	STR(hkey_LOCAL_MACHINE32) = SEE_intern_global("HKEY_LOCAL_MACHINE32");
	STR(hkey_CLASSES_ROOT32)  = SEE_intern_global("HKEY_CLASSES_ROOT32");
	STR(hkey_USERS32)         = SEE_intern_global("HKEY_USERS32");
	STR(hkey_CURRENT_USER32)  = SEE_intern_global("HKEY_CURRENT_USER32");
	STR(RegistryError)      = SEE_intern_global("RegistryError");
	STR(path)               = SEE_intern_global("path");
	STR(length)             = SEE_intern_global("length");
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
static void Registry_alloc(struct SEE_interpreter *interp) {
	SEE_MODULE_PRIVATE(interp, &Registry_module)
			= SEE_NEW(interp, struct module_private);
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


static void Registry_init(struct SEE_interpreter *interp) {
	struct SEE_object *Registry, *Registry_prototype, *RegistryError;
	struct SEE_object *hklm, *hku, *hkcu, *hkcr;
	struct SEE_value v;



	/* Convenience macro for adding properties to File */
#define PUTOBJ(parent, name, obj)                                       \
        SEE_SET_OBJECT(&v, obj);                                        \
        SEE_OBJECT_PUT(interp, parent, STR(name), &v, SEE_ATTR_DEFAULT);

	/* Convenience macro for adding functions to File.prototype */
#define PUTFUNC(obj, name, len)                                         \
        SEE_SET_OBJECT(&v, SEE_cfunction_make(interp, registry_proto_##name,\
                STR(name), len));                                       \
        SEE_OBJECT_PUT(interp, obj, STR(name), &v, SEE_ATTR_DEFAULT);

	/* Create the File.prototype object (cf. newfile()) */
	Registry_prototype = (struct SEE_object *) SEE_NEW(interp,
			struct Registry_object);
	SEE_native_init((struct SEE_native *) Registry_prototype, interp,
			&registry_inst_class, interp->Object_prototype);

	PUTFUNC(Registry_prototype, getValue, 1)
	PUTFUNC(Registry_prototype, setValue, 2)
	PUTFUNC(Registry_prototype, deleteKey, 1)
	PUTFUNC(Registry_prototype, openKey, 1)
	PUTFUNC(Registry_prototype, createKey, 1)

	/* Create the File object */
	Registry = (struct SEE_object *) SEE_NEW(interp, struct SEE_native);
	SEE_native_init((struct SEE_native *) Registry, interp,
			&registry_constructor_class, interp->Object_prototype);
	PUTOBJ(interp->Global, Registry, Registry);
	PUTOBJ(Registry, prototype, Registry_prototype)

    /* Create the File.FileError error object for I/O exceptions */
    RegistryError = SEE_Error_make(interp, STR(RegistryError));
    PUTOBJ(Registry, RegistryError, RegistryError);

    /* Keep pointers to our 'original' objects */
	PRIVATE(interp)->Registry_prototype = Registry_prototype;
	PRIVATE(interp)->Registry = Registry;
	PRIVATE(interp)->RegistryError = RegistryError;

	/* Create the root key objects */
    hklm = newregistry(interp, L"HKEY_LOCAL_MACHINE", false);
    PUTOBJ(Registry, hkey_LOCAL_MACHINE, hklm);

    hku = newregistry(interp, L"HKEY_USERS", false);
    PUTOBJ(Registry, hkey_USERS, hku);

    hkcu = newregistry(interp, L"HKEY_CURRENT_USER", false);
    PUTOBJ(Registry, hkey_CURRENT_USER, hkcu);

    hkcr = newregistry(interp, L"HKEY_CLASSES_ROOT", false);
    PUTOBJ(Registry, hkey_CLASSES_ROOT, hkcr);

	/* Create the root 32bits key objects */
    hklm = newregistry(interp, L"HKEY_LOCAL_MACHINE", true);
    PUTOBJ(Registry, hkey_LOCAL_MACHINE32, hklm);

    hku = newregistry(interp, L"HKEY_USERS", true);
    PUTOBJ(Registry, hkey_USERS32, hku);

    hkcu = newregistry(interp, L"HKEY_CURRENT_USER", true);
    PUTOBJ(Registry, hkey_CURRENT_USER32, hkcu);

    hkcr = newregistry(interp, L"HKEY_CLASSES_ROOT", true);
    PUTOBJ(Registry, hkey_CLASSES_ROOT32, hkcr);

	return;

}
#undef PUTFUNC
#undef PUTOBJ

#ifdef WIN32
/*
 * Converts an object into file_object, or throws a TypeError.
 *
 * This helper functon is called by the method functions in File.prototype
 * mainly to check that each method is being called with a correct 'this'.
 * Because a script may assign the member functions objects to a different
 * (non-file) object and invoke them, we cannot assume the thisobj pointer
 * always points to a 'struct file_object' structure.
 * In effect, this function is a 'safe' cast.
 * NOTE: There is a check for null because 'thisobj' can potentially
 * be a NULL pointer.
 */
static struct Registry_object *
toregistry(struct SEE_interpreter *interp, struct SEE_object *o) {
	if (!o || o->objectclass != &registry_inst_class)
		SEE_error_throw(interp, interp->TypeError, NULL);
	return (struct Registry_object *) o;
}
#endif

/*
 * Constructs a file object instance.
 * This helper function constructs and returns a new instance of a
 * file_object. It initialises the object with the given FILE pointer.
 */
static struct SEE_object *
newregistry(struct SEE_interpreter *interp, const wchar_t *szRegistry, bool b32bits) {
	struct Registry_object *obj;

	obj= SEE_NEW_FINALIZE(interp, struct Registry_object, registry_finalize, NULL);
	SEE_native_init(&obj->native, interp, &registry_inst_class,
			PRIVATE(interp)->Registry_prototype);
	if (szRegistry != NULL) {
		obj->b32bits = b32bits;
		SEE_string * s = SEE_WCharsToString (interp, szRegistry);
		SEE_value v ;
		SEE_SET_STRING(&v, s);
		SEE_OBJECT_PUT(interp, &obj->native.object, STR(path),  &v, SEE_ATTR_READONLY);
	}
	return (struct SEE_object *) obj;
}

/*
 * A finalizer function that is (eventually) called on lost file objects.
 * If a system crash or exit occurs, this function may not be called. SEE
 * cannot guarantee that this is ever called; however your garbage
 * collector implementation may guarantee it.
 */
static void registry_finalize(struct SEE_interpreter *interp, void *obj,
		void *closure) {

	// Nothing to do
}

/*
 * new Regsitry(pathame) -> object
 *
 * The Registry.[[Construct]] property is called when the user writes
 * "new Regisry()". This constructor expects one argument:
 *      argv[0] the key to open
 *
 *
 * Note: 'undefined' optional arguments are treated as if they were
 * missing. (This is a common style in ECMAScript objects.)
 */

static void registry_construct(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {
	struct SEE_object *newobj;
	struct SEE_string *pathString = NULL;

	SEE_parse_args(interp, argc, argv, "s", &pathString);
	if (!pathString)
		SEE_error_throw(interp, interp->RangeError, "missing argument");

	newobj = newregistry(interp, NULL, false);

	SEE_value v ;
	SEE_SET_STRING(&v, pathString);
	SEE_OBJECT_PUT(interp, newobj, STR(path),  &v, SEE_ATTR_READONLY);

	SEE_SET_OBJECT(res, newobj);
}

#ifdef WIN32

#ifndef KEY_WOW64_32KEY
#define KEY_WOW64_32KEY 0x0200
#endif

typedef LONG WINAPI (*RegDeleteKeyExWType) (
    HKEY hKey,
    LPCWSTR lpSubKey,
    REGSAM samDesired,
    DWORD Reserved
);

static RegDeleteKeyExWType customRegDeleteKeyExW = NULL;

static bool splitRegistryString (const wchar_t *szRegistry, HKEY &hRootKey, std::wstring &szChild) {
	wchar_t achRoot[50];
	const wchar_t *szTail;
	unsigned int i;

	for (i = 0; i < 50 && szRegistry[i] != '\\' && szRegistry[i]
			!= '\0'; i++) {
		achRoot[i] = szRegistry[i];
	}
	// Raiz demasiado larga
	if (i >= 50)
		return NULL;
	szTail = &szRegistry[i];
	if (szTail[0] == L'\\')
		szTail++;

	achRoot[i] = L'\0';

	if (wcscmp(achRoot, L"HKEY_CLASSES_ROOT") == 0)
		hRootKey = HKEY_CLASSES_ROOT;
	else if (wcscmp(achRoot, L"HKEY_LOCAL_MACHINE") == 0)
		hRootKey = HKEY_LOCAL_MACHINE;
	else if (wcscmp(achRoot, L"HKEY_CURRENT_USER") == 0)
		hRootKey = HKEY_CURRENT_USER;
	else if (wcscmp(achRoot, L"HKEY_USERS") == 0)
		hRootKey = HKEY_USERS;
	else {
		hRootKey = NULL;
		return false;
	}

//	wprintf (L"Original = %s Root = %s tail=%s i =%d\n", szRegistry, achRoot, szTail, i);

	szChild = szTail;
	return true;
}


static HKEY openRegistry(const wchar_t *szRegistry, bool b32bits, bool readwrite) {
	HKEY hRootKey;
	std::wstring tail;

	if (! splitRegistryString(szRegistry, hRootKey, tail))
		return NULL;

	HKEY hKey;
	if (readwrite && RegCreateKeyExW(hRootKey, tail.c_str(), 0, NULL,
			0, ( b32bits ? KEY_WOW64_32KEY: 0 ) | KEY_WRITE | KEY_READ,
			NULL, &hKey, NULL) == ERROR_SUCCESS ) {
		return hKey;
	}
	else if (RegOpenKeyExW(hRootKey, tail.c_str(), 0, (b32bits? KEY_WOW64_32KEY: 0 ) |
												(readwrite ? KEY_READ | KEY_WRITE: KEY_READ),
		&hKey) == ERROR_SUCCESS) {
		return hKey;
	}
	else
		return NULL;

}

static void deleteRegistry(const wchar_t *szRegistry, bool b32bits) {
	HKEY hRootKey;
	std::wstring tail;

	if (! splitRegistryString(szRegistry, hRootKey, tail))
		return ;
	if ( customRegDeleteKeyExW == NULL) {
		HINSTANCE hi = LoadLibraryA("ADVAPI32.DLL");
		customRegDeleteKeyExW = (RegDeleteKeyExWType) GetProcAddress(hi,"RegDeleteKeyExW");
	}
	if ( customRegDeleteKeyExW != NULL) {
		customRegDeleteKeyExW (hRootKey, tail.c_str(), (b32bits? KEY_WOW64_32KEY: 0 ) , 0);
	} else {
		RegDeleteKeyW (hRootKey, tail.c_str());
	}
}

static HKEY openRegistry(struct SEE_interpreter *interp, Registry_object *object, bool readwrite) {
	SEE_value res;
	SEE_value str;
	SEE_OBJECT_GET(interp, &object->native.object, STR(path), &res);
	SEE_ToString(interp, &res, &str);
	if (str._type == SEE_STRING) {
		std::wstring sz = SEE_StringToWChars(interp, str.u.string);
		HKEY hkey =  openRegistry(sz.c_str(), object->b32bits, readwrite);
		return hkey;
	} else {
		return NULL;
	}
}
#endif
/*
 * File.prototype.read([length]) -> string/undefined
 *
 * Reads and returns string data. If an argument is given, it limits the
 * length of the string read.
 * If the file is closed, this function return undefined.
 */
static void registry_proto_getValue(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {

#ifdef WIN32
	struct Registry_object *fo = toregistry(interp, thisobj);
	HKEY hKey = openRegistry(interp, fo, false);
	if (hKey == NULL)
	{
		SEE_error_throw(interp, PRIVATE(interp)->RegistryError,
				"Cannot open registry");
		return;
	}

	SEE_value path;
	SEE_ToString(interp, argv[0], &path);

	DWORD dw = FALSE;
	DWORD dwType;
	DWORD dwSize = 0;
	std::wstring value = SEE_StringToWChars(interp, path.u.string);
	dw = RegQueryValueExW(hKey, value.c_str(), NULL, &dwType, (LPBYTE) NULL,
			&dwSize);
	if (dwSize > 0) {
		char *buffer = (char*) malloc (dwSize+1);
		RegQueryValueExW(hKey, value.c_str(), NULL, &dwType, (LPBYTE) buffer,
				&dwSize);
		if (dwType == REG_SZ || dwType == REG_EXPAND_SZ) {
			wchar_t *wch = (wchar_t*) buffer;
			int len = wcslen(wch);
			SEE_string *s = SEE_string_new(interp, len);
			for (int i = 0; i < len; i++)
			{
				SEE_string_append_unicode(s, wch[i]);
			}
			SEE_SET_STRING(res, s);
		} else if (dwType == REG_DWORD) {
			DWORD *pdw = (DWORD*) buffer;
			SEE_SET_NUMBER(res, *pdw);
		} else if (dwType == REG_MULTI_SZ) {
			wchar_t *wch = (wchar_t*) buffer;
			// Count number of strings
			int number = 0;
			int len = wcslen(wch);
			while (len > 0) {
				number ++;
				wch += len;
				len = wcslen (wch);
			}
			SEE_value **v = SEE_NEW_ARRAY(interp, SEE_value*, len);
			wch = (wchar_t*) buffer;
			// Count number of strings
			number = 0;
			len = wcslen(wch);
			while (len > 0) {
				SEE_string * s = SEE_string_new(interp, len);
				memcpy ( s->data, wch, len * sizeof (wchar_t));
				v[number] = SEE_NEW(interp, SEE_value);
				SEE_SET_STRING(v[number], s);
				number ++;
				wch += len;
			}

			SEE_object * obj = SEE_Object_new(interp);
			SEE_OBJECT_CONSTRUCT (interp, interp->Array, obj, len, v, res);
		} else {
			SEE_SET_UNDEFINED(res);
		}
		free ((char*)buffer);
	} else {
		SEE_SET_UNDEFINED(res);
	}
	RegCloseKey(hKey);
#else
	SEE_error_throw(interp, PRIVATE(interp)->RegistryError,
			"Cannot open registry on unix systems");
#endif
}


/*
 * File.prototype.read([length]) -> string/undefined
 *
 * Reads and returns string data. If an argument is given, it limits the
 * length of the string read.
 * If the file is closed, this function return undefined.
 */
#ifdef WIN32
static void registry_set_string (struct SEE_interpreter *interp, HKEY hKey, const std::wstring &name, SEE_value *value, DWORD dwType) {
	SEE_value v2;
	SEE_ToString(interp, value, &v2);
	std::wstring stringValue = SEE_StringToWChars(interp, v2.u.string);
	RegSetValueExW (hKey, name.c_str(), NULL, dwType, (LPBYTE)stringValue.c_str(), sizeof (wchar_t) * (stringValue.length()));
}

static void registry_set_dword (struct SEE_interpreter *interp, HKEY hKey, const std::wstring &name, SEE_value *value) {
	DWORD dwValue = SEE_ToInt32(interp, value);
	RegSetValueExW (hKey, name.c_str(), NULL, REG_DWORD, (LPBYTE)&dwValue, sizeof (dwValue));
}

static void registry_set_multisz (struct SEE_interpreter *interp, HKEY hKey, const std::wstring &name, SEE_value *value) {
	// Test if it is an array
	SEE_value lenV;
	SEE_value objValue;
	SEE_ToObject(interp, value, &objValue);
	SEE_object *array = value->u.object;
	SEE_OBJECT_GET(interp, array, STR(length), &lenV);
	if (lenV._type == SEE_NUMBER) {
		wchar_t *pszString = (wchar_t*) malloc (sizeof (wchar_t));
		int strLen = 0;
		for (int i= 0; i < (int) lenV.u.number; i++) {
			SEE_value res, resStr;
			SEE_string *strIndex = SEE_string_sprintf(interp, "%d", i);
			SEE_OBJECT_GET(interp, array, strIndex, &res);
			SEE_ToString(interp, &res, &resStr);
			int newLen = resStr.u.string->length ;
			pszString = (wchar_t*) realloc (pszString, sizeof (wchar_t) * (strLen + newLen + 2));
			memcpy (&pszString[strLen], resStr.u.string->data, resStr.u.string->length * sizeof (wchar_t));
			pszString[strLen+newLen] = L'\0';
			strLen += newLen + 1;
			i ++;
		}
		pszString [strLen] = L'\0';
		RegSetValueExW (hKey, name.c_str(), NULL, REG_MULTI_SZ, (LPBYTE)pszString, (strLen) * sizeof (wchar_t));
	} else {
		SEE_error_throw(interp, interp->TypeError, "Object must be an array");
	}
}

#endif

static void registry_proto_setValue(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {

#ifdef WIN32

	if (argc < 2) {
		SEE_error_throw(interp, interp->RangeError, "missing argument");
		return ;
	}
	struct Registry_object *fo = toregistry(interp, thisobj);
	HKEY hKey = openRegistry(interp, fo, true);
	if (hKey == NULL)
	{
		SEE_error_throw(interp, PRIVATE(interp)->RegistryError,
				"Cannot open registry");
		return;
	}

	SEE_value path;
	SEE_ToString(interp, argv[0], &path);

	std::wstring name = SEE_StringToWChars(interp, path.u.string);
	if (argc == 3) {
		SEE_value typeValue;
		SEE_ToString(interp, argv[2], &typeValue);
		std::string type = SEE_StringToChars(interp, typeValue.u.string);
		if (type == "REG_SZ") {
			registry_set_string (interp, hKey, name, argv[1], REG_SZ);
		} else if (type == "REG_EXPAND_SZ") {
			registry_set_string (interp, hKey, name, argv[1], REG_EXPAND_SZ);
		} else if (type == "REG_DWORD") {
			registry_set_dword(interp, hKey, name, argv[1]);
		} else if (type == "REG_MULTI_SZ") {
			registry_set_multisz(interp, hKey, name, argv[1]);
		} else if (type == "REG_BINARY") {
			registry_set_string (interp, hKey, name, argv[1], REG_BINARY);
		} else  {
			SEE_error_throw(interp, interp->EvalError, "Allowed values are: REG_SZ, REG_EXPAND_SZ, REG_DWORD, REG_MULTISZ and REG_BINARY");
		}
	} else {
		if ( argv[1]->_type == SEE_UNDEFINED || argv[1]->_type == SEE_NULL) {
			RegDeleteValueW(hKey, name.c_str());
		} else if ( argv[1]->_type == SEE_NUMBER) {
			registry_set_dword(interp, hKey, name, argv[1]);
		} else if ( argv[1]->_type == SEE_BOOLEAN) {
			registry_set_dword(interp, hKey, name, argv[1]);
		} else if ( argv[1]->_type == SEE_STRING) {
			registry_set_string (interp, hKey, name, argv[1], REG_SZ);
		} else if (argv[1]->_type == SEE_OBJECT){
			registry_set_string (interp, hKey, name, argv[1], REG_BINARY);
		}
	}
	SEE_SET_UNDEFINED(res);
	RegCloseKey(hKey);
#else
	SEE_error_throw(interp, PRIVATE(interp)->RegistryError,
			"Cannot open registry on unix systems");
#endif
}

/*
 *
 * Deletes a key
 */
static void registry_proto_deleteKey(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {

#ifdef WIN32
	struct Registry_object *fo = toregistry(interp, thisobj);
	if (fo == NULL)
	{
		SEE_error_throw(interp, PRIVATE(interp)->RegistryError,
				"Cannot open registry");
		return;
	}

	// Obtenir el path actual

	SEE_value path;
	SEE_value pathStr;
	SEE_OBJECT_GET(interp, thisobj, STR(path), &path);
	SEE_ToString(interp, &path, &pathStr);

	SEE_value pathStr2;
	SEE_ToString(interp, argv[0], &pathStr2);

	std::wstring pathtoDelete = SEE_StringToWChars(interp, pathStr.u.string);
	pathtoDelete += L"\\";
	pathtoDelete += SEE_StringToWChars(interp, pathStr2.u.string);


	deleteRegistry(pathtoDelete.c_str(), fo->b32bits);

	SEE_SET_UNDEFINED(res);
#else
	SEE_error_throw(interp, PRIVATE(interp)->RegistryError,
			"Cannot open registry on unix systems");
#endif
}

/*
 * Opens a key
 */
static void registry_proto_openKey(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {

#ifdef WIN32
	struct Registry_object *fo = toregistry(interp, thisobj);
	if (fo == NULL)
	{
		SEE_error_throw(interp, PRIVATE(interp)->RegistryError,
				"Cannot open registry");
		return;
	}

	// Obtenir el path actual

	SEE_value path;
	SEE_value pathStr;
	SEE_OBJECT_GET(interp, thisobj, STR(path), &path);
	SEE_ToString(interp, &path, &pathStr);

	SEE_value pathStr2;
	SEE_ToString(interp, argv[0], &pathStr2);

	int len1 = pathStr.u.string->length;
	int len2 = pathStr2.u.string->length;
	int len = pathStr.u.string->length + 1 + pathStr2.u.string->length;
	wchar_t *psz = (wchar_t*) malloc ((len+1) * sizeof (wchar_t));
	if (len1 > 0)
		memcpy (psz, (wchar_t*) pathStr.u.string->data, len1 * sizeof (wchar_t));
	psz[len1] = L'\\';
	if (len2 > 0)
		memcpy (&psz[len1+1], (wchar_t*) pathStr2.u.string->data, len2 * sizeof (wchar_t));
	psz[len1+len2+1] = L'\0';

	struct SEE_object *newObject = newregistry(interp, psz, fo->b32bits);
	free (psz);

	SEE_SET_OBJECT(res, newObject);
#else
	SEE_error_throw(interp, PRIVATE(interp)->RegistryError,
			"Cannot open registry on unix systems");
#endif
}

static void registry_proto_createKey(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {

#ifdef WIN32
	struct Registry_object *fo = toregistry(interp, thisobj);
	if (fo == NULL)
	{
		SEE_error_throw(interp, PRIVATE(interp)->RegistryError,
				"Cannot open registry");
		return;
	}
	// Obtenir el path actual

	SEE_value path;
	SEE_value pathStr;
	SEE_OBJECT_GET(interp, thisobj, STR(path), &path);
	SEE_ToString(interp, &path, &pathStr);

	SEE_value pathStr2;
	SEE_ToString(interp, argv[0], &pathStr2);

	int len1 = pathStr.u.string->length;
	int len2 = pathStr2.u.string->length;
	int len = pathStr.u.string->length + 1 + pathStr2.u.string->length;
	wchar_t *psz = (wchar_t*) malloc ((len+1) * sizeof (wchar_t));
	if (len1 > 0)
		memcpy (psz, (wchar_t*) pathStr.u.string->data, len1 * sizeof (wchar_t));
	psz[len1] = L'\\';
	if (len2 > 0)
		memcpy (&psz[len1+1], (wchar_t*) pathStr2.u.string->data, len2 * sizeof (wchar_t));
	psz[len1+len2+1] = L'\0';

	struct SEE_object *newObject = newregistry(interp, psz, fo->b32bits);
	struct Registry_object *ro2 = toregistry(interp, newObject);
	HKEY hkey2 = openRegistry(interp, ro2, true);
	if (hkey2 == NULL) {
		SEE_error_throw(interp, PRIVATE(interp)->RegistryError,
				"Cannot create registry");
	}
	CloseHandle(hkey2);

	free (psz);

	SEE_SET_OBJECT(res, newObject);
#else
	SEE_error_throw(interp, PRIVATE(interp)->RegistryError,
			"Cannot open registry on unix systems");
#endif
}

