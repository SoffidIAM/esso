/* David Leonard, 2006. This file released into the public domain. */
/* Gabriel Buades 2011 */

/*
 * This module is intended to provide an example of how to write a module
 * and host objects for SEE. Feel free to use it for the basis of
 * your own works.
 *
 * When loaded, this example provides the following objects based on
 * stdio FILEs:
 *
 *      File                       - constructor/opener object
 *      File.prototype             - container object for common methods
 *      File.prototype.read        - reads data from the file
 *      File.prototype.readLine    - reads line from the file
 *      File.prototype.eof         - tells if a file is at EOF
 *      File.prototype.write       - writes data to a file
 *      File.prototype.writeLine   - writes line to a file
 *      File.prototype.flush       - flushes a file output
 *      File.prototype.close       - closes a file
 *      File.mkdir                 - creates a directory
 *      File.copy                  - copies a file
 *      File.remove                - removes a file
 *      File.move                  - moves a file
 *      File.isDirectory           - test if file is a directory
 *      File.getParent             - gets a file's directory
 *      File.canWrite              - test write access to a file
 *      File.canRead               - test read access to a file
 *      File.FileError             - object thrown when an error occurs
 *      File.In                    - standard input
 *      File.Out                   - standard output
 *      File.Err                   - standard error
 *      Directory                  - constructor/opener object
 *      Directory.prototype        - container object for common methods
 *      Directory.prototype.item   - gets a diretory member
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
#endif
#include <sys/stat.h>
#include "ecma.h"
#include <fstream>
#include <ios>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

/* Prototypes */
static int File_mod_init(void);
static void File_alloc(struct SEE_interpreter *);
static void File_init(struct SEE_interpreter *);

