// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Kiriflux Contributors

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QQuickStyle>
#include <QUrl>
#include <QtWebEngineQuick/qtwebenginequickglobal.h>

#include <KAboutData>
#include <KDBusService>
#include <KLocalizedContext>
#include <KLocalizedString>

#include "kiriflux-version.h"
#include "KirifluxImageProvider.h"
#include "core/AppController.h"
#include "core/KirifluxLogging.h"

int main(int argc, char *argv[])
{

    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--disable-gpu-compositing");
    QtWebEngineQuick::initialize();
    qCDebug(KIRIFLUX_LOG) << "Kiriflux starting...";
    qCDebug(KIRIFLUX_LOG) << "QML2_IMPORT_PATH:" << qgetenv("QML2_IMPORT_PATH");
    qCDebug(KIRIFLUX_LOG) << "QT_PLUGIN_PATH:" << qgetenv("QT_PLUGIN_PATH");
    qCDebug(KIRIFLUX_LOG) << "XDG_DATA_DIRS:" << qgetenv("XDG_DATA_DIRS");

    // 1. Core metadata
    QCoreApplication::setApplicationName(QStringLiteral("kiriflux"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    QCoreApplication::setOrganizationName(QStringLiteral("KDE"));
    QCoreApplication::setApplicationVersion(QStringLiteral(KIRIFLUX_VERSION_STRING));

    // 2. GUI Application initialization
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/icons/org.kde.kiriflux.svg"));
    
    KLocalizedString::setApplicationDomain("kiriflux");
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }

    // 3. KDE framework metadata
    KAboutData about(
        QStringLiteral("kiriflux"), 
        i18n("Kiriflux"),
        QStringLiteral(KIRIFLUX_VERSION_STRING),
        i18n("A desktop-first RSS reader for Miniflux"),
        KAboutLicense::GPL_V3,
        i18n("© 2026 Kiriflux Contributors"));
    about.addAuthor(i18n("Kiriflux Team"), QString(), QString(), QStringLiteral("https://codeberg.org/gnuslashdhruv/kiriflux"));
    about.setHomepage(QStringLiteral("https://codeberg.org/gnuslashdhruv/kiriflux"));
    about.setBugAddress(QByteArrayLiteral("https://codeberg.org/gnuslashdhruv/kiriflux/issues"));
    about.setDesktopFileName(QStringLiteral("org.kde.kiriflux"));
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
    engine.addImageProvider(QStringLiteral("kiriflux"), new KirifluxImageProvider());
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    
    // Register context properties via AppController
    appController.registerContextProperties(&engine);

    // 8. Load from explicit QRC path
    const QUrl url(QStringLiteral("qrc:/qt/qml/org/kde/kiriflux/Main.qml"));
    qCDebug(KIRIFLUX_LOG) << "Loading QML from:" << url;
    
    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load QML root objects!";
        return -1;
    }


    qCDebug(KIRIFLUX_LOG) << "Kiriflux running.";
    return app.exec();
}
