/* This file is part of the KDE project
   Copyright (C) 2007 Alessandro Praduroux <pradu@pradu.it>
             (C) 2001-2003 by Tim Jansen <tim@tjansen.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; version 2
   of the License.
*/
#include "connectioncontroller.h"
#include "connectioncontroller.moc"

#include <QX11Info>
#include <QHostInfo>
#include <QApplication>
#include <QDesktopWidget>
#include <QTcpSocket>
#include <QTimer>

#include <KConfig>
#include <KGlobal>
#include <KUser>
#include <KNotification>
#include <KLocale>


#include "invitationmanager.h"
#include "connectiondialog.h"
#include "events.h"
#include "krfbserver.h"

#include "krfbconfig.h"

#include <X11/Xutil.h>

static void clientGoneHook(rfbClientPtr cl)
{
    ConnectionController *cc = static_cast<ConnectionController *>(cl->clientData);
    cc->handleClientGone();
}


static bool checkPassword(const QString &p, unsigned char *ochallenge, const char *response, int len)
{

    if ((len == 0) && (p.length() == 0)) {
        return true;
    }

    char passwd[MAXPWLEN];
    unsigned char challenge[CHALLENGESIZE];

    memcpy(challenge, ochallenge, CHALLENGESIZE);
    bzero(passwd, MAXPWLEN);
    if (!p.isNull()) {
        strncpy(passwd, p.toLatin1(),
                (MAXPWLEN <= p.length()) ? MAXPWLEN : p.length());
    }

    rfbEncryptBytes(challenge, passwd);
    return memcmp(challenge, response, len) == 0;
}


ConnectionController::ConnectionController(struct _rfbClientRec *_cl, KrfbServer * parent)
    : QObject(parent), cl(_cl)
{
    cl->clientData = (void*)this;
}

ConnectionController::~ConnectionController()
{
}

enum rfbNewClientAction ConnectionController::handleNewClient()
{

    bool askOnConnect = KrfbConfig::askOnConnect();
    bool allowUninvited = KrfbConfig::allowUninvitedConnections();


#if 0
    int socket = cl->sock;
    // TODO: this drops the connection >.<
    QTcpSocket t;
    t.setSocketDescriptor(socket); //, QAbstractSocket::ConnectedState, QIODevice::NotOpen);
    remoteIp = t.peerAddress().toString();
#endif

    if (!allowUninvited && InvitationManager::self()->activeInvitations() == 0) {
        KNotification::event("ConnectionAttempted",
                             i18n("Attepted uninvited connection from %1: connection refused",
                                  remoteIp));
        return RFB_CLIENT_REFUSE;
    }

    if (!askOnConnect && InvitationManager::self()->activeInvitations() == 0) {
        KNotification::event("NewConnectionAutoAccepted",
                             i18n("Accepted uninvited connection from %1",
                                  remoteIp));

        emit sessionEstablished(remoteIp);
        return RFB_CLIENT_ACCEPT;
    }

    KNotification::event("NewConnectionOnHold",
                         i18n("Received connection from %1, on hold (waiting for confirmation)",
                              remoteIp));

    //cl->screen->authPasswdData = (void *)1;
    cl->clientGoneHook = clientGoneHook;

    ConnectionDialog *dialog = new ConnectionDialog(0);
    dialog->setRemoteHost(remoteIp);
    dialog->setAllowRemoteControl( true );

    connect(dialog, SIGNAL(okClicked()), SLOT(dialogAccepted()));
    connect(dialog, SIGNAL(cancelClicked()), SLOT(dialogRejected()));

    dialog->show();

    return RFB_CLIENT_ON_HOLD;
}

bool ConnectionController::handleCheckPassword(rfbClientPtr cl, const char *response, int len)
{
    bool allowUninvited = KrfbConfig::allowUninvitedConnections();
    QString password =  KrfbConfig::uninvitedConnectionPassword();

    bool authd = false;
    kDebug() << "about to start autentication" << endl;

    if (allowUninvited) {
        authd = checkPassword(password, cl->authChallenge, response, len);
    }

    if (!authd) {
        QList<Invitation> invlist = InvitationManager::self()->invitations();

        foreach(Invitation it, invlist) {
            kDebug() << "checking password" << endl;
            if (checkPassword(it.password(), cl->authChallenge, response, len) && it.isValid()) {
                authd = true;
                InvitationManager::self()->removeInvitation(it);
                break;
            }
        }
    }

    if (!authd) {
        if (InvitationManager::self()->invitations().size() > 0) {
            KNotification::event("InvalidPasswordInvitations",
                             i18n("Failed login attempt from %1: wrong password",
                                  remoteIp));
        } else {
            KNotification::event("InvalidPassword",
                             i18n("Failed login attempt from %1: wrong password",
                                  remoteIp));
        }
        return false;
    }


    return true;
}


void ConnectionController::handleKeyEvent(bool down, rfbKeySym keySym)
{
    if (controlEnabled) {
        KeyboardEvent ev(down, keySym);
        ev.exec();
    }
}

void ConnectionController::handlePointerEvent(int bm, int x, int y)
{
    if (controlEnabled) {
        PointerEvent ev(bm, x, y);
        ev.exec();
    }
}

void ConnectionController::handleClientGone()
{
    emit clientDisconnected(this);
    kDebug() << "client gone" << endl;
    deleteLater();
}

void ConnectionController::clipboardToServer(const QString &s)
{
    ClipboardEvent ev(this, s);
    ev.exec();
}

void ConnectionController::dialogAccepted()
{
    // rfbStartOnHoldClient(cl);
    cl->onHold = false;
}

void ConnectionController::dialogRejected()
{
    kDebug() << "refused connection" << endl;
    rfbRefuseOnHoldClient(cl);
}

void ConnectionController::setControlEnabled(bool enable)
{
    controlEnabled = enable;
}

