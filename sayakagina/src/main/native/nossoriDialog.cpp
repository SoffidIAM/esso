#include "nossori.h"
#include <wchar.h>
#include "nossoriDialog.h"
#include "Utils.h"
#include "sayaka.h"
#include "winwlx.h"

#include <MZNcompat.h>
#include <string.h>
#include <stdio.h>

//////////////////////////////////////////////////////////
static BOOL CALLBACK myDialogProc (HWND hwndDlg,	// handle to dialog box
		UINT uMsg,	// message
		WPARAM wParam,	// first message parameter
		LPARAM lParam 	// second message parameter
		)
{
	NossoriDialog *loginDialog = (NossoriDialog*) GetWindowLongA(hwndDlg, GWL_USERDATA);
	switch (uMsg)
	{
		case WM_COMMAND:
		{
			WORD wID = LOWORD(wParam);        // item, control, or accelerator identifier
			if (wID == IDOK)
     			return loginDialog->onOk();
			if (wID == IDCANCEL)
     			return loginDialog->onCancel();
			return false;
		}
		case WM_INITDIALOG:
		{
			loginDialog = (NossoriDialog*) lParam;
			SetWindowLongA(hwndDlg, GWL_USERDATA, (LPARAM) loginDialog);
			loginDialog->m_hWnd = hwndDlg;
			return loginDialog->onStartup();

		}
		case WM_CLOSE:
		{
			loginDialog->onCancel();
			return true;
		}
		default:
			return 0;
	}
}

NossoriDialog::NossoriDialog ()
{
	m_hWnd = NULL;
}

int NossoriDialog::showUser ()
{
	status = SHOW_USER;
	int result = pWinLogon->WlxDialogBoxParam(hWlx, hSayakaDll, MAKEINTRESOURCEW (IDD_NOSSORIDIALOG), NULL,
			myDialogProc, (LPARAM) this);
	m_hWnd = NULL;

	return result;
}

int NossoriDialog::showQuestion() {
	status = SHOW_QUESTION;
	int result = pWinLogon->WlxDialogBoxParam(hWlx, hSayakaDll, MAKEINTRESOURCEW (IDD_QUESTION), NULL,
			myDialogProc, (LPARAM) this);
	m_hWnd = NULL;

	return result;
}

int NossoriDialog::showPassword() {
	status = SHOW_PASSWORD;
	int result = pWinLogon->WlxDialogBoxParam(hWlx, hSayakaDll, MAKEINTRESOURCEW (IDD_PASSWORD), NULL,
			myDialogProc, (LPARAM) this);
	m_hWnd = NULL;

	return result;
}

bool NossoriDialog::onOk() {
	if (status == SHOW_USER)
	{
		Utils::getDlgCtrlText(m_hWnd, IDC_USER, user);
		if (user.empty())
		{
			pWinLogon->WlxMessageBox (hWlx, m_hWnd, Utils::LoadResourcesWString(2003).c_str(),  L"Soffid SSO", MB_ICONWARNING|MB_OK);
		}
		else
		{
			EndDialog(m_hWnd, IDOK);
		}
	}
	else if (status == SHOW_QUESTION)
	{
		while (m_answers.size () <= questionNumber)
			m_answers.insert(m_answers.end(), std::wstring());
		Utils::getDlgCtrlText(m_hWnd, IDC_ANSWER, m_answers[questionNumber]);
		if (m_answers[questionNumber].empty())
		{
			pWinLogon->WlxMessageBox (hWlx, m_hWnd, Utils::LoadResourcesWString(2003).c_str(), L"Soffid SSO", MB_ICONWARNING|MB_OK);
		}
		else
		{
			EndDialog(m_hWnd, IDOK);
		}
	}
	else if (status == SHOW_PASSWORD)
	{
		std::wstring p1, p2;
		Utils::getDlgCtrlText(m_hWnd, IDC_PASSWORD1, p1);
		Utils::getDlgCtrlText(m_hWnd, IDC_PASSWORD2, p2);
		if (p1 != p2)
		{
			pWinLogon->WlxMessageBox (hWlx, m_hWnd, Utils::LoadResourcesWString(2004).c_str(), L"Soffid SSO", MB_ICONWARNING|MB_OK);
		}
		else
		{
			password = p1;
			EndDialog(m_hWnd, IDOK);
		}
	}
	return false;
}

bool NossoriDialog::onStartup() {
	if (status == SHOW_USER)
	{
		SetFocus(GetDlgItem(m_hWnd, IDC_USER));
	}
	else if (status == SHOW_QUESTION)
	{
		SetDlgItemTextW(m_hWnd, IDC_USER, user.c_str());
		SetDlgItemTextW(m_hWnd, IDC_QUESTION, m_questions[questionNumber].c_str());
		SetFocus(GetDlgItem(m_hWnd, IDC_ANSWER));
	}
	else if (status == SHOW_PASSWORD)
	{
		SetDlgItemTextW(m_hWnd, IDC_USER, user.c_str());
		SendMessage(GetDlgItem(m_hWnd, IDC_PASSWORD1), EM_SETPASSWORDCHAR, '*', 0);
		SendMessage(GetDlgItem(m_hWnd, IDC_PASSWORD2), EM_SETPASSWORDCHAR, '*', 0);
		SetFocus(GetDlgItem(m_hWnd, IDC_PASSWORD1));
	}
	return false;
}

bool NossoriDialog::onCancel() {
	EndDialog(m_hWnd, IDCANCEL);
	return false;
}
