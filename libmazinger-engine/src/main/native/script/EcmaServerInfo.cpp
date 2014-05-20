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

#include <SeyconServer.h>

/* Prototypes */
static int ServerInfo_mod_init(void);
static void ServerInfo_alloc(struct SEE_interpreter *);
static void ServerInfo_init(struct SEE_interpreter *);

struct SEE_module ServerInfo_module = {
	SEE_MODULE_MAGIC, /* magic */
	"ServerInfo", /* name */
	"1.0", /* version */
	0, /* index (set by SEE) */
	ServerInfo_mod_init, /* mod_init */
	ServerInfo_alloc, /* alloc */
	ServerInfo_init /* init */
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
	struct SEE_object *ServerError; /* ServerInfo.NetworkError */
	struct SEE_object *ServerInfo_prototype;
};

#define PRIVATE(interp)  \
        ((struct module_private *)SEE_MODULE_PRIVATE(interp, &ServerInfo_module))

/*
 * To make string usage more efficient, we globally intern some common
 * strings and provide a STR() macro to access them.
 * Internalised strings are guaranteed to have unique pointers,
 * which means you can use '==' instead of 'strcmp()' to compare names.
 * The pointers must be allocated during mod_init() because the global
 * intern table is locked as soon as an interpreter instance is created.
 */
static struct SEE_string *s_ServerInfo;
static struct SEE_string *s_row;
static struct SEE_string *s_length;
static struct SEE_string *s_ServerError;
static struct SEE_string *s_prototype;

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
static void ServerInfo_construct(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) ;

static struct SEE_objectclass ServerInfo_constructor_class = { "ServerInfo", /* Class */
SEE_native_get, /* Get */
SEE_native_put, /* Put */
SEE_native_canput, /* CanPut */
SEE_native_hasproperty, /* HasProperty */
SEE_native_delete, /* Delete */
SEE_native_defaultvalue, /* DefaultValue */
SEE_native_enumerator, /* DefaultValue */
ServerInfo_construct, /* Construct */
NULL, /* Call */
NULL /* HasInstance */
};

