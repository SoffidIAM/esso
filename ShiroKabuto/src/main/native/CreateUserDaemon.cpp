#include <windows.h>
#include <winsock.h>
#include <ctype.h>
#include <winuser.h>
#include <wtsapi32.h>
typedef int socklen_t;

#include <stdio.h>
#include "CreateUserDaemon.h"
#include <string>
#include <ssoclient.h>
#include <lm.h>
#include <msg.h>

#include "ShiroSeyconDialog.h"
#include <MZNcompat.h>
#include <map>
#include <time.h>
#include "Tokenizer.h"
#include <MazingerInternal.h>

extern HANDLE hEventLog;
extern const wchar_t* generatePassword();
extern const wchar_t * errorMessage(int error);

CreateUserDaemon::CreateUserDaemon() {
	socket_in = 0;
	publicSocket = 0;
	stop = false;
}

int CreateUserDaemon::getErrorNumber() {
#ifdef WIN32
	return WSAGetLastError();
#else
	return errno;
#endif
}

///////////////////////////////////////////////////////////7
void CreateUserDaemon::createInputSocket() {
	socklen_t size;
	SOCKADDR_IN addrin;
	socket_in = socket(PF_INET, SOCK_STREAM, 0);
	SeyconCommon::debug("Created socket: %d\n", (int) socket_in);
	size = sizeof addrin;
	if (socket_in < 0) {
		fprintf(stderr, "Error creating socket\n");
		return;
	}
	addrin.sin_family = AF_INET;
	if (debug)
		addrin.sin_addr.s_addr = inet_addr("0.0.0.0");
	else
		addrin.sin_addr.s_addr = inet_addr("127.0.0.1");
	addrin.sin_port = 0;
	if (bind(socket_in, (LPSOCKADDR) &addrin, sizeof(addrin)) != 0) {
		printf("Error linking socket\n");
		return;
	}
	if (getsockname(socket_in, (LPSOCKADDR) &addrin, &size) != 0) {
		fprintf(stderr, "Error obtaining socket name:%d \n", getErrorNumber());
		return;
	}
	publicSocket = ntohs(addrin.sin_port);
	if (listen(socket_in, 2) != 0) {
		fprintf(stderr, "Error listening port\n");
		return;
	}
	char achPort[10];
	sprintf (achPort, "%d", (int) publicSocket);
	SeyconCommon::writeProperty("createUserSocket", achPort);
	printf ("Listening on port %d\n", publicSocket);
}

struct ThreadParam {
public:
	ThreadParam(CreateUserDaemon *d, SOCKET s) {
		this->daemon = d;
		this->s = s;
	}
	CreateUserDaemon *daemon;
	SOCKET s;
};

//////////////////////////////////////////////////////////
#ifdef WIN32
static int WINAPI _s_handleConnection(LPVOID lpv)
#else
		static void* _s_handleConnection (void * lpv)
#endif
		{
	ThreadParam *p = (ThreadParam*) lpv;
	p->daemon->handleConnection(p->s);
	delete p;
#ifdef WIN32
	ExitThread(0);
	return 0;
#else
	pthread_exit (NULL);
	return NULL;
#endif
}

static int counter = 1;

static bool existsUser(const wchar_t* user) {
	LPBYTE buffer;

	DWORD result = NetUserGetInfo(NULL, // Local Host
			user, 2, // level
			&buffer);

	NetApiBufferFree(buffer);

	if (result == NERR_UserNotFound)
		return false;
	else
		return true;
}

static void updateExpireTime (const wchar_t* user) {
	USER_INFO_4* ui;

	// Registering as local user
	std::string localUsers;
	SeyconCommon::readProperty("localUsers", localUsers);
	if (localUsers.empty())
		localUsers = MZNC_wstrtostr(user);
	else
	{
		bool found = false;
		std::string szUser = MZNC_wstrtostr(user);
		Tokenizer tok(localUsers, " ,");
		while (tok.NextToken())
		{
			const std::string token = tok.GetToken();
			if (token == szUser)
			{
				found = true;
				break;
			}
		}
		if (! found)
		{
			localUsers += ", ";
			localUsers += szUser;
		}
	}
	SeyconCommon::writeProperty("localUsers", localUsers.c_str());

	DWORD result = NetUserGetInfo(NULL, // Local Host
			user, 4, // level
			(LPBYTE*)&ui);

	if (result == NERR_Success) {
		time_t t;
		time (&t);

		t += 30L * 24L * 60L * 60L; // 1 month

		ui -> usri4_acct_expires = t;

		DWORD parm_err;
		result = NetUserSetInfo(NULL, user, 4, (LPBYTE) ui, &parm_err);
		if (result == NERR_Success) {
			printf ("Set account expiration\n");
		} else {
			printf ("Error setting account expiration\n");
		}
		NetApiBufferFree(ui);
	} else {
		printf("CANNOT GET USER PROPERTIES!!!\n");
	}

}

