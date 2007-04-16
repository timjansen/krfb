/***************************************************************************
                                   main.cpp
                             -------------------
    begin                : Sat Dec  8 03:23:02 CET 2001
    copyright            : (C) 2001-2003 by Tim Jansen
    email                : tim@tjansen.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "trayicon.h"
//#include "configuration.h"
#include "krfbserver.h"
#include "manageinvitationsdialog.h"

#include <QPixmap>
#include <kaction.h>
#include <kdebug.h>
#include <KNotification>
#include <ksystemtrayicon.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kaboutapplicationdialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <qwindowdefs.h>

#include <signal.h>

#define VERSION "1.0"

static const char description[] = I18N_NOOP("VNC-compatible server to share "
					   "KDE desktops");


int main(int argc, char *argv[])
{
	KAboutData aboutData( "krfb", I18N_NOOP("Desktop Sharing"),
        VERSION, description, KAboutData::License_GPL,
        "(c) 2007, Alessandro Praduroux\n"
            "(c) 2001-2003, Tim Jansen\n"
        "(c) 2001, Johannes E. Schindelin\n"
        "(c) 2000, heXoNet Support GmbH, D-66424 Homburg\n"
        "(c) 2000-2001, Const Kaplinsky\n"
        "(c) 2000, Tridia Corporation\n"
        "(c) 1999, AT&T Laboratories Cambridge\n");
    aboutData.addAuthor("Alessandro Praduroux", I18N_NOOP("KDE4 porting"), "pradu@pradu.it");
    aboutData.addAuthor("Tim Jansen", "", "tim@tjansen.de");
    aboutData.addAuthor("Ian Reinhart Geiser", I18N_NOOP("DCOP interface"), "geiseri@kde.org");
	aboutData.addCredit("Johannes E. Schindelin",
			    I18N_NOOP("libvncserver"));
	aboutData.addCredit("Const Kaplinsky",
			    I18N_NOOP("TightVNC encoder"));
	aboutData.addCredit("Tridia Corporation",
			    I18N_NOOP("ZLib encoder"));
	aboutData.addCredit("AT&T Laboratories Cambridge",
			    I18N_NOOP("original VNC encoders and "
				      "protocol design"));
	aboutData.addCredit("Jens Wagner (heXoNet Support GmbH)",
			    I18N_NOOP("X11 update scanner, "
				      "original code base"));
	aboutData.addCredit("Jason Spisak",
			    I18N_NOOP("Connection side image"),
			    "kovalid@yahoo.com");
	aboutData.addCredit("Karl Vogel",
			    I18N_NOOP("KDesktop background deactivation"));
	KCmdLineArgs::init(argc, argv, &aboutData);

	KApplication app;

    TrayIcon trayicon(new ManageInvitationsDialog);

	KrfbServer *server = KrfbServer::self(); // initialize the server manager
    if (!server->checkX11Capabilities()) {
        return 1;
    }

	QObject::connect(&app, SIGNAL(lastWindowClosed()), // do not show passivepopup
            &trayicon, SLOT(prepareQuit()));
	QObject::connect(&app, SIGNAL(lastWindowClosed()),
            server, SLOT(disconnectAndQuit()));

	QObject::connect(&trayicon, SIGNAL(enableDesktopControl(bool)),
			 server, SLOT(enableDesktopControl(bool)));
	QObject::connect(server, SIGNAL(sessionEstablished(QString)),
			 &trayicon, SLOT(showConnectedMessage(QString)));
	QObject::connect(server, SIGNAL(sessionFinished()),
			 &trayicon, SLOT(showDisconnectedMessage()));
	QObject::connect(server, SIGNAL(desktopControlSettingChanged(bool)),
			 &trayicon, SLOT(setDesktopControlSetting(bool)));
	QObject::connect(&trayicon, SIGNAL(quitApp()),
			 server, SLOT(disconnectAndQuit()));
    QObject::connect(server, SIGNAL(quitApp()),
                      &app, SLOT(quit()));

	sigset_t sigs;
	sigemptyset(&sigs);
	sigaddset(&sigs, SIGPIPE);
	sigprocmask(SIG_BLOCK, &sigs, 0);

	return app.exec();
}
