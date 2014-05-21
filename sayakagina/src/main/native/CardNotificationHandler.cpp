/*
 * LockOutManager.cpp
 *
 *  Created on: 11/02/2011
 *      Author: u07286
 */

#include "sayaka.h"
#include "winwlx.h"
#include <windows.h>

#define SECURITY_WIN32

#include <security.h>

#include "logindialog.h"
#include "LockOutManager.h"
#include "lm.h"
#include "Utils.h"
#include "time.h"

#include "NotificationHandler.h"
#include "CardNotificationHandler.h"
#include "Pkcs11Handler.h"
#include "CertificateHandler.h"


CardNotificationHandler::CardNotificationHandler(HWND* hWnd):
	m_log ("CardNotificationHandler") {
	this->hwnd = hWnd;
	cardInserted = false;
}
void CardNotificationHandler::onProviderAdd () {
	m_log.info ("onProviderAdd");
}
void CardNotificationHandler::onCardInsert () {
	m_log.info ("onCardInsert");
	cardInserted = true;
	if (*hwnd != NULL) {
		PostMessageA(*hwnd, WM_CLOSE, 0, 0);
	}
}
void CardNotificationHandler::onCardRemove () {
	m_log.info ("onCardRemove");
}

CardNotificationHandler::~CardNotificationHandler() {}

