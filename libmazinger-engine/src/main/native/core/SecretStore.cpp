/*
 * SecretStore.cpp
 *
 *  Created on: 02/07/2010
 *      Author: u07286
 */


#include "SecretStore.h"
#include <MazingerInternal.h>
#include "aes.h"
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void SecretStore::applySeed(unsigned char const *lpszSeed, int size, unsigned char* achKey, int & k)
{
    for (int i = 0; i < size || (size == -1 && lpszSeed[i]); i++)
    {
        int pos  = k / 2;
        if ( (k % 2) == 1 )
        {
            achKey[pos] = achKey[pos] ^ ( 15 & (lpszSeed[i] >> 4)) ;
            k = (k+1) % (AESKEY_SIZE * 2);
            pos = k / 2;
            achKey[pos] = achKey[pos] ^ ( (15 & lpszSeed[i]) << 4) ;
        } else {
            achKey[pos] = achKey[pos] ^ lpszSeed[i] ;
            k = (k+1) % (AESKEY_SIZE * 2);
        }
    }
}

#ifndef KEY_WOW64_64KEY
#define KEY_WOW64_64KEY 0x0100
#endif

SecretStore::SecretStore(const char *user) {
    int k = 0;
    unsigned char achKey [] = "SoffidRulesWorld";
    applySeed((unsigned char const *)__DATE__, -1, achKey, k);
    applySeed((unsigned char const *)__FILE__, -1, achKey, k);

    m_pEnv = MazingerEnv::getEnv(user);

#ifdef WIN32
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0,
			KEY_READ| KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS ||
		RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0,
			KEY_READ, &hKey) == ERROR_SUCCESS) {
		unsigned char achBuffer[4096] = "";
		DWORD dwType;
		DWORD size = sizeof achBuffer;
		if (ERROR_SUCCESS==
				RegQueryValueEx(hKey, "LicenseInfo", NULL, &dwType, (LPBYTE) achBuffer, &size))
		{
			applySeed (achBuffer, size, achKey, k);
		}
		size = sizeof achBuffer;
		if (ERROR_SUCCESS==
				RegQueryValueEx(hKey, "DigitalProductId", NULL, &dwType, (LPBYTE) achBuffer, &size))
		{
			applySeed (achBuffer, size, achKey, k);
		}
		RegCloseKey(hKey);
	}
#endif
    applySeed((unsigned char const *)user, -1, achKey, k);

    const char *szHostName = MZNC_getHostName ();
#ifdef TRACE_AES
	printf ("HOST =%s\n", szHostName);
#endif
    applySeed((unsigned char const *)szHostName, -1, achKey, k);
#ifdef TRACE_AES
    printf ("AES KEY:\n");
	for (int j = 0; j < AESKEY_SIZE; j++)
	{
		printf ("%02x ", (unsigned int)achKey[j]);
	}
	printf("\n");
#endif

    m_expkey = (unsigned char*) malloc (EXPKEY_SIZE);

    AESExpandKey(achKey, m_expkey);
}

SecretStore::SecretStore(const char *user, const char *desktop) {
//	MZNSendDebugMessageA("Opening secret store %s", user);
    int k = 0;
    unsigned char achKey [] = "TheKeyIsVeryWeak";
    const char *szHostName = MZNC_getHostName ();
    applySeed((unsigned char const *)szHostName, -1, achKey, k);

    m_pEnv = MazingerEnv::getEnv(user, desktop);

	m_expkey = (unsigned char*) malloc (EXPKEY_SIZE);
    AESExpandKey(achKey, m_expkey);
}

SecretStore::~SecretStore() {
}

void SecretStore::dump ()
{
	PMAZINGER_DATA pMazinger = m_pEnv->getData();
	if (pMazinger != NULL )
	{
		dump (pMazinger->achSecrets, pMazinger->debugLevel);
	}
}

void SecretStore::dump (wchar_t *wchSecrets, int debugLevel)
{
	int counter = 0;
	while(true) {
		wchar_t *achSecret = readString(wchSecrets, counter);
		if (achSecret == NULL || wcslen(achSecret) == 0) return;
		printf ("%ls", achSecret);
		freeSecret(achSecret);
		fflush(stdout);
		if (debugLevel > 1)
		{
			achSecret = readString(wchSecrets, counter);
			printf ("=[%ls]\n", achSecret);
			freeSecret(achSecret);
		} else {
			skipString(wchSecrets, counter);
			printf ("\n");
		}
	}

}

wchar_t * SecretStore::getSecret(const wchar_t * secret) {
	wchar_t * result = getSecret (secret, true);
	if (result == NULL || result [0] == L'\0') {
		freeSecret(result);
		result = getSecret(secret, false);
	}
	return result;
}

