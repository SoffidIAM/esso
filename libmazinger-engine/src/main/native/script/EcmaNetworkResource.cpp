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
#include <SecretStore.h>

#ifdef WIN32
#include <winnetwk.h>
#include <lm.h>
#else
#include <sys/mount.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif



/* Prototypes */
static int NetworkResource_mod_init(void);
static void NetworkResource_alloc(struct SEE_interpreter *);
static void NetworkResource_init(struct SEE_interpreter *);

struct SEE_module NetworkResource_module = {
	SEE_MODULE_MAGIC, /* magic */
	"NetworkResource", /* name */
	"1.0", /* version */
	0, /* index (set by SEE) */
	NetworkResource_mod_init, /* mod_init */
	NetworkResource_alloc, /* alloc */
	NetworkResource_init /* init */
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
	struct SEE_object *NetworkError; /* NetworkResource.NetworkError */
};

#define PRIVATE(interp)  \
        ((struct module_private *)SEE_MODULE_PRIVATE(interp, &NetworkResource_module))

/*
 * To make string usage more efficient, we globally intern some common
 * strings and provide a STR() macro to access them.
 * Internalised strings are guaranteed to have unique pointers,
 * which means you can use '==' instead of 'strcmp()' to compare names.
 * The pointers must be allocated during mod_init() because the global
 * intern table is locked as soon as an interpreter instance is created.
 */
