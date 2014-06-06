/*
 * Utils.h
 *
 *  Created on: 27/10/2010
 *      Author: u07286
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <string>

#define SECURITY_WIN32

#include <security.h>

class Utils {

public:
	Utils();
	virtual ~Utils();

	static bool convert (std::wstring &target, std::string &src);
	static bool convert (std::string &target, std::wstring &src);
	static bool getWindowText (HWND hwnd, std::string &str);
	static bool getDlgCtrlText (HWND hwnd, int id, std::string &str);
	static bool getWindowText (HWND hwnd, std::wstring &str);
	static bool getDlgCtrlText (HWND hwnd, int id, std::wstring &str);
	static bool getErrorMessage (std::wstring &msg, DWORD dwError);
	static bool getErrorMessage (std::string &msg, DWORD dwError);
	static bool getLastError (std::wstring &msg);
	static bool getLastError (std::string &msg);

	/** Load language messages from DLL
	 *
	 * Method that loads the specified language string defined in resources of DLL.
	 * @param id Resource ID to load.
	 * @return Message loaded from DLL.
	 */
	static  std::string LoadResourcesString (int id);
	static  std::wstring LoadResourcesWString (int id);
};

#endif /* UTILS_H_ */
