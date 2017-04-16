#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <MazingerInternal.h>
#include "../core/ComponentSpec.h"
#include <NativeComponent.h>
#include <ComponentMatcher.h>
#include <WebMatcher.h>
#include <HllMatcher.h>
#include "../web/WebComponentSpec.h"
#include "../web/WebApplicationSpec.h"
#include <vector>
#include <see/see.h>
#include "ecma.h"
#include <ScriptDialog.h>
#include <HllApplication.h>

/** Mazinger module **/
extern void createWindowObject (ComponentSpec *spec, SEE_interpreter *interp, SEE_string *name);
extern void createElementObject (WebComponentSpec *spec, SEE_interpreter *interp, SEE_string *name);
extern void createHllObject (HllApplication *spec, SEE_interpreter *interp);
extern struct SEE_string* getFocusString();
extern struct SEE_module Mazinger_module ;
extern void updateEnvironment (struct SEE_interpreter *interp);


/** Extern modules **/
extern struct SEE_module File_module;
extern struct SEE_module Hll_module;
extern struct SEE_module Web_module;
extern struct SEE_module Registry_module;
extern struct SEE_module SystemInfo_module;
extern struct SEE_module MailService_module;
extern struct SEE_module NetworkResource_module;
extern struct SEE_module ServerInfo_module;
extern struct SEE_module Message_module;



bool initialized = false;

struct SEE_interpreter interp_storage;

struct SEE_interpreter_state *saved_state;

/**
 * Creates a native string from a SEE_string object;
 */

std::string SEE_StringToChars (SEE_interpreter *i, SEE_string  *string)
{
	int len = string->length;
	wchar_t * lpstr = (wchar_t*) malloc (sizeof (wchar_t) * (len+1));
	for (int i = 0; i < len; i++)
		lpstr[i] = string->data[i];
	lpstr[len] = L'\0';

	std::string result = MZNC_wstrtostr(lpstr);
	free (lpstr);
	return result;
}

/**
 * Creates a native string from a SEE_string object;
 */

std::wstring SEE_StringToWChars (SEE_interpreter *i, SEE_string  *string)
{
	int len = string->length;
	wchar_t * lpstr = (wchar_t*) malloc (sizeof (wchar_t) * (len+1));
	for (int i = 0; i < len; i++)
		lpstr[i] = string->data[i];
	lpstr[len] = L'\0';
	std::wstring result (lpstr);
	free (lpstr);
	return result;
}

SEE_string *SEE_CharsToString (SEE_interpreter *i, const char* str)
{
	std::wstring wsz = MZNC_strtowstr(str);
	int cChars = wsz.size();

	if (cChars <= 0)
	{
		SEE_string *seestr = SEE_string_new(i, 0);
		return seestr;
	} else {
		SEE_string *seestr = SEE_string_new(i, (cChars) * sizeof (SEE_char_t));
		for (int i = 0; i < cChars; i++ )
			seestr->data[i] = wsz[i];
		seestr->length = cChars;
		return seestr;
	}
}

SEE_string *SEE_WCharsToString (SEE_interpreter *i, const wchar_t* str)
{

	int cChars = wcslen(str);
	if (cChars <= 0)
	{
		SEE_string *seestr = SEE_string_new(i, 0);
		return seestr;
	} else {
		SEE_string *seestr = SEE_string_new(i, (cChars) * sizeof (SEE_char_t));
		for (int i = 0; i < cChars; i++ )
			seestr->data[i] = str[i];
		seestr->length = cChars;
		return seestr;
	}
}


std::string SEE_StringToUTF8 (SEE_interpreter *i, SEE_string  *string)
{
	int len = string->length;
	wchar_t * lpstr = (wchar_t*) malloc ( sizeof (wchar_t) * (len+1));
	for (int i = 0; i < len; i++)
		lpstr[i] = string->data[i];
	lpstr[len] = L'\0';
	std::string result = MZNC_wstrtoutf8(lpstr);
	free (lpstr);
	return result;
}

SEE_string *SEE_UTF8ToString (SEE_interpreter *i, const char* str)
{

	std::wstring wsz = MZNC_utf8towstr(str);
	int cChars = wsz.size();
	if (cChars <= 0)
	{
		SEE_string *seestr = SEE_string_new(i, 0);
		return seestr;
	} else {
		SEE_string *seestr = SEE_string_new(i, (cChars) * sizeof (SEE_char_t));
		for (int i = 0; i < cChars; i++ )
			seestr->data[i] = wsz[i];
		seestr->length = cChars;
		return seestr;
	}
}

void MySeeAbortProc (struct SEE_interpreter *,
	const char *message) {
	MZNSendDebugMessage(message);
#ifdef WIN32
	ExitProcess (-1);
#else
	exit(-1);
#endif
}

void initializeSEE()
{
    if (!initialized) {
		SEE_init();
		SEE_system.abort = MySeeAbortProc ;
		SEE_module_add(&File_module);
		SEE_module_add(&Web_module);
		SEE_module_add(&Hll_module);
		SEE_module_add(&Mazinger_module);
		SEE_module_add(&Registry_module);
		SEE_module_add(&SystemInfo_module);
		SEE_module_add(&MailService_module);
		SEE_module_add(&NetworkResource_module);
		SEE_module_add(&ServerInfo_module);
		SEE_module_add(&Message_module);
		SEE_interpreter_init_compat(&interp_storage, SEE_COMPAT_JS15);
		initialized = true;
	}
}

