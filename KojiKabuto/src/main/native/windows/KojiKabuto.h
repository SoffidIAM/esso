/*
 * KojiKabuto.h
 *
 *  Created on: Feb 1, 2013
 *      Author: areina
 */

#ifndef KOJIKABUTO_H_
#define KOJIKABUTO_H_

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <shellapi.h>
#include <ssoclient.h>
#include "resource.h"
#include <MazingerHook.h>
#include <KojiHook.h>
#include <time.h>
#include <MZNcompat.h>

# include "KojiDialog.h"
# include "loginDialog.h"
# include "Utils.h"

class KojiKabuto
{
	public:

		static void enableTaskManager(DWORD dwEnable);
		static int runProgram (char *achString, char *achDir, int bWait);
		static void runScript (LPCSTR lpszScript);

		static void consumeMessages ();
		static void createProgressWindow (HINSTANCE hInstance);
		static void destroyProgressWindow ();
		static void displayProgressMessage(LPCSTR achString);
		static BOOL mountScriptDrive ();
		static void umountScriptDrive ();
		static void writeIntegerProperty ( HKEY hKey, const char *name, int value);
		static int readIntegerProperty ( HKEY hKey, const char *name);
		static bool startDisabled ();
		static void checkScreenSaver ();

		/** @brief Login error process
		 *
		 * Implements the functionality to process login error result.
		 * @param pForceLogin	Force login user on startup
		 * @param pLoginType Login type to access.
		 */
		static void LoginErrorProcess();

		/** @brief Login denied process
		 *
		 * Implements the functionality to process login denied result.
		 */
		static void LoginDeniedProcess();

		/** @brief Login success process
		 *
		 * Implements the functionality to process login success result.
		 */
		static void LoginSucessProcess();

		/** @brief Login status process
		 *
		 * Implements the functionality to evaluate the login status..
		 * @param pLoginResult Login status result.
		 */
		static bool LoginStatusProcess(int pLoginResult);

		/** @brief Start manual login
		 *
		 * Method that implements the functionality to try login using
		 * manual process (user-password).
		 * @return Login using manual method result.
		 */
		static int StartManualLogin();

		/** @brief Start manual login
		 *
		 * Method that implements the functionality to try login using
		 * credentias from credential provider
		 * @return Login using manual method result.
		 */
		static int StartSoffidLogin();

		/** @brief Start kerberos login
		 *
		 * Method that implements the functionality to try login using kerberos protocol.
		 * @return Login using kerberos protocol result.
		 */
		static int StartKerberosLogin();

		/** @brief Start login process
		 *
		 * Implements the functionality to start login process using the authentication
		 * method defined on 'LoginType' registry key.
		 * @return Login result.
		 */
		static int StartLoginProcess();

		/** @brief Force login process
		 *
		 * Implements the functionality for try force login if registry key parameter
		 * is configured to do it.
		 */
		static void ForceLogin();

		/** @brief Close KojiKabuto session
		 *
		 * Implements the functionality to close KojiKabuto session.
		 */
		static void CloseSession();

		/** @brief Start user init
		 *
		 * Implements the functionality for initialize user login process.
		 */
		static void StartUserInit();

		static DWORD WINAPI mainLoop (LPVOID param);
		static DWORD WINAPI loginThread (LPVOID param);
		static SeyconSession *session ;

		static bool usedKerberos;
		static bool desktopSession;
};


#endif /* KOJIKABUTO_H_ */
