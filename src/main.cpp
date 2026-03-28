// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Kiriflux Contributors

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>
#include <QDir>
#include <QFileInfo>

#include <KAboutData>
#include <KDBusService>
#include <KLocalizedContext>
#include <KLocalizedString>
#include <QtQml/QtQml>

#include "core/DatabaseManager.h"
#include "core/SyncEngine.h"
#include "core/FeedListModel.h"
#include "core/ArticleListModel.h"

int main(int argc, char *argv[])
{
    // 1. Core metadata (MUST be before any frameworks logic)
    QCoreApplication::setApplicationName(QStringLiteral("kiriflux"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    QCoreApplication::setOrganizationName(QStringLiteral("KDE"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    // 2. GUI Application initialization
    QApplication app(argc, argv);
    
    KLocalizedString::setApplicationDomain("kiriflux");
    QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));

    // 3. KDE framework metadata
    // KAboutData::setApplicationData will also call QCoreApplication::setApplicationName
    // based on the component name we provide here.
    KAboutData about(
        QStringLiteral("kiriflux"), 
        i18n("Kiriflux"),
        QStringLiteral("0.1.0"),
        i18n("A desktop-first RSS reader for Miniflux"),
        KAboutLicense::GPL_V3,
        i18n("© 2026 Kiriflux Contributors"));
    about.addAuthor(i18n("Kiriflux Team"), QString(), QStringLiteral("https://github.com"));
    KAboutData::setApplicationData(about);
    
    // Critical for Wayland and Portal-based menu mapping
    app.setDesktopFileName(QStringLiteral("org.kde.kiriflux"));

    // 4. DBus service registration
    // Because organizationDomain is kde.org and appName is kiriflux, 
    // KF6 KDBusService will register org.kde.kiriflux.
    KDBusService service(KDBusService::Unique);

    // 5. Backend Logic
    if (!DatabaseManager::instance().init()) {
        qWarning() << "Failed to initialize database.";
    }
    SyncEngine::instance().startIntervalSync();

    // 6. QML Interface
    qmlRegisterType<FeedListModel>("org.kde.kiriflux", 1, 0, "FeedListModel");
    qmlRegisterType<ArticleListModel>("org.kde.kiriflux", 1, 0, "ArticleListModel");

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.rootContext()->setContextProperty(QStringLiteral("syncEngine"), &SyncEngine::instance());

    const QString appDir = QCoreApplication::applicationDirPath();
    const QString devPath = appDir + QStringLiteral("/Main.qml");
    const QString installPath = QStringLiteral(KIRIFLUX_DATA_DIR) + QStringLiteral("/qml/Main.qml");

    QString qmlPath;
    if (QFileInfo::exists(devPath)) {
        qmlPath = devPath;
    } else if (QFileInfo::exists(installPath)) {
        qmlPath = installPath;
    } else {
        qWarning() << "Could not find Main.qml at:" << devPath << "or" << installPath;
        return -1;
    }

    engine.load(QUrl::fromLocalFile(qmlPath));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