static struct SEE_objectclass ServerInfo_inst_class = { "ServerInfo", /* Class */
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


static struct SEE_objectclass ServerInfo_row_class = { "ServerInfoRow", /* Class */
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

static std::string getRowName(SEE_interpreter *interp, int i)  {
	char achRowName[10];
	sprintf (achRowName, "%d", i);
	std::string result = achRowName;
	return result;
}

static void ServerInfo_construct(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {

	SEE_SET_UNDEFINED(res);
	if (argc != 1) {
		SEE_error_throw(interp, interp->RangeError, "missing argument");
	} else {
		SEE_value v;
		SEE_ToString(interp, argv[0], &v);
		std::wstring param = SEE_StringToWChars(interp, v.u.string);

		SeyconService service;
		SeyconResponse * result = service.sendUrlMessage(L"/query%ls?format=text/plain&nofail=true", param.c_str());
		if (result == NULL) {
			SEE_error_throw(interp, PRIVATE(interp)->ServerError, "Server did not respond");
		} else {
			int token = 0;
			std::string tag = result->getToken(token ++);
			if (tag != "OK") {
				SEE_error_throw(interp, PRIVATE(interp)->ServerError, "%s", tag.c_str());
			} else {
				std::string colsStr = result->getToken(token ++);
				int cols = 0;
				sscanf (colsStr.c_str(), " %d", &cols);
				if (cols > 0) {
					// *********************
					// Crear l'objecte pare
					//
					SEE_value v;
					SEE_native * obj = SEE_NEW(interp, struct SEE_native);
					SEE_native_init(obj, interp, &ServerInfo_inst_class,
							PRIVATE(interp)->ServerInfo_prototype);

					// Recuperar el nom de les columnes
					std::vector<std::string> colName;
					for (int i = 0; i < cols; i++) {
						colName.push_back(result->getToken(token ++));
					}

					// Recuperar les files
					std::string value;
					int rownum = 0;
					while (result->getToken(token ++, value)) {
						SEE_native *row = SEE_NEW(interp, struct SEE_native);

						SEE_native_init(row, interp, &ServerInfo_row_class,
								PRIVATE(interp)->ServerInfo_prototype);

						int colnum = 0;
						do {
							SEE_string *s2 =  SEE_CharsToString(interp, colName.at(colnum).c_str());
							SEE_intern_and_free(interp, &s2);
							SEE_SET_STRING(&v, SEE_CharsToString(interp, value.c_str()));
							SEE_OBJECT_PUT(interp, &row->object,
									s2,
									&v, SEE_ATTR_READONLY);
							colnum ++;
							if ( colnum == cols) break;
							result->getToken(token++, value);
						} while (true);

						SEE_SET_OBJECT(&v, &row->object);

						SEE_OBJECT_PUTA(interp, &obj->object, getRowName(interp, rownum).c_str(),
								&v,
								SEE_ATTR_READONLY);

						rownum ++;
					}


					SEE_SET_NUMBER(&v, rownum);
					SEE_OBJECT_PUT(interp, &obj->object, s_length, &v, SEE_ATTR_LENGTH);

					SEE_SET_OBJECT(res, &obj->object);
				}
			}
		}

	}

}

static void ServerInfo_row(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {

	SEE_SET_UNDEFINED(res);
	if (argc != 1) {
		SEE_error_throw(interp, interp->RangeError, "missing argument");
	} else {
		SEE_value v;
		SEE_ToNumber(interp, argv[0], &v);

		SEE_OBJECT_GETA(interp, thisobj, getRowName(interp, (int) v.u.number).c_str(), res);
	}
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
static int ServerInfo_mod_init() {
	s_ServerInfo = SEE_intern_global("ServerInfo");
	s_ServerError = SEE_intern_global("ServerError");
	s_row = SEE_intern_global("row");
	s_length = SEE_intern_global("length");
	s_prototype = SEE_intern_global("prototype");
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
static void ServerInfo_alloc(struct SEE_interpreter *interp) {
	SEE_MODULE_PRIVATE(interp, &ServerInfo_module) =
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

static void ServerInfo_init(struct SEE_interpreter *interp) {
	struct SEE_native *ServerInfo_constructor;
	struct SEE_native *ServerInfo_prototype;
	struct SEE_object *ServerError;
	struct SEE_value v;

	/* Convenience macro for adding properties to File */
#define PUTOBJ(parent, name, obj)                                       \
        SEE_SET_OBJECT(&v, obj);                                        \
        SEE_OBJECT_PUT(interp, parent, s_##name, &v, SEE_ATTR_DEFAULT);

	/* Convenience macro for adding functions to File.prototype */
#define PUTFUNC(obj, name, len)                                         \
        SEE_SET_OBJECT(&v, SEE_cfunction_make(interp,  ServerInfo_##name,\
                s_##name, len));                                       \
        SEE_OBJECT_PUT(interp, obj, s_##name, &v, SEE_ATTR_DEFAULT);

	/* Create the ServerInfo.prototype object  */
	ServerInfo_prototype = SEE_NEW(interp, struct SEE_native);
	SEE_native_init(ServerInfo_prototype, interp,
			&ServerInfo_inst_class, interp->Object_prototype);
	PUTFUNC(&ServerInfo_prototype->object, row, 1);
	PRIVATE(interp)->ServerInfo_prototype = &ServerInfo_prototype->object;

	/* Create the ServerInfo object  */
	ServerInfo_constructor = SEE_NEW(interp, struct SEE_native);
	SEE_native_init(ServerInfo_constructor, interp,
			&ServerInfo_constructor_class, interp->Object_prototype);
	PUTOBJ(&ServerInfo_constructor->object, prototype, &ServerInfo_prototype->object);


	/* Create the File.FileError error object for I/O exceptions */
	ServerError = SEE_Error_make(interp, s_ServerError);
	PUTOBJ(&ServerInfo_constructor->object, ServerError, ServerError);
	PRIVATE(interp)->ServerError = ServerError;

	PUTOBJ(interp->Global, ServerInfo, &ServerInfo_constructor->object);


	return;

}
#undef PUTFUNC
#undef PUTOBJ