wchar_t * SecretStore::getSecret(const wchar_t * secret, bool caseSensitive) {
	PMAZINGER_DATA pMazinger = m_pEnv->getData();
	if (pMazinger == NULL) {
		MZNSendDebugMessageA("*** FATAL ***");
		MZNSendDebugMessageA("Cannot access secret store ");
		wchar_t *result = (wchar_t*) malloc(sizeof (wchar_t));
		result[0] = L'\0';
		return result;
	}

	int counter = 0;
	while(true) {
		wchar_t *achSecret = readString(pMazinger->achSecrets, counter);
		if (achSecret == NULL || achSecret[0] == '\0')
		{
			freeSecret(achSecret);
			wchar_t *result = (wchar_t*) malloc(sizeof (wchar_t));
			result[0] = L'\0';
			return result;
		}
		else if ((caseSensitive && wcscmp  (achSecret, secret) == 0) ||
				 (!caseSensitive && wcsicmp  (achSecret, secret) == 0))
		{
			freeSecret (achSecret);
			wchar_t* result = readString(pMazinger->achSecrets, counter);
			return result;
		}
		else
		{
			freeSecret (achSecret);
			skipString (pMazinger->achSecrets, counter);
		}
	}
	return NULL;
}


std::map<std::wstring, std::wstring> SecretStore::getSecretsByPrefix(const wchar_t * secret) {
	std::map<std::wstring, std::wstring> secrets;
	PMAZINGER_DATA pMazinger = m_pEnv->getData();
	if (pMazinger == NULL) {
		MZNSendDebugMessageA("*** FATAL ***");
		MZNSendDebugMessageA("Cannot access secret store ");
		wchar_t *result = (wchar_t*) malloc(sizeof (wchar_t));
		result[0] = L'\0';
		return secrets;
	}

	int counter = 0;
	while(true) {
		wchar_t *achSecret = readString(pMazinger->achSecrets, counter);
		if (achSecret == NULL || achSecret[0] == '\0')
		{
			freeSecret(achSecret);
			break;
		}
		else if ( wcsncmp  (achSecret, secret, wcslen (secret)) == 0 )
		{
			std::wstring tag = achSecret;
			freeSecret (achSecret);
			wchar_t *value = readString(pMazinger->achSecrets, counter);
			secrets[tag] = std::wstring(value);
			freeSecret(value);
		}
		else
		{
			freeSecret (achSecret);
			skipString (pMazinger->achSecrets, counter);
		}
	}

	return secrets;
}

std::vector<std::pair<std::wstring, std::wstring> > SecretStore::getSecretsByPrefix2(const wchar_t * secret) {
	std::vector<std::pair<std::wstring, std::wstring> > secrets;
	PMAZINGER_DATA pMazinger = m_pEnv->getData();
	if (pMazinger == NULL) {
		MZNSendDebugMessageA("*** FATAL ***");
		MZNSendDebugMessageA("Cannot access secret store ");
		wchar_t *result = (wchar_t*) malloc(sizeof (wchar_t));
		result[0] = L'\0';
		return secrets;
	}

	int counter = 0;
	while(true) {
		wchar_t *achSecret = readString(pMazinger->achSecrets, counter);
		if (achSecret == NULL || achSecret[0] == '\0')
		{
			freeSecret(achSecret);
			break;
		}
		else if ( wcsncmp  (achSecret, secret, wcslen (secret)) == 0 )
		{
			std::wstring tag = achSecret;
			freeSecret (achSecret);
			wchar_t *value = readString(pMazinger->achSecrets, counter);
			secrets.push_back(std::pair<std::wstring,std::wstring>(tag, value));
			freeSecret(value);
		}
		else
		{
			freeSecret (achSecret);
			skipString (pMazinger->achSecrets, counter);
		}
	}

	return secrets;
}


std::vector<std::wstring> SecretStore::getSecrets(const wchar_t * secret) {
	std::vector<std::wstring> secrets;
	PMAZINGER_DATA pMazinger = m_pEnv->getData();
	if (pMazinger == NULL) {
		MZNSendDebugMessageA("*** FATAL ***");
		MZNSendDebugMessageA("Cannot access secret store ");
		wchar_t *result = (wchar_t*) malloc(sizeof (wchar_t));
		result[0] = L'\0';
		return secrets;
	}

	int counter = 0;
	while(true) {
		wchar_t *achSecret = readString(pMazinger->achSecrets, counter);
		if (achSecret == NULL || achSecret[0] == '\0')
		{
			freeSecret(achSecret);
			break;
		}
		else if ( wcscmp  (achSecret, secret) == 0)
		{
			freeSecret (achSecret);
			wchar_t *value = readString(pMazinger->achSecrets, counter);
			secrets.push_back(std::wstring(value));
			freeSecret(value);
		}
		else
		{
			freeSecret (achSecret);
			skipString (pMazinger->achSecrets, counter);
		}
	}

	return secrets;
}

