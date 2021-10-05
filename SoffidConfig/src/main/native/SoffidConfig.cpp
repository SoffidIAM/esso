# include <wchar.h>
# include <windows.h>

# include "winwlx.h"
# include "SeyconServer.h"
# include "SoffidEssoManager.h"
# include "Utils.h"
# include "resource.h"
# include <MZNcompat.h>

HINSTANCE hSoffidConfigDll;		// Soffid configuration tool dll handler
HWND hWindDlg;						// Window dialog handler
SoffidEssoManager manager;		// Soffid esso manager

/** @brief Disable configuration of GINA
 *
 * Method that disable the configuration parameters for GINA when the OS system
 * is not Windows XP.
 */
void DisableConfigGINA ()
{
	// Check Windows XP OS
	if (!SoffidEssoManager::IsWindowsXP())
	{
		EnableWindow(GetDlgItem(hWindDlg, IDC_GROUPBOX_GINA), FALSE);
		EnableWindow(GetDlgItem(hWindDlg, IDC_MSG_LOGIN_GINA_INFO), FALSE);
		EnableWindow(GetDlgItem(hWindDlg, IDC_CHECKBOX_SOFFID_GINA), FALSE);
	}
}


static void doDebug (const char*msg, int line) {
	char ach[2000];
	sprintf(ach, "%s [ %d ]", msg, line);
	MessageBox(NULL, ach, "DEBUG", MB_OK | MB_ICONINFORMATION);
}

#define DEBUG(x) doDebug(x,__LINE__)

/** @brief Retrieve certificate
 *
 * Method that implements the functionality for obtain the certificate from server
 * @return
 * <ul>
 * 		<li>
 * 			@code TRUE @endcode
 * 			If the certificate was retrieved correctly.
 * 		</li>
 *
 * 		<li>
 * 			@code FALSE @endcode
 * 			If the certificate was not retrieved correctly.
 * 		</li>
 * 	</ul>
 */
bool RetrieveCertificate (SoffidEssoManager &manager)
{
	bool retrieveOK = true;	// Retrieve certificate status

//	DEBUG("start retrieve");
	// Save URL Server
	std::wstring wServerUrl = MZNC_strtowstr(manager.getServerUrl().c_str());
	size_t size;
	std::string url = manager.GetFormatedURLServer();
//	DEBUG(url.c_str());
	// Check obtain certificate
	if (manager.SaveURLServer(url.c_str()))
	{
		MessageBox(NULL, Utils::LoadResourcesString(1).c_str(),
				Utils::LoadResourcesString(1000).c_str(), MB_OK | MB_ICONINFORMATION);
	}

	else
	{
		MessageBox(NULL, Utils::LoadResourcesString(2).c_str(),
				Utils::LoadResourcesString(1001).c_str(), MB_OK | MB_ICONERROR);

		retrieveOK = false;
	}

	return retrieveOK;
}


/**
 *
 */

bool getCert (const char* urlTemp)
{
	std::string headProtocol = "https://";	// Connection header protocol
	std::string serverURL;							// Server URL
	int serverPort;									// Server port
	int pos;								// Delimiter position
	int headPos = 0;							// Header position
	serverURL = urlTemp;

	headPos = serverURL.find(headProtocol);

	if (headPos != std::string::npos)
	{
		serverURL.erase(serverURL.find(headProtocol),
				headProtocol.size());
	}

	pos = serverURL.find(":");

	// Check port found
	if (pos != std::string::npos)
	{
		serverPort = atoi(
				serverURL.substr((pos + 1), serverURL.size()).c_str());
		serverURL = serverURL.substr(0, pos);

		manager.setServerUrl(serverURL);
		manager.setServerPort(serverPort);
	}

	return RetrieveCertificate(manager);
}

/** @brief Save server URL
 *
 * Method that implements the functionality for establish the URL server values on
 * configuration manager.
 * @return
 * <ul>
 * 		<li>
 * 			@code TRUE @endcode
 * 			If the URL parameters was saved correctly.
 * 		</li>
 *
 * 		<li>
 * 			@code FALSE @endcode
 * 			If the URL parameters was not saved correctly.
 * 		</li>
 * 	</ul>
 */
bool SaveServerURL (SoffidEssoManager &pManager)
{
	std::string serverURL;					// Server URL in configuration tool
	std::string headProtocol = "https://";	// Connection header protocol
	char urlTemp[200];						// Temporal URL obtained from dialog
	int serverPort;										// Server port
	bool saveServerURLOK = true;		// Save URL status
	int pos = 0;									// Position of find URL port

	GetDlgItemText(hWindDlg, IDC_EDIT_SERVER_URL, urlTemp, 200);

	serverURL = urlTemp;

	// Check server URL changed
	if (serverURL.compare(pManager.GetFormatedURLServer()) != 0)
	{
		serverURL.erase(serverURL.find(headProtocol), headProtocol.size());

		pos = serverURL.find(":");

		// Check port found
		if (pos != std::string::npos)
		{
			serverPort = atoi(serverURL.substr((pos + 1), serverURL.size()).c_str());
			serverURL = serverURL.substr(0, pos);

			pManager.setServerUrl(serverURL);
			pManager.setServerPort(serverPort);
		}

		if (!getCert (urlTemp))
			return false;
	}

	return saveServerURLOK;
}

