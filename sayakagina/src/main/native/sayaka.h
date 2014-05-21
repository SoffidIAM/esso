#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x5000
#endif

#ifdef _WIN32_IE
#undef _WIN32_IE
#endif
#define _WIN32_IE 0x0500


#define SECURITY_WIN32

#include <windows.h>
#include <stdio.h>
#include <winuser.h>
#include <resource.h>
#include <wchar.h>
#include "pkcs11.h"
#include <string>
#include <vector>

class CertificateHandler;
#include "Pkcs11Configuration.h"

struct CertData {
	CK_FUNCTION_LIST_PTR p11;
	CK_SLOT_ID slot_num;
	unsigned char *achId;
	unsigned long ulIdLength;
	char *achName;
	FILETIME expirationDate;
	unsigned char *pvCert;
	unsigned long ulCertLength;
	char tokenManufacturer[32];
	char tokenModel[16];
	char tokenSerialNumber[16];
	bool nearExpireDate;
};

struct _WLX_DISPATCH_VERSION_1_3;
extern struct CertData *pSelectedCert;
extern struct _WLX_DISPATCH_VERSION_1_3 *pWinLogon;
extern HANDLE hWlx;

// 0  => No esta presente
// 1  => Esta presente
// -1 => No se puede verificar

int testCardPresence ();
bool tryUnlock ();

LPWSTR sendUrlMessage(LPCWSTR url);
void getResultToken(LPCWSTR lpszResult, int id, LPWSTR lpszValue, int max);
///////////////////////////////////////////////////////////////
void getResultTokenA(LPCWSTR lpszResult, int id, LPSTR lpszValue, int max);
void toBase64(LPSTR result, LPBYTE source, int len);
int fromBase64(LPBYTE result, LPCSTR source);
void importCertificate();
void escapeString(LPWSTR lpszTarget, LPCSTR lpszSource);
extern int selectCert (std::vector<CertificateHandler*> &certs);
extern const char* enterPin();
extern void clearPin();


extern HINSTANCE hSayakaDll;


extern int securityDialog () ;

extern Pkcs11Configuration *p11Config;

void displayMessage (HDESK hDesktopParam, const wchar_t *lpszMessage);
void cancelMessage ();

LPCWSTR changePassword (const wchar_t *wchDomain, const wchar_t *wchUser, std::wstring &wchPass);
wchar_t* getCredentials(struct CertData *pCertData) ;

void displayLockMessage ();
void displayWelcomeMessage ();
int shutdownDialog () ;



