#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <MazingerInternal.h>
#include <HllApplication.h>
#include <vector>
#include <see/see.h>
#include "ecma.h"

/* Prototypes */
static int Hll_mod_init(void);
static void Hll_alloc(struct SEE_interpreter *);
static void Hll_init(struct SEE_interpreter *);


static void MZN_hll_getCursorLocation(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);
static void MZN_hll_setCursorLocation(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);
static void MZN_hll_getContent(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);
static void MZN_hll_sendKeys(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);
static void MZN_hll_sendText(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);

struct SEE_module Hll_module = { SEE_MODULE_MAGIC, /* magic */
	"MazingerHll", /* name */
	"1.0", /* version */
	0, /* index (set by SEE) */
	Hll_mod_init, /* mod_init */
	Hll_alloc, /* alloc */
	Hll_init /* init */
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
	struct SEE_object *hllPrototype;
};

#define PRIVATE(interp)  \
        ((struct module_private *)SEE_MODULE_PRIVATE(interp, &Hll_module))

/*
 * To make string usage more efficient, we globally intern some common
 * strings and provide a STR() macro to access them.
 * Internalised strings are guaranteed to have unique pointers,
 * which means you can use '==' instead of 'strcmp()' to compare names.
 * The pointers must be allocated during mod_init() because the global
 * intern table is locked as soon as an interpreter instance is created.
 */
#define STR(name) s_##name
static struct SEE_string *STR(hll);
static struct SEE_string *STR(getCursorLocation);
static struct SEE_string *STR(setCursorLocation);
static struct SEE_string *STR(getContent);
static struct SEE_string *STR(sendText);
static struct SEE_string *STR(sendKeys);
static struct SEE_string *STR(columns);
static struct SEE_string *STR(rows);
static struct SEE_string *STR(column);
static struct SEE_string *STR(row);
static struct SEE_string *STR(sessionName);
static struct SEE_string *STR(sessionId);

