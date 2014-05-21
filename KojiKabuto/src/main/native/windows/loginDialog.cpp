
# include "loginDialog.h"

extern HINSTANCE hKojiInstance;

static LoginDialog *loginDialog = NULL;

// Login dialog process
INT_PTR CALLBACK LoginDialog::loginDialogProc(HWND hwndDlg, UINT uMsg,
		WPARAM wParam, LPARAM lParam)
{
	WORD wID = LOWORD(wParam);	// item, control, or accelerator identifier

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			SendMessage (GetDlgItem(hwndDlg, IDC_PASSWORD), EM_SETPASSWORDCHAR, '*', 0);

			loginDialog -> m_hWnd = hwndDlg;

			// Check empty user ID
			if (loginDialog -> userID.empty())
			{
				SetFocus (GetDlgItem(hwndDlg, IDC_USER));
			}

			else
			{
				SetDlgItemTextW (hwndDlg, IDC_USER, loginDialog -> userID.c_str());

				SetFocus (GetDlgItem(hwndDlg, IDC_PASSWORD));
			}

			return false;
		}

		case WM_COMMAND:
		{
			Utils::getDlgCtrlText(hwndDlg, IDC_USER, loginDialog -> userID);
			Utils::getDlgCtrlText(hwndDlg, IDC_PASSWORD, loginDialog -> userPassword);

			// Check 'OK' button pushed
			if (wID == IDOK)
			{
				loginDialog == NULL;
				EndDialog (hwndDlg, 0);
			}

			// Check 'Cancel' button pushed
			if (wID == IDCANCEL)
			{
				loginDialog == NULL;
				EndDialog (hwndDlg, -1);
			}

			return true;
		}

		case WM_CLOSE:
		{
			loginDialog == NULL;
			EndDialog(hwndDlg, -1);

			return true;
		}

		default:
			return 0;
	}
}

// Default constructor
LoginDialog::LoginDialog ()
{
	m_hWnd = NULL;
}

// Get user ID
 const std::wstring& LoginDialog::getUserId() const
{
	return userID;
}

// Set user ID
 void LoginDialog::setUserId(const std::wstring& userId)
{
	userID = userId;
}

// Get user password
 const std::wstring& LoginDialog::getUserPassword() const
{
	return userPassword;
}

// Set user password
 void LoginDialog::setUserPassword(const std::wstring& userPassword)
{
	this -> userPassword = userPassword;
}

 // Raise loop
DWORD CALLBACK LoginDialog::raiseLoop (LPVOID param)
{
	while (loginDialog != NULL && loginDialog -> m_hWnd == NULL)
	{
		Sleep (1000);
	}

	while (loginDialog != NULL && loginDialog -> m_hWnd != NULL)
	{
		ShowWindow(loginDialog -> m_hWnd, SW_SHOWNORMAL);

		Sleep (1000);
	}

 	return 0;
 }

// Show dialog
int LoginDialog::show ()
{
	int result;	// Dialog result

	loginDialog = this;

	CreateThread(NULL, 0, raiseLoop, NULL, 0, NULL);

	result = DialogBoxA (hKojiInstance, MAKEINTRESOURCE(IDD_LOGINDIALOG),
			NULL, loginDialogProc);

	loginDialog = NULL;

	m_hWnd = NULL;

	return result;
}