static bool createUser(const wchar_t* user, const wchar_t* description, std::wstring pass) {
	printf ("Creating user %ls [%ls]: %ls\n", user, description, pass.c_str());
	USER_INFO_2 ui2;
	memset(&ui2, 0, sizeof ui2);
	ui2.usri2_name = (wchar_t*) user;
	ui2.usri2_password = (wchar_t*) pass.c_str();
	ui2.usri2_priv = USER_PRIV_USER;
	ui2.usri2_comment = (wchar_t*) L"Autogenerated by Soffid";
	ui2.usri2_flags = UF_NORMAL_ACCOUNT;
	ui2.usri2_full_name = (wchar_t*) description;
	ui2.usri2_acct_expires = TIMEQ_FOREVER;
	DWORD error;
	DWORD result = NetUserAdd(NULL, 2, (LPBYTE) &ui2, &error);
	if (result != NERR_Success) {
		const wchar_t * params[2] = { user, errorMessage(result) };
		ReportEventW(hEventLog, EVENTLOG_INFORMATION_TYPE, SHIRO_CATEGORY,
				SHIRO_LOCALERROR,
				NULL, 1, 0, (LPCWSTR*) &params, NULL);

		return false;
	}
	return true;
}

static bool createUser(const wchar_t* user, std::wstring pass) {
	return createUser(user, user, pass);
}


static std::wstring getFullName(const wchar_t* name) {
	SeyconService ss;
	SeyconResponse* r = ss.sendUrlMessage(L"/query/user/%ls?format=text", name);
	if (r == NULL) {
		printf("No response \n");
		return name;
	}
	else {
		std::string response = r->getResult();
		printf("Response %s\n", response.c_str());
		std::string token = r->getToken(0);
		printf("Token %s\n", token.c_str());
		if (token == "OK") {
			std::string fn = r->getToken(12);
			std::string ln = r->getToken(13);
			std::wstring fullName = MZNC_utf8towstr(fn.c_str()) + L" " + MZNC_utf8towstr(ln.c_str());
			delete r;
			return fullName;
		} else {
			delete r;
			return name;
		}
	}
}


static bool updateUser(const wchar_t* user, std::wstring pass) {
	LPBYTE buffer;
	DWORD error;
	DWORD result = NetUserGetInfo(NULL, // Local Host
			user, 2, // level
			&buffer);

	if (result != NERR_Success)
		return false;

	USER_INFO_2 *ui2 = (USER_INFO_2*) buffer;

	wchar_t *passStr = wcsdup (pass.c_str());
	ui2->usri2_password = passStr;
	ui2->usri2_flags = UF_NORMAL_ACCOUNT;

	result = NetUserSetInfo(NULL, user, 2, (LPBYTE) ui2, &error);

	NetApiBufferFree(buffer);

	memset (passStr, 0, pass.length());
	free (passStr);

	if (result != NERR_Success) {
		const wchar_t *params[2] = { user, errorMessage(result) };
		ReportEventW(hEventLog, EVENTLOG_ERROR_TYPE, SHIRO_CATEGORY,
				SHIRO_LOCALERROR, NULL, 2, 0, (LPCWSTR*) &params,
				NULL);
		return false;
	}
	return true;

}

void CreateUserDaemon::handleConnection(SOCKET s) {
	SOCKADDR_IN addrin;
	socklen_t size = sizeof addrin;

	if (getsockname(s, (LPSOCKADDR) &addrin, &size) != 0) {
		fprintf(stderr, "Error obtaining socket name:%d \n", getErrorNumber());
	} else {
		int publicSocket = ntohs(addrin.sin_port);
		const char *remote = inet_ntoa(addrin.sin_addr);

		printf ("Received connection from %s:%d\n", remote, publicSocket);
	}

	std::wstring line = readLine(s);

	printf("Got line %ls\n", line.c_str());

	int i = line.find(L" ");

	std::wstring cmd = i == std::wstring::npos ? line: line.substr(0, i);
	std::wstring params = i == std::wstring::npos ? L"": line.substr(i+1);
	if (cmd == L"LOCAL_USER")
		processLocalUser (s);

	if (cmd == L"VALIDATE")
		validatePassword(s, params);

	if (cmd == L"GET_CREDENTIALS")
		getCredentials(s, params);

	if (cmd == L"STORE_CREDENTIALS")
		storeCredentials(s, params);

	closesocket(s);
}

//////////////////////////////////////////////////////////
static int WINAPI _s_startLoop(LPVOID lpv) {
	ThreadParam *p = (ThreadParam*) lpv;
	p->daemon->run();
//	delete p;
	ExitThread(0);
	return 0;
}