// Save configuration GINA dll path
void SaveGinaDll (SoffidEssoManager &pManager)
{
	bool originalGina = true;	// Original GINA activated
	std::string previous;			// Previous GINA

	// Check Windows XP OS
	if (SoffidEssoManager::IsWindowsXP())
	{
		if (IsDlgButtonChecked(hWindDlg, IDC_CHECKBOX_SOFFID_GINA))
		{
			originalGina = false;
		}

		// Check configuration changed
		if (originalGina != pManager.getOriginalGina())
		{
			previous = pManager.getGinaDll();

			pManager.setGinaDll(pManager.getPreviousGinaDll());
			pManager.setPreviousGinaDll(previous);
		}
	}
}

// Save force login on startup
void SaveForceStartupLogin (SoffidEssoManager &pManager)
{
	std::string forceStartup = "false";	// Force startup logon activated

	// Check force startup login checked
	if (IsDlgButtonChecked(hWindDlg, IDC_CHECKBOX_FORCE_LOGIN))
	{
		forceStartup = "true";
	}

	// Check configuration changed
	if (forceStartup.compare(pManager.getForceLogin()) != 0)
	{
		pManager.setForceLogin(forceStartup);
	}
}

// Save enable close session by user
void SaveEnableCloseSession (SoffidEssoManager &pManager)
{
	std::string enableCloseSession = "false";	// Enable close session activated
	std::string enableSharedSession = "false";	// Enable close session activated

	// Check force startup login checked
	if (IsDlgButtonChecked(hWindDlg, IDC_CHECKBOX_CLOSE_SESSION))
	{
		enableCloseSession = "true";
	}

	if (IsDlgButtonChecked(hWindDlg, IDC_CHECKBOX_SHARED_SESSION))
	{
		enableSharedSession = "true";
	}

	// Check configuration changed
	if (enableCloseSession.compare(pManager.getEnableCloseSession())!= 0)
	{
		pManager.setEnableCloseSession(enableCloseSession);
	}
	// Check configuration changed
	if (enableSharedSession.compare(pManager.getEnableSharedSession())!= 0)
	{
		pManager.setEnableSharedSession(enableSharedSession);
	}
}

// Save login type
void SaveLoginType (SoffidEssoManager &pManager)
{
	std::string loginType;	// Login type activated

	// Check login 'kerberos'
	if (IsDlgButtonChecked(hWindDlg, IDC_RADIO_KERBEROS))
	{
		loginType = "kerberos";
	}

	// Check login 'manual'
	if (IsDlgButtonChecked(hWindDlg, IDC_RADIO_MANUAL))
	{
		loginType = "manual";
	}

	// Check login 'both'
	if (IsDlgButtonChecked(hWindDlg, IDC_RADIO_BOTH))
	{
		loginType = "both";
	}

	if (IsDlgButtonChecked(hWindDlg, IDC_RADIO_SOFFID))
	{
		loginType = "soffid";
	}

	// Check configuration changed
	if (loginType.compare(pManager.getLoginType()) != 0)
	{
		pManager.setLoginType(loginType);
	}
}

// Save configuration
bool SaveConfiguration (SoffidEssoManager &pManager)
{
	bool configOK = true;					// Configuration saved status
	std::string forceStartupLog;			// Force startup login
	std::string enableCloseSession;		// Enable close session activated
	std::string loginType;						// Login type activated

	// Check parameters of configuration
	SaveGinaDll(pManager);
	SaveForceStartupLogin(pManager);
	SaveEnableCloseSession(pManager);
	SaveLoginType(pManager);
	if (!SaveServerURL(pManager))
		return false;

	// Save changes and close dialog
	if (pManager.SaveConfiguration())
	{
		EndDialog(hWindDlg, 0);
	}

	else
	{
		configOK = false;
	}

	return configOK;
}

/** @brief Set configuration parameters in dialog window
 *
 * Method that marks the ESSO configuration param on dialog window
 * of Configuration tool.
 */
