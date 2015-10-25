/*
 * ssoclient.h
 *
 *  Created on: 19/07/2010
 *      Author: u07286
 */

#ifndef SEYCONSERVER_H_
#define SEYCONSERVER_H_
/**
 * Public classes
 */

#include <string>
#include <vector>

class SeyconCommon {
public:
	static void setDebugLevel (int debugLevel);

	static int getCardSupport() ;

	static void notifyError() ;

	static bool bNormalized() ;

	static int getServerList(char* servers, size_t dwSize) ;
	static int getServerAlternatePort() ;
	static int getServerPort() ;

	//////////////////////////////////////////////////////////
	static bool readProperty(const char* property, std::string &value) ;
	static void writeProperty(const char* property, const char *value) ;
	static int readIntProperty(const char* property) ;
	static void updateConfig(const char* lpszParam) ;

	static void getCitrixClientName(std::string &name) ;
	static void getCitrixClientIP(std::string &ip) ;
	static void getCitrixInitialProgram(std::string &app) ;
	/** Conversión base 64 **/
	static std::string toBase64  (const unsigned char* source, int len);
	static std::string fromBase64(const char* source);
	static std::string urlEncode  (const wchar_t* source);
	static std::wstring urlDecode (const char* source);
	static std::string crypt(const char* source);
	static std::string uncrypt(const char* source);


	static bool getServerList(std::string &servers) ;

	static void info (const char *szFormat, ...);
	static void warn (const char *szFormat, ...);
	static void debug (const char *szFormat, ...);

	static void wipe (std::wstring &str);
	static void wipe (std::string &str);

	static void updateHostAddress ();
private:
	static int seyconDebugLevel;
};

enum ServiceIteratorResult {
	SIR_SUCCESS,
	SIR_ERROR,
	SIR_RETRY
};

class SeyconServiceIterator {
public:
	SeyconServiceIterator () ;
	virtual ServiceIteratorResult iterate (const char* hostName, size_t dwPort) = 0;
	virtual ~SeyconServiceIterator ();
};


class SeyconResponse {
public:
	SeyconResponse (char *utf8Result, int size);
	~SeyconResponse ();
	///////////////////////////////////////////////////////////////
	bool getToken(int id, std::wstring &value) ;
	///////////////////////////////////////////////////////////////
	bool getToken(int id, std::string &value) ;
	bool getUtf8Token(int id, std::string &value) ;
	std::string getToken(int id) {
		std::string s;
		getToken(id, s);
		return s;
	}
	std::string getUtf8Token(int id) {
		std::string s;
		getUtf8Token(id, s);
		return s;
	}
	std::wstring getWToken(int id) {
		std::wstring s;
		getToken(id, s);
		return s;
	}

	bool getTail(int id, std::string &value) ;
	bool getUtf8Tail(int id, std::string &value) ;
	std::string getTail(int id) {
		std::string s;
		getTail(id, s);
		return s;
	}
	std::string getUtf8Tail(int id) {
		std::string s;
		getUtf8Tail(id, s);
		return s;
	}


	int getSize () { return size ; }
	const char *getResult () { return result; }
private:
	int size;
	char *result;
	bool findToken(int id, std::string &t, bool tillEnd) ;
};

class SeyconService {
public:
	SeyconService ();
	/** Conexión al servidor seycon */
	SeyconResponse* sendUrlMessage(const char* host, int port, const wchar_t* url, ...) ;
	SeyconResponse* sendUrlMessage(const wchar_t* url, ...) ;

	void resetServerStatus ();
	std::wstring escapeString(const char* lpszSource) ;
	std::wstring escapeString(const wchar_t* lpszSource) ;
	ServiceIteratorResult iterateServers(SeyconServiceIterator &it) ;

	SeyconService *newInstance();
private:
	static std::vector<std::string> hosts;
	static int preferedServer;
	static time_t lastError;
};

#endif