static struct SEE_string *s_connectPrinter;
static struct SEE_string *s_connectDrive;
static struct SEE_string *s_disconnectAllPrinters;
static struct SEE_string *s_disconnectPrinter;
static struct SEE_string *s_disconnectDrive;
static struct SEE_string *s_NetworkResource;
static struct SEE_string *s_NetworkError;

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
static struct SEE_objectclass networkResource_inst_class = { "NetworkResource", /* Class */
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


#ifdef WIN32

static std::vector<std::string> regList (HKEY hKey) {
	std::vector<std::string> result;
	DWORD dwType;
	DWORD dw;
	DWORD dwValueName, dwData;

	int num = 0;
	do {
		dwValueName = 0;
		dwData = 0;
		dw = RegEnumValue( hKey, num, NULL, &dwValueName, NULL, &dwType, NULL, &dwData);
		if (dw == ERROR_MORE_DATA) {
			char *name = (char*) malloc (dwValueName + 1);
			char *value = (char*) malloc (dwData + 1);
			dw = RegEnumValue ( hKey, num, name, &dwValueName, NULL, &dwType, (LPBYTE) value, &dwData);
			if (dw == ERROR_SUCCESS) {
				name[dwValueName] = '\0';
		 		std::string s = name;
				result.push_back(s);
			}
			free ((char*)name);
			free ((char*)value);
		}
		num ++;

	} while (dw == ERROR_SUCCESS);


	return result;

}

static std::vector<std::string> regListKeys (HKEY hKey) {
	std::vector<std::string> result;
	DWORD dw;
	DWORD dwKeySize = 0;
	RegQueryInfoKey (hKey, NULL, NULL, NULL,
			NULL,
			&dwKeySize,
			NULL, NULL, NULL, NULL, NULL, NULL );
	char *key = (char*) malloc (dwKeySize + 1);
	int num = 0;
	do {
		dw = RegEnumKey( hKey, num, key,  dwKeySize+1);
		if (dw == ERROR_SUCCESS) {
			key[dwKeySize] = '\0';
			result.push_back(std::string(key));
		}
		num ++;
	} while (dw == ERROR_SUCCESS);

	free ((char*)key);

	return result;
}

static std::string regQuery (HKEY hKey, const char *entry) {
	std::string result;

	DWORD dwSize = 0;
	DWORD dwType;
	DWORD dw;
	dw = RegQueryValueExA(hKey, entry, NULL, &dwType, (LPBYTE) NULL,
			&dwSize);
	if (dwSize > 0) {
		char *buffer = (char*) malloc (dwSize+1);
		RegQueryValueExA(hKey, entry, NULL, &dwType, (LPBYTE) buffer,
				&dwSize);
		if (dwType == REG_SZ || dwType == REG_EXPAND_SZ) {
			buffer [dwSize] = '\0';
			result = buffer;
		}
		free ((char*)buffer);
	}

	return result;

}


static HKEY regCreate (HKEY hKey, const char *entry) {

	HKEY hSubKey = NULL;
	RegCreateKeyA(hKey, entry, &hSubKey);

	return hSubKey;

}

static void regSet (HKEY hKey, const char *entry, const char *value) {
	RegSetValueExA (hKey, entry, NULL, REG_SZ, (LPBYTE)value, 1+ strlen(value));
}



static void connectPrinter (struct SEE_interpreter *interp, const char *printer) {
	HKEY hkDevices;
	HKEY hkPrinterPorts;
	HKEY hkConnections;


	if ( ERROR_SUCCESS != RegCreateKeyExA(HKEY_CURRENT_USER,
			"Software\\Microsoft\\Windows NT\\CurrentVersion\\Devices", 0, NULL, 0,
			KEY_READ|KEY_WRITE, NULL, &hkDevices, NULL)) {
		SEE_error_throw (interp, PRIVATE(interp)->NetworkError, "Error opening registry");
		return ;
	}
	if ( ERROR_SUCCESS != RegCreateKeyExA(HKEY_CURRENT_USER,
			"Software\\Microsoft\\Windows NT\\CurrentVersion\\PrinterPorts", 0, NULL, 0,
			KEY_READ|KEY_WRITE, NULL, &hkPrinterPorts, NULL)) {
		SEE_error_throw(interp, PRIVATE(interp)->NetworkError, "Error opening registry");
		return ;
	}
	if ( ERROR_SUCCESS != RegCreateKeyExA(HKEY_CURRENT_USER,
			"Printers\\Connections", 0, NULL, 0,
			KEY_READ|KEY_WRITE, NULL, &hkConnections, NULL)) {
		SEE_error_throw(interp, PRIVATE(interp)->NetworkError, "Error opening registry");
		return ;
	}


	std::string p = printer;
	do {
		size_t separator = p.find('\\');
		if (separator == std::string::npos) break;
		p.replace(separator, 1, 1, ',');
	} while (true);

	std::string server = printer;
	unsigned int separator = server.find_last_of('\\');
	if (separator != std::string::npos) {
		server = server.substr(0, separator);
	}
	separator = server.find_last_of('\\');
	if (separator != std::string::npos) {
		server = server.substr(separator+1);
	}

	HKEY hk = regCreate (hkConnections, p.c_str());
	regSet(hk, "Provider", "win32spl.dll");
	std::string slashServer = "\\\\";
	slashServer += server.c_str();
	regSet(hk, "Server", slashServer.c_str());

	std::string port = regQuery (hkDevices, "");
	if (port.length() == 0) {
		int num = 0;
		bool colision = false;
		char ach[7];
		do {
			sprintf (ach,"Ne%02d:", num++);
			std::vector<std::string> entries = regList(hkDevices);
			for (std::vector<std::string>::iterator it = entries.begin(); it != entries.end(); it++) {
				std::string value = regQuery(hkDevices, it->c_str());
				if (value.find(ach) != std::string::npos) {
					colision = true;
					break;
				}
			}
		} while (colision);
		port = "winspool,";
		port += ach;
		regSet(hkDevices, printer, port.c_str());
	}

	port += ",15,45";
	regSet(hkPrinterPorts, printer, port.c_str());

	RegCloseKey(hkDevices);
	RegCloseKey(hkPrinterPorts);
	RegCloseKey(hkConnections);

}

static void disconnectPrinter (struct SEE_interpreter *interp, const char *printer) {
	HKEY hkDevices;
	HKEY hkPrinterPorts;
	if ( ERROR_SUCCESS != RegCreateKeyExA(HKEY_CURRENT_USER,
			"Software\\Microsoft\\Windows NT\\CurrentVersion\\Devices", 0, NULL, 0,
			KEY_READ|KEY_WRITE, NULL, &hkDevices, NULL)) {
		SEE_error_throw (interp, PRIVATE(interp)->NetworkError, "Error opening registry");
		return ;
	}
	if ( ERROR_SUCCESS != RegCreateKeyExA(HKEY_CURRENT_USER,
			"Software\\Microsoft\\Windows NT\\CurrentVersion\\PrinterPorts", 0, NULL, 0,
			KEY_READ|KEY_WRITE, NULL, &hkPrinterPorts, NULL)) {
		SEE_error_throw(interp, PRIVATE(interp)->NetworkError, "Error opening registry");
		return ;
	}

	RegDeleteValueA(hkDevices, printer);
	RegDeleteValueA(hkPrinterPorts, printer);

	RegCloseKey(hkDevices);
	RegCloseKey(hkPrinterPorts);
}

static void disconnectAllPrinters (struct SEE_interpreter *interp) {
	HKEY hkDevices;
	HKEY hkPrinterPorts;
	HKEY hkPrinterConnections;
	if ( ERROR_SUCCESS != RegCreateKeyExA(HKEY_CURRENT_USER,
			"Software\\Microsoft\\Windows NT\\CurrentVersion\\Devices", 0, NULL, 0,
			KEY_READ|KEY_WRITE, NULL, &hkDevices, NULL)) {
		SEE_error_throw (interp, PRIVATE(interp)->NetworkError, "Error opening registry");
		return ;
	}
	if ( ERROR_SUCCESS != RegCreateKeyExA(HKEY_CURRENT_USER,
			"Software\\Microsoft\\Windows NT\\CurrentVersion\\PrinterPorts", 0, NULL, 0,
			KEY_READ|KEY_WRITE, NULL, &hkPrinterPorts, NULL)) {
		SEE_error_throw(interp, PRIVATE(interp)->NetworkError, "Error opening registry");
		return ;
	}
	if ( ERROR_SUCCESS != RegCreateKeyExA(HKEY_CURRENT_USER,
			"Printers\\Connections", 0, NULL, 0,
			KEY_READ|KEY_WRITE, NULL, &hkPrinterConnections, NULL)) {
		SEE_error_throw(interp, PRIVATE(interp)->NetworkError, "Error opening registry");
		return ;
	}
	std::vector<std::string> entries = regList(hkDevices);
	for (std::vector<std::string>::iterator it = entries.begin(); it != entries.end(); it++) {
		RegDeleteValueA(hkDevices, it->c_str());
	}
	entries = regList(hkPrinterPorts);
	for (std::vector<std::string>::iterator it = entries.begin(); it != entries.end(); it++) {
		RegDeleteValueA(hkPrinterPorts, it->c_str());
	}

	entries = regListKeys(hkPrinterConnections);
	for (std::vector<std::string>::iterator it = entries.begin(); it != entries.end(); it++) {
		RegDeleteKey(hkPrinterConnections, it->c_str());
	}

	RegCloseKey(hkDevices);
	RegCloseKey(hkPrinterPorts);
	RegCloseKey(hkPrinterConnections);
}

static std::string getErrorMessage() {
	LPSTR pstr;
	DWORD dw = GetLastError();
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, dw, 0, (LPSTR) & pstr, 0, NULL);
	std::string result = pstr;
	LocalFree(pstr);
	return result;
}

