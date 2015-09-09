/*
 * ConfigReader.cpp
 *
 *  Created on: 16/02/2010
 *      Author: u07286
 */

#include <MazingerInternal.h>

#include "ConfigReader.h"
#include "Action.h"
#include "ComponentSpec.h"
#include "../web/WebApplicationSpec.h"
#include "../web/WebComponentSpec.h"
#include "../web/WebInputSpec.h"
#include "../web/WebFormSpec.h"
#include <DomainPasswordCheck.h>
#include <stdio.h>
#include <string.h>
#include "../hll/HllPatternSpec.h"
#include <stdlib.h>

#ifndef WIN32
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <memory.h>
#endif

ConfigReader::ConfigReader(PMAZINGER_DATA pMazingerData) {
	m_size = pMazingerData->dwRulesSize;
	m_position = 0;
	m_pointer = NULL;
	m_level = 0;
	m_bTesting = false;
	m_achIndent = NULL;
	m_pMapView = NULL;

#ifdef WIN32
	m_hMapFile = OpenFileMappingW(FILE_MAP_READ, // Read/write permission.
			false, // Max. object size.
			pMazingerData->achRulesFile); // Name of mapping object.

	if (m_hMapFile != NULL) {
		m_pMapView = (byte*) MapViewOfFile(m_hMapFile, FILE_MAP_READ, 0, 0, m_size);
		MZNSendDebugMessageW(L"Reading rules file %s.\n", pMazingerData->achRulesFile);
		if (m_pMapView == NULL) {
			MZNSendDebugMessageW(L"Unable to read rules %s.\n", pMazingerData->achRulesFile);
		} else {
#else
	char achRules[4096];
    wcstombs(achRules, pMazingerData->achRulesFile, sizeof achRules - 1);
	m_file = open (achRules, O_RDONLY);
	if (m_file >= 0) {
		m_pMapView = (unsigned char*) mmap (NULL, m_size, PROT_READ, MAP_SHARED, m_file, 0);
		if (m_pMapView == MAP_FAILED) {
			MZNSendDebugMessageW(L"Unable to read rules %ls.\n", pMazingerData->achRulesFile);
		} else {
#endif
			m_pointer = m_pMapView;
			if (m_size < 5)
			{
				MZNSendDebugMessageW(L"Unable to parse rules %s.Invalid size %d\n", pMazingerData->achRulesFile, m_size);
				m_pointer = NULL;
			}
			// Verificar la signatura
			else if (memcmp(m_pointer, "MZN",3) !=0 )
			{
				MZNSendDebugMessageW(L"Unable to parse rules %s. Bad signature (%c%c%c)\n", pMazingerData->achRulesFile, m_pointer[0],m_pointer[1],m_pointer[2]);
				m_pointer = NULL;
			} else {
				m_pointer += 3;
				m_position += 3;
				unsigned int dwVersion = readInteger();
				if (dwVersion > 4)
				{
					MZNSendDebugMessageW (L"Unable to open file %s. Unable to parse version %d\n",
							pMazingerData->achRulesFile, dwVersion);
					m_pointer = NULL;
				}

			}
		}
	}

}


ConfigReader::~ConfigReader() {
#ifdef WIN32
	if (m_pMapView != NULL)
		UnmapViewOfFile (m_pMapView);
	if (m_hMapFile != NULL)
		CloseHandle(m_hMapFile);
#else
	munmap(m_pMapView, m_size);
	close(m_file);
#endif
	for (unsigned int i =0 ; i < m_components.size(); i++)
	{
		delete m_components[i];
	}
	for (unsigned int i =0 ; i < m_domainPasswordChecks.size(); i++)
	{
		delete m_domainPasswordChecks[i];
	}
	for (unsigned int i =0 ; i < m_globalActions.size(); i++)
	{
		delete m_globalActions[i];
	}
	if (m_achIndent != NULL)
		free (m_achIndent);
}

char ConfigReader::readChar() {
	m_position ++;
	if (m_position > m_size)
	{
		MZNSendDebugMessage("PARSER OVERFLOW !!!!");
		return '\0';
	}
	return *m_pointer++;
}

unsigned int ConfigReader::readInteger() {
	unsigned int dwValue = 0;
	m_position += 4;
	if (m_position > m_size)
	{
		MZNSendDebugMessage("PARSER OVERFLOW !!!!");
		return 0;
	}
	dwValue = (int) *m_pointer++ << 24 ;
	dwValue |= (int) *m_pointer++ << 1;
	dwValue |= (int) *m_pointer++ << 8 ;
	dwValue |= (int) *m_pointer++;
	PARSE_DEBUG("%d: Parsed %d\n", m_position, dwValue);

	return dwValue;
}

char* ConfigReader::readUTF8String(int isSkipOnly) {
	int position = m_position;
	int allocated = 32;
	char *pszString = isSkipOnly ? NULL : (char*) malloc(allocated);
	int used = 0;
	char chTag;
	do {
		chTag = readChar();
		if (!isSkipOnly) {

			if (used + 2 > allocated) {
				allocated *= 2;
				pszString = (char*) realloc(pszString, allocated);
			}
			pszString[used++] = chTag;
		}
	} while (chTag != '\0');
	if (!isSkipOnly) {
		pszString[used] = '\0';
	}

	PARSE_DEBUG("%d-%d: Parsed %s (skip=%d)\n", position, m_position, pszString, (int) isSkipOnly);

	return pszString;
}

char* ConfigReader::readString(int isSkipOnly) {
	char *pszString = readUTF8String (isSkipOnly);
	if (pszString != NULL) {
		return pszString;
	} else {
		return NULL;
	}
}

void ConfigReader::readStringMember (const char *attributeName, char* &attribute, int skipOnly) {
	if (m_bTesting)
		dumpStringAttribute(attributeName);
	else {
		char * s = readString(skipOnly);
		if (s != NULL )
			attribute = s;
	}
}

void ConfigReader::readBooleanMember (const char *attributeName, bool &attribute, int skipOnly) {
	if (m_bTesting)
		dumpStringAttribute(attributeName);
	else {
		char * s = readString(skipOnly);
		if (s != NULL)
		{
			attribute = (strcmp("true", s) == 0);
		}
	}
}

void ConfigReader::readIntegerMember (const char *attributeName, int &attribute, int skipOnly) {
	if (m_bTesting)
		dumpStringAttribute(attributeName);
	else {
		char * s = readString(skipOnly);
		if (s != NULL)
		{
			sscanf ( s, " %d", &attribute);
		}
	}
}

void ConfigReader::readRegExpMember (const char *attributeName, char* &attribute, regex_t* &re, int skipOnly) {
	if (m_bTesting)
		dumpRegExpAttribute(attributeName);
	else {
		char * s = readString(skipOnly);
		if (s != NULL && s[0] != '\0')
		{
			attribute = s;
			re = compileRegExp(attribute);
		}
	}
}

std::vector<ComponentSpec*>& ConfigReader::getComponents () {
	return m_components;
}

std::vector<DomainPasswordCheck*>& ConfigReader::getDomainPasswordChecks () {
	return m_domainPasswordChecks;
}
std::vector<Action*>& ConfigReader::getGlobalActions () {
	return m_globalActions;
}

void ConfigReader::dumpStringAttribute(const char*name) {
	char * s = readString(false);
	MZNSendDebugMessageA("%s%s='%s'",getIndent(),name, s);
	free(s);
}

void ConfigReader::dumpRegExpAttribute(const char*name) {
	char * s = readString(false);
	if (name == NULL)
		MZNSendDebugMessageA("%s%s",getIndent(), s);
	else
		MZNSendDebugMessageA("%s%s='%s'",getIndent(),name, s);
	if (s[0] != '\0') {
		regex_t* re = compileRegExp (s);
		if (re == NULL)
			MZNSendDebugMessageA("<!-- BAD REGEXP %s -->",getIndent(), s);
	}
	free(s);
}

regex_t* ConfigReader::compileRegExp(char *string) {
	if (string != NULL && string[0] != '\0') {
		regex_t *re = (regex_t*) malloc(sizeof(regex_t));
		int result = regcomp(re, string, REG_EXTENDED | REG_NOSUB);
		if (result != 0)
		{
			MZNSendDebugMessageA("Error compiling regular expression %s:", string);
			int size = regerror (result, re, NULL, 0);
			char *achBuffer = (char*) malloc (size+1);
			regerror (result, re, achBuffer, size);
			MZNSendDebugMessageA("%s", achBuffer);
			free(achBuffer);
			regfree(re);
			re = NULL;
		}
		return re;
	} else
		return NULL;
}

Action *ConfigReader::readAction(int skipOnly, int readHeader) {
	Action* pAction = NULL;
	if (!skipOnly && !m_bTesting) {
		pAction = new Action();
	}

	if (m_bTesting)
	{
		MZNSendDebugMessageA("%s<Action", getIndent());
		indent ();
	}
	// Leer cabecera
	if (readHeader)
	{
		int ch = readChar();
		if (ch != 'A')
		{
			MZNSendDebugMessageA("Invalid type %c for Action", ch);
			return NULL;
		}
	}
	// Leer atributes
	char chTag;
	for (chTag = readChar(); chTag != L'\0'; chTag = readChar()) {
		PARSE_DEBUG("Action tag = %c position=%d skip=%d\n", chTag, m_position, (int) skipOnly);
		if (chTag == 'T') {
			readStringMember ("type", pAction->szType, skipOnly);
		}
		else if (chTag == 't') {
			readStringMember ("text", pAction->szText, skipOnly);
		}
		else if (chTag == 'E') {
			readStringMember ("event", pAction->szEvent, skipOnly);
		}
		else if (chTag == 'R') {
			readBooleanMember ("repeat", pAction->m_canRepeat, skipOnly);
		}
		else if (chTag == 'D') {
			readIntegerMember ("delay", pAction->m_delay, skipOnly);
		}
		else {
			MZNSendDebugMessageA("Invalid TAG %c for Action", chTag);
			return NULL;
		}
	}
	PARSE_DEBUG("End Action tags position=%d (skip=%d)\n", m_position, (int) skipOnly);
	char *szContent = readUTF8String(skipOnly && !m_bTesting);
	if (m_bTesting)
	{
		MZNSendDebugMessageA("%s>",getIndent());
		MZNSendDebugMessageA("%s%s", getIndent(), szContent);
		unindent();
		MZNSendDebugMessageA("%s</Action>", getIndent());
		free(szContent);
	} else {
		if (szContent != NULL)
			pAction->szContent = szContent;
	}
	return pAction;
}

ComponentSpec *ConfigReader::readComponent(int skipOnly) {
	ComponentSpec* pComponent = NULL;
	if (!skipOnly && !m_bTesting) {
		pComponent = new ComponentSpec();
	}

	if (m_bTesting)
	{
		MZNSendDebugMessageA("%s<Component", getIndent());
		indent();
	}
	// Leer cabecera
	char ch = readChar();
	if (ch != 'j')
	{
		MZNSendDebugMessageA("Invalid type %c for Component at %d", ch, m_position);
		return NULL;
	}
	// Leer el tipo de match
	wchar_t achMatchType = readChar();
	if (m_bTesting && achMatchType != 'F')
	{
		MZNSendDebugMessageA("%scheck='partial'", getIndent());
	}
	if (!skipOnly && !m_bTesting) {
		if (achMatchType == 'F')
			pComponent->fullMatch = 1;
		else
			pComponent->fullMatch = 0;
	}

	// Leer atributes
	char chTag;
	for (chTag = readChar(); chTag != L'\0'; chTag = readChar()) {
		PARSE_DEBUG("En load Component tag = %c skip=%d", chTag, skipOnly);
		if (chTag == 'N') {
			readRegExpMember("name", pComponent->szName, pComponent->reName, skipOnly);
		}
		else if (chTag == 'C') {
			readRegExpMember("class", pComponent->szClass, pComponent->reClass, skipOnly);
		}
		else if (chTag == 'V') {
			readRegExpMember("text", pComponent->szText, pComponent->reText, skipOnly);
		}
		else if (chTag == 'T') {
			readRegExpMember("title", pComponent->szTitle, pComponent->reTitle, skipOnly);
		}
		else if (chTag == 'D') {
			readRegExpMember("dlgId", pComponent->szDlgId, pComponent->reDlgId, skipOnly);
		}
		else if (chTag == 'I') {
			readStringMember("ref-as", pComponent->szId, skipOnly);
		}
		else if (chTag == 'O') {
			readBooleanMember("optional", pComponent->optional, skipOnly);
		}
		else {
			MZNSendDebugMessageA("Invalid tag %c for Component", chTag);
			return NULL;
		}
	}

	if (m_bTesting)
	{
		MZNSendDebugMessageA("%s>", getIndent());
	}
	unsigned int dwActions = readInteger();

	for (unsigned int  i = 0; i < dwActions; i++) {
		Action *pAction = readAction(skipOnly, true);
		if (pAction != NULL) {
			pComponent->m_actions.push_back(pAction);
			pAction->m_component = pComponent;
		}
	}
	unsigned int  dwComponents = readInteger();
	for (unsigned int  i = 0; i < dwComponents; i++) {
		ComponentSpec* pChild = readComponent(skipOnly);
		if (pChild != NULL) {
			pComponent->m_children.push_back(pChild);
			pChild->m_parent = pComponent;
			pChild->m_order = i;
		}
	}
	if (m_bTesting)
	{
		unindent();
		MZNSendDebugMessageA("%s</Component>", getIndent());
	}
	return pComponent;
}


WebInputSpec *ConfigReader::readInput(int skipOnly) {
	WebInputSpec* pComponent = NULL;
	if (!skipOnly && !m_bTesting) {
		pComponent = new WebInputSpec();
	}

	if (m_bTesting)
	{
		MZNSendDebugMessageA("%s<Input", getIndent());
		indent();
	}
	// Leer cabecera
	readChar();
	// Leer atributes
	char chTag;
	for (chTag = readChar(); chTag != L'\0'; chTag = readChar()) {
		if (chTag == 'T') {
			readRegExpMember("type", pComponent->szType, pComponent->reType, skipOnly);
		}
		if (chTag == 'V') {
			readRegExpMember("value", pComponent->szValue, pComponent->reValue, skipOnly);
		}
		if (chTag == 'N') {
			readRegExpMember("name", pComponent->szName, pComponent->reName, skipOnly);
		}
		if (chTag == 'I') {
			readRegExpMember("id", pComponent->szId, pComponent->reId, skipOnly);
		}
		if (chTag == 'R') {
			readStringMember("ref-as", pComponent->szRefAs, skipOnly);
		}
		if (chTag == 'O') {
			readBooleanMember("optional", pComponent->m_bOptional, skipOnly);
		}
	}

	if (m_bTesting)
	{
		unindent();
		MZNSendDebugMessageA("%s/>", getIndent());
	}
	return pComponent;
}
HllRowPatternSpec *ConfigReader::readHllRowPatternSpec (int skipOnly)
{
	HllRowPatternSpec* pComponent = NULL;
	if (!skipOnly && !m_bTesting) {
		pComponent = new HllRowPatternSpec();
	}

	if (m_bTesting)
	{
		MZNSendDebugMessageA("%s<Pattern", getIndent());
		indent();
	}
	// Leer cabecera
	readChar();
	// Leer atributes
	char chTag;
	for (chTag = readChar(); chTag != L'\0'; chTag = readChar()) {
		if (chTag == 'R') {
			readStringMember("row", pComponent->row, skipOnly);
		}
	}
	if (m_bTesting)
		MZNSendDebugMessageA("%s>", getIndent());

	readRegExpMember(NULL, pComponent->pattern, pComponent->rePattern, skipOnly);
	if (m_bTesting)
	{
		unindent ();
		MZNSendDebugMessageA("%s</Pattern>", getIndent());
	}
	return pComponent;
}

WebFormSpec *ConfigReader::readForm(int skipOnly) {
	WebFormSpec* pComponent = NULL;
	if (!skipOnly && !m_bTesting) {
		pComponent = new WebFormSpec();
	}

	if (m_bTesting)
	{
		MZNSendDebugMessageA("%s<Form", getIndent());
		indent();
	}
	// Leer cabecera
	readChar();
	// Leer atributes
	char chTag;
	for (chTag = readChar(); chTag != L'\0'; chTag = readChar()) {
		if (chTag == 'A') {
			readRegExpMember("action", pComponent->szAction, pComponent->reAction, skipOnly);
		}
		if (chTag == 'M') {
			readRegExpMember("method", pComponent->szMethod, pComponent->reMethod, skipOnly);
		}
		if (chTag == 'N') {
			readRegExpMember("name", pComponent->szName, pComponent->reName, skipOnly);
		}
		if (chTag == 'I') {
			readRegExpMember("id", pComponent->szId, pComponent->reId, skipOnly);
		}
		if (chTag == 'R') {
			readStringMember("ref-as", pComponent->szRefAs, skipOnly);
		}
		if (chTag == 'O') {
			readBooleanMember("optional", pComponent->m_bOptional, skipOnly);
		}
	}

	if (m_bTesting)
		MZNSendDebugMessageA("%s>",getIndent());
	unsigned int dwInputs = readInteger();

	for (unsigned int i = 0; i < dwInputs; i++) {
		WebInputSpec *c = readInput(skipOnly);
		if (c != NULL) {
			pComponent->m_children.push_back(c);
		}
	}
	if (m_bTesting) {
		unindent();
		MZNSendDebugMessageA("%s</Form>",getIndent());
	}
	return pComponent;
}

void ConfigReader::readApplication(int iApplicationId) {
	// Leer atributes
	wchar_t chTag;
	bool skip = true;
	const char* lpszCmdLine2 = MZNC_getCommandLine();
	char * lpszCmdLine = (char*) malloc (strlen (lpszCmdLine2) +1);
	strcpy (lpszCmdLine, lpszCmdLine2);
	if (m_bTesting)
	{
		MZNSendDebugMessageA("%s<Application", getIndent());
		indent ();
		skip = false;
	}
	for (chTag = readChar(); chTag != L'\0'; chTag = readChar()) {
		PARSE_DEBUG("tag = %c", chTag);
		if (chTag == L'N') {
			if (m_bTesting)
			{
				dumpRegExpAttribute("cmdLine");
			} else if (m_bWeb){
				// Botar el valor. Estam en mode web
				readString(true);
			} else {
				char *str = readString(false);
				if (str != NULL)
				{
					regex_t* re = compileRegExp(str);
					regmatch_t rm[2];
					if (re != NULL && regexec(re, lpszCmdLine, 2, rm, 0) != 0)
					{
//						MZNSendDebugMessageA("Ignoring rules for %s", str);
					} else {
						skip = false;
						MZNSendDebugMessageA("Applying rule %d: [cmdLine='%s']", iApplicationId, str);
					}
					free(str);
					if (re != NULL)
						regfree(re);
				}
			}
		} else {
			MZNSendDebugMessage("Invalid attribute %c", chTag);
			return;
		}
	}
	if (m_bTesting)
		MZNSendDebugMessageA("%s>",getIndent());
	unsigned int dwComponents = readInteger();

	for (unsigned int i = 0; i < dwComponents; i++) {
		ComponentSpec *c = readComponent(skip);
		if (c != NULL) {
			m_components.push_back(c);
			c->m_order = i;
		}
	}
	if (m_bTesting) {
		unindent();
		MZNSendDebugMessageA("%s</Application>",getIndent());
	}
}

void ConfigReader::readWebApplication() {
	WebApplicationSpec* pApplication = NULL;
	int skipOnly = m_bTesting || ! m_bWeb;

	if (! skipOnly) {
		pApplication = new WebApplicationSpec();
		m_webApplications.push_back(pApplication);
	}

	if (m_bTesting)
	{
		MZNSendDebugMessageA("%s<WebApplication", getIndent());
		indent();
	}

	// Leer atributes
	char chTag;
	for (chTag = readChar(); chTag != L'\0'; chTag = readChar()) {
		PARSE_DEBUG("En load WebApplication tag = %c skip=%d", chTag, skipOnly);
		if (chTag == 'U') {
			readRegExpMember("url", pApplication->szUrl, pApplication->reUrl, skipOnly);
		}
		if (chTag == 'T') {
			readRegExpMember("title", pApplication->szTitle, pApplication->reTitle, skipOnly);
		}
		if (chTag == 'C') {
			readRegExpMember("content", pApplication->szContent, pApplication->reContent, skipOnly);
		}
	}

	if (m_bTesting)
	{
		MZNSendDebugMessageA("%s>", getIndent());
	}
	unsigned int dwInputs = readInteger();
	for (unsigned int  i = 0; i < dwInputs; i++) {
		WebInputSpec *pSpec = readInput(skipOnly);
		if (pSpec != NULL) {
			pApplication->m_components.push_back(pSpec);
		}
	}

	unsigned int dwForms = readInteger();
	for (unsigned int  i = 0; i < dwForms; i++) {
		WebFormSpec *pSpec = readForm(skipOnly);
		if (pSpec != NULL) {
			pApplication->m_components.push_back(pSpec);
		}
	}


	unsigned int dwActions = readInteger();

	for (unsigned int  i = 0; i < dwActions; i++) {
		Action *pAction = readAction(skipOnly, true);
		if (pAction != NULL) {
			pApplication->m_actions.push_back(pAction);
			pAction->m_component = NULL;
		}
	}
	if (m_bTesting)
	{
		unindent();
		MZNSendDebugMessageA("%s</WebApplication>", getIndent());
	}
}

void ConfigReader::	readHllPattern ()
{
	HllPatternSpec* pPattern = NULL;
	int skipOnly = m_bTesting || ! m_bHll;

	if (! skipOnly) {
		pPattern = new HllPatternSpec();
		m_hllPatterns.push_back(pPattern);
	}

	if (m_bTesting)
	{
		MZNSendDebugMessageA("%s<HllApplication", getIndent());
		indent();
	}

	// Leer atributes
	char chTag;
	for (chTag = readChar(); chTag != L'\0'; chTag = readChar()) {
		PARSE_DEBUG("En load WebApplication tag = %c skip=%d", chTag, skipOnly);
	}

	if (m_bTesting)
	{
		MZNSendDebugMessageA("%s>", getIndent());
	}
	unsigned int dwPattterns = readInteger();
	for (unsigned int  i = 0; i < dwPattterns; i++) {
		HllRowPatternSpec *pSpec = readHllRowPatternSpec(skipOnly);
		if (pSpec != NULL) {
			pPattern->m_patterns.push_back(pSpec);
		}
	}

	unsigned int dwActions = readInteger();

	for (unsigned int  i = 0; i < dwActions; i++) {
		Action *pAction = readAction(skipOnly, true);
		if (pAction != NULL) {
			pPattern->m_actions.push_back(pAction);
			pAction->m_component = NULL;
		}
	}
	if (m_bTesting)
	{
		unindent();
		MZNSendDebugMessageA("%s</HllApplication>", getIndent());
	}

}


void ConfigReader::readDomainPassword() {
	// Leer atributes
	wchar_t chTag;
	DomainPasswordCheck *p = NULL;
	int skipOnly = m_bTesting || m_bWeb;

	if (! skipOnly )
		p = new DomainPasswordCheck ();

	if (m_bTesting)
	{
		MZNSendDebugMessageA("%s<DomainPassword", getIndent());
		indent ();
	}

	for (chTag = readChar(); chTag != L'\0'; chTag = readChar()) {
		if (chTag == 'D')
		{
			readStringMember ("domain", p->m_szDomain, skipOnly);
		} else if (chTag == 'S') {
			readStringMember ("servers", p->m_szServers, skipOnly);
		} else if (chTag == 'U') {
			readStringMember ("userSecret", p->m_szUserSecret, skipOnly);
		} else if (chTag == 'P') {
			readStringMember ("passwordSecret", p->m_szPasswordSecret, skipOnly);
		}
		else
		{
			MZNSendDebugMessageA("PARSE ERROR: Invalid attribute %c", chTag);
			return;
		}

	}
	if (m_bTesting)
		MZNSendDebugMessageA("%s>", getIndent());

	unsigned int dwActions = readInteger();
	for (unsigned int i = 0; i < dwActions; i++) {
		Action *a = readAction(skipOnly, true);
		if (! skipOnly)
			p->m_actions.push_back(a);
	}


	if (m_bTesting)
	{
		unindent ();
		MZNSendDebugMessageA("%s</DomainPassword>", getIndent());
	}
	else if (p != NULL)
	{
		m_domainPasswordChecks.push_back(p);
	}
	PARSE_DEBUG("Parsed password");
}

void ConfigReader::readGlobalAction() {
	int skip = m_bTesting || m_bWeb;
	Action* pAction = readAction(skip, false);
	if ( ! skip )
		m_globalActions.push_back(pAction);
}

void ConfigReader::parse() {
	m_bWeb = false;
	if (m_pointer != NULL)
		doParse ();
}
void ConfigReader::parseWeb() {
	m_bWeb = true;
	if (m_pointer != NULL)
		doParse();
}
void ConfigReader::parseHll() {
	m_bHll = true;
	if (m_pointer != NULL)
		doParse();
}

void ConfigReader::doParse() {
	const char* lpszCmdLine = MZNC_getCommandLine();
	if (m_bWeb)
		MZNSendDebugMessageA("Compiling WEB rules for %s", lpszCmdLine);
	else if (m_bHll)
		MZNSendDebugMessageA("Compiling HLL API rules", lpszCmdLine);
	else
		MZNSendDebugMessageA("Compiling rules for %s", lpszCmdLine);

	for (std::vector<ComponentSpec*>::iterator it = m_components.begin();
			it != m_components.end();
			it ++)
	{
		if (*it != NULL)
		{
			delete *it;
		}
	}
	m_components.clear();

	unsigned int dwApplications = readInteger();

	for (unsigned int i = 0; i < dwApplications; i++) {
		// Leer cabecera
		char achType = readChar();
		if (achType == 'J') {
			readApplication(i);
		} else if (achType == 'W') {
			readWebApplication();
		} else if (achType == 'P') {
			readDomainPassword ();
		} else if (achType == 'H') {
			readHllPattern ();
		} else if (achType == 'A') {
			readGlobalAction ();
		} else {
			return;
		}
	}
}


void ConfigReader::testConfiguration() {
	m_bTesting = true;
	if (m_pointer == NULL) {
		MZNSendDebugMessage ("<Mazinger/>");
	} else {
		unsigned int dwApplications = readInteger();
		MZNSendDebugMessage ("<Mazinger>");
		m_level = 0;
		indent ();
		for (unsigned int i = 0; i < dwApplications; i++) {
			// Leer cabecera
			char achType = readChar();
			if (achType == 'J') {
				readApplication(i);
			} else if (achType == 'W') {
				readWebApplication();
			} else if (achType == 'H') {
				readHllPattern();
			} else if (achType == 'P') {
				readDomainPassword ();
			} else if (achType == 'A') {
				readGlobalAction ();
			} else {
				MZNSendDebugMessageA("Invalid rule type %c on rule %d", achType, i);
				return;
			}
		}
		unindent ();
		MZNSendDebugMessage ("</Mazinger>");
	}
	m_bTesting = false;
}

void ConfigReader::indent() {
	if (m_achIndent == NULL) {
		m_achIndent = (char*) malloc(132);
		m_level = 0;
	}
	if (m_level < 128)
	{
		m_achIndent[m_level++] = ' ';
		m_achIndent[m_level++] = ' ';
		m_achIndent[m_level++] = ' ';
		m_achIndent[m_level] = '\0';
	} else {
		m_level += 3;
	}

}
void ConfigReader::unindent() {
	m_level -= 3;
	if (m_level < 128)
		m_achIndent[m_level] = '\0';
}

const char* ConfigReader::getIndent() {
	if (m_achIndent == NULL)
		return "";
	else
		return m_achIndent;
}

std::vector<WebApplicationSpec*>& ConfigReader::getWebApplicationSpecs () {
	return m_webApplications;
}

std::vector<HllPatternSpec*>& ConfigReader::getHllPatternSpecs () {
	return m_hllPatterns;
}