void CreateUserDaemon::run() {
	SOCKADDR addr;
	SOCKET s;
	socklen_t len;

	while (!stop) {
		len = sizeof addr;
		s = accept(socket_in, &addr, &len);

		if (stop || s == (SOCKET) -1) {
#ifdef WIN32
			ExitThread(0);
#else
			pthread_exit (NULL);
#endif
		} else {
			ThreadParam *data = new ThreadParam(this, s);
#ifdef WIN32
			HANDLE hThread;
			hThread = CreateThread(NULL, 0,
					(LPTHREAD_START_ROUTINE) _s_handleConnection, (LPVOID) data,
					0, (LPDWORD) &hThread);
			CloseHandle(hThread);
#else
			pthread_t thread1;
			pthread_create( &thread1, NULL, _s_handleConnection, (void*) data);
#endif
		}
	}
}

//////////////////////////////////////////////////////////
int CreateUserDaemon::startDaemon() {
	WSADATA wsaData;
	WSAStartup(MAKEWORD(1, 1), &wsaData);

	createInputSocket();

	ThreadParam *data = new ThreadParam(this, socket_in);
	ThreadParam *data2 = new ThreadParam(this, socket_in);
	stop = false;
	DWORD dwThreadId, dwThreadId2;
	HANDLE hThreadListen = CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE) _s_startLoop, (LPVOID) data, 0,
			&dwThreadId);

	CloseHandle(hThreadListen);

	return publicSocket;
}

//////////////////////////////////////////////////////////
void CreateUserDaemon::stopDaemon() {
	stop = true;
#ifdef WIN32
	closesocket(socket_in);
#else
	close (socket_in);
#endif
}

void CreateUserDaemon::writeLine(SOCKET socket, const std::wstring &line) {
	send(socket, (const char*) line.c_str(), line.length() * sizeof (wchar_t), 0);
	send(socket, (const char*) L"\n", sizeof (wchar_t), 0);
}

std::wstring CreateUserDaemon::readLine(SOCKET socket) {
	std::wstring s;
	wchar_t wch;
	while (recv(socket, (char*) &wch, sizeof wch, 0) >= sizeof wch) {
		if (wch == L'\n')
			return s;
		s += wch;
	}
	return s;
}

void CreateUserDaemon::processLocalUser(SOCKET socket) {
	std::string maxString;
	std::string enabled;
	SeyconCommon::readProperty("maxLocalAccounts", maxString);
	SeyconCommon::readProperty("enableLocalAccounts", enabled);
	if (enabled == "true")
	{
		int max = 30;
		sscanf(maxString.c_str(), " %d", &max);
		if (counter > max)
			counter = 1;
		int next = counter++;

		wchar_t userName[30];
		swprintf(userName, L"ShiroUser_%d", next);

		const wchar_t *params = userName;
		ReportEventW(hEventLog, EVENTLOG_INFORMATION_TYPE, SHIRO_CATEGORY,
				SHIRO_CREATE_USER,
				NULL, 1, 0, (LPCWSTR*) &params, NULL);

		std::wstring newPass = generatePassword();
		if (existsUser(userName))
			updateUser(userName, newPass.c_str());
		else
			createUser(userName, newPass.c_str());

		Sleep ( 1000 ); // Wait for a second

		std::wstring response = userName;
		response += L'\n';
		response += newPass;
		int len = sizeof(wchar_t) * (response.length() + 1);
		send(socket, (const char*) response.c_str(), len, 0);
		printf ("Response:\n%ls\n", response.c_str());
	}
}


