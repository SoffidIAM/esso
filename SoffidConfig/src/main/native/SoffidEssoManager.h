/*
 * SoffidEssoManager.h
 *
 *  Created on: Jan 25, 2013
 *      Author: areina
 */

#ifndef SOFFIDESSOMANAGER_H_
#define SOFFIDESSOMANAGER_H_

# include <windows.h>
# include <winhttp.h>
# include <string>

/*** @brief Soffid Esso manager class
 *
 * Class that provides additional functionality to manage and check the system properties
 *  for admin the Soffid ESSO configuration tool.
 */
class SoffidEssoManager
{
	private:

		/*** @brief GINA DLL path
		 *
		 * GINA Dll access path.
		 */
		std::string ginaDll;

		/*** @brief Previous GINA DLL path
		 *
		 * Previous Dll access path.
		 */
		std::string previousGinaDll;

		/** @brief Use Soffid ESSO Gina
		 *
		 */
		bool originalGina;

		/*** @brief Force startup login
		 *
		 * Select to force login at startup.
		 * \remarks
		 * <ul>
		 * 		<li>
		 * 			@code true @endcode
		 * 			If the force login on startup is activated.
		 * 		</li>
		 *
		 * 		<li>
		 * 			@code false @endcode
		 * 			If the force login on startup is not activated.
		 * 		</li>
		 * 	</ul>
		 */
		std::string forceLogin;

		/*** @brief Enable close ESSO session
		 *
		 * Select to enable user close ESSO session.
		 * \remarks
		 * <ul>
		 * 		<li>
		 * 			@code true @endcode
		 * 			If the user ESSO close session is activated.
		 * 		</li>
		 *
		 * 		<li>
		 * 			@code false @endcode
		 * 			If the user ESSO close session is not activated.
		 * 		</li>
		 * 	</ul>
		 */
		std::string enableCloseSession;

		/** @brief ESSO login type
		 *
		 * Select ESSO kerberos login, manual login or both (try kerberos and
		 * fallback to manual)
		 * \remarks
		 * <ul>
		 * 		<li>
		 * 			@code kerberos @endcode
		 * 			Enables the login using Kerberos protocol.
		 * 		</li>
		 *
		 * 		<li>
		 * 			@code manual @endcode
		 * 			Enables manual login.
		 * 		</li>
		 *
		 * 		<li>
		 * 			@code both @endcode
		 * 			Enables the login using Kerberos protocol and manual method.
		 * 		</li>
		 * 	</ul>
		 */
		std::string loginType;

		/** @brief ESSO Server URL
		 *
		 * URL for ESSO Server.
		 */
		std::string serverURL;

		/** @brief ESSO server port
		 *
		 * ESSO server port
		 */
		int serverPort;

	public:

		/** @brief Connection agent
		 *
		 * Definition of connection agent for http calls.
		 */
		static const std::wstring DEF_CON_AGENT;

		/** @brief Session manager path
		 *
		 * Definition of session manager path.
		 */
		static const std::string DEF_SESSION_MNG;

		/** @brief Winlogon registry path
		 *
		 * Registry path for GINA windows logon.
		 */
		static const std::string DEF_WINLOGON;

		/** @brief Windows current version path
		 *
		 * Registry path for current Windows version.
		 */
		static const std::string DEF_WINCURRENT;

		/** @brief CAIB registry path
		 *
		 * Registry path for CAIB installation.
		 */
		static const std::string DEF_CAIB_REGISTRY;

		/** @brief Certificate name
		 *
		 * Certificate name.
		 */
		static const std::string DEF_CERTIFICATE_NAME;

		/** @brief Soffid GINA dll
		 *
		 * Registry path value for Soffid GINA
		 */
		static const std::string DEF_GINA_LOGON;

		/** @brief Constructor
		 *
		 * Default constructor of Soffid Esso manager.
		 */
		SoffidEssoManager ();

		/** @brief Destructor
		 *
		 * Default constructor of Soffid Esso Manager.
		 */
		virtual ~SoffidEssoManager ();

		/** @brief Get GINA path
		 *
		 * Get value for GINA dll path.
		 */
		const std::string& getGinaDll () const;

		/** @brief Set GINA path
		 *
		 * Set value for GINA dll path.
		 */
		void setGinaDll (const std::string& ginaDll);

		/** @brief Get login type
		 *
		 * Get value for login type.
		 */
		const std::string& getLoginType () const;

		/** @brief Set login type
		 *
		 * Set value for login type.
		 */
		void setLoginType (const std::string& loginType);

		/** @brief Get previous GINA path
		 *
		 * Gets the previous GINA path value.
		 */
		const std::string& getPreviousGinaDll () const;

		/** @brief Set previous GINA path
		 *
		 * Set the previous GINA path value.
		 */
		void setPreviousGinaDll (const std::string& previousGinaDll);

		/** @brief Get enable close session
		 *
		 * Get value of enable close session by user.
		 */
		const std::string& getEnableCloseSession () const;

		/** @brief Set enable close session
		 *
		 * Set value of enable close session by user.
		 */
		void setEnableCloseSession (const std::string& enableCloseSession);

		/** @brief Get force login on startup
		 *
		 * Get value of force login on startup.
		 */
		const std::string& getForceLogin () const;

