/*
 * MazingerFF.h
 *
 *  Created on: 22/11/2010
 *      Author: u07286
 */

#ifndef MAZINGERFF_H_
#define MAZINGERFF_H_

#include <MazingerInternal.h>


typedef const char* (*GetUrlHandler) (long  docid);
typedef const char* (*GetTitleHandler) (long  docid);
typedef const char* (*GetDomainHandler) (long  docid);
typedef long (*GetDocumentElementHandler) (long  docid);
typedef char* (*GetCookieHandler) (long  docid);
typedef long* (*GetElementsByTagNameHandler) (long  docid, const char *name);
typedef long (*GetElementByIdHandler) (long  docid, const char *id);
typedef long* (*GetImagesHandler) (long  docid);
typedef long* (*GetLinksHandler) (long  docid);
typedef long* (*GetAnchorsHandler) (long  docid);
typedef long* (*GetFormsHandler) (long  docid);
typedef void (*WriteHandler) (long  docid, const char *str);
typedef void (*WriteLnHandler) (long  docid, const char *str);

typedef const char *(*GetAttributeHandler) (long  docid, long elementid, const char *attribute);
typedef const long *(*GetChildrenHandler) (long  docid, long elementid);
typedef const char *(*GetTagNameHandler) (long  docid, long elementid);
typedef void (*SetAttributeHandler) (long  docid, long elementid, const char* atribute, const char *value);
typedef long (*GetParentHandler) (long  docid, long elementid);

typedef void (*ClickHandler) (long  docid, long elementid);
typedef void (*FocusHandler) (long  docid, long elementid);
typedef void (*BlurHandler) (long  docid, long elementid);


class AfroditaHandler {
public:
	GetUrlHandler getUrlHandler;
	GetTitleHandler getTitleHandler;
	GetDomainHandler getDomainHandler;
	GetDocumentElementHandler getDocumentElementHandler;
	GetCookieHandler getCookieHandler;
	GetElementsByTagNameHandler getElementsByTagNameHandler;
	GetElementByIdHandler getElementByIdHandler;
	GetImagesHandler getImagesHandler;
	GetLinksHandler getLinksHandler;
	GetAnchorsHandler getAnchorsHandler;
	GetFormsHandler getFormsHandler;
	WriteHandler writeHandler;
	WriteLnHandler writeLnHandler;

	GetAttributeHandler getAttributeHandler;
	SetAttributeHandler setAttributeHandler;
	GetParentHandler getParentHandler;
	GetChildrenHandler getChildrenHandler;
	GetTagNameHandler getTagNameHandler;

	ClickHandler clickHandler;
	FocusHandler focusHandler;
	BlurHandler blurHandler;

	AfroditaHandler();

	static AfroditaHandler handler;

};


#endif /* MAZINGERFF_H_ */