static bool MZNEvaluateJS_internal(const char *script, std::string& msg)
{

	bool success;
	initializeSEE();
    struct SEE_input *input = SEE_input_utf8(&interp_storage, script);
    SEE_try_context try_ctxt;
    struct SEE_value result;
    SEE_TRY(&
			interp_storage, try_ctxt) {
		/* Call the program evaluator */
		SEE_Global_eval(&interp_storage, input, &result);
	}
    SEE_INPUT_CLOSE(input);
    if (SEE_CAUGHT(try_ctxt)) {
		SEE_value v;
		SEE_ToString(&interp_storage, (SEE_value*)&try_ctxt.thrown, &v);
		SEE_string *str = v.u.string;
		msg = SEE_StringToChars(&interp_storage, str);
		std::wstring wstr = SEE_StringToWChars(&interp_storage, str);

		MZNSendDebugMessageW(L"\n**\n** Error\n**");
		MZNSendDebugMessageW(L"%ls", wstr.c_str());
		if (try_ctxt.throw_file != NULL)
		{
			char lineNumber[20];
			sprintf (lineNumber, "%d",  try_ctxt.throw_line);
			msg += "\nAt  ";
			msg += try_ctxt.throw_file;
			msg += ":";
			msg += lineNumber;

			MZNSendDebugMessage("At  %s:%d\n", try_ctxt.throw_file, try_ctxt.throw_line);
		}
		success = false;
	} else {
		success = true;
	}
	SEE_gcollect(&interp_storage);
	ScriptDialog::getScriptDialog()->cancelProgressMessage();


	return success;
}


void MZNEvaluateJSMatch(ComponentMatcher &matcher, const char *script) {
	if (MZNC_waitMutex2())
	{
		std::string msg;

		initializeSEE();
		std::vector<struct SEE_string*> nomsToDelete;
		for ( std::vector<ComponentSpec*>::iterator it=matcher.getAliasedComponents().begin();
				it != matcher.getAliasedComponents().end();
				it++)
		{
			const char* nom = (*it)->szId;
			SEE_string *str = SEE_UTF8ToString(&interp_storage, nom);
			str = SEE_intern(&interp_storage, str);
			nomsToDelete.push_back(str);
			createWindowObject ( *it, &interp_storage, str);
		}
		if (matcher.getFocusComponent() != NULL)
			createWindowObject (matcher.getFocusComponent(), &interp_storage, getFocusString());

		MZNEvaluateJS_internal(script, msg);

		for ( std::vector<struct SEE_string*>::iterator it=nomsToDelete.begin();
				it != nomsToDelete.end();
				it++)
		{
			SEE_OBJECT_DELETE(&interp_storage, interp_storage.Global, *it);
		}
		SEE_OBJECT_DELETE(&interp_storage, interp_storage.Global, getFocusString());

		SEE_gcollect(&interp_storage);
		ScriptDialog::getScriptDialog()->cancelProgressMessage();
		MZNC_endMutex2();
	}
}

void createWebComponents (std::vector<WebComponentSpec*> vector, std::vector<struct SEE_string*> nomsToDelete)
{
	for (std::vector<WebComponentSpec*>::iterator it=vector.begin();
			it != vector.end();
			it ++)
	{
		if ((*it)->getMatched() != NULL && (*it)->szRefAs != NULL)
		{
			const char* nom = (*it)->szRefAs;
			SEE_string *str = SEE_UTF8ToString(&interp_storage, nom);
			str = SEE_intern(&interp_storage, str);
			nomsToDelete.push_back(str);

			createElementObject((*it), &interp_storage, str);

		}
		createWebComponents((*it)->m_children, nomsToDelete);
	}


}

void createHllComponents (HllApplication *pApp)
{
	createHllObject(pApp, &interp_storage);

}

void MZNEvaluateJSMatch(WebMatcher &matcher, const char *script) {

	if (MZNC_waitMutex2())
	{
		std::string msg;
		initializeSEE();

		std::vector<struct SEE_string*> nomsToDelete;

		createWebComponents(matcher.getWebAppSpec()->m_components,nomsToDelete);

		createDocumentObject ( matcher.getWebApp(), &interp_storage);

		MZNEvaluateJS_internal(script, msg);


		for ( std::vector<struct SEE_string*>::iterator it=nomsToDelete.begin();
				it != nomsToDelete.end();
				it++)
		{
			SEE_OBJECT_DELETE(&interp_storage, interp_storage.Global, *it);
		}
		SEE_OBJECT_DELETE(&interp_storage, interp_storage.Global, getFocusString());

		SEE_gcollect(&interp_storage);
		ScriptDialog::getScriptDialog()->cancelProgressMessage();
		MZNC_endMutex2();
	}
}

void MZNEvaluateJSMatch(HllMatcher &matcher, const char *script) {

	if (MZNC_waitMutex2())
	{
		std::string msg;

		initializeSEE();

		std::vector<struct SEE_string*> nomsToDelete;

		createHllComponents(matcher.getHllApplication());

		MZNEvaluateJS_internal(script, msg);

		for ( std::vector<struct SEE_string*>::iterator it=nomsToDelete.begin();
				it != nomsToDelete.end();
				it++)
		{
			SEE_OBJECT_DELETE(&interp_storage, interp_storage.Global, *it);
		}
		SEE_OBJECT_DELETE(&interp_storage, interp_storage.Global, getFocusString());

		SEE_gcollect(&interp_storage);
		ScriptDialog::getScriptDialog()->cancelProgressMessage();
		MZNC_endMutex2();
	}
}



bool MZNEvaluateJS(const char *script, std::string& msg)
{
	bool result = false;
	if (MZNC_waitMutex2())
	{
		result = MZNEvaluateJS_internal(script, msg);
		MZNC_endMutex2();
	}
	return result;
}

bool MZNEvaluateJS(const char *script) {
	std::string msg;
	return MZNEvaluateJS(script, msg);
}
