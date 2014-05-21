/*   MailService class
 *
 *      MailService                    - constructor/opener object
 *      MailService.prototype          - container object for common methods
 *      MailService.prototype.setServer- reads data from the file
 *      MailService.prototype.setFrom  - tells if a file is at EOF
 *      MailService.prototype.setTo    - writes data to a file
 *      MailService.prototype.send     - flushes a file output
 *      MailServiceError               - exception sending email
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <MazingerInternal.h>
#include <vector>
#include <see/see.h>
#ifdef WIN32
#include <io.h>
#include <winsock.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
typedef int SOCKET;
#define closesocket close
#endif
#include <sys/stat.h>
#include "ecma.h"
#include <fstream>
#include <ios>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

/* Prototypes */
static int  MailService_mod_init(void);
static void MailService_alloc(struct SEE_interpreter *);
static void MailService_init(struct SEE_interpreter *);

static void MailService_construct(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void MailService_prototype_setServer(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void MailService_prototype_setFrom(struct SEE_interpreter *,
		struct SEE_object *, struct SEE_object *, int, struct SEE_value **,
		struct SEE_value *);
static void MailService_prototype_setTo(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void MailService_prototype_send(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);


static struct MailService_object *toMailService(struct SEE_interpreter *interp,
		struct SEE_object *);
static struct SEE_object *newMailService(struct SEE_interpreter *interp);

static void MailService_finalize(struct SEE_interpreter *, void *, void *);

/*
 * The MailService_module structure is the only symbol exported here.
 * It contains some simple identification information but, most
 * importantly, pointers to the major initialisation functions in
 * this MailService.
 * This structure is passed to SEE_module_add() once and early.
 */
struct SEE_module MailService_module = {
	SEE_MODULE_MAGIC, /* magic */
	"MailService", /* name */
	"1.0", /* version */
	0, /* index (set by SEE) */
	MailService_mod_init, /* mod_init */
	MailService_alloc, /* alloc */
	MailService_init /* init */
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
	struct SEE_object *MailService; /* The MailService object */
	struct SEE_object *MailService_prototype; /* MailService.prototype */
	struct SEE_object *MailServiceError; /* MailService.MailServiceError */
};

#define PRIVATE(interp)  \
        ((struct module_private *)SEE_MODULE_PRIVATE(interp, &MailService_module))

/*
 * To make string usage more efficient, we globally intern some common
 * strings and provide a STR() macro to access them.
 * Internalised strings are guaranteed to have unique pointers,
 * which means you can use '==' instead of 'strcmp()' to compare names.
 * The pointers must be allocated during mod_init() because the global
 * intern table is locked as soon as an interpreter instance is created.
 */
#define STR(name) s_##name
static struct SEE_string *STR(setServer), *STR(setFrom), *STR(setTo), *STR(send),
		*STR(MailService), *STR(prototype), *STR(MailServiceError);

/*
 * This next structure is the one we use for a host object. It will
 * behave just like a normal ECAMscript object, except that we
 * "piggyback" a stdio FILE* onto it. A new instance is created whenever
 * you evaluate 'new MailService()'.
 *
 * The 'struct file_object' wraps a SEE_native object, but I could have
 * chosen SEE_object if I didn't want users to be able to store values
 * on it. Choosing a SEE_native structure adds a property hash table
 * to the object.
 *
 * Instances of this structure can be passed wherever a SEE_object
 * or SEE_native is needed. (That's because the first field of a SEE_native
 * is a SEE_object.) Each instance's native.object.objectclass field must
 * point to the file_inst_class table defined below. Also, each instance's
 * native.object.prototype field (aka [[Prototype]]) should point to the
 * MailService.prototype object; that way the file methods can be found by the
 * normal prototype-chain method. (The search is automatically performed by
 * SEE_native_get.)
 *
 * The alert reader will notice that later we create the special object
 * MailService.prototype as an instance of this struct file_object too, except that
 * its file pointer is NULL, and its [[Prototype]] points to Object.prototype.
 */

struct MailService_object {
	struct SEE_native native;
	std::string *server;
	std::string *from;
	std::string *to;
};


/*
 * The 'MailService_inst_class' class structure describes how to carry out
 * all object operations on a mailService_object instance. You can see that
 * many of the function slots point directly to SEE_native_* functions.
 * This is because mailService_object wraps a SEE_native structure, and we can
 * get all the standard ('native') ECMAScript object behaviour for free.
 * If we didn't want this behaviour, and instead used struct SEE_object as
 * the basis of the mailService_object structure, then we should use the SEE_no_*
 * functions, or wrappers around them.
 *
 * In this class, there is no need for a [[Construct]] nor [[Call]]
 * property, so those are left at NULL.
 */
static struct SEE_objectclass mailService_inst_class = {
	"MailService", /* Class */
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
 * This is the class structure for the toplevel 'MailService' object. The 'MailService'
 * object doubles as both a constructor function and as a container object
 * to hold MailService.prototype and some other useful properties. 'MailService' has only
 * one important intrinsic property, namely the [[Construct]] method. It
 * is called whenever the expression 'new MailService()' is evaluated.
 */
static struct SEE_objectclass mailService_constructor_class = {
	"MailService", /* Class */
	SEE_native_get, /* Get */
	SEE_native_put, /* Put */
	SEE_native_canput, /* CanPut */
	SEE_native_hasproperty, /* HasProperty */
	SEE_native_delete, /* Delete */
	SEE_native_defaultvalue, /* DefaultValue */
	SEE_native_enumerator, /* DefaultValue */
	MailService_construct, /* Construct */
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
static int MailService_mod_init() {
	STR(setServer) = SEE_intern_global("setServer");
	STR(setFrom) = SEE_intern_global("setFrom");
	STR(setTo) = SEE_intern_global("setTo");
	STR(send) = SEE_intern_global("send");
	STR(MailService) = SEE_intern_global("MailService");
	STR(MailServiceError) = SEE_intern_global("MailServiceError");
	STR(prototype) = SEE_intern_global("prototype");
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
static void MailService_alloc(struct SEE_interpreter *interp) {
	SEE_MODULE_PRIVATE(interp, &MailService_module) =
			SEE_NEW(interp, struct module_private);
}

/*
 * init: Per-interpreter initialisation function.
 * This is the real workhorse of the module. Its job is to build up
 * an initial collection of host objects and install them into a fresh
 * interpreter instance. This function is called every time an interpreter
 * is created.
 *
 * Here we create the 'MailService' container/constructor object, and
 * populate it with 'MailService.prototype', 'MailService.In', etc. The 'MailService.prototype'
 * object is also created, and given its cfunction properties,
 * 'MailService.prototype.read', 'MailService.prototype.write', etc.
 * Functions and methods are most easily created using SEE_cfunction_make().
 *
 * Also, a 'MailService.MailServiceError' exception is also created (for use when
 * throwing read or write errors) and the whole tree of new objects is
 * published by inserting the toplevel 'MailService' object into the interpreter
 * by making it a property of the Global object.
 */
static void MailService_init(struct SEE_interpreter *interp) {
	struct SEE_object *MailService, *MailService_prototype, *MailServiceError;
	struct SEE_value v;

	/* Convenience macro for adding properties to MailService */
#define PUTOBJ(parent, name, obj)                                       \
        SEE_SET_OBJECT(&v, obj);                                        \
        SEE_OBJECT_PUT(interp, parent, STR(name), &v, SEE_ATTR_DEFAULT);

	/* Convenience macro for adding functions to MailService.prototype */
#define PUTFUNC(obj, name, len)                                         \
        SEE_SET_OBJECT(&v, SEE_cfunction_make(interp, obj##_##name,\
                STR(name), len));                                       \
        SEE_OBJECT_PUT(interp, obj, STR(name), &v, SEE_ATTR_DEFAULT);

	// ****************************************************************** MailService.prototype
	/* Create the MailService.prototype object (cf. newmailService()) */
	MailService_prototype = (struct SEE_object *) SEE_NEW(interp,
			struct MailService_object);
	SEE_native_init((struct SEE_native *) MailService_prototype, interp,
			&mailService_inst_class, interp->Object_prototype);

	PUTFUNC(MailService_prototype, setServer, 1);
	PUTFUNC(MailService_prototype, setFrom, 1);
	PUTFUNC( MailService_prototype, setTo, 1);
	PUTFUNC(MailService_prototype, send, 1);

	// ****************************************************************** MailService
	/* Create the MailService object */
	MailService = (struct SEE_object *) SEE_NEW(interp, struct SEE_native);
	SEE_native_init((struct SEE_native *) MailService, interp, &mailService_constructor_class,
			interp->Object_prototype);
	PUTOBJ(interp->Global, MailService, MailService);
	PUTOBJ(MailService, prototype, MailService_prototype)

	/* Create the MailService.MailServiceError error object for I/O exceptions */
	MailServiceError = SEE_Error_make(interp, STR(MailServiceError));
	PUTOBJ(MailService, MailServiceError, MailServiceError);

	/* Keep pointers to our 'original' objects */
	PRIVATE(interp)->MailService_prototype = MailService_prototype;
	PRIVATE(interp)->MailServiceError = MailServiceError;
	PRIVATE(interp)->MailService = MailService;

#undef PUTFUNC
#undef PUTOBJ
}

/*
 * Converts an object into MailService_object, or throws a TypeError.
 *
 * This helper functon is called by the method functions in MailService.prototype
 * mainly to check that each method is being called with a correct 'this'.
 * Because a script may assign the member functions objects to a different
 * (non-MailService) object and invoke them, we cannot assume the thisobj pointer
 * always points to a 'struct MailService_object' structure.
 * In effect, this function is a 'safe' cast.
 * NOTE: There is a check for null because 'thisobj' can potentially
 * be a NULL pointer.
 */
static struct MailService_object * toMailService(struct SEE_interpreter *interp, struct SEE_object *o) {
	if (!o || o->objectclass != &mailService_inst_class)
		SEE_error_throw(interp, interp->TypeError, NULL);
	return (struct MailService_object *) o;
}

/*
 * Constructs a MailService object instance.
 * This helper function constructs and returns a new instance of a
 * MailService_object. It initialises the object with the given FILE pointer.
 */
static struct SEE_object * newMailService(struct SEE_interpreter *interp) {
	struct MailService_object *obj;

	obj = SEE_NEW_FINALIZE(interp, struct MailService_object, MailService_finalize, NULL);
	SEE_native_init(&obj->native, interp, &mailService_inst_class,
			PRIVATE(interp)->MailService_prototype);
	obj->from = new std::string;
	obj->to = new std::string;
	obj->server = new std::string;
	return (struct SEE_object *) obj;
}

/*
 * A finalizer function that is (eventually) called on lost MailService objects.
 * If a system crash or exit occurs, this function may not be called. SEE
 * cannot guarantee that this is ever called; however your garbage
 * collector implementation may guarantee it.
 */
static void MailService_finalize(struct SEE_interpreter *interp, void *obj,
		void *closure) {
	struct MailService_object *ms = (struct MailService_object*) obj;
	if (ms != NULL) {
		delete ms->from;
		delete ms->to;
		delete ms->server;
	}
}

/*
 * new MailService() -> object
 *
 * The MailService.[[Construct]] property is called when the user writes
 * "new MailService()".
 *
 */
static void MailService_construct(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {
	SEE_object* newobj = newMailService(interp);
	SEE_SET_OBJECT(res, newobj);
}

/*
 * MailService.prototype.setServer(server) -> undefined
 *
 * Sets the mail server
 */
static void MailService_prototype_setServer(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {
	struct MailService_object *fo = toMailService(interp, thisobj);

	if (argc < 1)
		SEE_error_throw(interp, interp->RangeError, "missing argument");
	else {
		SEE_value s;
		SEE_ToString(interp, argv[0], &s);
		*fo->server = SEE_StringToChars(interp, s.u.string);
	}
	SEE_SET_UNDEFINED(res);
}

/*
 * MailService.prototype.setServer(server) -> undefined
 *
 * Sets the mail sender
 */
static void MailService_prototype_setFrom(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {
	struct MailService_object *fo = toMailService(interp, thisobj);

	if (argc < 1)
		SEE_error_throw(interp, interp->RangeError, "missing argument");
	else {
		SEE_value s;
		SEE_ToString(interp, argv[0], &s);
		*fo->from = SEE_StringToChars(interp, s.u.string);
	}

	SEE_SET_UNDEFINED(res);
}

/*
 * MailService.prototype.setServer(server) -> undefined
 *
 * Sets the mail target recipeint
 */
static void MailService_prototype_setTo(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {
	struct MailService_object *fo = toMailService(interp, thisobj);

	if (argc < 1)
		SEE_error_throw(interp, interp->RangeError, "missing argument");
	else {
		SEE_value s;
		SEE_ToString(interp, argv[0], &s);
		*fo->to = SEE_StringToChars(interp, s.u.string);
	}
	SEE_SET_UNDEFINED(res);
}



static std::string recvMessage (SOCKET s) {
	std::string result;

#ifdef WIN32
	int asyncFlag = 0;
	unsigned long l = 1;
	ioctlsocket (s, FIONBIO, &l);
#else
	int asyncFlag = MSG_DONTWAIT;
#endif

	time_t now;
	time_t stop;

	time (&now);
	stop = now + 60000L; // 60 segs = 1 minut


	bool done = false;
	do {
		char achBuffer[256];
		int read = recv (s, achBuffer, (sizeof achBuffer) -1, asyncFlag);
		if ( read == -1 ) {
#ifdef WIN32
			Sleep (1000);
#else
			sleep (1);
#endif
			time (&now);
		} else {
			achBuffer[read] = '\0';
			result += achBuffer;
			done =  (result.find('\n') != std::string::npos);
		}

	} while (!done && now < stop);
	if (now >= stop)
		result = "";
	return result;
}


static std::string sendMessage (SOCKET s, const char *msg) {
	std::string result;
	if (send (s, msg, strlen(msg), 0) == -1)
		return result;
	else
		result = recvMessage(s);

	return result;
}

static bool isErrorMessage (const char*msg) {
	return msg[0] != '2' && msg[0] != '3';
}

/*
 * MailService.prototype.setServer(server) -> undefined
 *
 * Sets the mail target recipeint
 */
static void MailService_prototype_send(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {
	struct MailService_object *fo = toMailService(interp, thisobj);

	if (argc < 1)
		SEE_error_throw(interp, interp->RangeError, "missing argument");

#ifdef WIN32
	WSADATA wsaData;;
	int err = WSAStartup(MAKEWORD(2, 0), &wsaData);
	if (err != 0) {
		SEE_error_throw(interp, PRIVATE(interp)->MailServiceError, "Error initializing windows sockets: ");
		return ;
	}
#endif
	SEE_value msg_v;
	SEE_ToString(interp, argv[0], &msg_v);
	std::string msg = SEE_StringToChars(interp, msg_v.u.string);

	struct sockaddr_in addrin;
    addrin.sin_family = AF_INET;
	addrin.sin_port = htons (25);
	struct hostent* hostentry = gethostbyname(fo->server->c_str());
	if (hostentry == NULL) {
		u_long addr = inet_addr (fo->server->c_str());
#ifdef WIN32
		addrin.sin_addr.S_un.S_addr = addr;
#else
		addrin.sin_addr.s_addr = addr;
#endif
		if (addr == 0 ) {
			SEE_error_throw(interp, PRIVATE(interp)->MailServiceError, "Error resolving host %s ", fo->server->c_str());
			return ;
		}
	} else {
#ifdef WIN32
		addrin.sin_addr.S_un.S_addr = *(u_long*) hostentry->h_addr_list[0];
#else
		addrin.sin_addr.s_addr = *(u_long*) hostentry->h_addr_list[0];
#endif
	}

    SOCKET s = socket (PF_INET, SOCK_STREAM, 0);
	if (connect (s,(struct sockaddr *)&addrin, sizeof (addrin)) != 0) {
		SEE_error_throw(interp, PRIVATE(interp)->MailServiceError, "Error connectig to host %s port 25 ", fo->server->c_str());
		closesocket(s);
		return ;
	}

	std::string line = "HELO ";
	line += MZNC_getHostName();
	line += "\n";
	std::string result = recvMessage(s);
	if (isErrorMessage(result.c_str())) {
		SEE_error_throw(interp, PRIVATE(interp)->MailServiceError, "Error connecting to host %s: %s",
				fo->server->c_str(), result.c_str());
		return ;
	}

	result = sendMessage (s, line.c_str());
	if (isErrorMessage(result.c_str())) {
		SEE_error_throw(interp, PRIVATE(interp)->MailServiceError, "Error sending message [%s] to host %s: %s",
				line.c_str(), fo->server->c_str(), result.c_str());
		return ;
	}

	line = "MAIL FROM: ";
	line += fo->from->c_str();
	line += "\r\n";
	result = sendMessage (s, line.c_str());
	if (isErrorMessage(result.c_str())) {
		SEE_error_throw(interp, PRIVATE(interp)->MailServiceError, "Error sending message [%s] to host %s: %s",
				line.c_str(), fo->server->c_str(), result.c_str());
		return ;
	}

	line = "RCPT TO: ";
	line += fo->to->c_str();
	line += "\r\n";
	result = sendMessage (s, line.c_str());
	if (isErrorMessage(result.c_str())) {
		SEE_error_throw(interp, PRIVATE(interp)->MailServiceError, "Error sending message [%s] to host %s: %s",
				line.c_str(), fo->server->c_str(), result.c_str());
		return ;
	}

	line = "DATA\r\n";
	result = sendMessage (s, line.c_str());
	if (isErrorMessage(result.c_str())) {
		SEE_error_throw(interp, PRIVATE(interp)->MailServiceError, "Error sending message [%s] to host %s: %s",
				line.c_str(), fo->server->c_str(), result.c_str());
		return ;
	}

	line = msg;
	line += "\r\n.\r\n";
	result = sendMessage (s, line.c_str());
	if (isErrorMessage(result.c_str())) {
		SEE_error_throw(interp, PRIVATE(interp)->MailServiceError, "Error sending message [%s] to host %s: %s",
				line.c_str(), fo->server->c_str(), result.c_str());
		return ;
	}

	line = "QUIT\r\n";
	result = sendMessage (s, line.c_str());
	if (isErrorMessage(result.c_str())) {
		SEE_error_throw(interp, PRIVATE(interp)->MailServiceError, "Error sending message [%s] to host %s: %s",
				line.c_str(), fo->server->c_str(), result.c_str());
		return ;
	}


	close (s);


	SEE_SET_UNDEFINED(res);
}