		/** @brief Set force login on startup
		 *
		 * Set value of force login on startup.
		 */
		void setForceLogin (const std::string& forceLogin);

		/** @brief Get if original GINA is setted
		 *
		 * @return
		 */
		const bool getOriginalGina () const;

		/** @brief Get server URL
		 *
		 * Get value of server URL to connect.
		 */
		const std::string& getServerUrl () const;

		/** @brief Set server URL
		 *
		 * Set value of server URL to connect.
		 */
		void setServerUrl (const std::string& serverUrl);

		/** @brief Get server URL port
		 *
		 * Get value of connection port for server URL.
		 */
		const int getServerPort () const;

		/** @brief Set server URL port
		 *
		 * Set value of connection port for server URL.
		 */
		void setServerPort (int serverPort);

		/** @brief Check x64 system
		 *
		 * This method checks if the execution system is x64 architecture.
		 * @return
		 * <ul>
		 * 		<li>
		 * 			@code TRUE @endcode
		 * 			If the execution system are x64 architecture.
		 * 		</li>
		 *
		 * 		<li>
		 * 			@code FALSE @endcode
		 * 			If the execution system are not x64 architecture.
		 * 		</li>
		 * 	</ul>
		 */
		static BOOL IsWow64 ();

		/** @brief x64 registry flag
		 *
		 * This method get the x64 registry flag from system if the execution system
		 * are this architecture.
		 * @return x64 registry flag.
		 */
		static DWORD Wow64Key (DWORD flag);

		/** @brief Check Windows XP OS
		 *
		 * This method check if the configuration tool is running on Windows XP OS.
		 * @return
		 * <ul>
		 * 		<li>
		 * 			@code TRUE @endcode
		 * 			If the configuration tool are running on Windows XP OS,
		 * 		</li>
		 *
		 * 		<li>
		 * 			@code FALSE @endcode
		 * 			If the configuration tool are not runnin on Windows XP OS.
		 * 		</li>
		 * 	</ul>
		 */
		static BOOL IsWindowsXP ();

		/** @brief Mazinger dir
		 *
		 * Method that implements the functionality for obtain the installation path
		 * of Mazinger.
		 */
		static LPCSTR getMazingerDir ();

		/** @brief Mazinger GINA path
		 *
		 * Method that gets the complete path for Soffid GINA dll.
		 * @return Soffid GINA complete path.
		 */
		static std::string MazingerGinaPath ();

		/** @brief Get server certificate
		 *
		 * Method that gets the certificate from the URL server especified.
		 */
		static LPSTR readURL (HINTERNET hSession, const wchar_t *host, int port,
				LPCWSTR path, BOOL allowUnknownCA, size_t *pSize);

		/** @brief Save server URL in registry
		 *
		 * Method that implements the functionality for save the parameters for
		 * connection with Soffid ESSO server.
		 */
		static bool SaveURLServer (const char* url);

		/** @brief Get URL Server
		 *
		 * Method that implements the functionality form obtain the formated URL server.
		 * @return URL to connect with ESSO Server.
		 */
		std::string GetFormatedURLServer ();

		/** @brief Load server URL
		 *
		 * Method that implements the functionality for load and store in the Soffid ESSO
		 * manager the URL of server.
		 */
		void LoadServerURL ();

		/** @brief Get log info
		 *
		 */
		static void log (const char *szFormat, ...);

		/** @brief Update GINA
		 *
		 *	Update GINA dll path.
		 */
		void UpdateGina (const char *ginaPath);

		/** @brief Load Soffid ESSO configuration
		 *
		 * This method implements the functionality to load the configuration parameter
		 * for Soffid ESSO stored previously.
		 * @return
		 * <ul>
		 * 		<li>
		 * 			@code TRUE @endcode
		 * 			If the configuration is loaded correctly,
		 * 		</li>
		 *
		 * 		<li>
		 * 			@code FALSE @endcode
		 * 			If the configuration is not loaded correctly.
		 * 		</li>
		 * 	</ul>
		 */
		bool LoadConfiguration ();

		/** @brief Load Windows logon configuration
		 *
		 * This method implements the functionality to load stored configuration about
		 * Windows logon.
		 */
		void LoadGinaConfiguration ();

		/** @brief Save Soffid ESSO configuration
		 *
		 * This method implements the functionality to save the configuration parameter
		 * for Soffid ESSO.
		 * @return
		 * <ul>
		 * 		<li>
		 * 			@code TRUE @endcode
		 * 			If the configuration is saved correctly,
		 * 		</li>
		 *
		 * 		<li>
		 * 			@code FALSE @endcode
		 * 			If the configuration is not saved correctly.
		 * 		</li>
		 * 	</ul>
		 */
		bool SaveConfiguration ();

		/** @brief Save GINA dll values
		 *
		 * This method implements the functionality to save the configuration parameter
		 * for GINA.
		 * @return
		 * <ul>
		 * 		<li>
		 * 			@code TRUE @endcode
		 * 			If the configuration is saved correctly,
		 * 		</li>
		 *
		 * 		<li>
		 * 			@code FALSE @endcode
		 * 			If the configuration is not saved correctly.
		 * 		</li>
		 * 	</ul>
		 */
		bool SaveGINAConfiguration ();
};

#endif /** SOFFIDESSOMANAGER_H_ */
