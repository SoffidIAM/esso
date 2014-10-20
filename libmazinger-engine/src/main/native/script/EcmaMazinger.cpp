#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <MazingerInternal.h>
#include "../core/ComponentSpec.h"
#include <NativeComponent.h>
#include <ComponentMatcher.h>
#include <SecretStore.h>
#include <AbstractWebApplication.h>
#include <vector>
#include <see/see.h>
#include "SendKeys.h"
#include "ecma.h"
#ifndef WIN32
#include <sys/wait.h>
#include <unistd.h>
#endif
#include <SeyconServer.h>
#include <ScriptDialog.h>


/* Prototypes */
static int Mazinger_mod_init(void);
static void Mazinger_alloc(struct SEE_interpreter *);
static void Mazinger_init(struct SEE_interpreter *);

static void MZNECMA_debug(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);

static void MZNECMA_sleep(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);

static void MZNECMA_env(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);

static void MZNECMA_exec(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);

static void MZNECMA_execWait(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);

static void MZNECMA_sendText(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);

static void MZNECMA_sendKeys(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res);

static void MZNECMA_getSecret(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);

static void MZNECMA_setSecret(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);

static void MZNECMA_getAccount(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);

static void MZNECMA_getAccounts(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);

static void MZNECMA_getPassword(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);

static void MZNECMA_setPassword(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);

static void MZNECMA_generatePassword(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);

static void MZNECMA_setText(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);

static void MZNECMA_getText(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);

static void MZNECMA_setFocus(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);

static void MZNECMA_click(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);



/*
 * The File_module structure is the only symbol exported here.
 * It contains some simple identification information but, most
 * importantly, pointers to the major initialisation functions in
 * this file.
 * This structure is passed to SEE_module_add() once and early.
 */
struct SEE_module Mazinger_module = { SEE_MODULE_MAGIC, /* magic */
"Mazinger", /* name */
"1.0", /* version */
0, /* index (set by SEE) */
Mazinger_mod_init, /* mod_init */
Mazinger_alloc, /* alloc */
Mazinger_init /* init */
};

extern struct SEE_module File_module;
extern struct SEE_module Registry_module;

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
	struct SEE_object *Window;
	struct SEE_object *Window_prototype;
};

#define PRIVATE(interp)  \
        ((struct module_private *)SEE_MODULE_PRIVATE(interp, &Mazinger_module))

/*
 * To make string usage more efficient, we globally intern some common
 * strings and provide a STR() macro to access them.
 * Internalised strings are guaranteed to have unique pointers,
 * which means you can use '==' instead of 'strcmp()' to compare names.
 * The pointers must be allocated during mod_init() because the global
 * intern table is locked as soon as an interpreter instance is created.
 */
#define STR(name) s_##name
static struct SEE_string *STR(debug);
static struct SEE_string *STR(secretStore);
static struct SEE_string *STR(sleep);
static struct SEE_string *STR(getSecret);
static struct SEE_string *STR(setSecret);
static struct SEE_string *STR(getAccounts);
static struct SEE_string *STR(getAccount);
static struct SEE_string *STR(getPassword);
static struct SEE_string *STR(setPassword);
static struct SEE_string *STR(generatePassword);
static struct SEE_string *STR(setText);
static struct SEE_string *STR(getText);
static struct SEE_string *STR(setFocus);
static struct SEE_string *STR(focus);
static struct SEE_string *STR(click);
static struct SEE_string *STR(Window);
static struct SEE_string *STR(prototype);
static struct SEE_string *STR(env);
static struct SEE_string *STR(exec);
static struct SEE_string *STR(execWait);
static struct SEE_string *STR(sendText);
static struct SEE_string *STR(sendKeys);


// Clase d'objectes per a finestres i components
struct MZN_window_object {
	struct SEE_native native;
	ComponentSpec* spec;
};


/*
 * The 'secretStore_inst_class' class structure describes how to carry out
 * all object operations on a secretStore snstance. You can see that
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
static struct SEE_objectclass secretStore_inst_class = { "SecretStore", /* Class */
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

