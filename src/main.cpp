// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Kiriflux Contributors

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QQuickStyle>
#include <QUrl>
#include <QDebug>
#include <QtWebEngineQuick/qtwebenginequickglobal.h>

#include <KAboutData>
#include <KDBusService>
#include <KLocalizedContext>
#include <KLocalizedString>

#include "core/AppController.h"
#include "core/KirifluxNAMFactory.h"

int main(int argc, char *argv[])
{

    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--disable-gpu-compositing");
    QtWebEngineQuick::initialize();
    qDebug() << "Kiriflux starting...";
    qDebug() << "QML2_IMPORT_PATH:" << qgetenv("QML2_IMPORT_PATH");
    qDebug() << "QT_PLUGIN_PATH:" << qgetenv("QT_PLUGIN_PATH");
    qDebug() << "XDG_DATA_DIRS:" << qgetenv("XDG_DATA_DIRS");

    // 1. Core metadata
    QCoreApplication::setApplicationName(QStringLiteral("kiriflux"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    QCoreApplication::setOrganizationName(QStringLiteral("KDE"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    // 2. GUI Application initialization
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/icons/org.kde.kiriflux.svg"));
    
    KLocalizedString::setApplicationDomain("kiriflux");
    QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));

    // 3. KDE framework metadata
    KAboutData about(
        QStringLiteral("kiriflux"), 
        i18n("Kiriflux"),
        QStringLiteral("0.1.0"),
        i18n("A desktop-first RSS reader for Miniflux"),
        KAboutLicense::GPL_V3,
        i18n("© 2026 Kiriflux Contributors"));
    about.addAuthor(i18n("Kiriflux Team"), QString(), QStringLiteral("https://github.com"));
    KAboutData::setApplicationData(about);
    
    app.setDesktopFileName(QStringLiteral("org.kde.kiriflux"));

    // 4. DBus service registration
    KDBusService service(KDBusService::Unique);

    // 5. Backend Logic
    AppController appController;
    if (!appController.init()) {
        qWarning() << "Failed to initialize application backend.";
    }

    // 6. QML Engine Setup
    QQmlApplicationEngine engine;
    engine.setNetworkAccessManagerFactory(new KirifluxNAMFactory());
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    
    // Register context properties via AppController
    appController.registerContextProperties(&engine);

    // 8. Load from explicit QRC path
    const QUrl url(QStringLiteral("qrc:/qt/qml/org/kde/kiriflux/Main.qml"));
    qDebug() << "Loading QML from:" << url;
    
    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load QML root objects!";
        return -1;
    }


    qDebug() << "Kiriflux running.";
    return app.exec();
}
