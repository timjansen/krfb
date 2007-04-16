/***************************************************************************
     Copyright 2002 Tim Jansen <tim@tjansen.de>
     Copyright 2002 Stefan Taferner <taferner@kde.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "invitation.h"
#include <KRandom>
/*
 * Function for (en/de)crypting strings for config file, taken from KMail
 * Author: Stefan Taferner <taferner@alpin.or.at>
 */
static QString cryptStr(const QString &aStr) {
        QString result;
        for ( int i = 0; i < aStr.length(); i++)
                result += (aStr[i].unicode() < 0x20) ? aStr[i] :
                                QChar(0x1001F - aStr[i].unicode());
        return result;
}

// a random string that doesn't contain i, I, o, O, 1, 0
// based on KRandom::randomString()
static QString readableRandomString(int length) {
   QString str;
   while (length)
   {
      int r = KRandom::random() % 62;
      r += 48;
      if (r > 57)
	      r += 7;
      if (r > 90)
	      r += 6;
      char c = char(r);
      if ((c == 'i') ||
	  (c == 'I') ||
	  (c == '1') ||
	  (c == 'o') ||
	  (c == 'O') ||
	  (c == '0'))
	      continue;
      str += c;
      length--;
   }
   return str;
}

Invitation::Invitation()
{
	m_password = readableRandomString(4)+'-'+readableRandomString(3);
	m_creationTime = QDateTime::currentDateTime();
	m_expirationTime = QDateTime::currentDateTime().addSecs(INVITATION_DURATION);
}

Invitation::Invitation(const Invitation &x)
    : m_password(x.m_password), m_creationTime(x.m_creationTime), m_expirationTime(x.m_expirationTime)
{
}

Invitation::Invitation(const KConfigGroup &config) {
	m_password = cryptStr(config.readEntry("password", QString()));
	m_creationTime = config.readEntry("creation", QDateTime());
	m_expirationTime = config.readEntry("expiration", QDateTime());
}

Invitation::~Invitation() {
}

Invitation &Invitation::operator= (const Invitation&x) {
	m_password = x.m_password;
	m_creationTime = x.m_creationTime;
	m_expirationTime = x.m_expirationTime;
	return *this;
}

void Invitation::save(KConfigGroup &config) const {
	config.writeEntry("password", cryptStr(m_password));
	config.writeEntry("creation", m_creationTime);
	config.writeEntry("expiration", m_expirationTime);
}

QString Invitation::password() const {
	return m_password;
}

QDateTime Invitation::expirationTime() const {
	return m_expirationTime;
}

QDateTime Invitation::creationTime() const {
	return m_creationTime;
}

bool Invitation::isValid() const {
	return m_expirationTime > QDateTime::currentDateTime();
}

bool Invitation::operator ==(const Invitation & ot)
{
    return m_creationTime == ot.m_creationTime && m_password == ot.m_password;
}