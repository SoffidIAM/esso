#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <MazingerInternal.h>
#include "../web/WebComponentSpec.h"
#include <AbstractWebApplication.h>
#include <AbstractWebElement.h>
#include <vector>
#include <see/see.h>
#include "ecma.h"

/* Prototypes */
static int Web_mod_init(void);
static void Web_alloc(struct SEE_interpreter *);
static void Web_init(struct SEE_interpreter *);


void MZN_document_get(struct SEE_interpreter *i, struct SEE_object *obj,
	struct SEE_string *prop, struct SEE_value *res);
void MZN_document_put(struct SEE_interpreter *i, struct SEE_object *obj,
	struct SEE_string *prop, struct SEE_value *val, int flags);
int  MZN_document_canput(struct SEE_interpreter *i, struct SEE_object *obj,
	struct SEE_string *prop);
int  MZN_document_hasproperty(struct SEE_interpreter *i, struct SEE_object *obj,
	struct SEE_string *prop);
void MZN_document_call (struct SEE_interpreter *i,
			struct SEE_object *obj, struct SEE_object *thisobj,
			int argc, struct SEE_value **argv,
			struct SEE_value *res);
void MZN_element_get(struct SEE_interpreter *i, struct SEE_object *obj,
	struct SEE_string *prop, struct SEE_value *res);
void MZN_element_put(struct SEE_interpreter *i, struct SEE_object *obj,
	struct SEE_string *prop, struct SEE_value *val, int flags);
int  MZN_element_canput(struct SEE_interpreter *i, struct SEE_object *obj,
	struct SEE_string *prop);
int  MZN_element_hasproperty(struct SEE_interpreter *i, struct SEE_object *obj,
	struct SEE_string *prop);
int  MZN_element_delete(struct SEE_interpreter *i, struct SEE_object *obj,
		struct SEE_string *prop);

static void MZN_document_getElementById(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);
static void MZN_document_getElementsByTagName(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);
static void MZN_document_write(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);
static void MZN_document_writeln(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);

static void MZN_collection_item(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);
static void MZN_collection_namedItem(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);

static void MZN_element_getAttribute (struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);
static void MZN_element_getProperty (struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);
static void MZN_element_setAttribute (struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);
static void MZN_element_getElementsByTagName (struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);
static void MZN_element_removeAttribute (struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);
static void MZN_element_click (struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);
static void MZN_element_blur (struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);
static void MZN_element_focus (struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);

/*
 * The File_module structure is the only symbol exported here.
 * It contains some simple identification information but, most
 * importantly, pointers to the major initialisation functions in
 * this file.
 * This structure is passed to SEE_module_add() once and early.
 */
struct SEE_module Web_module = { SEE_MODULE_MAGIC, /* magic */
	"MazingerWeb", /* name */
	"1.0", /* version */
	0, /* index (set by SEE) */
	Web_mod_init, /* mod_init */
	Web_alloc, /* alloc */
	Web_init /* init */
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
	struct SEE_object *documentPrototype;
	struct SEE_object *elementPrototype;
	struct SEE_object *collectionPrototype;
};

#define PRIVATE(interp)  \
        ((struct module_private *)SEE_MODULE_PRIVATE(interp, &Web_module))

/*
 * To make string usage more efficient, we globally intern some common
 * strings and provide a STR() macro to access them.
 * Internalised strings are guaranteed to have unique pointers,
 * which means you can use '==' instead of 'strcmp()' to compare names.
 * The pointers must be allocated during mod_init() because the global
 * intern table is locked as soon as an interpreter instance is created.
 */