static void connectDrive (struct SEE_interpreter *interp, const wchar_t *local, const wchar_t*share,
		const wchar_t *user, const wchar_t *password) {


	NETRESOURCEW nr;
	nr.dwType = RESOURCETYPE_ANY;
	nr.lpLocalName = (wchar_t*) local;
	nr.lpRemoteName = (wchar_t*) share;
	nr.lpProvider = NULL;


	DWORD result = WNetAddConnection2W (
			&nr,
			(wchar_t*) user,
			(wchar_t*) password,
			CONNECT_INTERACTIVE);
	if (result != ERROR_SUCCESS) {
		std::string s = MZNC_wstrtostr(share);
		std::string error = getErrorMessage();
		SEE_string* errorS = SEE_CharsToString(interp, error.c_str());
		SEE_error_throw (interp, PRIVATE(interp)->NetworkError, "Error connecting drive %s: %S", s.c_str(), errorS);
	}
}

static void disconnectDrive (struct SEE_interpreter *interp, const wchar_t *local) {
	DWORD result = WNetCancelConnectionW(local, false);
	if (result != ERROR_SUCCESS) {
		std::string s = MZNC_wstrtostr(local);
		std::string error = getErrorMessage();
		SEE_string* errorS = SEE_CharsToString(interp, error.c_str());
		SEE_error_throw (interp, PRIVATE(interp)->NetworkError, "Error disconnecting drive %s : %S", s.c_str(), errorS);
	}
}
#else

/* Note that caller frees the returned buffer if necessary */
static bool parse_server(std::string &unc_name, std::string &ipaddress_string)
{
	char * share;
	struct hostent * host_entry;
	struct in_addr server_ipaddr;
	int rc,j;
	char temp[64];

	do {
		size_t firstSlash = unc_name.find ('/');
		if (firstSlash == std::string::npos)
			break;
		unc_name = unc_name.replace(firstSlash, firstSlash, 1, '\\');
	} while (true);

	if(strncmp(unc_name.c_str(),"\\\\",2) == 0) {
		size_t firstBSlash = unc_name.find('\\', 2);
		if (firstBSlash != std::string::npos) {
			std::string server = unc_name.substr(2, firstBSlash-2);
			host_entry = gethostbyname(server.c_str());
			if(host_entry == NULL) {
				return false;
			} else {
				memcpy (&server_ipaddr, host_entry->h_addr_list[0], sizeof server_ipaddr);
				const char*ipaddress = inet_ntoa(server_ipaddr);
				if(ipaddress == NULL) {
					return false;
				} else {
					ipaddress_string = ipaddress;
					return true;
				}

			}
		}
	}
	return false;
}