void CreateUserDaemon::validatePassword(SOCKET socket,
		const std::wstring &params) {
	std::string type;
	SeyconCommon::readProperty("LoginType", type);
	if (type != "soffid") {
		writeLine(socket, L"ERROR UNAUTHORIZED");
	} else {
		int i = params.find(L" ");
		if ( i == std::wstring::npos)
		{
			writeLine(socket, L"ERROR");
			return;
		} else {
			SeyconSession session;
			ShiroSeyconDialog dlg;

			session.updateConfiguration();
			session.setDialog(&dlg);
			if (debug)
				SeyconCommon::setDebugLevel(2);

			std::wstring user = params.substr(0, i);
			std::wstring pass = params.substr(i+1);
			std::wstring newpass;

			i = pass.find(L" ");
			if (i != std::wstring::npos) {
				dlg.newPass = newpass = SeyconCommon::urlDecode(MZNC_wstrtostr(pass.substr(i+1).c_str()).c_str());
				pass = SeyconCommon::urlDecode(MZNC_wstrtostr(pass.substr(0, i).c_str()).c_str());
				printf ("Current password = %ls\n", pass.c_str());
				printf ("New password = %ls\n", dlg.newPass.c_str());
			} else {
				printf ("Password = %ls\n", pass.c_str());
				newpass = pass = SeyconCommon::urlDecode(MZNC_wstrtostr(pass.c_str()).c_str());
			}
			std::string user2 = MZNC_wstrtostr(user.c_str());


			int result = session.passwordSessionPrepare(user2.c_str(), pass.c_str());
			if (result == LOGIN_DENIED || result == LOGIN_UNKNOWNUSER) {
				if (dlg.needsNewPassword)
					writeLine(socket, L"EXPIRED");
				else {
					std::wstring msg = L"DENIED ";
					msg +=  MZNC_strtowstr(session.getErrorMessage());
					writeLine(socket, msg.c_str());
				}
			} else if (result == LOGIN_SUCCESS) {
				if (existsUser(user.c_str()))
					updateUser(user.c_str(), newpass.c_str());
				else {
					std::wstring fullName = getFullName(user.c_str());
					printf("Full name = %ls\n", fullName.c_str());
					printf("Creating new user\n");
					if (!createUser(user.c_str(), fullName.c_str(), newpass.c_str())) {
						writeLine(socket, L"ERROR CREATE_USER");
					}
				}
				updateExpireTime(user.c_str());
				writeLine(socket, L"SUCCESS");
			} else if (result == LOGIN_ERROR) {
				if (dlg.needsNewPassword)
					writeLine(socket, L"EXPIRED");
				else
					writeLine(socket, L"ERROR REMOTE");
			}
		}
	}
}


static std::map<std::wstring,std::wstring> credentials;

void CreateUserDaemon::getCredentials(SOCKET socket,
		const std::wstring &params) {
	std::map<std::wstring,std::wstring>::iterator i = credentials.find(params);
	if (i != credentials.end()) {
		printf("Found [%ls]\n", params.c_str());
		std::wstring r = L"OK ";
		r += i->second.c_str();
		writeLine(socket, r);
		credentials.erase(i);
	}
	else
	{
		printf("Cannot find [%ls]\n", params.c_str());
		writeLine(socket, L"ERROR");
	}
}


static void runScript(const std::string entry, bool asRoot) {
	SeyconService service;
	std::wstring le = service.escapeString(entry.c_str());
	SeyconResponse *response = service.sendUrlMessage(
			L"/getapplication?codi=%ls", le.c_str());
	if (response != NULL)
	{
		std::string status = response->getToken(0);
		if (status == "OK")
		{
			std::string type = response->getToken(1);
			std::string content = response->getUtf8Tail(2);
			if (type == "MZN")
			{
				SeyconCommon::debug("Executing ROOT script:\n"
						"========================================\n"
						"%s\n"
						"=====================================\n", content.c_str());
				std::string exception;
				if (!MZNEvaluateJS(content.c_str(), exception))
				{
						SeyconCommon::warn("Error executing root script: %s\n",
							exception.c_str());
				}
				SeyconCommon::debug("Executed root script");
			}
			else
			{
				SeyconCommon::debug("SHIRO: Application type not supported for logon: %s",
						type.c_str());
			}
		}
		else
		{
			std::string details = response->getToken(1);
			SeyconCommon::debug("SHIRO: Cannot open application with code %s\n%s: %s",
					entry.c_str(), status.c_str(), details.c_str());
		}
		delete response;
	}
	else
	{
		SeyconCommon::warn("Cannot get application %s", entry.c_str());
	}

}

void CreateUserDaemon::storeCredentials(SOCKET socket,
		const std::wstring &params) {
	SeyconCommon::debug("SHIRO: Storing user credentials");
	int i = params.find(L" ");
	if ( i == std::wstring::npos)
	{
		writeLine(socket, L"ERROR");
		return;
	} else {
		SeyconCommon::debug("SHIRO: Storing user credentials");
		std::wstring user = params.substr(0, i);
		std::wstring pass = params.substr(i+1);
		printf("Pass %ls\n", pass.c_str());
		std::wstring decoded = SeyconCommon::urlDecode(MZNC_wstrtostr(pass.c_str()).c_str());
		printf("Storing %ls\n", decoded.c_str());
		credentials [ user ] = decoded;
		writeLine(socket, L"OK");

		SeyconCommon::debug("SHIRO: Updating logonEntryRoot\n");
		SeyconCommon::updateConfig("LogonEntryRoot");
		std::string logonEntryRoot;
		SeyconCommon::readProperty("LogonEntryRoot", logonEntryRoot);
		SeyconSession session;
		if (logonEntryRoot.size() > 0) {
			SeyconCommon::debug("SHIRO: Executing [%s]\n", logonEntryRoot.c_str());
			runScript(logonEntryRoot, true);
		}
	}
}