#define STR(name) s_##name
static struct SEE_string *STR(document);
static struct SEE_string *STR(getElementById);
static struct SEE_string *STR(getElementsByTagName);
static struct SEE_string *STR(write);
static struct SEE_string *STR(writeln);
static struct SEE_string *STR(length);
static struct SEE_string *STR(item);
static struct SEE_string *STR(namedItem);
static struct SEE_string *STR(URL);
static struct SEE_string *STR(domain);
static struct SEE_string *STR(title);
static struct SEE_string *STR(cookie);
static struct SEE_string *STR(anchors);
static struct SEE_string *STR(forms);
static struct SEE_string *STR(images);
static struct SEE_string *STR(links);
static struct SEE_string *STR(documentElement);
static struct SEE_string *STR(getProperty);
static struct SEE_string *STR(getAttribute);
static struct SEE_string *STR(setAttribute);
static struct SEE_string *STR(removeAttribute);
static struct SEE_string *STR(click);
static struct SEE_string *STR(blur);
static struct SEE_string *STR(focus);
static struct SEE_string *STR(childNodes);
static struct SEE_string *STR(disabled);
static struct SEE_string *STR(id);
static struct SEE_string *STR(tagName);
static struct SEE_string *STR(parentNode);

static struct SEE_objectclass prototype_class = {
	"Object", /* Class */
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

struct MZN_document_object {
	struct SEE_native native;
	AbstractWebApplication* spec;
};


static struct SEE_objectclass document_inst_class = {
	"Document", /* Class */
	MZN_document_get, /* Get */
	MZN_document_put, /* Put */
	MZN_document_canput, /* CanPut */
	MZN_document_hasproperty, /* HasProperty */
	SEE_native_delete, /* Delete */
	SEE_native_defaultvalue, /* DefaultValue */
	SEE_native_enumerator, /* DefaultValue */
	NULL, /* Construct */
	NULL, /* Call */
	NULL /* HasInstance */
};


struct MZN_element_object {
	struct SEE_native native;
	AbstractWebElement* spec;
};

static struct SEE_objectclass element_inst_class = {
	"Element", /* Class */
	MZN_element_get, /* Get */
	MZN_element_put, /* Put */
	MZN_element_canput, /* CanPut */
	MZN_element_hasproperty, /* HasProperty */
	MZN_element_delete, /* Delete */
	SEE_native_defaultvalue, /* DefaultValue */
	SEE_native_enumerator, /* DefaultValue */
	NULL, /* Construct */
	NULL, /* Call */
	NULL /* HasInstance */
};

struct MZN_collection_object {
	struct SEE_native native;
	std::vector <AbstractWebElement*> *elements;
};

static struct SEE_objectclass collection_inst_class = {
	"Collection", /* Class */
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
static int Web_mod_init() {
	STR(document) = SEE_intern_global("document");
	STR(getElementById)=SEE_intern_global("getElementById");
	STR(getElementsByTagName)=SEE_intern_global("getElementsByTagName");
	STR(write)=SEE_intern_global("write");
	STR(writeln)=SEE_intern_global("writeln");
	STR(length)=SEE_intern_global("length");
	STR(item)=SEE_intern_global("item");
	STR(namedItem)=SEE_intern_global("namedItem");
	STR(URL) = SEE_intern_global("url");
	STR(domain) = SEE_intern_global("domain");
	STR(title) = SEE_intern_global("title");
	STR(cookie) = SEE_intern_global("cookie");
	STR(anchors) = SEE_intern_global("anchors");
	STR(forms) = SEE_intern_global("forms");
	STR(images) = SEE_intern_global("images");
	STR(links) = SEE_intern_global("links");
	STR(documentElement) = SEE_intern_global("documentElement");
	STR(getProperty) = SEE_intern_global("getProperty");
	STR(getAttribute) = SEE_intern_global("getAttribute");
	STR(setAttribute) = SEE_intern_global("setAttribute");
	STR(removeAttribute) = SEE_intern_global("removeAttribute");
	STR(click) = SEE_intern_global("click");
	STR(blur) = SEE_intern_global("blur");
	STR(focus) = SEE_intern_global("focus");
	STR(childNodes) = SEE_intern_global("childNodes");
	STR(disabled) = SEE_intern_global("disabled");
	STR(id) = SEE_intern_global("id");
	STR(tagName) = SEE_intern_global("tagName");
	STR(parentNode) = SEE_intern_global("parentNode");
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
static void Web_alloc(struct SEE_interpreter *interp) {
	SEE_MODULE_PRIVATE(interp, &Web_module) =
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
static void Web_init(struct SEE_interpreter *interp) {
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
    PUTFUNC(proto, MZN_document, getElementById, 1)
	PUTFUNC(proto, MZN_document, getElementsByTagName, 1)
	PUTFUNC(proto, MZN_document, write, 1)
	PUTFUNC(proto, MZN_document, writeln, 1)
	PRIVATE(interp)->documentPrototype = &proto->object;

	/** Crear los prototipos de Collection  **/
    proto = SEE_NEW(interp, SEE_native);
    SEE_native_init(proto, interp, &prototype_class, interp->Object_prototype);
	PUTFUNC(proto, MZN_collection, item, 1)
	PUTFUNC(proto, MZN_collection, namedItem, 1)
	PRIVATE(interp)->collectionPrototype = &proto->object;



	/** Crear los prototipos de Element  **/
    proto = SEE_NEW(interp, SEE_native);
    SEE_native_init(proto, interp, &prototype_class, interp->Object_prototype);
	PUTFUNC(proto, MZN_element, getProperty, 1)
	PUTFUNC(proto, MZN_element, getAttribute, 1)
	PUTFUNC(proto, MZN_element, setAttribute, 2)
	PUTFUNC(proto, MZN_element, getElementsByTagName, 1)
	PUTFUNC(proto, MZN_element, removeAttribute, 1)
	PUTFUNC(proto, MZN_element, click, 0)
	PUTFUNC(proto, MZN_element, blur, 0)
	PUTFUNC(proto, MZN_element, focus, 0)
	PRIVATE(interp)->elementPrototype = &proto->object;


}


void createDocumentObject (AbstractWebApplication *spec, SEE_interpreter *interp)
{
	/** Creates SECRET STORE **/
	struct MZN_document_object * seeWebApp =
			(struct MZN_document_object *) SEE_NEW (interp, struct MZN_document_object);
	SEE_native_init(&seeWebApp->native, interp,
			&document_inst_class, PRIVATE(interp)->documentPrototype);

	seeWebApp->spec = spec;

	struct SEE_value v;

	// Agregar los metodos
    // Crear el objeto document
	SEE_SET_OBJECT(&v, &seeWebApp->native.object);
	SEE_OBJECT_PUT(interp, interp->Global,
			STR(document),
			&v, SEE_ATTR_DEFAULT);

}

void finalizeElementObject (struct SEE_interpreter *i, void *object,
			void *closure)
{
	struct MZN_element_object * seeElement = (struct MZN_element_object*) object;
	if (seeElement->spec != NULL)
	{
		seeElement->spec -> release();
		seeElement->spec = NULL;
	}
}

MZN_element_object* createElementObject (AbstractWebElement *spec, SEE_interpreter *interp)
{
	/** Creates SECRET STORE **/
	struct MZN_element_object * seeElement =
			(struct MZN_element_object *) SEE_NEW_FINALIZE (interp, struct MZN_element_object,
					finalizeElementObject, NULL);
	SEE_native_init(&seeElement->native, interp,
			&element_inst_class, PRIVATE(interp)->elementPrototype);
	seeElement->spec = spec;

	return seeElement;

}

void createElementObject (WebComponentSpec *spec, SEE_interpreter *interp, SEE_string *name) {
	struct SEE_value v;
	struct MZN_element_object *obj = createElementObject(spec->getMatched(), interp);
	SEE_SET_OBJECT(&v, &obj->native.object);
	SEE_OBJECT_PUT(interp, interp->Global,
			name,
			&v, SEE_ATTR_DEFAULT);
}

void finalizeCollectionObject (struct SEE_interpreter *i, void *object,
			void *closure)
{
	struct MZN_collection_object * seeCollection = (struct MZN_collection_object*) object;
	for (std::vector<AbstractWebElement*>::iterator it = seeCollection->elements->begin();
			it != seeCollection->elements->end();
			it ++)
	{
		AbstractWebElement *pElement = *it;
		pElement -> release ();
	}
	seeCollection->elements->clear();
	delete seeCollection->elements;
}

MZN_collection_object *createCollectionObject (std::vector<AbstractWebElement*> &v, SEE_interpreter *interp)
{
	/** Creates SECRET STORE **/
	struct MZN_collection_object * seeCollection =
			SEE_NEW_FINALIZE (interp, struct MZN_collection_object,
					finalizeCollectionObject, NULL);

	SEE_native_init(&seeCollection->native, interp,
			&collection_inst_class, PRIVATE(interp)->collectionPrototype);

	seeCollection -> elements = new std::vector<AbstractWebElement*>();
	for (std::vector<AbstractWebElement*>::iterator it = v.begin();
			it != v.end();
			it++)
	{
		seeCollection->elements->push_back(*it);
	}

	SEE_value v2;
	SEE_SET_NUMBER(&v2, seeCollection->elements->size());
	SEE_OBJECT_PUT(interp, &seeCollection->native.object, STR(length), &v2, SEE_ATTR_READONLY);
	return seeCollection;

}

void MZN_document_get(struct SEE_interpreter *i, struct SEE_object *obj,
	struct SEE_string *prop, struct SEE_value *res)
{
	MZN_document_object*pObj = (MZN_document_object*) obj;

	if (pObj == NULL ||
			pObj->spec == NULL)
	{
		SEE_native_get(i, obj, prop, res);
	} else {
		if (SEE_string_cmp(prop, STR(URL)) == 0)
		{
			std::string s;
			pObj->spec->getUrl(s);
			SEE_string *str = SEE_UTF8ToString(i, s.c_str());
			SEE_SET_STRING(res, str );
		}
		else if (SEE_string_cmp(prop, STR(title)) == 0)
		{
			std::string s;
			pObj->spec->getTitle(s);
			SEE_string *str = SEE_UTF8ToString(i, s.c_str());
			SEE_SET_STRING(res, str );
		}
		else if (SEE_string_cmp(prop, STR(domain)) == 0)
		{
			std::string s;
			pObj->spec->getDomain(s);
			SEE_string *str = SEE_UTF8ToString(i, s.c_str());
			SEE_SET_STRING(res, str );
		}
		else if (SEE_string_cmp(prop, STR(cookie)) == 0)
		{
			std::string s;
			pObj->spec->getCookie(s);
			SEE_string *str = SEE_UTF8ToString(i, s.c_str());
			SEE_SET_STRING(res, str );
		}
		else if (SEE_string_cmp(prop, STR(documentElement)) == 0)
		{
			AbstractWebElement *pEl = pObj->spec->getDocumentElement();
			if (pEl == NULL)
				SEE_SET_NULL(res);
			else
				SEE_SET_OBJECT(res, &createElementObject(pEl, i)->native.object);
		}
		else if (SEE_string_cmp(prop, STR(forms)) == 0)
		{
			std::vector<AbstractWebElement *> v;
			pObj->spec->getForms(v);
			MZN_collection_object *pObj = createCollectionObject(v, i);
			SEE_SET_OBJECT(res, &pObj->native.object);
		}
		else if (SEE_string_cmp(prop, STR(anchors)) == 0)
		{
			std::vector<AbstractWebElement *> v;
			pObj->spec->getAnchors(v);
			MZN_collection_object *pObj = createCollectionObject(v, i);
			SEE_SET_OBJECT(res, &pObj->native.object);
		}
		else if (SEE_string_cmp(prop, STR(images)) == 0)
		{
			std::vector<AbstractWebElement *> v;
			pObj->spec->getImages(v);
			MZN_collection_object *pObj = createCollectionObject(v, i);
			SEE_SET_OBJECT(res, &pObj->native.object);
		}
		else if (SEE_string_cmp(prop, STR(links)) == 0)
		{
			std::vector<AbstractWebElement *> v;
			pObj->spec->getLinks(v);
			MZN_collection_object *pObj = createCollectionObject(v, i);
			SEE_SET_OBJECT(res, &pObj->native.object);
		}
		else
		{
			SEE_native_get(i, obj, prop, res);
		}

	}
}
void MZN_document_put(struct SEE_interpreter *interp, struct SEE_object *obj,
	struct SEE_string *prop, struct SEE_value *val, int flags)
{
	SEE_native_put(interp, obj, prop, val, flags);
}
int  MZN_document_canput(struct SEE_interpreter *interp, struct SEE_object *obj,
	struct SEE_string *prop)
{
	return false;
}

int  MZN_document_hasproperty(struct SEE_interpreter *interp, struct SEE_object *obj,
	struct SEE_string *prop)
{

	return (SEE_string_cmp_ascii(prop, "URL") == 0 ||
			SEE_string_cmp_ascii(prop, "title") == 0 ||
			SEE_string_cmp_ascii(prop, "domain") == 0 ||
			SEE_string_cmp_ascii(prop, "cookie") == 0 ||
			SEE_string_cmp_ascii(prop, "anchors") == 0 ||
			SEE_string_cmp_ascii(prop, "forms") == 0 ||
			SEE_string_cmp_ascii(prop, "images") == 0 ||
			SEE_string_cmp_ascii(prop, "links") == 0 ||
			SEE_string_cmp_ascii(prop, "documentElemnt") == 0 ||
			SEE_native_hasproperty(interp, obj, prop));
}

static void MZN_document_getElementById(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res)
{
	char* s = NULL;

	MZN_document_object *pObj = (MZN_document_object*) thisobj;

	SEE_parse_args(interp, argc, argv, "A", &s);
	if (s == NULL)
	{
		SEE_SET_UNDEFINED(res);
	} else {
		AbstractWebElement *pEl = pObj->spec->getElementById(s);
		if (pEl == NULL)
		{
			SEE_SET_NULL(res);
		}
		else
		{
			SEE_SET_OBJECT(res, &createElementObject(pEl, interp)->native.object);
		}
	}

}

static void MZN_document_getElementsByTagName(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res)
{

	SEE_string* s = NULL;

	MZN_document_object *pObj = (MZN_document_object*) thisobj;

	SEE_parse_args(interp, argc, argv, "s", &s);
	if (s == NULL)
	{
		SEE_SET_UNDEFINED(res);
	} else {
		std::string str = SEE_StringToUTF8(interp, s);
		std::vector<AbstractWebElement *>v;
		pObj->spec->getElementsByTagName(str.c_str(), v);
		SEE_SET_OBJECT(res, &createCollectionObject(v, interp)->native.object);
	}
}

static void MZN_document_write(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res)
{
	char* s = NULL;

	MZN_document_object *pObj = (MZN_document_object*) thisobj;

	SEE_parse_args(interp, argc, argv, "A", &s);
	if (s != NULL)
	{
		pObj->spec->write(s);
	}
	SEE_SET_UNDEFINED(res);
}

static void MZN_document_writeln(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res)
{
	char* s = NULL;

	MZN_document_object *pObj = (MZN_document_object*) thisobj;

	SEE_parse_args(interp, argc, argv, "A", &s);
	if (s != NULL)
	{
		pObj->spec->writeln(s);
	}
	SEE_SET_UNDEFINED(res);

}



static void MZN_collection_item(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res)
{
	SEE_int32_t i = -1;
	MZN_collection_object *pObj = (MZN_collection_object*) thisobj;
	SEE_parse_args(interp, argc, argv, "i", &i);
	if (i < 0 || i >= (SEE_int32_t) pObj->elements->size())
	{
		SEE_SET_UNDEFINED(res);
	} else {
		AbstractWebElement *pEl = pObj->elements->at(i);
		if (pEl == NULL)
		{
			SEE_SET_NULL(res);
		}
		else
		{
			SEE_SET_OBJECT(res, &createElementObject(pEl, interp)->native.object);
		}
	}
}

static void MZN_collection_namedItem(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res)
{
	SEE_string *s;
	MZN_collection_object *pObj = (MZN_collection_object*) thisobj;
	if (argc != 1)
	{
		SEE_SET_UNDEFINED(res);
	} else {
		SEE_SET_NULL(res);
		SEE_parse_args(interp, argc, argv, "s", &s);
		std::string ach = SEE_StringToUTF8(interp, s);
		bool found = false;
		for ( std::vector<AbstractWebElement*>::iterator it = pObj->elements->begin();
				!found && it != pObj->elements->end();
				it ++)
		{
			AbstractWebElement *el = *it;
			std::string s;
			el->getAttribute("id", s);
			if (s.compare(ach) == 0)
			{
				AbstractWebElement *pEl2 = el->clone();
				SEE_SET_OBJECT(res, &createElementObject(pEl2, interp)->native.object);
				found = true;
			}
		}
		if (! found) {
			for ( std::vector<AbstractWebElement*>::iterator it = pObj->elements->begin();
					!found && it != pObj->elements->end();
					it ++)
			{
				AbstractWebElement *el = *it;
				std::string s;
				el->getAttribute("name", s);
				if (s.compare(ach) == 0)
				{
					AbstractWebElement *pEl2 = el->clone();
					SEE_SET_OBJECT(res, &createElementObject(pEl2, interp)->native.object);
					found = true;
				}
			}
		}
	}
}


void MZN_element_get(struct SEE_interpreter *interp, struct SEE_object *obj,
	struct SEE_string *prop, struct SEE_value *res)
{
	MZN_element_object*pObj = (MZN_element_object*) obj;
	if (pObj == NULL ||
			pObj->spec == NULL)
	{
		SEE_native_get(interp, obj, prop, res);
	} else {
		if (SEE_string_cmp(prop, STR(childNodes)) == 0)
		{
			std::vector<AbstractWebElement *> v;
			pObj->spec->getChildren(v);
			MZN_collection_object *pObj = createCollectionObject(v, interp);
			SEE_SET_OBJECT(res, &pObj->native.object);
		}
		else if (SEE_string_cmp(prop, STR(disabled)) == 0)
		{
			std::string s;
			pObj->spec->getAttribute("disabled", s);
			if (s.compare("true")){
				SEE_SET_BOOLEAN(res, true);
			} else {
				SEE_SET_BOOLEAN(res, false);
			}
		}
		else if (SEE_string_cmp(prop, STR(id)) == 0)
		{
			std::string s;
			pObj->spec->getAttribute("id", s);
			SEE_string *str = SEE_UTF8ToString(interp, s.c_str());
			SEE_SET_STRING(res, str );
		}
		else if (SEE_string_cmp(prop, STR(tagName)) == 0)
		{
			std::string s;
			pObj->spec->getTagName(s);
			SEE_string *str = SEE_UTF8ToString(interp, s.c_str());
			SEE_SET_STRING(res, str );
		}
		else if (SEE_string_cmp(prop, STR(parentNode)) == 0)
		{
			AbstractWebElement *pEl = pObj->spec->getParent();
			if (pEl == NULL)
				SEE_SET_NULL(res);
			else
				SEE_SET_OBJECT(res, &createElementObject(pEl, interp)->native.object);
		}
		else
		{
			SEE_native_get(interp, obj, prop, res);
		}

	}
}

void MZN_element_put(struct SEE_interpreter *interp, struct SEE_object *obj,
	struct SEE_string *prop, struct SEE_value *val, int flags)
{
}

int  MZN_element_delete(struct SEE_interpreter *i, struct SEE_object *obj,
		struct SEE_string *prop)
{
	return 0;
}

int  MZN_element_canput(struct SEE_interpreter *i, struct SEE_object *obj,
	struct SEE_string *prop)
{
	return false;
}

int  MZN_element_hasproperty(struct SEE_interpreter *interp, struct SEE_object *obj,
	struct SEE_string *prop)
{
	return (SEE_string_cmp_ascii(prop, "childNodes") == 0 ||
			SEE_string_cmp_ascii(prop, "disabled") == 0 ||
			SEE_string_cmp_ascii(prop, "id") == 0 ||
			SEE_string_cmp_ascii(prop, "tagName") == 0 ||
			SEE_string_cmp_ascii(prop, "parentNode") == 0 ||
			SEE_native_hasproperty(interp, obj, prop));

}


static void MZN_element_setAttribute (struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res)
{
	SEE_string *s1;
	SEE_string *s2;
	MZN_element_object *pObj = (MZN_element_object*) thisobj;
	SEE_SET_UNDEFINED(res);
	if (argc == 2)
	{
		SEE_parse_args(interp, argc, argv, "s|s", &s1, &s2);
		std::string ach1 = SEE_StringToUTF8(interp, s1);
		std::string ach2 = SEE_StringToUTF8(interp, s2);
		pObj->spec->setAttribute(ach1.c_str(), ach2.c_str());
	}
}

static void findElementsByTagName( AbstractWebElement *node, const std::string &wantedTag, std::vector<AbstractWebElement *> &result)
{
	std::vector<AbstractWebElement *> v1;
	node->getChildren (v1);
	for ( std::vector<AbstractWebElement*>::iterator it = v1.begin();
			it != v1.end();
			it++)
	{
		AbstractWebElement* el = *it;
		findElementsByTagName(el, wantedTag, result);
		std::string tagname;
		el->getTagName(tagname);
		if (tagname.compare(wantedTag))
		{
			result.push_back(el);
		}
		else
		{
			el->release();
		}
	}

}

static void MZN_element_getElementsByTagName (struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res)
{
	SEE_string *s1;
	MZN_element_object *pObj = (MZN_element_object*) thisobj;
	SEE_SET_UNDEFINED(res);
	if (argc == 1)
	{
		SEE_parse_args(interp, argc, argv, "s", &s1);
		std::string wantedTag = SEE_StringToUTF8(interp, s1);
		std::vector<AbstractWebElement *> v2;
		findElementsByTagName(pObj->spec, wantedTag, v2);
		SEE_SET_OBJECT(res, &createCollectionObject(v2, interp)->native.object);
	}
}

static void MZN_element_removeAttribute (struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res)
{
	SEE_string *s1;
	MZN_element_object *pObj = (MZN_element_object*) thisobj;
	SEE_SET_UNDEFINED(res);
	if (argc == 1)
	{
		SEE_parse_args(interp, argc, argv, "s", &s1);
		std::string ach1 = SEE_StringToUTF8(interp, s1);
		pObj->spec->setAttribute(ach1.c_str(), "");
	}
}

static void MZN_element_click (struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res)
{
	MZN_element_object *pObj = (MZN_element_object*) thisobj;
	pObj->spec->click();
}

static void MZN_element_getAttribute (struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res)
{
	SEE_string *s1;
	SEE_string *s2;
	MZN_element_object *pObj = (MZN_element_object*) thisobj;
	SEE_SET_UNDEFINED(res);
	if (argc > 0)
	{
		SEE_parse_args(interp, argc, argv, "s", &s1, &s2);
		std::string ach1 = SEE_StringToUTF8(interp, s1);
		std::string v;
		pObj->spec->getAttribute(ach1.c_str(), v);
		SEE_string *str = SEE_UTF8ToString(interp, v.c_str());
		SEE_SET_STRING(res, str);
	}
}

static void MZN_element_getProperty (struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res)
{
	SEE_string *s1;
	SEE_string *s2;
	MZN_element_object *pObj = (MZN_element_object*) thisobj;
	SEE_SET_UNDEFINED(res);
	if (argc > 0)
	{
		SEE_parse_args(interp, argc, argv, "s", &s1, &s2);
		std::string ach1 = SEE_StringToUTF8(interp, s1);
		std::string v;
		pObj->spec->getProperty(ach1.c_str(), v);
		SEE_string *str = SEE_UTF8ToString(interp, v.c_str());
		SEE_SET_STRING(res, str);
	}
}

static void MZN_element_blur (struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res)
{
	MZN_element_object *pObj = (MZN_element_object*) thisobj;
	pObj->spec->blur();
}
static void MZN_element_focus (struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res)
{
	MZN_element_object *pObj = (MZN_element_object*) thisobj;
	pObj->spec->focus();
}


