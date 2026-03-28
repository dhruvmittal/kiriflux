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
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("kiriflux");
    QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));

    if (!DatabaseManager::instance().init()) {
        qWarning() << "Failed to initialize database.";
    }
    SyncEngine::instance().startIntervalSync();

    KDBusService service(KDBusService::Unique);

    qmlRegisterType<FeedListModel>("org.kde.kiriflux", 1, 0, "FeedListModel");
    qmlRegisterType<ArticleListModel>("org.kde.kiriflux", 1, 0, "ArticleListModel");

    KAboutData about(
        QStringLiteral("kiriflux"),
        i18n("Kiriflux"),
        QStringLiteral("0.1.0"),
        i18n("A desktop-first RSS reader for Miniflux"),
        KAboutLicense::GPL_V3,
        i18n("© 2026 Kiriflux Contributors"));
    about.addAuthor(i18n("Kiriflux Team"), QString(), QStringLiteral("https://github.com"));
    KAboutData::setApplicationData(about);

    QQmlApplicationEngine engine;

    // Provide KLocalizedContext for i18n() in QML
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    
    // Expose SyncEngine as a context property
    engine.rootContext()->setContextProperty(QStringLiteral("syncEngine"), &SyncEngine::instance());

    // Load Main.qml from the filesystem
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
