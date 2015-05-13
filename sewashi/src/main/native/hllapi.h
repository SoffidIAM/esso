/*
 * hllapi.h
 *
 *  Created on: 07/07/2011
 *      Author: u07286
 */

#ifndef HLLAPI_H_
#define HLLAPI_H_
#include <windows.h>
#include <string>


// SESSION TYPES
#define HL_ST_3270DISPLAY "D"
#define HL_ST_3270PRINTER "P"
#define HL_ST_5250DISPLAY "F"
#define HL_ST_5250PRINTER "G"
#define HL_ST_ASCIIVT "H"

typedef long __stdcall (* HllApiType) (LPINT, LPSTR, LPINT, LPINT);

class HllApi {
public:
	HllApi (const wchar_t *dllName);
	int setSessionParameters ();

	int querySesssionStatus (const char session,
			std::string &id, std::string& name, std::string& sessionType, int &rows, int &cols, int &codepage );
	int getPresentationSpace (const char session,
			std::string &content );

	int startHostNotification (const char session, HANDLE &handle);
	int stopHostNotification (const char session);
	int startCommunicationNotification (const char session, HANDLE &handle);
	int sendKeys (const char session, const char *szString);
	int getCursorLocation (const char session, int &row, int &column);
	int setCursorLocation (const char session, int row, int column);

	const char *getErrorMessage (int i);
	bool isConnected () { return api != NULL; }
private:
	int doCall (int function, char*data, int &len) ;
	int doCall (int function, char*data, int &len, int psposition) ;
	HllApiType api;
};

#endif /* HLLAPI_H_ */
