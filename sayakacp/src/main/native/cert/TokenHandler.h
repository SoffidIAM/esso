/*
 * TokenHandler.h
 *
 *  Created on: 09/02/2011
 *      Author: u07286
 */

#ifndef TOKENHANDLER_H_
#define TOKENHANDLER_H_

#include <pkcs11.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

class Pkcs11Handler;
class CertificateHandler;

class TokenHandler {
public:
	TokenHandler();
	virtual ~TokenHandler();


    Pkcs11Handler *getHandler() const
    {
        return m_handler;
    }

    CK_SLOT_ID getSlot_num() const
    {
        return m_slot_num;
    }

    const unsigned char* getTokenManufacturer() const
    {
        return m_tokenManufacturer;
    }

    const unsigned char* getTokenModel() const
    {
        return m_tokenModel;
    }

    const unsigned char* getTokenSerialNumber() const
    {
        return m_tokenSerialNumber;
    }


    void setHandler(Pkcs11Handler *m_handler)
    {
        this->m_handler = m_handler;
    }

    void setSlot_num(CK_SLOT_ID m_slot_num)
    {
        this->m_slot_num = m_slot_num;
    }

    void setTokenManufacturer(const unsigned char *m_tokenManufacturer)
    {
        memcpy(this->m_tokenManufacturer, m_tokenManufacturer, sizeof this->m_tokenManufacturer);
    }

    void setTokenModel(const unsigned char *m_tokenModel)
    {
        memcpy(this->m_tokenModel, m_tokenModel, sizeof this->m_tokenModel);
    }

    void setTokenSerialNumber(const unsigned char *m_tokenSerialNumber)
    {
        memcpy(this->m_tokenSerialNumber, m_tokenSerialNumber, sizeof this->m_tokenSerialNumber);
    }


    const char *getPin () {
    	return m_pin.c_str();
    }

    void setPin (const char *szPin) {
    	m_pin.assign(szPin);
    }

    void clearPin () {
    	m_pin.clear();
    }

	bool isDNIe () ;

	const char *getTokenDescription () ;

private:
	std::string m_pin;
	std::vector<CertificateHandler*> m_certs;
	std::string m_name;

	Pkcs11Handler *m_handler;
	CK_SLOT_ID m_slot_num;
	unsigned char m_tokenManufacturer[32];
	unsigned char m_tokenModel[16];
	unsigned char m_tokenSerialNumber[16];
};

#endif /* TOKENHANDLER_H_ */
