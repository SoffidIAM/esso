#include "sayaka.h"
#include "winwlx.h"
#include "Pkcs11Configuration.h"
#include "CardNotificationHandler.h"

const char *achMessage =
		"\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n"
				"\r\n\r\n\r\n\r\n\r\n\r\n"
				" Aquesta estaci� de feina us permetr� accedir a la xarxa Intranet del Govern de les Illes Balears "
				"\r\n\r\n"
				"L'�s d'aquest equipament , els serveis i els sistemes d'informaci� "
				" disponibles a la Intranet , com tamb� l'acc�s a Internet, s�n exclusivament "
				"per exercir les funcions assignades al vostre lloc de treball .\r\n\r\n"
				"Els mecanismes d'autenticaci�, com ara contrasenyes o targetes criptogr�fiques, "
				"son d'�s exclusiu i intransferible del seu titular .\r\n\r\n"
				"D'acord amb l'article 23 del RD 3/2010, "
				" de 8 de gener , es registraran les vostres activitats , i es retendr� la informaci� "
				" necess�ria per tal de monitorar , analitzar , investigar i documentar "
				" activitats indegudes o no autoritzades , i permetre identificar en cada moment la persona que actua , "
				" sempre d'acord amb la normativa sobre protecci� de dades personals, de funci� p�blica o laboral , "
				"i la resta de disposicions que siguin aplicables.\r\n\r\n"
				" En particular, podeu exercir els drets d'acc�s, rectificaci� , cancel�laci� i oposici� "
				" recollits en la Llei org�nica de protecci� de dades mitjan�ant un escrit "
				" adre�at a:\r\n\r\nDirector general de Tecnologia i Comunicacions \r\nCarrer de Sant Pere , 7\r\n07012 Palma \r\n\r\n"
				" En cas de detectar anomalies o incid�ncies de seguretat , les heu de notificar per "
				" correu electr�nic a l'adre�a seguretat @dgtic.caib.es\r\n\r\n\r\n"
				"\r\n\r\n\r\n\r\n\r\n\r\n"
				" Pijau ALT, CONTROL i SUPR simult�niament per iniciar la sessi� "
				"\r\n\r\n"
				" Si disposau de DNI electr�nic o targeta criptogr�fica del Govern, "
				" podeu utilitzar-la per identificar-vos introduint-la al lector ."
				"\r\n";

static HWND dialogHwnd = NULL;

static DWORD WINAPI dlgLoop (LPVOID param)
{
	int position = 0;
	HWND hwndDlg = (HWND) param;
	bool firstloop = true;
	char ach[100];

	while (achMessage[position] != '\0')
	{
		int pos1 = 0;
		do
		{
			ach[pos1++] = achMessage[position++];
		} while (achMessage[position] == '\r'
				|| (firstloop && achMessage[position] == '\n'));
		ach[pos1] = '\0';
		HWND hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT1);
		if (hwndEdit == NULL)
			return 0;
		SendMessage(hwndEdit, EM_SETSEL, position, position);
		SendMessage(hwndEdit, EM_REPLACESEL, false, (LPARAM) ach);
		Sleep(40);
		firstloop = false;
	}
	return 0;
}

//////////////////////////////////////////////////////////
static BOOL CALLBACK wellcomeDialogProc (HWND hwndDlg,	// handle to dialog box
		UINT uMsg,	// message
		WPARAM wParam,	// first message parameter
		LPARAM lParam 	// second message parameter
		)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			dialogHwnd = hwndDlg;

			ShowWindow(hwndDlg, SW_SHOW);
			SetForegroundWindow(hwndDlg);
			SetFocus(hwndDlg);
			CreateThread(NULL, 0, dlgLoop, (void *) hwndDlg, 0, NULL);

			return 0;
		}

		case WM_CLOSE:
		{
			printf("Received wm_close");
			EndDialog(hwndDlg, 9999);

			return true;
		}
	}
	return 0;
}

void displayWelcomeMessage ()
{
	dialogHwnd = NULL;
	CardNotificationHandler notifier(&dialogHwnd);

	if (p11Config != NULL)
		p11Config->setNotificationHandler(&notifier);

	DWORD result = pWinLogon->WlxDialogBox(hWlx, hSayakaDll,
			MAKEINTRESOURCEW (IDD_LOGOUT), NULL, wellcomeDialogProc);

	if (notifier.isCardInserted())
		pWinLogon->WlxSasNotify(hWlx, WLX_SAS_TYPE_SC_INSERT);

	if (p11Config != NULL)
		p11Config->setNotificationHandler(NULL);
}