void SecretStore::setSecret(const wchar_t * secret, const wchar_t * value) {

	PMAZINGER_DATA pMazinger = m_pEnv->getDataRW();
	if (pMazinger == NULL)
		return;

	int sourceCounter = 0;
	int targetCounter = 0;
	while(true) {
		wchar_t *achSecret = readString(pMazinger->achSecrets, sourceCounter);
		if (achSecret == NULL || achSecret[0] == L'\0')
		{
			freeSecret(achSecret);
			break;
		}
		else if (wcsicmp  (achSecret, secret) == 0)
		{
			freeSecret (achSecret); // Skip secret name
			skipString (pMazinger->achSecrets, sourceCounter); // Skip value
		}
		else
		{
			putString(pMazinger->achSecrets, targetCounter, achSecret);
			freeSecret (achSecret);
			moveString(pMazinger->achSecrets, sourceCounter, targetCounter);
		}
	}

	putString(pMazinger->achSecrets, targetCounter, secret);
	putString(pMazinger->achSecrets, targetCounter, value);
	putString(pMazinger->achSecrets, targetCounter, L"");

}

void SecretStore::setSecrets(const wchar_t * secrets) {
	PMAZINGER_DATA pMazinger = m_pEnv->getData();
	if (pMazinger == NULL)
		return;

	MZNSendDebugMessageA("Storing secrets for %s", m_pEnv->getUser());
	int counter = 0;
	int targetCounter = 0;
	memset (pMazinger->achSecrets, 0,  sizeof pMazinger->achSecrets);
#ifdef TRACE_AES
	wprintf (L"Setting secrets\n");
#endif
	while ( secrets[counter] != L'\0')
	{
//		MZNSendDebugMessageW(L"Storing secret %ls", &secrets[counter]);
#ifdef TRACE_AES
		wprintf (L"Set secret at %d: %s\n", targetCounter, &secrets[counter]);
#endif
		putString(pMazinger->achSecrets, targetCounter, &secrets[counter]);
#ifdef TRACE_AES
		wprintf (L"Counter now at %d\n", targetCounter);
#endif
		counter += wcslen (&secrets[counter])+1;
	}
	putString(pMazinger->achSecrets, targetCounter, L"");
}


wchar_t *SecretStore::readString (const wchar_t* buffer, int &index) {
	unsigned short int size = (short int) buffer[index] ; // size in wchars
	if (size == 0)
		return NULL;

	int bufferSizeBytes = AESCHUNK_SIZE * ( 1 + (size* sizeof (wchar_t))/AESCHUNK_SIZE); // size in bytes
	int bufferSizeChars = bufferSizeBytes / sizeof (wchar_t);

	if (index + bufferSizeChars + 1 >= SECRETS_BUFFER_SIZE)
		return NULL;

	index ++;
	unsigned char *out = (unsigned char*) malloc (bufferSizeBytes);

	unsigned char b[AESCHUNK_SIZE];
	for (unsigned int i = 0; i < bufferSizeBytes; i+=AESCHUNK_SIZE)
	{
		memcpy (b, (unsigned char*) &buffer[index], AESCHUNK_SIZE);
#ifdef TRACE_AES
		printf ("Decrypting\n");
		for (int j = 0; j < AESCHUNK_SIZE; j++)
		{
			printf ("%02x ", (unsigned int)b[j]);
		}
		printf("\n");
#endif
		AESDecrypt(b, m_expkey, &out[i]);
#ifdef TRACE_AES
		printf ("Descrypted\n");
		for (int j = 0; j < AESCHUNK_SIZE; j++)
		{
			printf ("%02x ", (unsigned int)out[i+j]);
		}
		printf("\n");
#endif
		index += AESCHUNK_SIZE / sizeof (wchar_t);
	}
	wchar_t * wout = (wchar_t*) out;
	wout[size] = L'\0';
	return wout;
}

void SecretStore::skipString (const wchar_t* buffer, int &index) {
	short int size = (short int) buffer[index] ; // size in wchars

	if (size == 0)
		return;

	int bufferSizeBytes = AESCHUNK_SIZE * ( 1 + (size*sizeof (wchar_t))/AESCHUNK_SIZE); // size in bytes
	int bufferSizeChars = bufferSizeBytes / sizeof (wchar_t);

	if (index + bufferSizeChars + 1 < SECRETS_BUFFER_SIZE)
		index += bufferSizeChars + 1;
}

