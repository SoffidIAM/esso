/*
 * Pkcs11Handler.cpp
 *
 *  Created on: 03/02/2011
 *      Author: u07286
 */

#include "sayaka.h"

#include "Pkcs11Handler.h"

#include "sayaka.h"
#include <pkcs11.h>
#include <stdio.h>
#include <string.h>
#include <credentialprovider.h>
#include "../cert/CertificateHandler.h"
#include "SayakaProvider.h"
#include "TokenHandler.h"

# include "Utils.h"

#define ATTR_METHOD(ATTR, TYPE) \
TYPE \
Pkcs11Handler::get##ATTR(CK_SESSION_HANDLE sess, CK_OBJECT_HANDLE obj) \
{ \
	TYPE		type = 0; \
	CK_ATTRIBUTE	attr = { CKA_##ATTR, &type, sizeof(type) }; \
	CK_RV		rv; \
 \
	rv = m_p11->C_GetAttributeValue(sess, obj, &attr, 1); \
	if (rv != CKR_OK) \
		p11_warn("C_GetAttributeValue(" #ATTR ")", rv); \
	return type; \
}

#define VARATTR_METHOD(ATTR, TYPE) \
TYPE * \
Pkcs11Handler::get##ATTR(CK_SESSION_HANDLE sess, CK_OBJECT_HANDLE obj, CK_ULONG_PTR pulCount) \
{ \
	CK_ATTRIBUTE	attr = { CKA_##ATTR, NULL, 0 }; \
	CK_RV		rv; \
 \
	rv = m_p11->C_GetAttributeValue(sess, obj, &attr, 1); \
	if (rv == CKR_OK) { \
		if (!(attr.pValue = calloc(1, attr.ulValueLen + 1))) \
			m_log.warn("Out of memory in get" #ATTR ":"); \
		rv = m_p11->C_GetAttributeValue(sess, obj, &attr, 1); \
		if (pulCount) \
			*pulCount = attr.ulValueLen / sizeof(TYPE); \
	} else {\
		p11_warn("C_GetAttributeValue(" #ATTR ")", rv); \
	} \
	return (TYPE *) attr.pValue; \
}

/*
 * Define attribute accessors
 */
ATTR_METHOD(CLASS, CK_OBJECT_CLASS)
#if 0
ATTR_METHOD(TOKEN, CK_BBOOL);
ATTR_METHOD(LOCAL, CK_BBOOL);
ATTR_METHOD(SENSITIVE, CK_BBOOL);
ATTR_METHOD(ALWAYS_SENSITIVE, CK_BBOOL);
ATTR_METHOD(NEVER_EXTRACTABLE, CK_BBOOL);
ATTR_METHOD(PRIVATE, CK_BBOOL)
ATTR_METHOD(MODIFIABLE, CK_BBOOL)
ATTR_METHOD(ENCRYPT, CK_BBOOL)
ATTR_METHOD(DECRYPT, CK_BBOOL)
ATTR_METHOD(SIGN, CK_BBOOL)
#endif
#if 0
ATTR_METHOD(SIGN_RECOVER, CK_BBOOL)
ATTR_METHOD(VERIFY, CK_BBOOL)
#endif
#if 0
ATTR_METHOD(VERIFY_RECOVER, CK_BBOOL)
ATTR_METHOD(WRAP, CK_BBOOL)
ATTR_METHOD(UNWRAP, CK_BBOOL)
#endif
#if 0
ATTR_METHOD(DERIVE, CK_BBOOL)
ATTR_METHOD(EXTRACTABLE, CK_BBOOL)
ATTR_METHOD(KEY_TYPE, CK_KEY_TYPE)
#endif
ATTR_METHOD(CERTIFICATE_TYPE, CK_CERTIFICATE_TYPE)
#if 0
VARATTR_METHOD(LABEL, char)
ATTR_METHOD(MODULUS_BITS, CK_ULONG)
VARATTR_METHOD(APPLICATION, char)
#endif
VARATTR_METHOD(ID, unsigned char)

#if 0
VARATTR_METHOD(OBJECT_ID, unsigned char)
VARATTR_METHOD(MODULUS, unsigned char)
#endif
VARATTR_METHOD(VALUE, unsigned char)

static const char * CKR2Str(CK_ULONG res) {
	switch (res) {
	case CKR_OK:
		return "CKR_OK";
	case CKR_CANCEL:
		return "CKR_CANCEL";
	case CKR_HOST_MEMORY:
		return "CKR_HOST_MEMORY";
	case CKR_SLOT_ID_INVALID:
		return "CKR_SLOT_ID_INVALID";
	case CKR_GENERAL_ERROR:
		return "CKR_GENERAL_ERROR";
	case CKR_FUNCTION_FAILED:
		return "CKR_FUNCTION_FAILED";
	case CKR_ARGUMENTS_BAD:
		return "CKR_ARGUMENTS_BAD";
	case CKR_NO_EVENT:
		return "CKR_NO_EVENT";
	case CKR_NEED_TO_CREATE_THREADS:
		return "CKR_NEED_TO_CREATE_THREADS";
	case CKR_CANT_LOCK:
		return "CKR_CANT_LOCK";
	case CKR_ATTRIBUTE_READ_ONLY:
		return "CKR_ATTRIBUTE_READ_ONLY";
	case CKR_ATTRIBUTE_SENSITIVE:
		return "CKR_ATTRIBUTE_SENSITIVE";
	case CKR_ATTRIBUTE_TYPE_INVALID:
		return "CKR_ATTRIBUTE_TYPE_INVALID";
	case CKR_ATTRIBUTE_VALUE_INVALID:
		return "CKR_ATTRIBUTE_VALUE_INVALID";
	case CKR_DATA_INVALID:
		return "CKR_DATA_INVALID";
	case CKR_DATA_LEN_RANGE:
		return "CKR_DATA_LEN_RANGE";
	case CKR_DEVICE_ERROR:
		return "CKR_DEVICE_ERROR";
	case CKR_DEVICE_MEMORY:
		return "CKR_DEVICE_MEMORY";
	case CKR_DEVICE_REMOVED:
		return "CKR_DEVICE_REMOVED";
	case CKR_ENCRYPTED_DATA_INVALID:
		return "CKR_ENCRYPTED_DATA_INVALID";
	case CKR_ENCRYPTED_DATA_LEN_RANGE:
		return "CKR_ENCRYPTED_DATA_LEN_RANGE";
	case CKR_FUNCTION_CANCELED:
		return "CKR_FUNCTION_CANCELED";
	case CKR_FUNCTION_NOT_PARALLEL:
		return "CKR_FUNCTION_NOT_PARALLEL";
	case CKR_FUNCTION_NOT_SUPPORTED:
		return "CKR_FUNCTION_NOT_SUPPORTED";
	case CKR_KEY_HANDLE_INVALID:
		return "CKR_KEY_HANDLE_INVALID";
	case CKR_KEY_SIZE_RANGE:
		return "CKR_KEY_SIZE_RANGE";
	case CKR_KEY_TYPE_INCONSISTENT:
		return "CKR_KEY_TYPE_INCONSISTENT";
	case CKR_KEY_NOT_NEEDED:
		return "CKR_KEY_NOT_NEEDED";
	case CKR_KEY_CHANGED:
		return "CKR_KEY_CHANGED";
	case CKR_KEY_NEEDED:
		return "CKR_KEY_NEEDED";
	case CKR_KEY_INDIGESTIBLE:
		return "CKR_KEY_INDIGESTIBLE";
	case CKR_KEY_FUNCTION_NOT_PERMITTED:
		return "CKR_KEY_FUNCTION_NOT_PERMITTED";
	case CKR_KEY_NOT_WRAPPABLE:
		return "CKR_KEY_NOT_WRAPPABLE";
	case CKR_KEY_UNEXTRACTABLE:
		return "CKR_KEY_UNEXTRACTABLE";
	case CKR_MECHANISM_INVALID:
		return "CKR_MECHANISM_INVALID";
	case CKR_MECHANISM_PARAM_INVALID:
		return "CKR_MECHANISM_PARAM_INVALID";
	case CKR_OBJECT_HANDLE_INVALID:
		return "CKR_OBJECT_HANDLE_INVALID";
	case CKR_OPERATION_ACTIVE:
		return "CKR_OPERATION_ACTIVE";
	case CKR_OPERATION_NOT_INITIALIZED:
		return "CKR_OPERATION_NOT_INITIALIZED";
	case CKR_PIN_INCORRECT:
		return "CKR_PIN_INCORRECT";
	case CKR_PIN_INVALID:
		return "CKR_PIN_INVALID";
	case CKR_PIN_LEN_RANGE:
		return "CKR_PIN_LEN_RANGE";
	case CKR_PIN_EXPIRED:
		return "CKR_PIN_EXPIRED";
	case CKR_PIN_LOCKED:
		return "CKR_PIN_LOCKED";
	case CKR_SESSION_CLOSED:
		return "CKR_SESSION_CLOSED";
	case CKR_SESSION_COUNT:
		return "CKR_SESSION_COUNT";
	case CKR_SESSION_HANDLE_INVALID:
		return "CKR_SESSION_HANDLE_INVALID";
	case CKR_SESSION_PARALLEL_NOT_SUPPORTED:
		return "CKR_SESSION_PARALLEL_NOT_SUPPORTED";
	case CKR_SESSION_READ_ONLY:
		return "CKR_SESSION_READ_ONLY";
	case CKR_SESSION_EXISTS:
		return "CKR_SESSION_EXISTS";
	case CKR_SESSION_READ_ONLY_EXISTS:
		return "CKR_SESSION_READ_ONLY_EXISTS";
	case CKR_SESSION_READ_WRITE_SO_EXISTS:
		return "CKR_SESSION_READ_WRITE_SO_EXISTS";
	case CKR_SIGNATURE_INVALID:
		return "CKR_SIGNATURE_INVALID";
	case CKR_SIGNATURE_LEN_RANGE:
		return "CKR_SIGNATURE_LEN_RANGE";
	case CKR_TEMPLATE_INCOMPLETE:
		return "CKR_TEMPLATE_INCOMPLETE";
	case CKR_TEMPLATE_INCONSISTENT:
		return "CKR_TEMPLATE_INCONSISTENT";
	case CKR_TOKEN_NOT_PRESENT:
		return "CKR_TOKEN_NOT_PRESENT";
	case CKR_TOKEN_NOT_RECOGNIZED:
		return "CKR_TOKEN_NOT_RECOGNIZED";
	case CKR_TOKEN_WRITE_PROTECTED:
		return "CKR_TOKEN_WRITE_PROTECTED";
	case CKR_UNWRAPPING_KEY_HANDLE_INVALID:
		return "CKR_UNWRAPPING_KEY_HANDLE_INVALID";
	case CKR_UNWRAPPING_KEY_SIZE_RANGE:
		return "CKR_UNWRAPPING_KEY_SIZE_RANGE";
	case CKR_UNWRAPPING_KEY_TYPE_INCONSISTENT:
		return "CKR_UNWRAPPING_KEY_TYPE_INCONSISTENT";
	case CKR_USER_ALREADY_LOGGED_IN:
		return "CKR_USER_ALREADY_LOGGED_IN";
	case CKR_USER_NOT_LOGGED_IN:
		return "CKR_USER_NOT_LOGGED_IN";
	case CKR_USER_PIN_NOT_INITIALIZED:
		return "CKR_USER_PIN_NOT_INITIALIZED";
	case CKR_USER_TYPE_INVALID:
		return "CKR_USER_TYPE_INVALID";
	case CKR_USER_ANOTHER_ALREADY_LOGGED_IN:
		return "CKR_USER_ANOTHER_ALREADY_LOGGED_IN";
	case CKR_USER_TOO_MANY_TYPES:
		return "CKR_USER_TOO_MANY_TYPES";
	case CKR_WRAPPED_KEY_INVALID:
		return "CKR_WRAPPED_KEY_INVALID";
	case CKR_WRAPPED_KEY_LEN_RANGE:
		return "CKR_WRAPPED_KEY_LEN_RANGE";
	case CKR_WRAPPING_KEY_HANDLE_INVALID:
		return "CKR_WRAPPING_KEY_HANDLE_INVALID";
	case CKR_WRAPPING_KEY_SIZE_RANGE:
		return "CKR_WRAPPING_KEY_SIZE_RANGE";
	case CKR_WRAPPING_KEY_TYPE_INCONSISTENT:
		return "CKR_WRAPPING_KEY_TYPE_INCONSISTENT";
	case CKR_RANDOM_SEED_NOT_SUPPORTED:
		return "CKR_RANDOM_SEED_NOT_SUPPORTED";
	case CKR_RANDOM_NO_RNG:
		return "CKR_RANDOM_NO_RNG";
	case CKR_DOMAIN_PARAMS_INVALID:
		return "CKR_DOMAIN_PARAMS_INVALID";
	case CKR_BUFFER_TOO_SMALL:
		return "CKR_BUFFER_TOO_SMALL";
	case CKR_SAVED_STATE_INVALID:
		return "CKR_SAVED_STATE_INVALID";
	case CKR_INFORMATION_SENSITIVE:
		return "CKR_INFORMATION_SENSITIVE";
	case CKR_STATE_UNSAVEABLE:
		return "CKR_STATE_UNSAVEABLE";
	case CKR_CRYPTOKI_NOT_INITIALIZED:
		return "CKR_CRYPTOKI_NOT_INITIALIZED";
	case CKR_CRYPTOKI_ALREADY_INITIALIZED:
		return "CKR_CRYPTOKI_ALREADY_INITIALIZED";
	case CKR_MUTEX_BAD:
		return "CKR_MUTEX_BAD";
	case CKR_MUTEX_NOT_LOCKED:
		return "CKR_MUTEX_NOT_LOCKED";
	case CKR_VENDOR_DEFINED:
		return "CKR_VENDOR_DEFINED";
	}
	return "Unknown PKCS11 error";
}

void Pkcs11Handler::p11_fatal(const char *func, CK_RV rv) {
	m_log.warn ("PKCS11 function %s failed: rv = %s (0x%0x)\n", func,
			CKR2Str(rv), (unsigned int) rv);
}

void Pkcs11Handler::p11_warn(const char *func, CK_RV rv) {
	m_log.warn ("PKCS11 function %s failed: rv = %s (0x%0x)\n", func,
			CKR2Str(rv), (unsigned int) rv);
}

Pkcs11Handler::Pkcs11Handler(const char *pszLibraryName, HANDLE hMutex):
	m_log ("Pkcs11Handler")
{
	m_hParentWindow = NULL;
	m_p11 = NULL;
	m_hMutex = hMutex;

	if (pszLibraryName == NULL)
		pszLibraryName = "bit4ipki.dll";


	libraryName.assign (pszLibraryName);
	char achPath[MAX_PATH];
	if (! ExpandEnvironmentStrings(pszLibraryName, achPath, MAX_PATH))
		expandedLibraryName.assign(pszLibraryName);
	else
		expandedLibraryName.assign(achPath);


	std::string s;
	s.assign ("Pkcs11Handler/");
	s.append(expandedLibraryName.c_str());
	m_log.setName(s.c_str());

	m_log.info("Loading module %s", expandedLibraryName.c_str());
	m_hInstance = LoadLibrary(expandedLibraryName.c_str());
	if (m_hInstance == NULL) {
		m_log.warn("Cannot find library: %s", expandedLibraryName.c_str());
		return;
	}
	m_log.info("Loaded module %s", expandedLibraryName.c_str());


}

void Pkcs11Handler::loadModule () {
	CK_RV (*c_get_function_list)(CK_FUNCTION_LIST_PTR_PTR);
	int rv;


	if (m_p11 != NULL || m_hInstance == NULL)
		return;

	std::string s;

	/* Get the list of function pointers */
	c_get_function_list = (CK_RV(*)(CK_FUNCTION_LIST_PTR_PTR)) GetProcAddress(
			m_hInstance, "C_GetFunctionList");
	if (!c_get_function_list)
		return;
	CK_FUNCTION_LIST_PTR funcs;
	rv = c_get_function_list(&funcs);
	if (rv == CKR_OK)
	{
		m_p11 = funcs;
		CK_C_INITIALIZE_ARGS args;
		args.CreateMutex = NULL;
		args.DestroyMutex = NULL;
		args.LockMutex = NULL;
		args.UnlockMutex = NULL;
		args.flags = CKF_LIBRARY_CANT_CREATE_OS_THREADS | CKF_OS_LOCKING_OK;
		args.pReserved = NULL;

		m_p11->C_Initialize(NULL);
	}

	return;
}

Pkcs11Handler::~Pkcs11Handler() {
	clearTokenVector();
	unloadModule ();
}

void Pkcs11Handler::unloadModule () {
	if (m_p11 == NULL)
		return;

	m_p11->C_Finalize (NULL);
	m_p11 = NULL;
}



void Pkcs11Handler::addCert(CertificateHandler *cert) {
	boolean add = true;

	m_log.info ("Adding cert %s", cert->getName());
	for (std::vector<CertificateHandler*>::iterator it = m_certs.begin();
			it != m_certs.end();
			)
	{
		CertificateHandler *cert2 = *it;
		if (strcmp(cert2->getName(), cert->getName()) == 0) {
			add = false;
			FILETIME ft2 = cert2->getExpirationDate();
			FILETIME ft = cert->getExpirationDate();
			if (ft2.dwHighDateTime < ft.dwHighDateTime
					|| (ft2.dwHighDateTime == ft.dwHighDateTime
							&& ft2.dwLowDateTime > ft.dwLowDateTime)) {
				m_log.info("Deleting duplicated certificate");
				it = m_certs.erase(it);
				delete cert2;
				m_log.info("Deleted duplicated certificate");
			} else {
				it ++;
			}
		} else {
			it ++;
		}
	}

	m_certs.push_back(cert);
}

void Pkcs11Handler::evaluateCert (CK_SESSION_HANDLE sess, CK_OBJECT_HANDLE obj, TokenHandler *pToken) {
	CK_CERTIFICATE_TYPE cert_type = getCERTIFICATE_TYPE(sess, obj);
	CK_ULONG sizeId;
	CK_ULONG sizeValue;
	unsigned char *id;
	unsigned char *value;

	if (cert_type == CKC_X_509) {
		if ((id = getID(sess, obj, &sizeId)) != NULL && sizeId) {
			if ((value = getVALUE(sess, obj, &sizeValue)) != NULL
					&& sizeValue) {

				CertificateHandler *cert = new CertificateHandler(this);
				cert->setTokenHandler(pToken);
				cert->setId(id, sizeId);
				cert->setRawCert(value, sizeValue);


				PCCERT_CONTEXT pCertContext = cert->getCertContext();
				if (pCertContext != NULL) {
					m_log.info("Certificado: %s\n", (LPCSTR) cert->getName());
					m_log.info("     Issuer: %s\n", (LPCSTR) cert->getIssuer());

					if (strncmp (cert->getIssuer(), "AC DNI", 6) == 0 &&
							strstr (cert->getName(), "(FIRMA)") != NULL ) {
						m_log.info ("Ignoring DNIE signature cert\n");
						delete cert;
						return ;
					}
					bool isCA = false;
					// Si el certificat �s v�lid
					if (CertVerifyTimeValidity(NULL, pCertContext->pCertInfo)
							!= 0) {
						m_log.info ("Certificate not valid (yet)");
						delete cert;
						return;
					}
					// Determinar si es o no CA

					CERT_EXTENSION *extension = CertFindExtension(
							szOID_BASIC_CONSTRAINTS,
							pCertContext->pCertInfo->cExtension,
							pCertContext->pCertInfo->rgExtension);
					if (extension != NULL) {
						m_log.info ("Found constraints extension\n");
						CERT_BASIC_CONSTRAINTS_INFO* info;
						DWORD dwSize;
						if (CryptDecodeObjectEx(X509_ASN_ENCODING,
								szOID_BASIC_CONSTRAINTS,
								extension->Value.pbData,
								extension->Value.cbData,
								CRYPT_DECODE_ALLOC_FLAG, NULL, (void*) &info,
								&dwSize)) {
							char ch = info->SubjectType.pbData[0];
							if (ch & CERT_CA_SUBJECT_FLAG)
								isCA = true;
							else
								m_log.info ("Is not CA");
							LocalFree(info);
						}
					} else {
						CERT_EXTENSION *extension2 = CertFindExtension(
								szOID_BASIC_CONSTRAINTS2,
								pCertContext->pCertInfo->cExtension,
								pCertContext->pCertInfo->rgExtension);
						if (extension2 != NULL) {
							m_log.info ("Found constraints2 extension");
							CERT_BASIC_CONSTRAINTS2_INFO* info;
							DWORD dwSize;
							if (CryptDecodeObjectEx(X509_ASN_ENCODING,
									szOID_BASIC_CONSTRAINTS2,
									extension2->Value.pbData,
									extension2->Value.cbData,
									CRYPT_DECODE_ALLOC_FLAG, NULL,
									(void*) &info, &dwSize)) {

								if (info->fCA)
									isCA = true;
								else
									m_log.info ("Is not CA");
								LocalFree(info);
							}
						} else {
							m_log.info ("No constraints extension found");
						}

					}

					if (!isCA) {
						m_log.info ("Adding cert");
						addCert(cert);

					} else {
						delete cert;
					}
				}

				free(value);
			}
			free(id);
		}
	}
}

void Pkcs11Handler::enumerateToken(CK_SESSION_HANDLE sess, TokenHandler *pToken) {
	CK_OBJECT_HANDLE object;
	CK_ULONG count;
	CK_RV rv;

	// Localizar claves privadas
	rv = m_p11->C_FindObjectsInit(sess, NULL, 0);
	if (rv != CKR_OK) {
		p11_fatal("C_FindObjectsInit", rv);
		return;
	}

	while (true) {
		rv = m_p11->C_FindObjects(sess, &object, 1, &count);
		if (rv != CKR_OK) {
			p11_fatal("C_FindObjects", rv);
			return;
		}
		if (count == 0)
			break;
		// Determinar si es una clave privada y determinar su label
		CK_OBJECT_CLASS cls = getCLASS(sess, object);
		if (cls == CKO_CERTIFICATE) {
			evaluateCert( sess, object, pToken);
		}
	}
	m_p11->C_FindObjectsFinal(sess);
}


bool Pkcs11Handler::login(CK_SESSION_HANDLE session, TokenHandler *token) {
	const char *szPin = token->getPin();

	if (szPin == NULL || szPin[0] == '\0')
	{
		return false;
	}
	CK_RV rv;

	/* Identify which pin to enter */

	rv = m_p11->C_Login(session, CKU_USER, (CK_UTF8CHAR *) szPin, strlen(szPin));
	if (rv != CKR_OK) {
		// Hack dnie
		p11_fatal("C_Login", rv);
		return false;
	}

	return true;

}


static const char *p11_utf8_to_local(CK_UTF8CHAR *string, size_t len) {
	static char buffer[512];
	size_t n, m;

	while (len && string[len - 1] == ' ')
		len--;

	/* For now, simply copy this thing */
	for (n = m = 0; n < sizeof(buffer) - 1; n++) {
		if (m >= len)
			break;
		buffer[n] = string[m++];
	}
	buffer[n] = '\0';
	return buffer;
}

struct flag_info {
	CK_FLAGS value;
	const char * name;
};

static const char *p11_flag_names(struct flag_info *list, CK_FLAGS value) {
	static char buffer[1024];
	const char *sepa = "";

	buffer[0] = '\0';
	while (list->value) {
		if (list->value & value) {
			strcat(buffer, sepa);
			strcat(buffer, list->name);
			value &= ~list->value;
			sepa = ", ";
		}
		list++;
	}
	if (value) {
		sprintf(buffer + strlen(buffer), "%sother flags=0x%x", sepa,
				(unsigned int) value);
	}
	return buffer;
}

static const char *p11_token_info_flags(CK_FLAGS value)
{
	static struct flag_info	slot_flags[] = {
		{ CKF_RNG, "rng" },
		{ CKF_WRITE_PROTECTED, "readonly" },
		{ CKF_LOGIN_REQUIRED, "login required" },
		{ CKF_USER_PIN_INITIALIZED, "PIN initialized" },
		{ CKF_PROTECTED_AUTHENTICATION_PATH, "PIN pad present" },
		{ CKF_TOKEN_INITIALIZED, "token initialized" },
		{ 0, NULL }
	};

	return p11_flag_names(slot_flags, value);
}

static const char *p11_slot_info_flags(CK_FLAGS value)
{
	static struct flag_info	slot_flags[] = {
		{ CKF_TOKEN_PRESENT, "token present" },
		{ CKF_REMOVABLE_DEVICE, "removable device" },
		{ CKF_HW_SLOT, "hardware slot" },
		{ 0, NULL }
	};

	return p11_flag_names(slot_flags, value);
}

TokenHandler* Pkcs11Handler::createToken (CK_SLOT_ID slot) {
	CK_TOKEN_INFO info;
	CK_RV rv;

	rv = m_p11->C_GetTokenInfo(slot, &info);
	if (rv != CKR_OK) {
		p11_fatal("C_GetTokenInfo", rv);
		return NULL;
	}

/*	if (!(info.flags & CKF_TOKEN_INITIALIZED)) {
		if (! isDNIe(info)) {
			m_log.info("  token state:   uninitialized\n");
			return NULL;
		}
	}
	*/
	m_log.info("  token label:   %s", p11_utf8_to_local(info.label,sizeof(info.label)));
	m_log.info("  token manuf:   %s", p11_utf8_to_local(info.manufacturerID,sizeof(info.manufacturerID)));
	m_log.info("  token model:   %s", p11_utf8_to_local(info.model,sizeof(info.model)));
	m_log.info("  token flags:   %s", p11_token_info_flags(info.flags));
	m_log.info("  serial num  :  %s", p11_utf8_to_local(info.serialNumber,sizeof(info.serialNumber)));
	if (info.manufacturerID[0] == '\0' ||
		info.serialNumber[0] == '\0') return NULL;

	TokenHandler *token = new TokenHandler();

	token->setHandler(this);
	token->setSlot_num(slot);
	token->setTokenManufacturer(info.manufacturerID);
	token->setTokenModel(info.model);
	token->setTokenSerialNumber(info.serialNumber);
	return token;
}


std::vector<CertificateHandler*>& Pkcs11Handler::enumCertificates (TokenHandler* token) {
	m_certs.clear ();
	CK_SESSION_HANDLE session;
	CK_RV  rv;

	rv = m_p11->C_OpenSession(token->getSlot_num(), CKF_SERIAL_SESSION, NULL, NULL, &session);

	if (rv != CKR_OK) {
		p11_warn("OpenSession", rv);
		return m_certs;
	}

	if (login(session, token)) {
		enumerateToken(session, token);
		m_p11->C_Logout(session);
	} else {
		MessageBox (m_hParentWindow,
				Utils::LoadResourcesString(1).c_str(),
				Utils::LoadResourcesString(2).c_str(),
				MB_OK|MB_ICONINFORMATION);
	}

	m_log.info("Token analyzed");

	m_p11->C_CloseSession(session);

	return m_certs;
}

void Pkcs11Handler::clearTokenVector () {
	for ( std::vector<TokenHandler*>::iterator it = m_tokens.begin();
			it != m_tokens.end(); it++)
	{
		delete (*it);
	}
	m_tokens.clear();
}

std::vector<TokenHandler*>& Pkcs11Handler::enumTokens () {
	CK_SLOT_INFO info;
	CK_RV rv;
	CK_SLOT_ID_PTR p11_slots = NULL;
	CK_ULONG p11_num_slots = 0;

	clearTokenVector();
	m_log.info ("Waiting for mutex");
	if (WAIT_OBJECT_0 == WaitForSingleObject(m_hMutex, INFINITE))  // no time-out interval
	{
		loadModule();
		if (m_p11 == NULL) {
			m_log.info ("Cannot load library");
			ReleaseMutex(m_hMutex);
			return m_tokens;
		}
		m_log.info ("Enumerating tokenss");

		/* Get the list of slots */
		rv = m_p11->C_GetSlotList(false, p11_slots, &p11_num_slots);
		if (rv != CKR_OK && rv != CKR_BUFFER_TOO_SMALL)
			p11_fatal("C_GetSlotList", rv);
		m_log.info ("Reading %d slots", p11_num_slots);
		if (p11_num_slots == 0) {
			m_log.info("No slot present");
			ReleaseMutex(m_hMutex);
			return m_tokens;
		}
		p11_slots = (CK_SLOT_ID *) calloc(p11_num_slots, sizeof(CK_SLOT_ID));
		if (p11_slots == NULL) {
			m_log.warn("Unable to allocate memory");
			ReleaseMutex(m_hMutex);
			return m_tokens;
		}
		rv = m_p11->C_GetSlotList(false, p11_slots, &p11_num_slots);
		if (rv != CKR_OK) {
			p11_fatal("C_GetSlotList", rv);
			ReleaseMutex(m_hMutex);
			return m_tokens;
		}

		for (unsigned int n = 0; n < p11_num_slots; n++) {
			rv = m_p11->C_GetSlotInfo(p11_slots[n], &info);
			if (rv != CKR_OK) {
				continue;
			}
			if (!(info.flags & CKF_TOKEN_PRESENT)) {
				m_log.info ("(empty)\n");
				continue;
			}

			m_log.info ("%s", p11_utf8_to_local(info.slotDescription,
					sizeof(info.slotDescription)));
			m_log.info ("  manufacturer:  %s", p11_utf8_to_local(info.manufacturerID,
					sizeof(info.manufacturerID)));
			m_log.info ("  hardware ver:  %u.%u", info.hardwareVersion.major,
					info.hardwareVersion.minor);
			m_log.info ("  firmware ver:  %u.%u", info.firmwareVersion.major,
					info.firmwareVersion.minor);
			m_log.info ("  flags:         %s", p11_slot_info_flags(info.flags));
			if (info.flags & CKF_TOKEN_PRESENT) {
				TokenHandler *token = createToken (p11_slots[n]);
				if (token != NULL)
					m_tokens.push_back(token);
			}
		}
		ReleaseMutex(m_hMutex);
	}
	m_log.info ("End enum tokens");
	return m_tokens;

}


bool Pkcs11Handler::sign( CertificateHandler *cert,
		const char *szDataToSign,
		unsigned char **ppData,
		unsigned long *pdwSize) {
	if (WAIT_OBJECT_0 == WaitForSingleObject(m_hMutex, INFINITE))  // no time-out interval
	{

		loadModule();
		CK_SESSION_HANDLE session;
		CK_RV rv = m_p11->C_OpenSession(cert->getTokenHandler()->getSlot_num(),
				CKF_SERIAL_SESSION, NULL, NULL, &session);

		if (rv != CKR_OK) {
			p11_warn("OpenSession", rv);
			ReleaseMutex(m_hMutex);
			return FALSE;
		}
		if (!login(session, cert->getTokenHandler()))
		{
			m_p11->C_CloseSession(session);
			ReleaseMutex(m_hMutex);
			return FALSE;
		}

		// Obtenir la clau

		CK_ATTRIBUTE attrs[2];

		attrs[0].pValue = cert->getId();
		attrs[0].ulValueLen = cert->getIdLength();
		attrs[0].type = CKA_ID;
		CK_OBJECT_CLASS objclass = CKO_PRIVATE_KEY;
		attrs[1].pValue = &objclass;
		attrs[1].ulValueLen = sizeof objclass;
		attrs[1].type = CKA_CLASS;

		rv = m_p11->C_FindObjectsInit(session, attrs, 2);
		if (rv != CKR_OK) {
			p11_warn("C_FindObjects", rv);
			m_p11->C_CloseSession(session);
			ReleaseMutex(m_hMutex);
			return FALSE;
		}
		CK_OBJECT_HANDLE key;
		unsigned long cObjects;
		rv = m_p11->C_FindObjects(session, &key, 1, &cObjects);
		if (rv != CKR_OK) {
			p11_warn("C_FindObjects", rv);
			m_p11->C_CloseSession(session);
			ReleaseMutex(m_hMutex);
			return FALSE;
		}
		m_p11->C_FindObjectsFinal(session);

		if (cObjects == 0) {
			m_log.warn("Cannot find certificate when trying to sign with %s", cert->getName());
			m_p11->C_CloseSession(session);
			ReleaseMutex(m_hMutex);
			return false;
		}

		CK_MECHANISM mecha;
		mecha.mechanism = CKM_RSA_PKCS;
		mecha.pParameter = NULL;
		mecha.ulParameterLen = 0;
		rv = m_p11->C_SignInit(session, &mecha, key);

		*pdwSize = 0;
		rv = m_p11->C_Sign(session, (unsigned char*) szDataToSign, strlen(
				szDataToSign), NULL, pdwSize);
		if (rv != CKR_BUFFER_TOO_SMALL && rv != CKR_OK) {
			p11_warn("c_sign(pre)", rv);
			m_p11->C_CloseSession(session);
			ReleaseMutex(m_hMutex);
			return false;
		}
		*ppData = (unsigned char*) malloc(*pdwSize);
		rv = m_p11->C_Sign(session, (unsigned char*) szDataToSign, strlen(
				szDataToSign), *ppData, pdwSize);

		if (rv != CKR_OK) {
			p11_warn("c_sign", rv);
			m_p11->C_CloseSession(session);
			ReleaseMutex(m_hMutex);
			return false;
		}
		m_p11->C_Logout(session);
		m_p11->C_CloseSession(session);

		ReleaseMutex (m_hMutex);
	}
	return TRUE;

}


bool Pkcs11Handler::getSavedCredentials(CertificateHandler *cert, wchar_t* &szUser, wchar_t *&szPassword) {
	bool found = false;

	if (WAIT_OBJECT_0 == WaitForSingleObject(m_hMutex, INFINITE))  // no time-out interval
	{
		CK_SESSION_HANDLE session;
		CK_RV rv = m_p11->C_OpenSession(cert->getTokenHandler()->getSlot_num(),
				CKF_SERIAL_SESSION | CKF_RW_SESSION, NULL, NULL, &session);

		if (rv != CKR_OK) {
			p11_warn("OpenSession", rv);
			ReleaseMutex(m_hMutex);
			return FALSE;
		}

		// Ignorar fallo de autenticaci�n
		if (!login(session, cert->getTokenHandler())) {
			ReleaseMutex(m_hMutex);
			return FALSE;
		}

		// Esborrar la clau antiga
		CK_UTF8CHAR labelValue[] = "seyconpwd";

		CK_ATTRIBUTE attrs[2];
		attrs[0].pValue = labelValue;
		attrs[0].ulValueLen = sizeof labelValue;
		attrs[0].type = CKA_LABEL;
		CK_OBJECT_CLASS objclass = CKO_DATA;
		attrs[1].pValue = &objclass;
		attrs[1].ulValueLen = sizeof objclass;
		attrs[1].type = CKA_CLASS;

		rv = m_p11->C_FindObjectsInit(session, attrs, 2);
		if (rv != CKR_OK) {
			p11_warn("C_FindObjects", rv);
			m_p11->C_CloseSession(session);
			ReleaseMutex(m_hMutex);
			return FALSE;
		}
		CK_OBJECT_HANDLE obj;
		unsigned long cObjects;
		rv = m_p11->C_FindObjects(session, &obj, 1, &cObjects);
		while (rv == CKR_OK && cObjects >= 1) {
			unsigned long ulSize;
			wchar_t *value = (wchar_t*) getVALUE(session, obj, &ulSize);
			if (value != NULL && ! found) {
				found = true;
				int len1 = wcslen(value) + 1;
				szUser = (wchar_t*) CoTaskMemAlloc (sizeof (wchar_t) * (len1));
				wcscpy(szUser, value);
				int len2 = wcslen(&value[len1]) + 1;
				szPassword = (wchar_t*) CoTaskMemAlloc (sizeof (wchar_t) * (len2));
				wcscpy(szPassword, &value[len1]);
			}
			rv = m_p11->C_FindObjects(session, &obj, 1, &cObjects);
		}
		m_p11->C_FindObjectsFinal(session);

		m_p11->C_Logout(session);
		m_p11->C_CloseSession(session);

		ReleaseMutex (m_hMutex);
	}
	return found;

}

bool Pkcs11Handler::saveCredentials(CertificateHandler *cert, wchar_t* szUser, wchar_t *szPassword) {
	if (WAIT_OBJECT_0 == WaitForSingleObject(m_hMutex, INFINITE))  // no time-out interval
	{
		CK_SESSION_HANDLE session;
		CK_RV rv = m_p11->C_OpenSession(cert->getTokenHandler()->getSlot_num(),
				CKF_SERIAL_SESSION | CKF_RW_SESSION, NULL, NULL, &session);

		if (rv != CKR_OK) {
			p11_warn("OpenSession", rv);
			ReleaseMutex(m_hMutex);
			return FALSE;
		}

		// Ignorar fallo de autenticaci�n
		if (!login(session, cert->getTokenHandler()))
		{
			m_p11->C_CloseSession(session);
			ReleaseMutex(m_hMutex);
			return FALSE;
		}

		// Esborrar la clau antiga
		CK_UTF8CHAR labelValue[] = "seyconpwd";

		CK_ATTRIBUTE attrs[2];
		attrs[0].pValue = labelValue;
		attrs[0].ulValueLen = sizeof labelValue;
		attrs[0].type = CKA_LABEL;
		CK_OBJECT_CLASS objclass = CKO_DATA;
		attrs[1].pValue = &objclass;
		attrs[1].ulValueLen = sizeof objclass;
		attrs[1].type = CKA_CLASS;

		rv = m_p11->C_FindObjectsInit(session, attrs, 2);
		if (rv != CKR_OK) {
			p11_warn("C_FindObjects", rv);
			m_p11->C_CloseSession(session);
			ReleaseMutex(m_hMutex);
			return FALSE;
		}
		CK_OBJECT_HANDLE obj;
		unsigned long cObjects;
		rv = m_p11->C_FindObjects(session, &obj, 1, &cObjects);
		while (rv == CKR_OK && cObjects >= 1) {
			rv = m_p11->C_DestroyObject(session, obj);
			if (rv != CKR_OK)
				p11_warn("c_dEstroyobject", rv);
			rv = m_p11->C_FindObjects(session, &obj, 1, &cObjects);
		}
		m_p11->C_FindObjectsFinal(session);

		// Guardar la nova clau

		int len1 = wcslen(szUser) + 1;
		int len2 = wcslen(szPassword) + 1;
		int len = sizeof(wchar_t) * (len1 + len2);
		wchar_t *wchContent = (wchar_t*) malloc(len);
		wcscpy(wchContent, szUser);
		wcscpy(&wchContent[len1], szPassword);

		CK_OBJECT_CLASS dataClass = CKO_DATA;
		CK_UTF8CHAR application[] = "SEYCON";
		CK_BYTE_PTR dataValue = (CK_BYTE_PTR) wchContent;
		CK_BBOOL isTokenValue = true;

		CK_ATTRIBUTE dataTemplate[] = {
				{ CKA_CLASS, &dataClass, sizeof(dataClass) }, { CKA_TOKEN,
						&isTokenValue, sizeof(isTokenValue) }, { CKA_APPLICATION,
						application, sizeof(application) - 1 }, { CKA_LABEL,
						labelValue, sizeof(labelValue) }, { CKA_VALUE, dataValue,
						len } };

		CK_OBJECT_HANDLE hData;

		rv = m_p11->C_CreateObject(session, dataTemplate, 5, &hData);

		if (rv != CKR_OK) {
			p11_warn("c_createobject", rv);
			m_p11->C_CloseSession(session);
			return FALSE;
		}
		m_p11->C_Logout(session);
		m_p11->C_CloseSession(session);
		ReleaseMutex(m_hMutex);
	}

	return true;
}

