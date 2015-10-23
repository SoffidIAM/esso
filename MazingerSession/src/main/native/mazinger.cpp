#ifdef WIN32
#include <windows.h>
#include <io.h>
#else
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <wchar.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <ssoclient.h>
#include "mazinger.h"
#include "httpHandler.h"
#include "sessioncommon.h"
#include <MazingerHook.h>
#include "ssodaemon.h"
#include <MZNcompat.h>
#include <MazingerInternal.h>
#include <ScriptDialog.h>
#include <MazingerEnv.h>

void SeyconSession::parseAndStoreSecrets (SeyconResponse *resp)
{
	wchar_t *wchComposedSecrets = (wchar_t*) malloc(resp->getSize() * sizeof (wchar_t));
	if (wchComposedSecrets == NULL)
		return;
	long last = 0;
	int secretNumber = 1;
	std::wstring secret;
	resp->getToken(secretNumber, secret);
	while (secret.size() > 0)
	{
		std::wstring secret2 = SeyconCommon::urlDecode (MZNC_wstrtoutf8(secret.c_str()).c_str());
		if (last + secret2.size() > SECRETS_BUFFER_SIZE-2)
		{
			if (m_dialog != NULL)
				m_dialog -> notify("Cannot handle so many secrets. Internal buffer size is too small");
			wchComposedSecrets[last] = wchComposedSecrets[last+1] = L'\0';
			break;
		}
		wcscpy(&wchComposedSecrets[last], secret2.c_str());
		last += secret2.size() + 1;
		SeyconCommon::wipe(secret2);
		SeyconCommon::wipe(secret);
		secretNumber++;
		resp->getToken(secretNumber, secret);
	}
	MZNSetSecrets(MZNC_getUserName(), wchComposedSecrets);
	memset(wchComposedSecrets, sizeof wchComposedSecrets, 0);
	free (wchComposedSecrets);
}

void SeyconSession::startMazinger (SeyconResponse *resp, const char* configFile)
{
	MZNSetDebugLevel(MZNC_getUserName(), 0);
	if (configFile != NULL && configFile[0] != '\0')
	{
		std::wstring wConfigFile = MZNC_strtowstr(configFile);
		SeyconCommon::debug("Loading %ls for %s", wConfigFile.c_str(), MZNC_getUserName());
		if (!MZNLoadConfiguration(MZNC_getUserName(), wConfigFile.c_str()))
		{
			SeyconCommon::warn("Unable to load file %s\n", configFile);
		}
		SeyconCommon::debug("Configuration loaded");
	}
	else
	{
		MZNLoadConfiguration(MZNC_getUserName(), NULL);
	}
	parseAndStoreSecrets(resp);
	SeyconCommon::debug("Starting mazinger for %s", MZNC_getUserName());
	MZNStart(MZNC_getUserName());
	SeyconCommon::debug("Mazinger started");
	MZNCheckPasswords(MZNC_getUserName());
	SeyconCommon::debug("Passwords checked");
	// Descargar script inicial
	SeyconCommon::updateConfig("LogonEntry");
	std::string logonEntry;
	SeyconCommon::readProperty("LogonEntry", logonEntry);
	if (logonEntry.size() > 0)
	{
		SeyconService service;
		std::wstring le = service.escapeString(logonEntry.c_str());
		SeyconResponse *response = service.sendUrlMessage(
				L"/getapplication?user=%hs&key=%hs&codi=%ls", soffidUser.c_str(),
				sessionKey.c_str(), le.c_str());
		if (response != NULL)
		{
			std::string status = response->getToken(0);
			if (status == "OK")
			{
				std::string type = response->getToken(1);
				std::string content = response->getUtf8Tail(2);
				if (type == "MZN")
				{
					SeyconCommon::debug("Executing script:\n"
							"========================================\n"
							"%s\n"
							"=====================================\n", content.c_str());
					std::string exception;
					if (!MZNEvaluateJS(content.c_str(), exception))
					{
						ScriptDialog::getScriptDialog()->alert(exception.c_str());
						SeyconCommon::debug("Error executing script: %s\n",
								exception.c_str());
					}
				}
				else
				{
					SeyconCommon::warn("Application type not supported for logon: %s",
							type.c_str());
				}
			}
			else
			{
				std::string details = response->getToken(1);
				SeyconCommon::warn("Cannot open application with code %s\n%s: %s",
						logonEntry.c_str(), status.c_str(), details.c_str());
			}
			delete response;
		}
		else
		{
			SeyconCommon::warn("Cannot get application %s", logonEntry.c_str());
		}
		saveOfflineScript();
	}
}

