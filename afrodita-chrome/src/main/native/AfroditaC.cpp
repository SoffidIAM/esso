/*
 * AfroditaC.cpp
 *
 *  Created on: 22/11/2010
 *      Author: u07286
 */

#include "AfroditaC.h"
#include <MazingerInternal.h>
#include <stdio.h>
#include <string.h>
#include "ChromeWebApplication.h"
#include "CommunicationManager.h"
#include "json/JsonMap.h"
#include "json/JsonValue.h"
#include <SeyconServer.h>
#include <stdlib.h>

#define MIME_TYPE_DESCRIPTION "application/soffid-sso-plugin:sso:Soffid SSO Plugin"

using namespace mazinger_chrome;
using namespace std;
using namespace json;


#ifdef WIN32

#include <io.h>
#include <fcntl.h>

#else
extern "C" void __attribute__((constructor)) startup() {
	setbuf(stdin, NULL);
	setbuf(stdout, NULL );
}
#endif


extern "C" int main (int argc, char **argv)
{
#ifdef WIN32
	setmode(fileno(stdout), O_BINARY);
	setmode(fileno(stdin), O_BINARY);
#endif
	SeyconCommon::setDebugLevel(0);
	DEBUG ("Started AfroditaC");
	CommunicationManager* manager = CommunicationManager::getInstance();

	manager->mainLoop();

#ifdef WIN32
	ExitProcess(0);
#else
	exit(0);
#endif

}
