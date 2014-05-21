
#include "sayaka.h"
#include "winwlx.h"
#include "ssoclient.h"
#include "LoginStatus.h"
#include "TokenHandler.h"
#include "CertificateHandler.h"
#include "Pkcs11Handler.h"
#include "Pkcs11Configuration.h"
#include "ChangePasswordManager.h"

#include <string>
#include <vector>

# include <MZNcompat.h>
# include "Utils.h"

static PWSTR allocateString (const wchar_t *sz)
{
	PWSTR newsz = (PWSTR) LocalAlloc ( LMEM_FIXED, sizeof (wchar_t) * (wcslen (sz) + 1 ));
	wcscpy (newsz, sz);
	return newsz;
}

static CertificateHandler* getCardPassword (std::wstring &wszUser, std::wstring &wszPassword) {
	CertificateHandler *result = NULL;
	if (p11Config == NULL)
		return result;

	std::vector<TokenHandler*> tokens = p11Config->enumTokens();
	if (tokens.size () > 0 )
	{
		LPCSTR szPin = enterPin();
		if (szPin != NULL)
		{
			displayMessage(NULL, MZNC_strtowstr(Utils::LoadResourcesString(29).c_str()).c_str());
			std::vector<CertificateHandler*> certs;
			for (std::vector<TokenHandler*>::iterator it = tokens.begin(); it != tokens.end(); it++)
			{
				TokenHandler *token = *it;
				token->setPin(szPin);
				std::vector<CertificateHandler*> &tokenCerts = token->getHandler()->enumCertificates(token);
				for (std::vector<CertificateHandler*>::iterator itCert = tokenCerts.begin();
						itCert != tokenCerts.end(); itCert++){
					CertificateHandler *cert = *itCert;
					certs.push_back(cert);
				}
			}

			int i = certs.size() == 1 ? 0 : selectCert(certs);

			CertificateHandler *pCert = certs[i];
			if (pCert->obtainCredentials() ) {
				if (wszUser == pCert->getUser()) {
					wszPassword = pCert->getPassword();
					result = pCert;
				}
			} else {
				MessageBox (NULL, pCert->getErrorMessage(),
						Utils::LoadResourcesString(31).c_str(),
						MB_OK|MB_ICONEXCLAMATION);
			}
			cancelMessage();
		}
	}
	return result;

}

static bool tryChangePassword ()
{
	Log log ("securityDialog");
	std::wstring szPassword;

	CertificateHandler *cert = getCardPassword(LoginStatus::current.szUser, szPassword);

	ChangePasswordManager cpm ;

	if (cpm. changePassword(
			LoginStatus::current.szUser,
			LoginStatus::current.szDomain,
			szPassword))
	{
		log.info ("Password changed");
		pWinLogon->WlxMessageBox (hWlx, NULL,
				(wchar_t*) MZNC_strtowstr(Utils::LoadResourcesString(42).c_str()).c_str(),
				(wchar_t*) MZNC_strtowstr(Utils::LoadResourcesString(43).c_str()).c_str(),
				MB_OK|MB_ICONEXCLAMATION);
		PWLX_MPR_NOTIFY_INFO pMprInfo = (PWLX_MPR_NOTIFY_INFO) LocalAlloc(0, sizeof (*pMprInfo));
		pMprInfo->pszDomain = allocateString(LoginStatus::current.szDomain.c_str());
		pMprInfo->pszOldPassword = allocateString(szPassword.c_str());
		pMprInfo->pszPassword = allocateString(cpm.szNewPassword1.c_str());
		pMprInfo->pszUserName = allocateString(LoginStatus::current.szUser.c_str());
		pWinLogon->WlxChangePasswordNotifyEx(hWlx, pMprInfo, 0, NULL, 0);
		log.info ("Change notified to winlogon");
		if (cert != NULL) {
			cert->getHandler()->saveCredentials(cert, LoginStatus::current.szUser, szPassword);
		}
		log.info ("Credentials saved");
		return true;
	}
	else
		return false;
}

//////////////////////////////////////////////////////////
static BOOL CALLBACK securityDialogProc(
    HWND hwndDlg,	// handle to dialog box
    UINT uMsg,	// message
    WPARAM wParam,	// first message parameter
    LPARAM lParam 	// second message parameter
   )
{
	switch (uMsg)
	{
	case WM_COMMAND:
	{
		if (lParam != 0) {
			WORD wID = LOWORD(wParam);         // item, control, or accelerator identifier
			if (wID == IDC_LOCK_BUTTON)
			{
				EndDialog(hwndDlg, WLX_SAS_ACTION_LOCK_WKSTA);
			}
			if (wID == IDC_LOGOUT_BUTTON)
			{
				EndDialog(hwndDlg, WLX_SAS_ACTION_LOGOFF);
			}
			if (wID == IDC_PASSWORD_BUTTON)
			{
				if (tryChangePassword())
					EndDialog(hwndDlg, WLX_SAS_ACTION_NONE);
			}
			if (wID == IDC_TASKMGR_BUTTON)
			{
				EndDialog(hwndDlg, WLX_SAS_ACTION_TASKLIST);
			}
			if (wID == IDC_SHUTDOWN_BUTTON)
			{
				DWORD dw = shutdownDialog ();
				if (dw != WLX_SAS_ACTION_NONE && dw >= 0)
					EndDialog(hwndDlg, dw);
			}
			if (wID == IDC_CANCEL)
			{
				EndDialog(hwndDlg, WLX_SAS_ACTION_NONE);
			}
		}
		else  if (LOWORD(wParam) == IDCANCEL){
			EndDialog(hwndDlg, WLX_SAS_ACTION_NONE);
		}
		return 0;
	}
	case WM_INITDIALOG:
	{
		SetDlgItemTextW(hwndDlg, IDC_LOGINTIME, LoginStatus::current.szLogonTime.c_str());
		if (LoginStatus::current.szOriginalUser == LoginStatus::current.szUser) {
			SetDlgItemTextW(hwndDlg, IDC_USERNAME, LoginStatus::current.szUser.c_str());
			EnableWindow(GetDlgItem(hwndDlg, IDC_PASSWORD_BUTTON), true);
		} else {
			std::wstring msg;
			msg.assign ( LoginStatus::current.szOriginalUser);
			msg.append ( MZNC_strtowstr(Utils::LoadResourcesString(44).c_str()).c_str());
			msg.append ( LoginStatus::current.szUser);
			msg.append ( L")");
			SetDlgItemTextW(hwndDlg, IDC_USERNAME, msg.c_str());
			EnableWindow(GetDlgItem(hwndDlg, IDC_PASSWORD_BUTTON), false);
		}
		return 0;
	}
	default:
		return 0;
	}
}

int securityDialog ()
{
	int r = pWinLogon->WlxDialogBox(hWlx, hSayakaDll, MAKEINTRESOURCEW (IDD_LOGGEDIN), NULL, securityDialogProc);
	switch (r)
	{
	case WLX_DLG_INPUT_TIMEOUT:
	case WLX_DLG_SAS:
		return WLX_SAS_ACTION_NONE;
	case WLX_DLG_SCREEN_SAVER_TIMEOUT:
		return WLX_SAS_ACTION_LOCK_WKSTA;
	case WLX_DLG_USER_LOGOFF:
		return WLX_SAS_ACTION_LOGOFF;
	default:
		return r;
	}
}
