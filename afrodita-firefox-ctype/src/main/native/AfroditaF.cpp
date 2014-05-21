/*
 * MazingerFF.cpp
 *
 *  Created on: 22/11/2010
 *      Author: u07286
 */

#include "AfroditaF.h"
#include "FFWebApplication.h"
#include <MazingerInternal.h>

#ifdef WIN32
#include <windows.h>
#else
#include <string.h>
#endif

#include <MZNcompat.h>

AfroditaHandler AfroditaHandler::handler;

AfroditaHandler::AfroditaHandler() {
	getUrlHandler = NULL;
}

extern "C" void AFRsetHandler (const char *id, void * handler) {
	if (strcmp (id, "GetUrl") == 0)
		AfroditaHandler::handler.getUrlHandler = (GetUrlHandler) handler;
	else if (strcmp (id, "GetTitle") == 0)
		AfroditaHandler::handler.getTitleHandler = (GetTitleHandler) handler;
	else if (strcmp (id, "GetDocumentElement") == 0)
		AfroditaHandler::handler.getDocumentElementHandler = (GetDocumentElementHandler) handler;
	else if (strcmp (id, "GetElementsByTagName") == 0)
		AfroditaHandler::handler.getElementsByTagNameHandler = (GetElementsByTagNameHandler) handler;
	else if (strcmp (id, "GetDomain") == 0)
		AfroditaHandler::handler.getDomainHandler = (GetDomainHandler) handler;
	else if (strcmp (id, "GetCookie" ) == 0)
		AfroditaHandler::handler.getCookieHandler = (GetCookieHandler) handler;
	else if (strcmp (id, "GetElementById" ) == 0)
		AfroditaHandler::handler.getElementByIdHandler = (GetElementByIdHandler) handler;
	else if (strcmp (id, "GetImages" ) == 0)
		AfroditaHandler::handler.getImagesHandler = (GetImagesHandler) handler;
	else if (strcmp (id, "GetLinks" ) == 0)
		AfroditaHandler::handler.getLinksHandler = (GetLinksHandler) handler;
	else if (strcmp (id, "GetAnchors" ) == 0)
		AfroditaHandler::handler.getAnchorsHandler = (GetAnchorsHandler) handler;
	else if (strcmp (id, "GetForms" ) == 0)
		AfroditaHandler::handler.getFormsHandler = (GetFormsHandler) handler;
	else if (strcmp (id, "Write" ) == 0)
			AfroditaHandler::handler.writeHandler = (WriteHandler) handler;
	else if (strcmp (id, "WriteLn" ) == 0)
			AfroditaHandler::handler.writeLnHandler = (WriteLnHandler) handler;
	else if (strcmp (id, "GetAttribute" ) == 0)
			AfroditaHandler::handler.getAttributeHandler = (GetAttributeHandler) handler;
	else if (strcmp (id, "SetAttribute" ) == 0)
			AfroditaHandler::handler.setAttributeHandler = (SetAttributeHandler) handler;
	else if (strcmp (id, "GetParent" ) == 0)
			AfroditaHandler::handler.getParentHandler = (GetParentHandler) handler;
	else if (strcmp (id, "GetChildren" ) == 0)
			AfroditaHandler::handler.getChildrenHandler = (GetChildrenHandler) handler;
	else if (strcmp (id, "GetTagName" ) == 0)
			AfroditaHandler::handler.getTagNameHandler = (GetTagNameHandler) handler;
	else if (strcmp (id, "Click" ) == 0)
			AfroditaHandler::handler.clickHandler = (ClickHandler) handler;
	else if (strcmp (id, "Blur" ) == 0)
			AfroditaHandler::handler.blurHandler = (BlurHandler) handler;
	else if (strcmp (id, "Focus" ) == 0)
			AfroditaHandler::handler.focusHandler = (FocusHandler) handler;

	else
	{
#ifdef WIN32
		MessageBox (NULL, id, "Wrong Handler", MB_OK);
#endif
	}
}


extern "C" void AFRevaluate (long  id) {

	//MZNC_waitMutex();
	FFWebApplication app(id);

	MZNWebMatch(&app);

	//MZNC_endMutex();
}

extern "C" void Test (const char *id) {
//	MessageBox (NULL, id, "Test", MB_OK);
}


#ifdef WIN32
extern "C" BOOL __stdcall DllMain(HINSTANCE hinstDLL, DWORD dwReason,
		LPVOID lpvReserved) {

	if (dwReason == DLL_PROCESS_ATTACH) {
		hMazingerInstance = hinstDLL;
	}
	return TRUE;
}
#endif