static std::string getLocalDir (const wchar_t* local) {
	if (local[0] == L'/')
		return MZNC_wstrtostr(local);

	std::string dir;
	struct passwd *pwd = getpwnam (MZNC_getUserName());
	if (pwd != NULL) {
		dir = pwd->pw_dir;
		dir += "/network";
		mkdir (dir.c_str(), 0700);
		chown (dir.c_str(), pwd->pw_uid, pwd->pw_gid);
		dir += "/"+MZNC_wstrtostr(local);
		mkdir (dir.c_str(), 0700);
		chown (dir.c_str(), pwd->pw_uid, pwd->pw_gid);
	}

	return dir;

}
static void connectDrive (struct SEE_interpreter *interp, const wchar_t *local, const wchar_t*share,
		const wchar_t *user, const wchar_t *password) {

	std::string dir = getLocalDir (local);
	std::string unc = MZNC_wstrtostr(share);
	std::string ip;

	if (!parse_server(unc, ip)) {
		SEE_error_throw (interp, PRIVATE(interp)->NetworkError, "Cannot resolve server ip");
		return;
	}
	struct passwd *pwd = getpwnam (MZNC_getUserName());
	if (pwd == NULL) {
		SEE_error_throw (interp, PRIVATE(interp)->NetworkError, "Cannot get user uid/gid");
		return;
	}

	char opts[4096];
	sprintf(opts,"unc=%s,uid=%d,gid=%d,file_mode=0700,dir_mode=0700,ver=1,iocharset=utf8",
			unc.c_str(),pwd->pw_uid, pwd->pw_gid);
	std::string opts_str= opts;
	opts_str += ",username=";
	SecretStore s (MZNC_getUserName()) ;
	if (user != NULL) {
		opts_str += MZNC_wstrtostr(user).c_str();
	} else {
		opts_str += MZNC_wstrtostr(s.getSecret(L"user"));
	}
	opts_str += ",pass=";
	if (password != NULL) {
		opts_str += MZNC_wstrtostr(password).c_str();
	} else {
		opts_str += MZNC_wstrtostr(s.getSecret(L"password"));
	}
	opts_str += ",unc=";
	opts_str += unc;
	opts_str += ",ip=";
	opts_str += ip;

	if (mount(unc.c_str(), dir.c_str(), "cifs", MS_MANDLOCK | MS_MGC_VAL, opts_str.c_str()) == -1) {
		SEE_error_throw (interp, PRIVATE(interp)->NetworkError, "Cannot mount directory: %d", errno);
	}
}


static void disconnectDrive (struct SEE_interpreter *interp, const wchar_t *local) {

	std::string dir = getLocalDir (local);
	umount2 (dir.c_str(), MNT_DETACH);
}
#endif

/**
 * NetworkResource.connectPrinter
 */

static void NetworkResource_connectPrinter(struct SEE_interpreter *interp,
			struct SEE_object *self, struct SEE_object *thisobj, int argc,
			struct SEE_value **argv, struct SEE_value *res) {

#ifdef WIN32
	SEE_value p1;
	SEE_ToString(interp, argv[0], &p1);
	std::string printer = SEE_StringToChars(interp, p1.u.string);
	connectPrinter(interp, printer.c_str());
#endif
	SEE_SET_UNDEFINED(res);
}

/**
 * NetworkResource.disconnectPrinter
 */

static void NetworkResource_disconnectPrinter(struct SEE_interpreter *interp,
			struct SEE_object *self, struct SEE_object *thisobj, int argc,
			struct SEE_value **argv, struct SEE_value *res) {

#ifdef WIN32
	SEE_value p1;
	SEE_ToString(interp, argv[0], &p1);
	std::string printer = SEE_StringToChars(interp, p1.u.string);
	disconnectPrinter(interp, printer.c_str());
#endif
	SEE_SET_UNDEFINED(res);
}



/**
 * NetworkResource.disconnectAllPrinters
 */

