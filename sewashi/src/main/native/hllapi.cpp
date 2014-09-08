#include "hllapi.h"
#include <stdio.h>
#include <string.h>
#include <stdio.h>

#define HS_SUCCESS 0
#define CALL(number,data) {INT x = sizeof data; INT result = doCall (number, (char*) &data, x); if (result != 0) return result;}
#define CALL_NOFAIL(number,data)  {INT x = sizeof data; doCall (number, (char*) &data, x); }


static
const char *hl_messages[] = {
	"The function successfully executed, or no update since the last call was issued.",    // 0
	"An incorrect host presentation space ID was specified. The specified session either was not connected, does not exist, or is a logical printer session.", // 1
	"A parameter error was encountered, or an incorrect function number was specified. (Refer to the individual function for details.)", // 2
	"The execution of the function was inhibited because the target presentation space was busy, in X CLOCK state (X []), or in X SYSTEM state.", //3
	"The execution of the function was inhibited for some reason other than those stated in return code 4.", // 4
	"A data error was encountered due to specification of an incorrect parameter (for example, a length error causing truncation).", // 5
	"The specified presentation space position was not valid.", // 6
	"A functional procedure error was encountered (for example, use of conflicting functions or missing prerequisite functions).", // 7
	"No prior Start Host Notification (23) function was called for the host presentation space ID. ", // 8
	"A system error was encountered.",  // 9
	"This function is not available for EHLLAPI.", // 10
	"This resource is not available.", // 11
	"This session stopped.", // 12
	"The string was not found, or the presentation space is unformatted.", // 13
	"Keystrokes were not available on input queue.", // 14
	"A host event occurred. See Query Host Update (24) for details.", //15
	"File transfer was ended by a Ctrl+Break command.", //16
	"Field length was 0.",// 17
	"Keystroke queue overflow. Keystrokes were lost.", // 18
	"An application has already connected to this session for communications.", // 19
	"Reserved.", // 20
	"The OIA was updated", // 21
	"The presentation space was updated.", //22
	"Both the OIA and the host presentation space were updated.", //23
	"The message sent to the host was canceled.", //24
	"The message sent from the host was canceled.", //25
	"Contact with the host was lost.", // 26
	"Inbound communication has been disabled.", //27
	"The requested function has not completed its execution.", // 28
	"Another DDM session is already connected.", // 29
	"The disconnection attempt was successful, but there were asynchronous requests that had not been completed at the time of the disconnection.", // 30
	"The buffer you requested is being used by another application.", //31
	"There are no outstanding requests that match.", // 32
	"The API was already locked by another EHLLAPI application (on LOCK) or API not locked (on UNLOCK).", // 33
	"", // 34
	"", // 35
	"", // 36
	"", // 37
	"", // 38
	"", // 39
	"", // 40
	"", // 41
	"", // 42
	"", // 43
	"Printing has completed in the printer session." //44

};


HllApi::HllApi(const wchar_t *dllName)
{
	api = NULL;

	HINSTANCE hInstance = LoadLibraryW (dllName);
	if (hInstance != NULL)
		api = (HllApiType) GetProcAddress (hInstance, "hllapi");
}




const char *HllApi::getErrorMessage(int i)
{
	if ( i == -1 )
		return "Library not found";
	else
		return hl_messages [i];
}




#define HL_ConnectPresentationSpaceFunction 1
struct ConnectPresentationSpaceStruct {
	union {
		struct {
			char session;
			char reserved0[3];
		} __attribute__ ((packed)) request;
		struct {
			char reserved0[4];
		} __attribute__ ((packed)) response;
	};
};

#define HL_SendKeysFunction 3

#define HL_QuerySessionStatusFunction 22
struct QuerySessionStatusStruct {
	union {
		struct {
			char session;
		} __attribute__ ((packed)) request;
		struct {
			char session;
			char reserved0[3];
			char name[8];
			char type;
			char characteristics;
			short int rows;
			short int cols;
			short int codepage;
		} __attribute__ ((packed)) response;
	};
};

#define HL_QueryCursorLocation 7

#define HL_SetCursor 40

#define HL_ConvertPosition 99

#define HL_StartHostNotificationFunction 23
struct StartHostNotificationStruct {
	char session;
	char reserved0[3];
	char type;
	char reserved1[3];
	DWORD dwHandle;
	char subtype;
	char reserver2[3];
};

#define HL_QueryHostUpdateFunction 24
struct QueryHostUpdateStruct {
	char session;
	char reserved0[3];
};