static struct SEE_objectclass window_constructor_class = { "Window", /* Class */
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

static struct SEE_objectclass window_inst_class = { "Window", /* Class */
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
static int Mazinger_mod_init() {
	STR(sleep) = SEE_intern_global("sleep");
	STR(debug) = SEE_intern_global("debug");
	STR(getSecret) = SEE_intern_global("getSecret");
	STR(setSecret) = SEE_intern_global("setSecret");
	STR(getAccounts) = SEE_intern_global("getAccounts");
	STR(getAccount) = SEE_intern_global("getAccount");
	STR(getPassword) = SEE_intern_global("getPassword");
	STR(setPassword) = SEE_intern_global("setPassword");
	STR(generatePassword) = SEE_intern_global("generatePassword");
	STR(secretStore) = SEE_intern_global("secretStore");
	STR(setText) = SEE_intern_global("setText");
	STR(getText) = SEE_intern_global("getText");
	STR(click) = SEE_intern_global("click");
	STR(setFocus) = SEE_intern_global("setFocus");
	STR(focus) = SEE_intern_global("focus");
	STR(Window) = SEE_intern_global("Window");
	STR(prototype) = SEE_intern_global("prototype");
	STR(env) = SEE_intern_global("env");
	STR(exec) = SEE_intern_global("exec");
	STR(execWait) = SEE_intern_global("execWait");
	STR(sendText) = SEE_intern_global("sendText");
	STR(sendKeys) = SEE_intern_global("sendKeys");
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
static void Mazinger_alloc(struct SEE_interpreter *interp) {
	SEE_MODULE_PRIVATE(interp, &Mazinger_module) =
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
static void Mazinger_init(struct SEE_interpreter *interp) {
	struct SEE_value v;

	/* Convenience macro for adding properties to File */
#define PUTOBJ(parent, name, obj)                                       \
        SEE_SET_OBJECT(&v, obj);                                        \
        SEE_OBJECT_PUT(interp, parent, STR(name), &v, SEE_ATTR_DEFAULT);

	/* Convenience macro for adding functions to File.prototype */
#define PUTFUNC(obj, name, len)                                         \
        SEE_SET_OBJECT(&v, SEE_cfunction_make(interp, MZNECMA_##name,\
                STR(name), len));                                       \
        SEE_OBJECT_PUT(interp, obj, STR(name), &v, SEE_ATTR_DEFAULT);

	PUTFUNC(interp->Global, debug, 1)
	PUTFUNC(interp->Global, sleep, 1)
	PUTFUNC(interp->Global, env, 1)
	PUTFUNC(interp->Global, execWait, 1)
	PUTFUNC(interp->Global, exec, 1)
	PUTFUNC(interp->Global, sendText, 1)
	PUTFUNC(interp->Global, sendKeys, 1)

	/** Creates SECRET STORE **/
	struct SEE_object * secretStore =
			(struct SEE_object *) SEE_NEW (interp, struct SEE_native);
	SEE_native_init((struct SEE_native *) secretStore, interp,
			&secretStore_inst_class, interp->Object_prototype);
	PUTFUNC(secretStore, getSecret, 1);
	PUTFUNC(secretStore, setSecret, 2);
	PUTFUNC(secretStore, getAccounts, 1);
	PUTFUNC(secretStore, getAccount, 1);
	PUTFUNC(secretStore, getPassword, 2);
	PUTFUNC(secretStore, setPassword, 3);
	PUTFUNC(secretStore, generatePassword, 2);
	PUTOBJ (interp->Global, secretStore, secretStore);

	/** Create Window.prototype object **/
	struct SEE_object *windowProto =
			(struct SEE_object*) SEE_NEW (interp, struct MZN_window_object);
	SEE_native_init((struct SEE_native *) windowProto, interp, &window_inst_class,
			interp->Object_prototype);
	((struct MZN_window_object*) windowProto)->spec = NULL;
	PUTFUNC(windowProto, getText, 0);
	PUTFUNC(windowProto, setText, 1);
	PUTFUNC(windowProto, setFocus, 0);
	PUTFUNC(windowProto, click, 0);

	/** Create Window object ?? **/
	struct SEE_object *Window =
			(struct SEE_object*) SEE_NEW (interp, struct SEE_native);
	SEE_native_init((struct SEE_native *) Window, interp,
			&window_constructor_class, interp->Object_prototype);

	PUTOBJ (interp->Global, Window, Window);
	PUTOBJ (Window, prototype, windowProto);

	PRIVATE(interp)->Window = Window;
	PRIVATE(interp)->Window_prototype = windowProto;

}

/*
 * debug(message)
 *
 * Popup window
 * The string must be 8-bit data only.
 */
static void MZNECMA_debug(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {
	if (argc != 1) {
		SEE_error_throw(interp, interp->RangeError, "missing argument");
		return;
	}

	SEE_value v;
	SEE_ToString(interp, argv[0], &v);

	std::string str = SEE_StringToChars(interp, v.u.string);
	MZNSendDebugMessage("%s", str.c_str());
	SeyconCommon::debug("%s", str.c_str());
	SEE_SET_UNDEFINED(res);
}

/*
 * sleep(milliseconds)
 *
 * Milliseconds to wait
 */
static void MZNECMA_sleep(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {
	SEE_uint32_t millis;

	SEE_parse_args(interp, argc, argv, "i", &millis);
	if (millis > 0)
	{
#ifdef WIN32
		Sleep(millis);
#else
		usleep (millis * 1000);
#endif
	}

	SEE_SET_UNDEFINED(res);
}

/*
 * sleep(milliseconds)
 *
 * Milliseconds to wait
 */
static void MZNECMA_env(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {

	struct SEE_string *message;

	SEE_parse_args(interp, argc, argv, "s", &message);
	if (message == NULL)
		SEE_error_throw(interp, interp->RangeError, "missing argument");


#ifdef WIN32
	std::wstring messageString = SEE_StringToWChars(interp, message);
	wchar_t *str = _wgetenv(messageString.c_str());
	if (str == NULL)
		SEE_SET_UNDEFINED(res);
	else {
		struct SEE_string *buf = SEE_WCharsToString(interp, str);
		SEE_SET_STRING(res, buf);
	}
#else
	std::string messageString = SEE_StringToChars(interp, message);
	char *str = getenv(messageString.c_str());
	if (str == NULL)
		SEE_SET_UNDEFINED(res);
	else {
		struct SEE_string *buf = SEE_CharsToString(interp, str);
		SEE_SET_STRING(res, buf);
	}
#endif

}

/*
 * exec
 *
 * Executa aplicació
 */
static void MZNECMA_exec(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {

	struct SEE_string *message;

	SEE_parse_args(interp, argc, argv, "s", &message);
	if (message == NULL)
		SEE_error_throw(interp, interp->RangeError, "missing argument");

	std::wstring wDir;
	std::string lDir;
	if (argc >= 2) {
		SEE_value v;
		SEE_ToString(interp, argv[1], &v);
		wDir = SEE_StringToWChars(interp, v.u.string);
		lDir = SEE_StringToChars(interp, v.u.string);
	}

#ifdef WIN32
	std::wstring cmdLine = SEE_StringToWChars(interp,message);

	STARTUPINFOW startupInfo;
	memset (&startupInfo, 0, sizeof startupInfo);
	startupInfo.cb = sizeof startupInfo;
	startupInfo.lpReserved = NULL;
	startupInfo.lpDesktop = NULL;
	startupInfo.lpTitle = NULL;
	startupInfo.dwFlags = 0;
	PROCESS_INFORMATION processInformation;

	if (! CreateProcessW (NULL, (LPWSTR) cmdLine.c_str(),
			NULL, NULL,
			FALSE,
			NORMAL_PRIORITY_CLASS, NULL,
			wDir.size() > 0 ? wDir.c_str(): NULL,
			&startupInfo,
			&processInformation)) {
		SEE_error_throw(interp, interp->EvalError, "Cannot create process");
	}

#else
	if (fork() == 0)
	{
		if (lDir.size() > 0) {
			chdir (lDir.c_str());
		}
		std::string cmdLine = SEE_StringToChars(interp,message);
		execlp ("/bin/bash", "-c", cmdLine.c_str(), NULL);
		exit(1);
	}
#endif
	SEE_SET_UNDEFINED(res);
}

/*
 * exec
 *
 * Executa aplicació
 */
static void MZNECMA_execWait(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {

	struct SEE_string *message;

	SEE_parse_args(interp, argc, argv, "s", &message);
	if (message == NULL)
		SEE_error_throw(interp, interp->RangeError, "missing argument");

	std::wstring wDir;
	std::string lDir;
	if (argc >= 2) {
		SEE_value v;
		SEE_ToString(interp, argv[1], &v);
		wDir = SEE_StringToWChars(interp, v.u.string);
		lDir = SEE_StringToChars(interp, v.u.string);
	}
#ifdef WIN32

	std::wstring cmdLine = SEE_StringToWChars(interp,message);
	STARTUPINFOW startupInfo;
	memset (&startupInfo, 0, sizeof startupInfo);
	startupInfo.cb = sizeof startupInfo;
	startupInfo.lpReserved = NULL;
	startupInfo.lpDesktop = NULL;
	startupInfo.lpTitle = NULL;
	startupInfo.dwFlags = 0;
	PROCESS_INFORMATION processInformation;

	if (!CreateProcessW (NULL, (LPWSTR) cmdLine.c_str(),
			NULL, NULL,
			FALSE,
			NORMAL_PRIORITY_CLASS, NULL,
			wDir.size() > 0 ? wDir.c_str(): NULL,
			&startupInfo,
			&processInformation)) {
		SEE_error_throw(interp, interp->EvalError, "Cannot create process");
	}


	WaitForSingleObject(processInformation.hProcess, INFINITE);

#else
	int f = fork ();
	if (f == 0)
	{
		std::string cmdLine = SEE_StringToChars(interp,message);
		if (lDir.size() > 0) {
			chdir (lDir.c_str());
		}
		execlp ("/bin/bash", "-c", cmdLine.c_str(), NULL);
	} else {
		int status;
		do  {
			waitpid (f, &status, 0);
		} while (! WIFEXITED(status));
	}
#endif
	SEE_SET_UNDEFINED(res);
}

/*
 * secretStorege.getSecret()
 *
 *
 */
static void MZNECMA_getSecret(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {
	struct SEE_string *message;

	SEE_parse_args(interp, argc, argv, "s", &message);
	if (message == NULL)
		SEE_error_throw(interp, interp->RangeError, "missing argument");

	std::wstring secret = SEE_StringToWChars(interp, message);

	SecretStore s (MZNC_getUserName()) ;
	wchar_t *str = s.getSecret(secret.c_str());

	if (str == NULL)
		SEE_SET_UNDEFINED(res);
	else {
		struct SEE_string *buf = SEE_WCharsToString(interp, str);
		SEE_SET_STRING(res, buf);
		s.freeSecret(str);
	}
}

/*
 * secretStorege.getSecret()
 *
 *
 */
static void MZNECMA_setSecret(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {

	if (argc != 2)
		SEE_error_throw(interp, interp->RangeError, "missing argument");

	SEE_value secretValue;
	SEE_value valueValue;
	SEE_ToString(interp, argv[0], &secretValue);
	SEE_ToString(interp, argv[1], &valueValue);

	std::wstring secret = SEE_StringToWChars(interp, secretValue.u.string);
	std::wstring value = SEE_StringToWChars(interp, valueValue.u.string);

	SecretStore s (MZNC_getUserName()) ;
	wchar_t *sessionKey = s.getSecret(L"sessionKey");
	wchar_t *user = s.getSecret(L"user");


	SeyconService ss;

	SeyconResponse *response = ss.sendUrlMessage(L"/setSecret?user=%ls&key=%ls&secret=%ls&value=%ls",
			user, sessionKey, secret.c_str(), value.c_str());

	if (response == NULL)
	{
		SEE_error_throw(interp, interp->EvalError, "Error connecting to Soffid server");
	}
	else
	{
		std::string token = response->getToken(0);
		if (token != "OK")
		{
			SEE_error_throw(interp, interp->EvalError, token.c_str());
		}
		else
		{
			s.setSecret(secret.c_str(), value.c_str());
		}
		delete response;
	}

	s.freeSecret(sessionKey);
	s.freeSecret(user);

	SEE_SET_UNDEFINED(res);
}

/*
 * secretStore.getAccounts()
 *
 *
 */
static void MZNECMA_getAccounts(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {
	struct SEE_string *message;

	SEE_parse_args(interp, argc, argv, "s", &message);
	if (message == NULL)
		SEE_error_throw(interp, interp->RangeError, "missing argument");


	std::wstring secret = L"account.";
	secret += SEE_StringToWChars(interp, message);

	SecretStore s (MZNC_getUserName()) ;
	std::vector<std::wstring> accounts;
	accounts = s.getSecrets(secret.c_str());

	if (accounts.empty())
		SEE_SET_UNDEFINED(res);
	else {
		SEE_OBJECT_CONSTRUCT(interp, interp->Array, NULL, 0, NULL, res);
		if (res->_type == SEE_OBJECT)
		{
			int colnum = 0;
			struct SEE_string *ind;
			ind = SEE_string_new(interp, 16);
			SEE_value v;

			for (std::vector<std::wstring>::const_iterator it = accounts.begin();
					it != accounts.end(); it++)
			{
				std::wstring account = *it;
				// Index
				ind->length = 0;
                SEE_string_append_int(ind, colnum++);
                // Value
        		struct SEE_string *buf = SEE_WCharsToString(interp, account.c_str());
                SEE_SET_STRING(&v, buf);
                // Set array element
				SEE_OBJECT_PUT(interp, res->u.object,
						SEE_intern(interp, ind), &v, 0);
			}
		}
	}
}

/*
 * secretStore.getAccounts()
 *
 *
 */
static void MZNECMA_getAccount(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {
	struct SEE_string *message;

	SEE_parse_args(interp, argc, argv, "s", &message);
	if (message == NULL)
		SEE_error_throw(interp, interp->RangeError, "missing argument");


	std::wstring secret = L"account.";
	secret += SEE_StringToWChars(interp, message);

	SecretStore s (MZNC_getUserName()) ;
	std::vector<std::wstring> accounts;
	accounts = s.getSecrets(secret.c_str());

	if (accounts.empty())
		SEE_SET_UNDEFINED(res);
	else if (accounts.size() == 1)
	{
		struct SEE_string *buf = SEE_WCharsToString(interp, accounts[0].c_str());
		SEE_SET_STRING(res, buf);
	}
	else
	{
		ScriptDialog* sd;
		sd = ScriptDialog::getScriptDialog();
		if (sd == NULL)
		{
			SEE_error_throw(interp, interp->EvalError, "missing dialog manager");
		}
		else
		{
			std::vector<std::wstring> accountDescriptions;
			std::wstring prefix = L"accdesc.";
			prefix += SEE_StringToWChars(interp, message);
			prefix += L".";
			for (std::vector<std::wstring>::iterator it = accounts.begin();
					it != accounts.end();
					it++)
			{
				std::wstring account = *it;
				std::wstring secret = prefix+account;
				std::wstring description = s.getSecret(secret.c_str());
				accountDescriptions.push_back(description);
			}
			std::wstring account = sd->selectAccount(accounts, accountDescriptions);
			if (account.empty())
				SEE_error_throw(interp, interp->EvalError, "No account selected");
			else
			{
				struct SEE_string *buf = SEE_WCharsToString(interp, account.c_str());
				SEE_SET_STRING(res, buf);
			}
		}
	}
}

/*
 * secretStore.getAccounts()
 *
 *
 */
static void MZNECMA_getPassword(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {
	if (argc != 2)
	{
		SEE_error_throw(interp, interp->RangeError, "missing argument");
		return;
	}
	SEE_value system, account;
	SEE_ToString(interp, argv[0], &system);
	SEE_ToString(interp, argv[1], &account);

	std::wstring secret = L"pass.";
	secret += SEE_StringToWChars(interp, system.u.string);
	secret += L".";
	secret += SEE_StringToWChars(interp, account.u.string);

	SecretStore s (MZNC_getUserName()) ;
	wchar_t *str = s.getSecret(secret.c_str());

	if (str == NULL)
		SEE_SET_UNDEFINED(res);
	else {
		struct SEE_string *buf = SEE_WCharsToString(interp, str);
		SEE_SET_STRING(res, buf);
		s.freeSecret(str);
	}
}

/*
 * secretStorege.setPassword()
 *
 *
 */
static void MZNECMA_setPassword(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {

	if (argc != 3)
		SEE_error_throw(interp, interp->RangeError, "missing argument");

	SEE_value systemValue;
	SEE_value accountValue;
	SEE_value valueValue;
	SEE_ToString(interp, argv[0], &systemValue);
	SEE_ToString(interp, argv[1], &accountValue);
	SEE_ToString(interp, argv[2], &valueValue);

	std::wstring secret = L"pass.";
	std::wstring system = SEE_StringToWChars(interp, systemValue.u.string);
	secret += system;
	secret += L".";
	std::wstring account = SEE_StringToWChars(interp, accountValue.u.string);
	secret += account;

	std::wstring value = SEE_StringToWChars(interp, valueValue.u.string);

	SecretStore s (MZNC_getUserName()) ;
	wchar_t *sessionKey = s.getSecret(L"sessionKey");
	wchar_t *user = s.getSecret(L"user");

	SeyconService ss;
	SeyconResponse *response = ss.sendUrlMessage(L"/setSecret?user=%ls&key=%ls&system=%ls&account=%ls&value=%ls",
			user, sessionKey, system.c_str(), account.c_str(), value.c_str());

	if (response == NULL)
	{
		SEE_error_throw(interp, interp->EvalError, "Error connecting to Soffid server");
	}
	else
	{
		std::string token = response->getToken(0);
		if (token != "OK")
		{
			SEE_error_throw(interp, interp->EvalError, token.c_str());
		}
		else
		{
			s.setSecret(secret.c_str(), value.c_str());
		}
		delete response;
	}

	s.freeSecret(sessionKey);
	s.freeSecret(user);

	SEE_SET_UNDEFINED(res);
}


/*
 * secretStorege.getSecret()
 *
 *
 */
static void MZNECMA_generatePassword(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {

	if (argc != 2)
		SEE_error_throw(interp, interp->RangeError, "missing argument");

	SEE_value systemValue;
	SEE_value accountValue;
	SEE_ToString(interp, argv[0], &systemValue);
	SEE_ToString(interp, argv[1], &accountValue);

	std::wstring secret = L"pass.";
	std::wstring system = SEE_StringToWChars(interp, systemValue.u.string);
	secret += system;
	secret += L".";
	std::wstring account = SEE_StringToWChars(interp, accountValue.u.string);
	secret += account;

	SecretStore s (MZNC_getUserName()) ;
	wchar_t *sessionKey = s.getSecret(L"sessionKey");
	wchar_t *user = s.getSecret(L"user");

	SeyconService ss;
	SeyconResponse *response = ss.sendUrlMessage(L"/generatePassword?user=%ls&key=%ls&system=%ls&account=%ls",
			user, sessionKey, system.c_str(), account.c_str());


	SEE_SET_UNDEFINED(res);

	if (response == NULL)
	{
		SEE_error_throw(interp, interp->EvalError, "Error connecting to Soffid server");
	}
	else
	{
		std::string token = response->getToken(0);
		if (token != "OK")
		{
			SEE_error_throw(interp, interp->EvalError, token.c_str());
		}
		else
		{
			std::wstring password;
			response->getToken(1, password);

			SEE_string * str = SEE_WCharsToString(interp, password.c_str());
			SEE_SET_STRING(res, str);
		}
		delete response;
	}

	s.freeSecret(sessionKey);
	s.freeSecret(user);
}


/*
 * Funcions de finestra
 */
static struct MZN_window_object * towindow(struct SEE_interpreter *interp,
		struct SEE_object *o) {
	if (!o || o->objectclass != &window_inst_class)
		SEE_error_throw(interp, interp->TypeError, NULL);
	return (struct MZN_window_object *) o;
}

static void MZNECMA_getText(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {

	MZN_window_object *obj = towindow(interp, thisobj);
	if (obj == NULL ||
			obj->spec == NULL ||
			obj->spec->m_pMatchedComponent == NULL)
		SEE_SET_UNDEFINED(res);
	else {
		std::string s;
		obj->spec->m_pMatchedComponent->getAttribute("text", s);
		struct SEE_string *buf;
		buf = SEE_string_new(interp, s.length() + 1);
		const char *str = s.c_str();
		for (int i = 0; str[i] != 0; i++)
			SEE_string_addch(buf, str[i]);
		SEE_SET_STRING(res, buf);

	}
}

static void MZNECMA_setText(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {
	SEE_string *message;

	MZN_window_object *obj = towindow(interp, thisobj);
	if (obj != NULL ||
			obj->spec != NULL ||
			obj->spec->m_pMatchedComponent != NULL)
	{
		SEE_parse_args(interp, argc, argv, "s", &message);
		if (message == NULL)
			SEE_error_throw(interp, interp->RangeError, "missing argument");
		std::string text = SEE_StringToChars(interp, message);
		obj->spec->m_pMatchedComponent->setAttribute("text", text.c_str());
	}
	SEE_SET_UNDEFINED(res);
}


static void MZNECMA_setFocus(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {

	MZN_window_object *obj = towindow(interp, thisobj);
	if (obj != NULL ||
			obj->spec != NULL ||
			obj->spec->m_pMatchedComponent != NULL)
	{
		obj->spec->m_pMatchedComponent->setFocus();

	}
	SEE_SET_UNDEFINED(res);
}


static void MZNECMA_click(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {
	MZN_window_object *obj = towindow(interp, thisobj);
	if (obj != NULL ||
			obj->spec != NULL ||
			obj->spec->m_pMatchedComponent != NULL)
	{
		obj->spec->m_pMatchedComponent->click();
	}
	SEE_SET_UNDEFINED(res);
}


void createWindowObject (ComponentSpec *spec, SEE_interpreter *interp, SEE_string *name)
{
	struct SEE_value v;
	struct MZN_window_object *obj =
			SEE_NEW(interp, struct MZN_window_object);
	obj->spec = spec;
	SEE_native_init ((struct SEE_native*) obj, interp,
			&window_inst_class,
			PRIVATE(interp)->Window_prototype);
	struct SEE_object *obj2 = (struct SEE_object*) obj;
	SEE_SET_OBJECT(&v, obj2);
	SEE_OBJECT_PUT(interp, interp->Global,
			name,
			&v, SEE_ATTR_DEFAULT);
}

struct SEE_string* getFocusString() {
	return STR(focus);
}



/*
 * typeText
 *
 * Text to type
 */
static void MZNECMA_sendKeys(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {

	SEE_string *s;

	SEE_parse_args(interp, argc, argv, "s", &s);
	if (s == NULL)
		SEE_error_throw(interp, interp->RangeError, "missing argument");
	std::string message = SEE_StringToChars(interp, s);

	CSendKeys k;
	k.SendKeys(message.c_str(), true);

	SEE_SET_UNDEFINED(res);
}

/*
 * typeText
 *
 * Text to type
 */
static void MZNECMA_sendText(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {

	SEE_string *s;

	SEE_parse_args(interp, argc, argv, "s", &s);
	if (s == NULL)
		SEE_error_throw(interp, interp->RangeError, "missing argument");
	std::wstring message = SEE_StringToWChars(interp, s);

	CSendKeys k;
	k.SendLiteral(message.c_str(), true);

	SEE_SET_UNDEFINED(res);
}