static void NetworkResource_disconnectAllPrinters(struct SEE_interpreter *interp,
			struct SEE_object *self, struct SEE_object *thisobj, int argc,
			struct SEE_value **argv, struct SEE_value *res) {

#ifdef WIN32
	disconnectAllPrinters(interp);
#endif
	SEE_SET_UNDEFINED(res);
}


/**
 * NetworkResource.connectDrive
 */

static void NetworkResource_connectDrive(struct SEE_interpreter *interp,
			struct SEE_object *self, struct SEE_object *thisobj, int argc,
			struct SEE_value **argv, struct SEE_value *res) {

	SEE_value p;
	SEE_ToString(interp, argv[0], &p);
	std::wstring local = SEE_StringToWChars(interp, p.u.string);

	SEE_ToString(interp, argv[1], &p);
	std::wstring share = SEE_StringToWChars(interp, p.u.string);

	std::wstring user;
	const wchar_t *szUser = NULL;
	if (argc > 2) {
		SEE_ToString(interp, argv[2], &p);
		user = SEE_StringToWChars(interp, p.u.string);
		szUser = user.c_str();
	}

	std::wstring pass;
	const wchar_t *szPassword = NULL;
	if (argc > 3) {
		SEE_ToString(interp, argv[3], &p);
		pass = SEE_StringToWChars(interp, p.u.string);
		szPassword = pass.c_str();
	}

	connectDrive(interp, local.c_str(), share.c_str(), szUser, szPassword);
	SEE_SET_UNDEFINED(res);
}



/**
 * NetworkResource.disconnectDrive
 */

static void NetworkResource_disconnectDrive(struct SEE_interpreter *interp,
			struct SEE_object *self, struct SEE_object *thisobj, int argc,
			struct SEE_value **argv, struct SEE_value *res) {

	SEE_value p;
	SEE_ToString(interp, argv[0], &p);
	std::wstring local = SEE_StringToWChars(interp, p.u.string);

	disconnectDrive(interp, local.c_str());
	SEE_SET_UNDEFINED(res);
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
static int NetworkResource_mod_init() {
	s_NetworkResource = SEE_intern_global("NetworkResource");
	s_connectDrive = SEE_intern_global("connectDrive");
	s_connectPrinter = SEE_intern_global("connectPrinter");
	s_disconnectAllPrinters = SEE_intern_global("disconnectAllPrinters");
	s_disconnectDrive = SEE_intern_global("disconnectDrive");
	s_disconnectPrinter = SEE_intern_global("disconnectPrinter");
	s_NetworkError = SEE_intern_global("NetworkError");
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
static void NetworkResource_alloc(struct SEE_interpreter *interp) {
	SEE_MODULE_PRIVATE(interp, &NetworkResource_module) =
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

static void NetworkResource_init(struct SEE_interpreter *interp) {
	struct SEE_native *networkResource_object;
	struct SEE_object *NetworkError;
	struct SEE_value v;

	/* Convenience macro for adding properties to File */
#define PUTOBJ(parent, name, obj)                                       \
        SEE_SET_OBJECT(&v, obj);                                        \
        SEE_OBJECT_PUT(interp, parent, s_##name, &v, SEE_ATTR_DEFAULT);

	/* Convenience macro for adding functions to File.prototype */
#define PUTFUNC(obj, name, len)                                         \
        SEE_SET_OBJECT(&v, SEE_cfunction_make(interp,  NetworkResource_##name,\
                s_##name, len));                                       \
        SEE_OBJECT_PUT(interp, obj, s_##name, &v, SEE_ATTR_DEFAULT);

	/* Create the File.prototype object (cf. newfile()) */
	networkResource_object = SEE_NEW(interp, struct SEE_native);
	SEE_native_init(networkResource_object, interp,
			&networkResource_inst_class, interp->Object_prototype);

	PUTFUNC(&networkResource_object->object, connectDrive, 2);
	PUTFUNC(&networkResource_object->object, disconnectDrive, 1);
	PUTFUNC(&networkResource_object->object, connectPrinter, 1);
	PUTFUNC(&networkResource_object->object, disconnectPrinter, 1);
	PUTFUNC(&networkResource_object->object, disconnectAllPrinters, 0);

	/* Create the File.FileError error object for I/O exceptions */
	NetworkError = SEE_Error_make(interp, s_NetworkError);
	PUTOBJ(&networkResource_object->object, NetworkError, NetworkError);
	PRIVATE(interp)->NetworkError = NetworkError;

	PUTOBJ(interp->Global, NetworkResource, &networkResource_object->object);


	return;

}
#undef PUTFUNC
#undef PUTOBJ