void SeyconSession::downloadMazingerConfig (std::string &configFile)
{
	SeyconService service;

    std::string version;
    SeyconCommon::readProperty("soffid.mazinger.version", version);
    if (version.empty())
    	version = "3";

    SeyconResponse *resp = service.sendUrlMessage(
			L"/getmazingerconfig?user=%hs&version=%hs", soffidUser.c_str(), version.c_str());

	configFile.clear();
	// Obtener las reglas
	if (resp != NULL)
	{
		std::string dir;
#ifdef WIN32
		LPCSTR profile = getenv("APPDATA");
		if (profile == NULL)
			profile = getenv("TMP");

		configFile.assign(profile);
		configFile.append("\\SoffidESSO");
		CreateDirectoryA(configFile.c_str(), NULL);
		configFile.append("\\config.mzn");
#else
		struct passwd *pwd = getpwnam (user.c_str());
		if (pwd == NULL)
		{
			configFile.assign("/var/tmp/mazinger");
		}
		else
		{
			configFile.assign (pwd->pw_dir);
			configFile.append ("/.config");
			mkdir (configFile.c_str(), 0755);
			chown(configFile.c_str(), pwd->pw_uid, pwd->pw_gid);
			configFile.append ("/mazinger");
		}
		mkdir (configFile.c_str(), 0777);
		if (pwd != NULL)
		chown(configFile.c_str(), pwd->pw_uid, pwd->pw_gid);
		configFile.append ("/config.");
		configFile.append (user.c_str());
		if (pwd != NULL)
		chown(configFile.c_str(), pwd->pw_uid, pwd->pw_gid);
#endif
		MZNSendDebugMessageA("Creating file %s", configFile.c_str());
		FILE *f = fopen(configFile.c_str(), "wb");
		if (f == NULL)
		{
			SeyconCommon::warn("Cannot create file %s", configFile.c_str());
			MZNSendDebugMessageA("Cannot create file %s", configFile.c_str());
		}
		else
		{
			fwrite(resp->getResult(), 1, resp->getSize(), f);
			fclose(f);
		}
		delete resp;
	} else {
		SeyconCommon::warn("Cannot get mazinger config from server");
	}
}

ServiceIteratorResult SeyconSession::launchMazinger (const char* wchHostName, int dwPort,
		const char* lpszServlet)
{
	SeyconService service;
	std::wstring wSession;
	ServiceIteratorResult result = SIR_ERROR;

	SeyconCommon::debug("Launching Mazinger for user %s", soffidUser.c_str());
	SeyconResponse *resp = service.sendUrlMessage(wchHostName, dwPort,
			L"/%hs?action=getSecrets&challengeId=%hs&encode=true", lpszServlet, sessionKey.c_str());
	// Parse results
	if (resp != NULL)
	{
		std::string status;
		resp->getToken(0, status);
		if (status == "OK")
		{
			// Generar el archivo de configuración
			std::string configFile ("");
			downloadMazingerConfig(configFile);
			if (configFile.length() > 0)
				startMazinger(resp, configFile.c_str());
			result = SIR_SUCCESS;
		}
		delete resp;
	}
	return result;

}

#define  MAX_SECRET_CHAR  62
static char intToChar (int n)
{
	int d = n / MAX_SECRET_CHAR;
	int r = n - d * MAX_SECRET_CHAR;
	if (r < 0)
		r += MAX_SECRET_CHAR;
	if (r < 10)
		return (char) (r + '0');
	else if (n < 36)
		return (char) (r + 'a' - 10);
	else
		return (char) (r + 'A' - 36);
}

static int charToInt (char ch)
{
	if (ch >= '0' && ch <= '9')
		return (int) ch - '0';
	else if (ch >= 'a' && ch <= 'z')
		return (int) ch - 'a' + 10;
	else
		return (int) ch - 'A' + 36;
}