void SetConfigOnDlg (SoffidEssoManager manager)
{
	// Use original GINA
	if (SoffidEssoManager::IsWindowsXP()
			&& (manager.getGinaDll() == SoffidEssoManager::MazingerGinaPath()))
	{
		CheckDlgButton(hWindDlg, IDC_CHECKBOX_SOFFID_GINA, BST_CHECKED);
	}

	else
	{
		CheckDlgButton(hWindDlg, IDC_CHECKBOX_SOFFID_GINA, BST_UNCHECKED);
	}

	// Force startup login
	if (manager.getForceLogin().compare("true") == 0)
	{
		CheckDlgButton(hWindDlg, IDC_CHECKBOX_FORCE_LOGIN, BST_CHECKED);
	}

	else
	{
		CheckDlgButton(hWindDlg, IDC_CHECKBOX_FORCE_LOGIN, BST_UNCHECKED);
	}

	// Enable close ESSO session
	if (manager.getEnableCloseSession().compare("true") == 0)
	{
		CheckDlgButton(hWindDlg, IDC_CHECKBOX_CLOSE_SESSION, BST_CHECKED);
	}

	else
	{
		CheckDlgButton(hWindDlg, IDC_CHECKBOX_CLOSE_SESSION, BST_UNCHECKED);
	}

	// Enable Shared session
	if (manager.getEnableSharedSession().compare("true") == 0)
	{
		CheckDlgButton(hWindDlg, IDC_CHECKBOX_SHARED_SESSION, BST_CHECKED);
	}

	else
	{
		CheckDlgButton(hWindDlg, IDC_CHECKBOX_SHARED_SESSION, BST_UNCHECKED);
	}


	// ESSO login type
	// Check 'kerberos' login
	if (manager.getLoginType().compare("kerberos") == 0)
	{
		CheckDlgButton(hWindDlg, IDC_RADIO_KERBEROS, BST_CHECKED);
	}

	// Check manual login
	else if (manager.getLoginType().compare("manual") == 0)
	{
		CheckDlgButton(hWindDlg, IDC_RADIO_MANUAL, BST_CHECKED);
	}

	else if (manager.getLoginType().compare("soffid") == 0)
	{
		CheckDlgButton(hWindDlg, IDC_RADIO_SOFFID, BST_CHECKED);
	}

	else
	{
		CheckDlgButton(hWindDlg, IDC_RADIO_BOTH, BST_CHECKED);
	}

	// ESSO server URL
	SetDlgItemText(hWindDlg, IDC_EDIT_SERVER_URL,
			(LPCSTR) manager.GetFormatedURLServer().c_str());
}


/** @brief Configuration tool process
 *
 * Method that defines the Soffid ESSO configuration tool process functionality.
 * @param hwndDlg Handle to dialog box
 * @param uMsg Message
 * @param wParam First message parameter
 * @param lParam Second message parameter
 */
INT_PTR CALLBACK SoffidConfigProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HKEY hKey;	// Handle to open registry key

	hWindDlg = hwndDlg;

	switch (uMsg)
	{
		case WM_INITDIALOG:	// Functionality when dialog starts

			// Check not admin user logged
			if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
					(LPCSTR) SoffidEssoManager::DEF_SESSION_MNG.c_str(), 0,
					SoffidEssoManager::Wow64Key(KEY_ALL_ACCESS), &hKey) != ERROR_SUCCESS)
			{
				MessageBox(NULL, Utils::LoadResourcesString(6).c_str(),
						Utils::LoadResourcesString(1001).c_str(), MB_OK | MB_ICONERROR);

				EndDialog(hwndDlg, 0);

				return TRUE;
			}

			DisableConfigGINA();

			// Check configuration loaded correctly
			if (!manager.LoadConfiguration())
			{
				MessageBox(NULL, Utils::LoadResourcesString(3).c_str(),
						Utils::LoadResourcesString(1001).c_str(), MB_OK | MB_ICONERROR);
			}

			SetConfigOnDlg(manager);

		break;

		case WM_COMMAND:	// Check command in dialog
			switch (wParam)
			{
				case IDOK:	// Button OK

					// Save changes confirmation
					if (true || MessageBox(NULL, Utils::LoadResourcesString(7).c_str(),
							Utils::LoadResourcesString(1002).c_str(),
							MB_YESNO | MB_ICONQUESTION) == IDYES)
					{
						// Save changes and close dialog
						if (SaveConfiguration(manager))
						{
							EndDialog(hwndDlg, 0);
						}

						else
						{
							MessageBox(NULL, Utils::LoadResourcesString(5).c_str(),
									Utils::LoadResourcesString(1001).c_str(),
									MB_OK | MB_ICONERROR);
						}
					}

					return TRUE;

				break;

				case IDCANCEL:	// Button cancel

					EndDialog(hwndDlg, 0);
				break;

				case IDC_BUTTON_RETRIEVE_CERT:	// Retrieve certificate

					char urlTemp[200];					// URL server on dialog

					GetDlgItemText(hWindDlg, IDC_EDIT_SERVER_URL, urlTemp, 200);

					getCert (urlTemp);

				break;
			}
		break;

		case WM_CLOSE:	// Close dialog

			EndDialog(hwndDlg, 0);
		break;
	}

	return FALSE;
}

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hInst2, LPSTR cmdLine, int nShow)
{
	hSoffidConfigDll = hInstance;
	int result = DialogBox(hInstance, MAKEINTRESOURCE (IDD_DIALOG_ESSO_CONFIG),
			NULL, (DLGPROC)SoffidConfigProc);

	// Dialog error
	if (result == -1)
	{
		std::string errorMessage;
		Utils::getErrorMessage(errorMessage, result);

		MessageBox(NULL, Utils::LoadResourcesString(1001).c_str(), errorMessage.c_str(),
				MB_OK | MB_ICONERROR);
	}
}
