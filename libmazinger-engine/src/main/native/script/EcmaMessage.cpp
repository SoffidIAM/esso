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
#include <ScriptDialog.h>

class DefaultScriptDialog: public ScriptDialog {
public:
	virtual ~DefaultScriptDialog();
	DefaultScriptDialog();
	virtual void alert(const char *msg);
	virtual void progressMessage (const char *msg) ;
	virtual void cancelProgressMessage ();
	virtual  std::wstring selectAccount(std::vector<std::wstring>accounts,
			std::vector<std::wstring>accountDescriptions) ;
};


///////////////////// Dialog suport
static ScriptDialog *currentDialog = NULL;


ScriptDialog::ScriptDialog () {

}

ScriptDialog::~ScriptDialog () {

}

bool ScriptDialog::isScriptDialogSet()
{
	return currentDialog != NULL;
}

ScriptDialog * ScriptDialog::getScriptDialog () {
	if (currentDialog == NULL)
		return new DefaultScriptDialog();
	else
		return currentDialog;
}

void ScriptDialog::setScriptDialog (ScriptDialog *d) {
	currentDialog = d;
}

DefaultScriptDialog::DefaultScriptDialog () {

}

DefaultScriptDialog::~DefaultScriptDialog () {

}


/* Prototypes */
static int Message_mod_init(void);
static void Message_alloc(struct SEE_interpreter *);
static void Message_init(struct SEE_interpreter *);

struct SEE_module Message_module = {
	SEE_MODULE_MAGIC, /* magic */
	"Message", /* name */
	"1.0", /* version */
	0, /* index (set by SEE) */
	Message_mod_init, /* mod_init */
	Message_alloc, /* alloc */
	Message_init /* init */
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
	struct SEE_object *ServerError; /* Message.NetworkError */
	struct SEE_object *Message_prototype;
};

#define PRIVATE(interp)  \
        ((struct module_private *)SEE_MODULE_PRIVATE(interp, &Message_module))

/*
 * To make string usage more efficient, we globally intern some common
 * strings and provide a STR() macro to access them.
 * Internalised strings are guaranteed to have unique pointers,
 * which means you can use '==' instead of 'strcmp()' to compare names.
 * The pointers must be allocated during mod_init() because the global
 * intern table is locked as soon as an interpreter instance is created.
 */
static struct SEE_string *s_alert;
static struct SEE_string *s_progress;
static struct SEE_string *s_cancelProgress;


#ifdef WIN32
#include <windows.h>

static HWND hwndProgress = NULL;
static HWND hwndProgressMsg = NULL;
static HANDLE hProgressThread = NULL;
static std::string progressMessage;


static LRESULT CALLBACK ProgressWndProc(
    HWND hwnd,        // handle to window
    UINT uMsg,        // message identifier
    WPARAM wParam,    // first message parameter
    LPARAM lParam)    // second message parameter
{

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
    return 0;
}


static BOOL createProgressWndClass ()
{
    WNDCLASSEX wcx;

    // Fill in the window class structure with parameters
    // that describe the main window.

    wcx.cbSize = sizeof(wcx);          // size of structure
    wcx.style = CS_HREDRAW |
        CS_VREDRAW;                    // redraw if size changes
    wcx.lpfnWndProc = ProgressWndProc;     // points to window procedure
    wcx.cbClsExtra = 0;                // no extra class memory
    wcx.cbWndExtra = 0;                // no extra window memory
    wcx.hInstance = GetModuleHandleA(NULL);         // handle to instance
    wcx.hIcon = LoadIcon(NULL, IDI_INFORMATION);              // predefined app. icon
    wcx.hCursor = LoadCursor(NULL, IDC_ARROW);                    // predefined arrow
    wcx.hbrBackground = (HBRUSH) GetStockObject(HOLLOW_BRUSH);                  // white background brush
    wcx.lpszMenuName =  NULL;    // name of menu resource
    wcx.lpszClassName = "MazingerProgressWClass";  // name of window class
    wcx.hIconSm = NULL;
    // Register the window class.

    return RegisterClassEx(&wcx);
}