void SecretStore::moveString (wchar_t* buffer, int &indexSource, int &indexTarget) {
	short int size = (*(short int*) (&buffer[indexSource])) ;

	int bufferSizeBytes = AESCHUNK_SIZE * ( 1 + (size*sizeof (wchar_t))/AESCHUNK_SIZE); // size in bytes
	int bufferSizeChars = bufferSizeBytes / sizeof (wchar_t);

	if (indexTarget != indexSource)
	{
		memmove(&buffer[indexTarget], &buffer[indexSource], sizeof (wchar_t) + bufferSizeBytes);
	}
	indexSource += bufferSizeChars + 1;
	indexTarget += bufferSizeChars + 1;
}

void SecretStore::putString (wchar_t* buffer, int &index, const wchar_t *text) {
	short int size = wcslen(text);

#ifdef TRACE_AES
	wprintf (L"Putting secret %s at %d\n", text, index);
#endif
	int bufferSizeBytes = AESCHUNK_SIZE * ( 1 + (size*sizeof (wchar_t))/AESCHUNK_SIZE); // size in bytes
	int bufferSizeChars = bufferSizeBytes / sizeof (wchar_t);

	if (index + bufferSizeChars + 1 >= SECRETS_BUFFER_SIZE)
	{
		// printf ("Secret store: Buffer overflow\n");
		return;
	}

	buffer[index] = size;
	index ++;
	unsigned char *tmp = (unsigned char*)malloc (bufferSizeBytes);
	wcscpy ((wchar_t*)tmp, text);

	for (int i = 0; i < bufferSizeBytes; i+=AESCHUNK_SIZE)
	{
#ifdef TRACE_AES
		printf ("Encrypting\n");
		for (int j = 0; j < AESCHUNK_SIZE; j++)
		{
			printf ("%02x ", (int)tmp[i+j]);
		}
		printf("\n");
#endif
		unsigned char*out = (unsigned char*) &buffer[index];
		AESEncrypt(&tmp[i], m_expkey,out);
#ifdef TRACE_AES
		printf ("Encrypted\n");
		for (int j = 0; j < AESCHUNK_SIZE; j++)
		{
			printf ("%02x ", (unsigned int)out[j]);
		}
		printf("\n");
#endif
		index += AESCHUNK_SIZE/sizeof (wchar_t);
	}
	freeSecret((wchar_t*) tmp);
}

void SecretStore::freeSecret (wchar_t* buffer) {
	if (buffer != NULL)
	{
		for (int i = 0; buffer[i]; i++)
			buffer[i]='\0';
		free(buffer);
	}
}

void SecretStore::removeSecret(const wchar_t* secret, const wchar_t* value) {
	PMAZINGER_DATA pMazinger = m_pEnv->getDataRW();
	if (pMazinger == NULL)
		return;

	int sourceCounter = 0;
	int targetCounter = 0;
	while(true) {
		wchar_t *achSecret = readString(pMazinger->achSecrets, sourceCounter);
		if (achSecret == NULL || achSecret[0] == L'\0')
		{
			freeSecret(achSecret);
			break;
		}
		else if (wcsicmp  (achSecret, secret) == 0)
		{
			wchar_t *achSecret2 = readString(pMazinger->achSecrets, sourceCounter);
			if (wcsicmp (achSecret, value))
			{
				// Do not copy
			}
			else
			{
				putString(pMazinger->achSecrets, targetCounter, achSecret);
				putString(pMazinger->achSecrets, targetCounter, achSecret2);
			}
			freeSecret (achSecret);
			freeSecret (achSecret2);
		}
		else
		{
			putString(pMazinger->achSecrets, targetCounter, achSecret);
			freeSecret (achSecret);
			moveString(pMazinger->achSecrets, sourceCounter, targetCounter);
		}
	}

	putString(pMazinger->achSecrets, targetCounter, L"");
}

void SecretStore::putSecret(const wchar_t* secret, const wchar_t* value) {
	PMAZINGER_DATA pMazinger = m_pEnv->getDataRW();
	if (pMazinger == NULL)
		return;

	int sourceCounter = 0;
	int targetCounter = 0;
	while(true) {
		targetCounter = sourceCounter;
		wchar_t *achSecret = readString(pMazinger->achSecrets, sourceCounter);
		if (achSecret == NULL || achSecret[0] == L'\0')
		{
			freeSecret(achSecret);
			break;
		}
		else
		{
			skipString(pMazinger->achSecrets, sourceCounter);
			freeSecret (achSecret);
		}
	}

	putString(pMazinger->achSecrets, targetCounter, secret);
	putString(pMazinger->achSecrets, targetCounter, value);
	putString(pMazinger->achSecrets, targetCounter, L"");
}
