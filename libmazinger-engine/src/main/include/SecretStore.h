/*
 * SecretStore.h
 *
 *  Created on: 02/07/2010
 *      Author: u07286
 */

#ifndef SECRETSTORE_H_
#define SECRETSTORE_H_

#include <string>
#include <vector>
#include <map>

class MazingerEnv;

class SecretStore {
public:
	SecretStore(const char *user);
	SecretStore(const char *user, const char *desktop);
	virtual ~SecretStore();

	wchar_t * getSecret(const wchar_t * secret);
	std::map<std::wstring,std::wstring> getSecretsByPrefix(const wchar_t * prefix);
	std::vector<std::pair<std::wstring,std::wstring> > getSecretsByPrefix2(const wchar_t * prefix);
	std::vector<std::wstring> getSecrets(const wchar_t * secret);
	void setSecret(const wchar_t * secret, const wchar_t * value);
	void removeSecret(const wchar_t * secret, const wchar_t * value);
	void putSecret(const wchar_t * secret, const wchar_t * value);
	void setSecrets(const wchar_t *);
	void dump();
	void dump(wchar_t *wchSecrets, int debugLevel);
	void freeSecret (wchar_t* buffer) ;
private:
	wchar_t * getSecret(const wchar_t * secret, bool caseSensitive);
	unsigned char* m_expkey;
    void applySeed(const unsigned char *lpszSeed, int size, unsigned char achKey[], int & k);
    wchar_t *readString (const wchar_t* buffer, int &index);
	void skipString (const wchar_t* buffer, int &index);
	void putString (wchar_t* buffer, int &index, const wchar_t *text);
	void moveString (wchar_t* buffer, int &indexSource, int &indexTarget);
	MazingerEnv *m_pEnv;
};

#endif /* SECRETSTORE_H_ */
