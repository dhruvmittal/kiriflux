// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Kiriflux Contributors

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>
#include <QDebug>
#include <QtWebEngineQuick/qtwebenginequickglobal.h>

#include <KAboutData>
#include <KDBusService>
#include <KLocalizedContext>
#include <KLocalizedString>

#include "core/DatabaseManager.h"
#include "core/SyncEngine.h"
#include "core/FeedListModel.h"
#include "core/ArticleListModel.h"

int main(int argc, char *argv[])
{
    QtWebEngineQuick::initialize();
    qDebug() << "Kiriflux starting...";

    // 1. Core metadata
    QCoreApplication::setApplicationName(QStringLiteral("kiriflux"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    QCoreApplication::setOrganizationName(QStringLiteral("KDE"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    // 2. GUI Application initialization
    QApplication app(argc, argv);
    
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
    if (!DatabaseManager::instance().init()) {
        qWarning() << "Failed to initialize database.";
    }
    SyncEngine::instance().startIntervalSync();

    // 6. Manual QML Registration
    qmlRegisterType<FeedListModel>("org.kde.kiriflux", 1, 0, "FeedListModel");
    qmlRegisterType<ArticleListModel>("org.kde.kiriflux", 1, 0, "ArticleListModel");

    // 7. QML Engine Setup
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.rootContext()->setContextProperty(QStringLiteral("syncEngine"), &SyncEngine::instance());

    // 8. Load from explicit QRC path
    const QUrl url(QStringLiteral("qrc:/Main.qml"));
    qDebug() << "Loading QML from:" << url;
    
    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load QML root objects!";
        return -1;
    }

    qDebug() << "Kiriflux running.";
    return app.exec();
}