void SeyconSession::renewSecrets (const char* achNewKey)
{
	int i;

	SeyconCommon::debug("Change credential notification received");

	char achNewSessionKey[512];
	const char *oldSession = sessionKey.c_str();
	for (i = 0; oldSession[i]; i++)
	{
		char ch1 = (char) oldSession[i];
		char ch2 = (char) achNewKey[i];
		int dif = charToInt(ch2) + charToInt(ch1);
		if (dif > MAX_SECRET_CHAR)
			dif -= MAX_SECRET_CHAR;
		achNewSessionKey[i] = intToChar(dif);

	}
	achNewSessionKey[i] = '\0';

	MZNC_waitMutex();
	int pending = newKeys.size();
	newKeys.push_back(std::string(achNewSessionKey));
	MZNC_endMutex();

	if (pending == 0)
	{
		bool repeat;
		do
		{
			repeat = true;

			MZNC_waitMutex();
			int size1 = newKeys.size();
			MZNC_endMutex();
#ifdef WIN32
			Sleep(2000);
#else
			sleep(2);
#endif
			MZNC_waitMutex();
			int size2 = newKeys.size();
			// Si no hi ha hagut canvis en els dos darrers segons, consultar l'estat dels secrets
			if (size1 == size2)
			{
				MZNC_waitMutex();
				std::string newKey = newKeys.back();
				newKeys.pop_back();
				if (newKeys.size() == 0)
					repeat = false;

				SeyconService service;
				SeyconResponse *resp = service.sendUrlMessage(
						L"/getSecrets?user=%hs&key=%hs&key2=%hs&encode=true", soffidUser.c_str(),
						oldSession, newKey.c_str());
				// Parse results
				if (resp != NULL)
				{
					std::string status;
					resp->getToken(0, status);
					if (status == "OK")
					{
						parseAndStoreSecrets(resp);
						SeyconCommon::debug("Credentials updated", newKey.c_str());
						if (repeat)
						{
							MZNC_waitMutex();
							newKeys.clear();
							MZNC_endMutex();
							repeat = false;
						}
					}
					delete resp;
				}
			}
			MZNC_endMutex();
		} while (repeat);
	}
}

void SeyconSession::updateMazingerConfig ()
{
	SeyconCommon::info("Updating rules for user %s\n", soffidUser.c_str());
	MZNSendDebugMessage("Updating rules for user %s", soffidUser.c_str());

	std::string configFile;
	downloadMazingerConfig(configFile);

	if (configFile.size() > 0)
	{
		std::wstring wConfigFile = MZNC_strtowstr(configFile.c_str());
		if (!MZNLoadConfiguration(MZNC_getUserName(), wConfigFile.c_str()))
		{
			SeyconCommon::warn("Unable to load file %s\n", configFile.c_str());
		}
		else
		{
			SeyconCommon::debug("Loaded file %s\n", configFile.c_str());
		}
	}
}

#ifdef WIN32
static std::string getOfflineScript ()
{
	std::string offlineScript;
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion",
			0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		static char achPath[4096] = "";
		DWORD dwType;
		DWORD size = -150 + sizeof achPath;
		size = sizeof achPath;
		RegQueryValueEx(hKey, "ProgramFilesDir", NULL, &dwType, (LPBYTE) achPath, &size);
		RegCloseKey(hKey);
		offlineScript = achPath;
		offlineScript += "\\SoffidESSO";
	}
	else
	{
		offlineScript = "C:\\Program Files\\SoffidESSO";
	}
	offlineScript += "\\cache\\offline.mzn";
	return offlineScript;
}
#else
static std::string getOfflineScript()
{
	return std::string("/etc/mazinger/offline.mzn");
}
#endif

void SeyconSession::saveOfflineScript ()
{

	SeyconCommon::updateConfig("OfflineEntry");
	std::string entry;
	SeyconCommon::readProperty("OfflineEntry", entry);
	if (entry.size() == 0)
		entry = "offline";

	SeyconService service;
	std::wstring le = service.escapeString(entry.c_str());
	SeyconResponse *response = service.sendUrlMessage(
			L"/getapplication?user=%hs&key=%hs&codi=%ls", soffidUser.c_str(),
			sessionKey.c_str(), le.c_str());
	if (response != NULL)
	{
		std::string status = response->getToken(0);
		if (status == "OK")
		{
			std::string type = response->getToken(1);
			std::string content = response->getUtf8Tail(2);
			if (type == "MZN")
			{
				std::string os = getOfflineScript();
				FILE *f = fopen(os.c_str(), "wb");
				if (f != NULL)
				{
					fwrite(content.c_str(), 1, content.size(), f);
					fclose(f);
				}
				else
				{
					SeyconCommon::warn("Cannot create file: %s", os.c_str());
				}
			}
			else
			{
				SeyconCommon::warn("Application type not supported for logon: %s",
						type.c_str());
			}
		}
		else
		{
			SeyconCommon::warn("Cannot open application with code [%s]", entry.c_str());
		}
		delete response;
	}
}

void SeyconSession::executeOfflineScript ()
{
	std::string os = getOfflineScript();
	FILE *f = fopen(os.c_str(), "rb");
	if (f != NULL)
	{
		std::string content;
		do
		{
			char b[1025];
			int read = fread(b, 1, 1024, f);
			if (read <= 0)
				break;
			b[read] = '\0';
			content += b;
		} while (true);
		fclose(f);
		std::string exception;
		if (!MZNEvaluateJS(content.c_str(), exception))
		{
			ScriptDialog::getScriptDialog()->alert(exception.c_str());
			SeyconCommon::debug("Error executing script: %s\n", exception.c_str());
		}
	}
}
