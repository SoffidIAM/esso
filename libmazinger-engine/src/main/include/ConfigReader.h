/*
 * ConfigReader.h
 *
 *  Created on: 16/02/2010
 *      Author: u07286
 */

#ifndef CONFIGREADER_H_
#define CONFIGREADER_H_

#include <pcreposix.h>
#include <vector>

#include <MazingerEnv.h>

class ComponentSpec;
class Action;
class DomainPasswordCheck;
class WebApplicationSpec;
class WebInputSpec;
class WebFormSpec;
class HllPatternSpec;
class HllRowPatternSpec;
class WebTransport;

class ConfigReader {
public:
	ConfigReader(PMAZINGER_DATA pMazingerData);
	virtual ~ConfigReader();
	char readChar ();
	unsigned int readInteger ();
	char *readString (int skipOnly);
	char *readUTF8String (int skipOnly);
	regex_t* compileRegExp (char *string);

	std::vector<ComponentSpec*>& getComponents ();
	std::vector<DomainPasswordCheck*>& getDomainPasswordChecks ();
	std::vector<WebApplicationSpec*>& getWebApplicationSpecs ();
	std::vector<HllPatternSpec*>& getHllPatternSpecs ();
	std::vector<Action*>& getGlobalActions ();
	std::vector<WebTransport*>& getWebTransports ();
	void parse ();
	void parseWeb ();
	void parseWebTransport ();
	void parseHll ();
	void testConfiguration();

private:
	void readApplication (int iApplicationId);
	void readWebApplication ();
	void readWebTransport();
	void readHllPattern ();
	ComponentSpec *readComponent (int skipOnly);
	HllRowPatternSpec *readHllRowPatternSpec (int skipOnly);
	WebInputSpec *readInput (int skipOnly);
	WebFormSpec *readForm (int skipOnly);
	Action *readAction(int skipOnly, int readHeader);
	void readDomainPassword();
	void readGlobalAction();
	void doParse ();

	void readStringMember (const char *attributeName, char* &attribute, int skipOnly);
	void readIntegerMember (const char *attributeName, int &attribute, int skipOnly);
	void readBooleanMember (const char *attributeName, bool &attribute, int skipOnly);
	void readRegExpMember (const char *attributeName, char* &attribute, regex_t* &re, int skipOnly);

	int m_bTesting;
	unsigned char *m_pointer;
	int m_size;
	int m_position;
	int m_bWeb; // Web Configuration
	int m_bWebTransport; // Web transport configuration
	int m_bHll; // Hll Configuration
#ifdef _WIN32_WINNT
	HANDLE m_hMapFile;
#else
	int m_file;
#endif
	unsigned char *m_pMapView;

	char *m_achIndent;
	int m_level;
	void indent ();
	void unindent ();
	const char *getIndent();
	void dumpStringAttribute(const char*attribute);
	void dumpRegExpAttribute(const char*attribute);

	std::vector<ComponentSpec*> m_components;
	std::vector<WebApplicationSpec*> m_webApplications;
	std::vector<DomainPasswordCheck*> m_domainPasswordChecks;
	std::vector<Action*> m_globalActions;
	std::vector<HllPatternSpec* >m_hllPatterns;
	std::vector<WebTransport* >m_webTransports;
};

#endif /* CONFIGREADER_H_ */