static DWORD WINAPI progressMessageLoop (LPVOID param) {

    DWORD units = GetDialogBaseUnits();
    DWORD xunit = LOWORD(units);
    DWORD yunit = HIWORD(units);

    DWORD x = GetSystemMetrics(SM_CXFULLSCREEN);
    DWORD y = GetSystemMetrics(SM_CYFULLSCREEN);
    DWORD cx = xunit * 50;
    DWORD cy = yunit * 5;
    x = ( x - cx ) / 2;
    y = ( y - cy ) / 2;
    // Create the main window.

    HINSTANCE hInstance = GetModuleHandleA(NULL);

    createProgressWndClass ();

	hwndProgress = CreateWindow(
        "MazingerProgressWClass",        // name of window class
        "Progress",            // title-bar string
        WS_POPUP, // top-level window
        x,       // default horizontal position
        y,       // default vertical position
        cx,       // default width
        cy,       // default height
        (HWND) NULL,         // no owner window
        (HMENU) NULL,        // use class menu
        hInstance,           // handle to application instance
        (LPVOID) NULL);      // no window-creation data


	CreateWindow ("STATIC", "",
    		WS_CHILD|WS_VISIBLE,
    		0, 0,
    		cx, cy,
    		hwndProgress,
    		NULL,
    		hInstance,
    		NULL);



    hwndProgressMsg = CreateWindow ("STATIC", progressMessage.c_str(),
    		WS_CHILD|SS_CENTER|SS_NOPREFIX|WS_VISIBLE,
    		0, yunit*2,
    		cx, yunit,
    		hwndProgress,
    		NULL,
    		hInstance,
    		NULL);

    // Show the window and send a WM_PAINT message to the window
    // procedure.


	ShowWindow(hwndProgress, SW_SHOWNORMAL);
    UpdateWindow(hwndProgress);


	MSG msg;
	while ( GetMessageA(&msg, (HWND) NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

	if (hProgressThread == GetCurrentThread())
		hProgressThread = NULL;

	return 0;

}


static void disableProgressWindow () {
	hwndProgressMsg = NULL;
	if (hProgressThread != NULL) {
		hProgressThread = NULL;
	}
	if (hwndProgress != NULL) {
		ShowWindow(hwndProgress, SW_HIDE);
		DestroyWindow(hwndProgress);
		hwndProgress = NULL;
	}

}

static void setProgressMessage (const char *lpszMessage) {
	progressMessage = lpszMessage;
	if (hProgressThread == NULL) {
		hProgressThread = CreateThread(NULL, 0, progressMessageLoop, NULL, 0, NULL);
	}
	if (hwndProgressMsg != NULL) {
		SetWindowTextA(hwndProgressMsg, lpszMessage);
	}
}


static int iSelectedIndex = -1;
static std::vector<std::wstring> *s_accounts;
static std::vector<std::wstring> *s_accountDescriptions;

#include <engine/resources/engine-resource.h>
//////////////////////////////////////////////////////////
static INT_PTR CALLBACK selectAccountDialogProc(
    HWND hwndDlg,	// handle to dialog box
    UINT uMsg,	// message
    WPARAM wParam,	// first message parameter
    LPARAM lParam 	// second message parameter
   )
{
	switch (uMsg)
	{
	case WM_COMMAND:
	{
		WORD wID = LOWORD(wParam);         // item, control, or accelerator identifier
		if (wID == IDOK)
		{
			HWND hwndList = GetDlgItem (hwndDlg, IDC_LIST1);
			int selected = SendMessage(hwndList, LB_GETCURSEL,0, 0);
			if (selected == LB_ERR)
			{
				MessageBox (hwndDlg, "An account must be selected",
						"Warning",
						MB_OK|MB_ICONEXCLAMATION);
			}
			else
			{
				iSelectedIndex = selected;
				EndDialog (hwndDlg, 0);
			}
		}
		if (wID == IDCANCEL) {
			iSelectedIndex = -1;
			EndDialog (hwndDlg, 1);
		}
		return 0;
	}
	case WM_INITDIALOG:
	{
		HWND hwndList = GetDlgItem (hwndDlg, IDC_LIST1);

		for (std::vector<std::wstring>::iterator it = s_accounts->begin(),
				it2 = s_accountDescriptions->begin();
				it != s_accounts->end();
				it ++, it2++)
		{
			std::wstring account = *it;
			std::wstring &description = *it2;
			if (! description.empty())
			{
				account += L" - ";
				account += description;
			}
			SendMessageW(hwndList, LB_ADDSTRING, 0, (LPARAM)account.c_str());
		}
		SendMessage(hwndList, LB_SETCURSEL, 0, 0);

		SetWindowPos (hwndDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW );
		return 0;
	}
	case WM_NEXTDLGCTL:
	{
		ShowWindow(hwndDlg, SW_SHOWNORMAL);
		return 0;
	}
	default:
		return 0;
	}
}




void DefaultScriptDialog::alert (const char *msg) {
	MessageBoxW(NULL, MZNC_strtowstr(msg).c_str(), L"Warning", MB_OK|MB_ICONINFORMATION);
}

void DefaultScriptDialog::progressMessage(const char *msg) {
	setProgressMessage(msg);
}

void DefaultScriptDialog::cancelProgressMessage() {
	disableProgressWindow();
}

std::wstring DefaultScriptDialog::selectAccount(
		std::vector<std::wstring> accounts,
		std::vector<std::wstring> accountDescriptions) {
	s_accounts = &accounts;
	s_accountDescriptions = &accountDescriptions;
	int result = DialogBox (hMazingerInstance, MAKEINTRESOURCE (IDD_SELACCOUNT),
			NULL,  selectAccountDialogProc);
	MZNSendDebugMessage("Result = %d", result);
	if (result == 0)
	{
		return accounts[iSelectedIndex];
	}
	else
		return std::wstring();
}
#else

std::wstring DefaultScriptDialog::selectAccount(
		std::vector<std::wstring> accounts,
		std::vector<std::wstring> accountDescriptions) {
	return std::wstring();
}

void DefaultScriptDialog::alert (const char *msg) {
	fprintf (stderr, "%s\n", msg);
}

void DefaultScriptDialog::progressMessage(const char *msg) {
	fprintf (stderr, "%s\r", msg);
}

void DefaultScriptDialog::cancelProgressMessage() {
	fprintf (stderr, "\n");
}
#endif


static void Message_alert(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {

	SEE_SET_UNDEFINED(res);
	if (argc != 1) {
		SEE_error_throw(interp, interp->RangeError, "missing argument");
	} else {
		SEE_value v;
		SEE_ToString(interp, argv[0], &v);
		std::string msg = SEE_StringToChars(interp, v.u.string);
		ScriptDialog::getScriptDialog()->alert(msg.c_str());
	}
}



static void Message_progress(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {

	SEE_SET_UNDEFINED(res);
	if (argc != 1) {
		SEE_error_throw(interp, interp->RangeError, "missing argument");
	} else {
		SEE_value v;
		SEE_ToString(interp, argv[0], &v);
		std::string msg = SEE_StringToChars(interp, v.u.string);
		ScriptDialog::getScriptDialog()->progressMessage(msg.c_str());
	}
}



static void Message_cancelProgress(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj, int argc,
		struct SEE_value **argv, struct SEE_value *res) {
	ScriptDialog::getScriptDialog()->cancelProgressMessage();

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
static int Message_mod_init() {
	s_alert = SEE_intern_global("alert");
	s_progress = SEE_intern_global("progress");
	s_cancelProgress = SEE_intern_global("cancelProgress");
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
static void Message_alloc(struct SEE_interpreter *interp) {
	SEE_MODULE_PRIVATE(interp, &Message_module) =
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

static void Message_init(struct SEE_interpreter *interp) {
	struct SEE_value v;

	/* Convenience macro for adding properties to File */
#define PUTOBJ(parent, name, obj)                                       \
        SEE_SET_OBJECT(&v, obj);                                        \
        SEE_OBJECT_PUT(interp, parent, s_##name, &v, SEE_ATTR_DEFAULT);

	/* Convenience macro for adding functions to File.prototype */
#define PUTFUNC(obj, name, len)                                         \
        SEE_SET_OBJECT(&v, SEE_cfunction_make(interp,  Message_##name,\
                s_##name, len));                                       \
        SEE_OBJECT_PUT(interp, obj, s_##name, &v, SEE_ATTR_DEFAULT);

	/* Create the global functions  */
	PUTFUNC(interp->Global, alert, 1);
	PUTFUNC(interp->Global, progress, 1);
	PUTFUNC(interp->Global, cancelProgress, 0);

	return;

}

#undef PUTFUNC
#undef PUTOBJ