static struct SEE_objectclass prototype_class = {
	"HllObject", /* Class */
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


// Clase d'objectes per a documents i elements WEB

struct MZN_hll_object {
	struct SEE_native native;
	HllApplication* app;
};


static struct SEE_objectclass hll_inst_class = {
	"Hll", /* Class */
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
static int Hll_mod_init() {
	STR(hll) = SEE_intern_global("hll");
	STR(getCursorLocation)=SEE_intern_global("getCursorLocation");
	STR(setCursorLocation)=SEE_intern_global("setCursorLocation");
	STR(getContent)=SEE_intern_global("getContent");
	STR(sendText)=SEE_intern_global("sendText");
	STR(sendKeys)=SEE_intern_global("sendKeys");
	STR(columns)=SEE_intern_global("columns");
	STR(rows)=SEE_intern_global("rows");
	STR(column)=SEE_intern_global("column");
	STR(row)=SEE_intern_global("row");
	STR(sessionName) = SEE_intern_global("sessionName");
	STR(sessionId) = SEE_intern_global("sessionId");
	return 0;
}

/*
 * alloc: Per-interpreter allocation function.
 * This optional function is called early during interpreter initialisation,
 * but before the interpreter is completely initialised. At this stage,
 * the interpreter is not really ready for use; only some storage has been
 * allocated, so you should not invoke any property accessors at this stage.
 * So, why is this function available? It turns out to be useful if you have
 * mutually dependent modules that, during init(), need to find pointers in
 * to the other modules.
 *
 * In this module, we use the alloc function simply to allocate the
 * per-interpreter module-private storage structure, which we access
 * later through the PRIVATE() macro.
 */
static void Hll_alloc(struct SEE_interpreter *interp) {
	SEE_MODULE_PRIVATE(interp, &Hll_module) =
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
 * 'File.prototype.read', 'File.prototype.write', etc.
 * Functions and methods are most easily created using SEE_cfunction_make().
 *
 * Also, a 'File.FileError' exception is also created (for use when
 * throwing read or write errors) and the whole tree of new objects is
 * published by inserting the toplevel 'File' object into the interpreter
 * by making it a property of the Global object.
 */
static void Hll_init(struct SEE_interpreter *interp) {
    SEE_native *proto;
    SEE_value v;

	/* Convenience macro for adding properties to File */
#define PUTOBJ(parent, name, obj)                                       \
        SEE_SET_OBJECT(&v, obj);                                        \
        SEE_OBJECT_PUT(interp, parent, STR(name), &v, SEE_ATTR_DEFAULT);

	/* Convenience macro for adding functions to File.prototype */
#define PUTFUNC(obj, prefix, name, len)                                         \
        SEE_SET_OBJECT(&v, SEE_cfunction_make(interp, prefix##_##name, \
                STR(name), len));                                       \
        SEE_OBJECT_PUT(interp, &obj->object, STR(name), &v, SEE_ATTR_DEFAULT);

	/** Crear los prototipos de Document  **/
    proto = SEE_NEW(interp, SEE_native);
    SEE_native_init(proto, interp, &prototype_class, interp->Object_prototype);
    PUTFUNC(proto, MZN_hll, getCursorLocation, 0)
	PUTFUNC(proto, MZN_hll, setCursorLocation, 2)
	PUTFUNC(proto, MZN_hll, getContent, 0)
	PUTFUNC(proto, MZN_hll, sendText, 1)
	PUTFUNC(proto, MZN_hll, sendKeys, 1)
	PRIVATE(interp)->hllPrototype = &proto->object;

}


void createHllObject (HllApplication *app, SEE_interpreter *interp)
{
	/** Creates SECRET STORE **/
	struct MZN_hll_object * seeHll =
			(struct MZN_hll_object *) SEE_NEW (interp, struct MZN_hll_object);
	SEE_native_init(&seeHll->native, interp,
			&hll_inst_class, PRIVATE(interp)->hllPrototype);

	seeHll->app = app;

	struct SEE_value v;

	// Agregar los metodos
    // Crear el objeto document
	SEE_SET_OBJECT(&v, &seeHll->native.object);
	SEE_OBJECT_PUT(interp, interp->Global,
			STR(hll),
			&v, SEE_ATTR_DEFAULT);

	std::string id, name, sessionType;
	int rows, cols, codepage;
	app->querySesssionStatus(id, name, sessionType, rows, cols, codepage);

	SEE_SET_STRING(&v, SEE_CharsToString(interp, id.c_str()));
	SEE_OBJECT_PUT(interp, &seeHll->native.object, STR(sessionId), &v, SEE_ATTR_READONLY);

	SEE_SET_STRING(&v, SEE_CharsToString(interp, name.c_str()));
	SEE_OBJECT_PUT(interp, &seeHll->native.object, STR(sessionName), &v, SEE_ATTR_READONLY);

	SEE_SET_NUMBER(&v, rows);
	SEE_OBJECT_PUT(interp, &seeHll->native.object, STR(rows), &v, SEE_ATTR_READONLY);

	SEE_SET_NUMBER(&v, cols);
	SEE_OBJECT_PUT(interp, &seeHll->native.object, STR(columns), &v, SEE_ATTR_READONLY);
}



static void MZN_hll_getCursorLocation(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res)
{
	char* s = NULL;

	MZN_hll_object *pObj = (MZN_hll_object*) thisobj;

	int row;
	int column;

	pObj->app->getCursorLocation(row, column);
	SEE_parse_args(interp, argc, argv, "A", &s);
	if (s == NULL)
	{
		SEE_SET_UNDEFINED(res);
	} else {
		SEE_native *result = SEE_NEW(interp, SEE_native);

		SEE_native_init(result, interp, &prototype_class, interp->Object_prototype);

		SEE_value v;
		SEE_SET_NUMBER(&v, row);
		SEE_OBJECT_PUT(interp, &result->object, STR(row), &v, SEE_ATTR_READONLY);

		SEE_SET_NUMBER(&v, column);
		SEE_OBJECT_PUT(interp, &result->object, STR(column), &v, SEE_ATTR_READONLY);

		SEE_SET_OBJECT(&v, &result->object);
	}
}

static void MZN_hll_setCursorLocation(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res)
{
	MZN_hll_object *pObj = (MZN_hll_object*) thisobj;

	if (argc != 2)
	{
		SEE_error_throw(interp, interp->RangeError, NULL);
	}
	else
	{
		SEE_value v;
		SEE_ToNumber(interp, argv[0], &v);
		int rows = v.u.number;
		SEE_ToNumber(interp, argv[1], &v);
		int columns = v.u.number;

		pObj->app->setCursorLocation(rows, columns);
		SEE_SET_UNDEFINED(res);
	}
}

static void MZN_hll_getContent(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res)
{

	MZN_hll_object *pObj = (MZN_hll_object*) thisobj;

	std::string id, name, sessionType;
	int rows, cols, codepage;
	pObj->app->querySesssionStatus(id, name, sessionType, rows, cols, codepage);

	int row0 = 0;
	int row1 = rows - 1;
	int col0 = 0;
	int col1 = cols - 1;
	if (argc > 0)
	{
		SEE_value v;
		SEE_ToNumber(interp, argv[0], &v);
		row0 = v.u.number - 1;
		if (row0 < 0 || row0 >= rows)
		{
			SEE_error_throw(interp, interp->RangeError, NULL);
			return;
		}
	}
	if (argc > 1)
	{
		SEE_value v;
		SEE_ToNumber(interp, argv[1], &v);
		col0 = v.u.number - 1;
		if (col0 < 0 || col0 >= cols)
		{
			SEE_error_throw(interp, interp->RangeError, NULL);
			return;
		}
	}
	if (argc > 2)
	{
		SEE_value v;
		SEE_ToNumber(interp, argv[2], &v);
		row1 = v.u.number - 1;
		if (row1 < row0 || row1 >= rows)
		{
			SEE_error_throw(interp, interp->RangeError, NULL);
			return;
		}
	}
	if (argc > 3)
	{
		SEE_value v;
		SEE_ToNumber(interp, argv[3], &v);
		col1 = v.u.number - 1;
		if (col1 < col0 || col1 >= cols)
		{
			SEE_error_throw(interp, interp->RangeError, NULL);
			return;
		}
	}
	std::string content;
	std::wstring wcontent;
	if (pObj->app->getPresentationSpace(content) != 0)
	{
		SEE_SET_UNDEFINED(res);
	}
	else
	{
		wcontent = MZNC_strtowstr(content.c_str());
		const wchar_t*wsz = wcontent.c_str();
		std::wstring wcontent2;
		for (int i = row0; i <= row1; i++)
		{
			int offset = i * cols + col0;
			wcontent2.append(&wsz[offset], col1 - col0 + 1);
		}
		SEE_SET_STRING(res, SEE_WCharsToString(interp, wcontent2.c_str()));
	}
}

static void MZN_hll_sendText(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res)
{
	MZN_hll_object *pObj = (MZN_hll_object*) thisobj;

	if (argc != 1)
	{
		SEE_error_throw(interp, interp->EvalError, NULL);
		return;

	}
	else
	{
		SEE_value v;
		SEE_ToString(interp, argv[0], &v);
		std::string chars = SEE_StringToChars(interp, v.u.string);
		pObj->app->sendText(chars.c_str());
		SEE_SET_UNDEFINED(res);
	}

}

static void MZN_hll_sendKeys(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res)
{
	MZN_hll_object *pObj = (MZN_hll_object*) thisobj;

	if (argc != 1)
	{
		SEE_error_throw(interp, interp->EvalError, NULL);
		return;

	}
	else
	{
		SEE_value v;
		SEE_ToString(interp, argv[0], &v);
		std::string chars = SEE_StringToChars(interp, v.u.string);
		pObj->app->sendKeys(chars.c_str());
		SEE_SET_UNDEFINED(res);
	}

}