#define HL_StartCommunicationNotificationFunction 80
struct StartCommunicationNotificationStruct {
	char session;
	char reserved0[3];
	char type;
	char reserved1[3];
	DWORD dwHandle;
	char subtype;
	char reserver2[3];
};

#define HL_CopyPresentationSpaceToStringFunction 8


int HllApi::doCall (int function, char*data, int &len) {
	INT result;
	api (&function, data, &len, &result);
	return result;

}

int HllApi::getCursorLocation(const char session, int& row, int& column) {
	if (api == NULL)
		return -1;

	row = -1;
	column = -1;
	INT len = 0;
	INT ps;

	ConnectPresentationSpaceStruct data;
	data.request.session = session;
	CALL (HL_ConnectPresentationSpaceFunction, data);

	int result = doCall(HL_QueryCursorLocation, NULL, len) ;
	if (result == 0)
	{
		char str[8];
		str[0]= session;
		str[1]='P';
		str[4]='P';
		ps = 0;
		int result = doCall(HL_ConvertPosition, str, ps, len);
		row = ps;
		column = result;
		return 0;
	}
	else
	{
		return 1;
	}
}

int HllApi::setCursorLocation(const char session, int row, int column) {
	if (api == NULL)
		return -1;

	INT len;
	INT ps;

	ConnectPresentationSpaceStruct data;
	data.request.session = session;
	CALL (HL_ConnectPresentationSpaceFunction, data);

	char str[8];
	str[0]= session;
	str[4]='R';
//	str[2]='\0';
	ps = column;
	len = row;

	int result = doCall(HL_ConvertPosition, str, len, ps);

	if (doCall(HL_SetCursor, NULL, len, result) == 0)
	{
		return 0;
	}
	else
		return 1;
}

int HllApi::doCall(int function, char*data, int &len, int value) {
	INT result = value;
	api (&function, data, &len, &result);
	return result;

}
int HllApi::querySesssionStatus(const char session, std::string & id, std::string & name, std::string & type, int & rows, int & cols, int & codepage)
{
	if (api == NULL)
		return -1;

	QuerySessionStatusStruct data;
	data.request.session = session;

	CALL (HL_QuerySessionStatusFunction, data);

	id = data.response.session;
	name.assign (data.response.name, 8);
	type = data.response.type;
	rows = data.response.rows;
	cols = data.response.cols;
	codepage = data.response.codepage;

	return 0;
}

int HllApi::getPresentationSpace(const char session, std::string & content)
{
	if (api == NULL)
		return -1;

	std::string id;
	std::string name;
	std::string type;
	int rows, cols, codepage;

	int result = querySesssionStatus(session, id, name, type, rows, cols, codepage);
	if (result != 0)
		return result;


	printf ("Querying session %c\n", session);
	ConnectPresentationSpaceStruct data;
	data.request.session = session;
	CALL (HL_ConnectPresentationSpaceFunction, data);
	printf ("Connnected\n");


	int size = rows * cols;
	char *achBuffer = (char*) malloc(size);
	result = doCall (HL_CopyPresentationSpaceToStringFunction, achBuffer, size, 1);

	if (result != 0) {
		printf ("Result  = %d\n", result);
		return result;
	}

	printf ("Copied\n");
	content.assign(achBuffer, size);

	return 0;
}



int HllApi::startHostNotification (const char session, HANDLE &handle) {
	if (api == NULL)
		return -1;

	StartHostNotificationStruct data;
	data.session = session;
	data.type = 'A';
	data.subtype = 'B';

	CALL (HL_StartHostNotificationFunction, data);

	handle = *((HANDLE*) data.dwHandle);
	return 0;
}

int HllApi::startCommunicationNotification (const char session, HANDLE &handle) {
	if (api == NULL)
		return -1;

	StartCommunicationNotificationStruct data;
	data.session = session;
	data.type = 'A';
	data.subtype = 'C';

	CALL (HL_StartCommunicationNotificationFunction, data);

	handle = *((HANDLE*) data.dwHandle);
	return 0;
}


#if 0
int HllApi::queryHostUpdate (const char session) {
	if (api == NULL)
		return -1;

	QueryHostUpdateStruct data;
	data.session = session;

	CALL (HL_QueryHostUpdateFunction, data);

	return 0;
}
#endif

int HllApi::sendKeys (const char session, const char *szString) {
	if (api == NULL)
		return -1;

	ConnectPresentationSpaceStruct data;
	data.request.session = session;
	CALL (HL_ConnectPresentationSpaceFunction, data);


	char *sz2 = strdup (szString);

	int len = strlen(szString);
	int result = doCall (HL_SendKeysFunction, sz2 , len, 0);

	free (sz2);

	if (result != 0)
		return result;


	return 0;
}