static void file_construct(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void File_mkdir(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void File_prototype_read(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void File_prototype_readLine(struct SEE_interpreter *,
		struct SEE_object *, struct SEE_object *, int, struct SEE_value **,
		struct SEE_value *);
static void File_prototype_eof(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void File_prototype_write(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void File_prototype_writeLine(struct SEE_interpreter *,
		struct SEE_object *, struct SEE_object *, int, struct SEE_value **,
		struct SEE_value *);
static void File_prototype_flush(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void File_prototype_close(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void File_copy(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void File_move(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void File_remove(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void File_isDirectory(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void File_canRead(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void File_canWrite(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void File_getParent(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);

static void directory_construct(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void Directory_prototype_item(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_object *, int, struct SEE_value **, struct SEE_value *);

static struct file_object *tofile(struct SEE_interpreter *interp,
		struct SEE_object *);
static struct SEE_object *newfile(struct SEE_interpreter *interp, FILE *file);
static void file_finalize(struct SEE_interpreter *, void *, void *);

static struct directory_object *todirectory(struct SEE_interpreter *interp,
		struct SEE_object *);
static struct SEE_object *newdirectory(struct SEE_interpreter *interp,
		const char* dirName);
static void directory_finalize(struct SEE_interpreter *, void *, void *);

/*
 * The File_module structure is the only symbol exported here.
 * It contains some simple identification information but, most
 * importantly, pointers to the major initialisation functions in
 * this file.
 * This structure is passed to SEE_module_add() once and early.
 */
struct SEE_module File_module = { SEE_MODULE_MAGIC, /* magic */
"File", /* name */
"1.0", /* version */
0, /* index (set by SEE) */
File_mod_init, /* mod_init */
File_alloc, /* alloc */
File_init /* init */
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
	struct SEE_object *File; /* The File object */
	struct SEE_object *File_prototype; /* File.prototype */
	struct SEE_object *FileError; /* File.FileError */
	struct SEE_object *Directory; /* The Directory object */
	struct SEE_object *Directory_prototype; /* Directory.prototype */
};

#define PRIVATE(interp)  \
        ((struct module_private *)SEE_MODULE_PRIVATE(interp, &File_module))

/*
 * To make string usage more efficient, we globally intern some common
 * strings and provide a STR() macro to access them.
 * Internalised strings are guaranteed to have unique pointers,
 * which means you can use '==' instead of 'strcmp()' to compare names.
 * The pointers must be allocated during mod_init() because the global
 * intern table is locked as soon as an interpreter instance is created.
 */
#define STR(name) s_##name
static struct SEE_string *STR(Err), *STR(File), *STR(FileError), *STR(In),
		*STR(Out), *STR(close), *STR(eof), *STR(flush), *STR(prototype),
		*STR(read), *STR(write), *STR(mkdir), *STR(readLine),
		*STR(writeLine), *STR(copy), *STR(remove), *STR(move),
		*STR(isDirectory), *STR(canRead), *STR(canWrite), *STR(getParent),
		*STR(Directory), *STR(length), *STR(item);

/*
 * This next structure is the one we use for a host object. It will
 * behave just like a normal ECAMscript object, except that we
 * "piggyback" a stdio FILE* onto it. A new instance is created whenever
 * you evaluate 'new File()'.
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
 * File.prototype object; that way the file methods can be found by the
 * normal prototype-chain method. (The search is automatically performed by
 * SEE_native_get.)
 *
 * The alert reader will notice that later we create the special object
 * File.prototype as an instance of this struct file_object too, except that
 * its file pointer is NULL, and its [[Prototype]] points to Object.prototype.
 */

struct file_object {
	struct SEE_native native;
	FILE *file;
};

struct directory_object {
	struct SEE_native native;
	std::string *fileName;
	std::vector<std::string> *children;
};

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
static struct SEE_objectclass file_inst_class = { "File", /* Class */
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
 * This is the class structure for the toplevel 'File' object. The 'File'
 * object doubles as both a constructor function and as a container object
 * to hold File.prototype and some other useful properties. 'File' has only
 * one important intrinsic property, namely the [[Construct]] method. It
 * is called whenever the expression 'new File()' is evaluated.
 */
static struct SEE_objectclass file_constructor_class = { "File", /* Class */
SEE_native_get, /* Get */
SEE_native_put, /* Put */
SEE_native_canput, /* CanPut */
SEE_native_hasproperty, /* HasProperty */
SEE_native_delete, /* Delete */
SEE_native_defaultvalue, /* DefaultValue */
SEE_native_enumerator, /* DefaultValue */
file_construct, /* Construct */
NULL /* Call */
};

static struct SEE_objectclass directory_inst_class = { "Directory", /* Class */
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
 * This is the class structure for the toplevel 'Directory' object. The 'Directory'
 * object doubles as both a constructor function and as a container object
 * to hold Directory.prototype and some other useful properties. 'Directory' has only
 * one important intrinsic property, namely the [[Construct]] method. It
 * is called whenever the expression 'new Directory()' is evaluated.
 */
static struct SEE_objectclass directory_constructor_class = {
	"Directory", /* Class */
	SEE_native_get, /* Get */
	SEE_native_put, /* Put */
	SEE_native_canput, /* CanPut */
	SEE_native_hasproperty, /* HasProperty */
	SEE_native_delete, /* Delete */
	SEE_native_defaultvalue, /* DefaultValue */
	SEE_native_enumerator, /* DefaultValue */
	directory_construct, /* Construct */
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
static int File_mod_init() {
	STR(Err) = SEE_intern_global("Err");
	STR(File) = SEE_intern_global("File");
	STR(FileError) = SEE_intern_global("FileError");
	STR(In) = SEE_intern_global("In");
	STR(Out) = SEE_intern_global("Out");
	STR(close) = SEE_intern_global("close");
	STR(eof) = SEE_intern_global("eof");
	STR(flush) = SEE_intern_global("flush");
	STR(prototype) = SEE_intern_global("prototype");
	STR(read) = SEE_intern_global("read");
	STR(readLine) = SEE_intern_global("readLine");
	STR(writeLine) = SEE_intern_global("writeLine");
	STR(write) = SEE_intern_global("write");
	STR(mkdir) = SEE_intern_global("mkdir");
	STR(copy) = SEE_intern_global("copy");
	STR(canRead) = SEE_intern_global("canRead");
	STR(canWrite) = SEE_intern_global("canWrite");
	STR(remove) = SEE_intern_global("remove");
	STR(move) = SEE_intern_global("move");
	STR(isDirectory) = SEE_intern_global("isDirectory");
	STR(getParent) = SEE_intern_global("getParent");
	STR(Directory) = SEE_intern_global("Directory");
	STR(item) = SEE_intern_global("item");
	STR(length) = SEE_intern_global("length");
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
static void File_alloc(struct SEE_interpreter *interp) {
	SEE_MODULE_PRIVATE(interp, &File_module) =
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
static void File_init(struct SEE_interpreter *interp) {
	struct SEE_object *File, *File_prototype, *FileError;
	struct SEE_object *Directory;
	struct SEE_object *Directory_prototype;
	struct SEE_value v;

	/* Convenience macro for adding properties to File */
#define PUTOBJ(parent, name, obj)                                       \
        SEE_SET_OBJECT(&v, obj);                                        \
        SEE_OBJECT_PUT(interp, parent, STR(name), &v, SEE_ATTR_DEFAULT);

	/* Convenience macro for adding functions to File.prototype */
#define PUTFUNC(obj, name, len)                                         \
        SEE_SET_OBJECT(&v, SEE_cfunction_make(interp, obj##_##name,\
                STR(name), len));                                       \
        SEE_OBJECT_PUT(interp, obj, STR(name), &v, SEE_ATTR_DEFAULT);

	// ****************************************************************** File.prototype
	/* Create the File.prototype object (cf. newfile()) */
	File_prototype = (struct SEE_object *) SEE_NEW(interp,
			struct file_object);
	SEE_native_init((struct SEE_native *) File_prototype, interp,
			&file_inst_class, interp->Object_prototype);
	((struct file_object *) File_prototype)->file = NULL;

	PUTFUNC(File_prototype, read, 0);
	PUTFUNC(File_prototype, write, 1);
	PUTFUNC( File_prototype, readLine, 0);
	PUTFUNC(File_prototype, writeLine, 1);
	PUTFUNC( File_prototype, close, 0);
	PUTFUNC(File_prototype, eof, 0);
	PUTFUNC( File_prototype, flush, 0);

	// ****************************************************************** File
	/* Create the File object */
	File = (struct SEE_object *) SEE_NEW(interp, struct SEE_native);
	SEE_native_init((struct SEE_native *) File, interp, &file_constructor_class,
			interp->Object_prototype);
	PUTOBJ(interp->Global, File, File);
	PUTOBJ(File, prototype, File_prototype)

	/* Create the File.FileError error object for I/O exceptions */
	FileError = SEE_Error_make(interp, STR(FileError));
	PUTOBJ(File, FileError, FileError);

	/* Keep pointers to our 'original' objects */
	PRIVATE(interp)->File_prototype = File_prototype;
	PRIVATE(interp)->FileError = FileError;
	PRIVATE(interp)->File = File;

	/* Now we can build our well-known files */
	PUTOBJ(File, In, newfile(interp, stdin));
	PUTOBJ(File, Out, newfile(interp, stdout));
	PUTOBJ(File, Err, newfile(interp, stderr));
	PUTFUNC( File, mkdir, 1);
	PUTFUNC(File, copy, 1);
	PUTFUNC(File, remove, 1);
	PUTFUNC( File, move, 2);
	PUTFUNC(File, isDirectory, 1);
	PUTFUNC(File, canRead, 1);
	PUTFUNC( File, canWrite, 1);
	PUTFUNC(File, getParent, 1)

	// ****************************************************************** Directory.prototype
	/* Create the Direcotry.prototype object  */
	Directory_prototype = (struct SEE_object *) SEE_NEW(interp,
			struct directory_object);
	SEE_native_init((struct SEE_native*)Directory_prototype, interp,
			&directory_inst_class, interp->Object_prototype);

	PUTFUNC(Directory_prototype, item, 1);

	/* Create the Directory object */
	Directory = (struct SEE_object *) SEE_NEW(interp, struct SEE_native);
	SEE_native_init((struct SEE_native *) Directory, interp, &directory_constructor_class,
			interp->Object_prototype);
	PUTOBJ(interp->Global, Directory, Directory);
	PUTOBJ(Directory, prototype, Directory_prototype)

	/* Keep pointers to our 'original' objects */
	PRIVATE(interp)->Directory = Directory;
	PRIVATE(interp)->Directory_prototype = Directory_prototype;
#undef PUTFUNC
#undef PUTOBJ
}

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
static struct file_object * tofile(struct SEE_interpreter *interp, struct SEE_object *o) {
	if (!o || o->objectclass != &file_inst_class)
		SEE_error_throw(interp, interp->TypeError, NULL);
	return (struct file_object *) o;
}

/*
 * Constructs a file object instance.
 * This helper function constructs and returns a new instance of a
 * file_object. It initialises the object with the given FILE pointer.
 */
static struct SEE_object * newfile(struct SEE_interpreter *interp, FILE *file) {
	struct file_object *obj;

	obj = SEE_NEW_FINALIZE(interp, struct file_object, file_finalize, NULL);
	SEE_native_init(&obj->native, interp, &file_inst_class,
			PRIVATE(interp)->File_prototype);
	obj->file = file;
	return (struct SEE_object *) obj;
}

/*
 * A finalizer function that is (eventually) called on lost file objects.
 * If a system crash or exit occurs, this function may not be called. SEE
 * cannot guarantee that this is ever called; however your garbage
 * collector implementation may guarantee it.
 */
static void file_finalize(struct SEE_interpreter *interp, void *obj,
		void *closure) {
	struct file_object *fo = (struct file_object *) obj;

	if (fo->file) {
		fclose(fo->file);
		fo->file = NULL;
	}
}

/*
 * new File(pathame [,mode]) -> object
 *
 * The File.[[Construct]] property is called when the user writes
 * "new File()". This constructor expects two arguments (one is optional):
 *      argv[0] the filename to open
 *      argv[1] the file mode (eg "r", "+b", etc) defaults to "r"
 * e.g.: new File("/tmp/foo", "r")
 * Throws a RangeError if the first argument is missing.
 *
 * Note: 'undefined' optional arguments are treated as if they were
 * missing. (This is a common style in ECMAScript objects.)
 */
static void file_construct(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {
	std::string path;
	char *mode = (char *) "r";
	FILE *file;
	struct SEE_object *newobj;
	struct SEE_string *pathString;

	SEE_parse_args(interp, argc, argv, "s|Z", &pathString, &mode);
	if (!pathString)
		SEE_error_throw(interp, interp->RangeError, "missing argument");

	path = SEE_StringToChars(interp, pathString);
	file = fopen(path.c_str(), mode);
	if (!file) {
		/* May have too many files open; collect and try again */
		SEE_gcollect(interp);
		file = fopen(path.c_str(), mode);
	}
	if (!file)
		SEE_error_throw(interp, PRIVATE(interp)->FileError, "%s",
				strerror(errno));

	newobj = newfile(interp, file);
	SEE_SET_OBJECT(res, newobj);
}

/*
 * File.prototype.read([length]) -> string/undefined
 *
 * Reads and returns string data. If an argument is given, it limits the
 * length of the string read.
 * If the file is closed, this function return undefined.
 */
static void File_prototype_read(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {
	struct file_object *fo = tofile(interp, thisobj);
	SEE_uint32_t len, i;
	struct SEE_string *buf;
	int unbound;

	if (argc == 0 || SEE_VALUE_GET_TYPE(argv[0]) == SEE_UNDEFINED) {
		unbound = 1;
		len = 1024;
	} else {
		unbound = 0;
		len = SEE_ToUint32(interp, argv[0]);
	}
	if (!fo->file) {
		SEE_SET_UNDEFINED(res);
		return;
	}
	buf = SEE_string_new(interp, len);
	for (i = 0; unbound || i < len; i++) {
		int ch = fgetc(fo->file);
		if (ch == EOF)
			break;
		SEE_string_addch(buf, ch);
	}
	SEE_SET_STRING(res, buf);
}

/*
 * File.prototype.readLine() -> string/undefined
 *
 * Reads and returns string data until new line
 * If the file is closed, this function return undefined.
 */
static void File_prototype_readLine(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {
	struct file_object *fo = tofile(interp, thisobj);

	if (!fo->file) {
		SEE_SET_UNDEFINED(res);
		return;
	}
	std::string buffer;
	bool eol = false;
	do {
		char ach[1024];
		if (fgets(ach, (sizeof ach) - 1, fo->file) == NULL)
			eol = true;
		else {
			int len = strlen(ach);
			if (len > 0 && ach[len - 1] == '\n') {
				ach[len - 1] = '\0';
				eol = true;
			}
			buffer += ach;
		}

	} while (!eol);

	SEE_SET_STRING(res, SEE_CharsToString(interp, buffer.c_str()));
}

/*
 * File.prototype.eof() -> boolean/undefined
 *
 * Returns true if the last read resulted in an EOF.
 * Closed files return undefined.
 */
static void File_prototype_eof(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res)

		{
	struct file_object *fo = tofile(interp, thisobj);

	if (!fo->file)
		SEE_SET_UNDEFINED(res);
	else
		SEE_SET_BOOLEAN(res, feof(fo->file));
}

/*
 * File.prototype.write(data)
 *
 * Writes the string argument to the file.
 * The string must be 8-bit data only.
 * Closed files throw an exception.
 */
static void File_prototype_write(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {
	struct file_object *fo = tofile(interp, thisobj);
	struct SEE_value v;
	unsigned int len;

	if (argc < 1)
		SEE_error_throw(interp, interp->RangeError, "missing argument");
	if (!fo->file)
		SEE_error_throw(interp, PRIVATE(interp)->FileError, "file is closed");
	SEE_ToString(interp, argv[0], &v);
	for (len = 0; len < v.u.string->length; len++) {
		if (v.u.string->data[len] > 0xff)
			SEE_error_throw(interp, interp->RangeError, "bad data");
		if (fputc(v.u.string->data[len], fo->file) == EOF)
			SEE_error_throw(interp, PRIVATE(interp)->FileError, "write error");
	}
	SEE_SET_UNDEFINED(res);
}

/*
 * File.prototype.writeLine(data)
 *
 * Writes the string argument to the file.
 * The string must be 8-bit data only.
 * Closed files throw an exception.
 */
static void File_prototype_writeLine(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {
	struct file_object *fo = tofile(interp, thisobj);
	struct SEE_value v;

	if (argc < 1)
		SEE_error_throw(interp, interp->RangeError, "missing argument");
	if (!fo->file)
		SEE_error_throw(interp, PRIVATE(interp)->FileError, "file is closed");

	SEE_ToString(interp, argv[0], &v);
	std::string s = SEE_StringToChars(interp, v.u.string);
	fprintf(fo->file, "%s\n", s.c_str());
	SEE_SET_UNDEFINED(res);
}
/*
 * File.prototype.flush()
 *
 * Flushes the file, if not already closed.
 */
static void File_prototype_flush(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {
	struct file_object *fo = tofile(interp, thisobj);

	if (fo->file)
		fflush(fo->file);
	SEE_SET_UNDEFINED(res);
}

/*
 * File.prototype.close()
 *
 * Closes the file. The 'file' field is set to NULL to let other
 * member functions know that the file is closed.
 */
static void File_prototype_close(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {
	struct file_object *fo = tofile(interp, thisobj);

	if (fo->file) {
		fclose(fo->file);
		fo->file = NULL;
	}
	SEE_SET_UNDEFINED(res);
}

/*
 * File.mkdir()
 *
 * creates the directory
 */
static void File_mkdir(struct SEE_interpreter *interp, struct SEE_object *self,
		struct SEE_object *thisobj, int argc, struct SEE_value **argv,
		struct SEE_value *res) {
	if (argc < 1) {
		SEE_error_throw(interp, interp->RangeError, "missing argument");
		return;
	}
	struct SEE_value v;
	SEE_ToString(interp, argv[0], &v);
	std::string path = SEE_StringToChars(interp, v.u.string);
#ifdef WIN32
	CreateDirectoryA(path.c_str(), NULL);
#else
	mkdir(path.c_str(), 0666);
#endif
	SEE_SET_UNDEFINED(res);
}

/*
 * File.copy()
 *
 * copies a file
 */
static bool doCopy(struct SEE_interpreter *interp, struct SEE_object *self,
		struct SEE_object *thisobj, int argc, struct SEE_value **argv,
		struct SEE_value *res, bool deleteAfterCopy) {
	SEE_SET_UNDEFINED(res);

	if (argc < 2) {
		SEE_error_throw(interp, interp->RangeError, "missing argument");
		return false;
	}
	struct SEE_value v;
	SEE_ToString(interp, argv[0], &v);
	std::string source = SEE_StringToChars(interp, v.u.string);

	SEE_ToString(interp, argv[1], &v);
	std::string target = SEE_StringToChars(interp, v.u.string);

	std::ifstream in(source.c_str(), std::ios::in);
	if (in.fail()) {
		SEE_error_throw(interp, PRIVATE(interp)->FileError,
				"Cannot open source file %s", source.c_str());
		return false;
	} else {
		std::ofstream out(target.c_str(),
				std::ios::binary | std::ios::out | std::ios::trunc);
		if (out.fail()) {
			SEE_error_throw(interp, PRIVATE(interp)->FileError,
					"Cannot open target file %s", target.c_str());
			return false;
		} else {
			char b[2048];
			do {
				in.read(b, sizeof b);
				out.write(b, in.gcount());
				if (out.fail()) {
					SEE_error_throw(interp, PRIVATE(interp)->FileError,
							"Cannot open source file %s", source.c_str());
					out.close();
					in.close();
					return false;
				}
			} while (!in.fail() && !in.eof());
			out.close();
		}
		in.close();
		if (deleteAfterCopy)
			unlink(source.c_str());
		return true;
	}
}

static void File_copy(struct SEE_interpreter *interp, struct SEE_object *self,
		struct SEE_object *thisobj, int argc, struct SEE_value **argv,
		struct SEE_value *res) {
	SEE_SET_UNDEFINED(res);
	doCopy(interp, self, thisobj, argc, argv, res, false);
}

/*
 * File.move()
 *
 * moves a file
 */
static void File_move(struct SEE_interpreter *interp, struct SEE_object *self,
		struct SEE_object *thisobj, int argc, struct SEE_value **argv,
		struct SEE_value *res) {
	SEE_SET_UNDEFINED(res);
	doCopy(interp, self, thisobj, argc, argv, res, true);

}
/*
 * File.remove()
 *
 * removes a file
 */
static void File_remove(struct SEE_interpreter *interp, struct SEE_object *self,
		struct SEE_object *thisobj, int argc, struct SEE_value **argv,
		struct SEE_value *res) {
	SEE_SET_UNDEFINED(res);

	if (argc < 1) {
		SEE_error_throw(interp, interp->RangeError, "missing argument");
		return;
	} else {
		struct SEE_value v;
		SEE_ToString(interp, argv[0], &v);
		std::string source = SEE_StringToChars(interp, v.u.string);
		if (unlink(source.c_str()) != 0) {
			SEE_error_throw(interp, PRIVATE(interp)->FileError,
					"Cannot remove source file %s", source.c_str());
		}
	}

}

/*
 * File.isDirectory()
 *
 * returns true if param is a directory
 */
static void File_isDirectory(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {
	SEE_SET_UNDEFINED(res);

	if (argc < 1) {
		SEE_error_throw(interp, interp->RangeError, "missing argument");
		return;
	} else {
		struct SEE_value v;
		SEE_ToString(interp, argv[0], &v);
		std::string source = SEE_StringToChars(interp, v.u.string);
		struct stat s;
		if (stat(source.c_str(), &s) == -1) {
			SEE_SET_BOOLEAN(res, false);
		} else {
			SEE_SET_BOOLEAN(res, S_ISDIR(s.st_mode));
		}
	}

}
static void File_canRead(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {

	SEE_SET_UNDEFINED(res);

	if (argc < 1) {
		SEE_error_throw(interp, interp->RangeError, "missing argument");
		return;
	} else {
		struct SEE_value v;
		bool ok = false;
		SEE_ToString(interp, argv[0], &v);
		std::string file = SEE_StringToChars(interp, v.u.string);
		int o = open(file.c_str(), O_RDONLY);
		if (o != -1) {
			close(o);
			ok = true;
		}
		SEE_SET_BOOLEAN(res, ok);
	}
}
static void File_canWrite(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {
	SEE_SET_UNDEFINED(res);

	if (argc < 1) {
		SEE_error_throw(interp, interp->RangeError, "missing argument");
		return;
	} else {
		struct SEE_value v;
		bool ok = false;
		SEE_ToString(interp, argv[0], &v);
		std::string file = SEE_StringToChars(interp, v.u.string);
		int o = open(file.c_str(), O_WRONLY);
		if (o == -1 && errno == ENOENT) {
			o = open(file.c_str(), O_WRONLY | O_CREAT | O_EXCL);
			if (o != -1) {
				close(o);
				unlink(file.c_str());
				ok = true;
			}
		} else if (o == -1){
			ok = false;
		} else {
			close(o);
			ok = true;
		}
		SEE_SET_BOOLEAN(res, ok);
	}
}
static void File_getParent(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {

	SEE_SET_UNDEFINED(res);

	if (argc < 1) {
		SEE_error_throw(interp, interp->RangeError, "missing argument");
	} else {
		struct SEE_value v;
		SEE_ToString(interp, argv[0], &v);
		std::string file = SEE_StringToChars(interp, v.u.string);
#ifdef WIN32
		const char separator = '\\';
		if (file.length() < 3 || file.at(1) != ':' || file.at(2) != '\\') {
			const char *cwd = getcwd(NULL, 0);
			std::string f2 = cwd;
			file = f2 + "\\" + file;
		}
#else
		const char separator = '/';
		if (file.length() < 1 || file.at(0) != '/') {
			const char *cwd = getcwd(NULL, 0);
			std::string f2 = cwd;
			file = f2 + "/" + file;
		}
#endif
		std::vector<std::string> split;
		unsigned int i = 0;
		unsigned int j = 0;
		while (i < file.length()) {
			j = i;
			while (j < file.length() && file[j] != separator)
				j++;
			std::string part = file.substr(i, j - i);
			if (j > i) {
				if (part == "..") {
					if (split.size() > 0)
						split.erase(split.end() - 1);
				} else if (part == ".") {
					// Ignore
				} else {
					split.push_back(part);
				}
			}
			i = j + 1;

		}
		std::string result;
#ifndef WIN32
		result = "/";
#endif
		for (std::vector<std::string>::iterator i = split.begin();
				i != split.end() - 1; i++) {
			result += *i;
			if (i < split.end() - 1)
				result += separator;
		}
		SEE_SET_STRING(res, SEE_CharsToString(interp, result.c_str()));
	}
}


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
static struct directory_object * todirectory(struct SEE_interpreter *interp, struct SEE_object *o) {
	if (!o || o->objectclass != &directory_inst_class)
		SEE_error_throw(interp, interp->TypeError, NULL);
	return (struct directory_object *) o;
}

/*
 * Constructs a file object instance.
 * This helper function constructs and returns a new instance of a
 * file_object. It initialises the object with the given FILE pointer.
 */
static struct SEE_object * newdirectory(struct SEE_interpreter *interp, const char *dirName) {
	struct directory_object *obj;

	obj = SEE_NEW_FINALIZE(interp, struct directory_object, directory_finalize, NULL);
	SEE_native_init(&obj->native, interp, &directory_inst_class,
			PRIVATE(interp)->Directory_prototype);
	obj->fileName = new std::string (dirName);
	obj->children = new std::vector<std::string>;
	DIR* d = opendir (dirName);

	struct dirent *entry = readdir (d);
	while (entry != NULL) {
		std::string s = entry->d_name;
		obj->children->push_back(s);
		entry = readdir (d);
	}
	closedir(d);

	SEE_value v;
    SEE_SET_NUMBER(&v, obj->children->size());
    SEE_OBJECT_PUT(interp, &obj->native.object, STR(length), &v, SEE_ATTR_READONLY);


	return (struct SEE_object *) obj;
}

/*
 * A finalizer function that is (eventually) called on lost file objects.
 * If a system crash or exit occurs, this function may not be called. SEE
 * cannot guarantee that this is ever called; however your garbage
 * collector implementation may guarantee it.
 */
static void directory_finalize(struct SEE_interpreter *interp, void *obj,
		void *closure) {
	struct directory_object *fo = (struct directory_object *) obj;
	delete fo->fileName;
	delete fo->children;
}

/*
 * new File(pathame [,mode]) -> object
 *
 * The File.[[Construct]] property is called when the user writes
 * "new File()". This constructor expects two arguments (one is optional):
 *      argv[0] the filename to open
 *      argv[1] the file mode (eg "r", "+b", etc) defaults to "r"
 * e.g.: new File("/tmp/foo", "r")
 * Throws a RangeError if the first argument is missing.
 *
 * Note: 'undefined' optional arguments are treated as if they were
 * missing. (This is a common style in ECMAScript objects.)
 */
static void directory_construct(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {
	std::string path;
	struct SEE_object *newobj;

	if (argc < 1) {
		SEE_error_throw(interp, interp->RangeError, "missing argument");
	} else {
		struct SEE_value v;
		SEE_ToString(interp, argv[0], &v);
		std::string name = SEE_StringToChars(interp, v.u.string);
		newobj = newdirectory(interp, name.c_str());
		SEE_SET_OBJECT(res, newobj);
	}
}


static void Directory_prototype_item(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {

	struct directory_object *dir = todirectory(interp, thisobj);

	if (dir != NULL) {
		if (argc < 1) {
			SEE_error_throw(interp, interp->RangeError, "missing argument");
		} else {
			SEE_uint32_t result = SEE_ToInt32(interp, argv[0]);
			if (result < 0 || result >= dir->children->size()) {
				SEE_SET_UNDEFINED(res);
			} else {
				SEE_SET_STRING(res, SEE_CharsToString(interp, dir->children->at(result).c_str()));
			}

		}
	}
}

static void Directory_prototype_length(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {

	struct directory_object *dir = todirectory(interp, thisobj);

	if (dir != NULL)
		SEE_SET_NUMBER(res, dir->children->size());
}
