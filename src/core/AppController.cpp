// SPDX-License-Identifier: GPL-3.0-or-later
#include "AppController.h"
#include "DatabaseManager.h"
#include "SyncEngine.h"
#include "FeedListModel.h"
#include "ArticleListModel.h"
#include "CategoryListModel.h"
#include "SettingsManager.h"
#include "kiriflux.h"
#include <QDebug>

AppController::AppController(QObject *parent)
    : QObject(parent)
{
}

AppController::~AppController() = default;

bool AppController::init()
{
    if (!DatabaseManager::instance().init()) {
        qWarning() << "Failed to initialize database.";
        return false;
    }

    m_feedListModel = new FeedListModel(this);
    m_articleListModel = new ArticleListModel(this);
    m_categoryListModel = new CategoryListModel(this);

    // Perform weekly database maintenance if enabled
    if (Settings::self()->autoPruneEnabled()) {
        QDateTime lastVacuum = Settings::self()->lastVacuumDate();
        if (!lastVacuum.isValid() || lastVacuum.daysTo(QDateTime::currentDateTime()) >= 7) {
            qDebug() << "Running weekly database optimization...";
            DatabaseManager::instance().optimizeDatabase();
        }
    }

    SyncEngine::instance().startIntervalSync();
    return true;
}

void AppController::registerContextProperties(QQmlApplicationEngine* engine)
{
    if (!engine) return;

    QQmlContext* ctx = engine->rootContext();
    ctx->setContextProperty(QStringLiteral("databaseManager"), &DatabaseManager::instance());
    ctx->setContextProperty(QStringLiteral("syncEngine"), &SyncEngine::instance());
    ctx->setContextProperty(QStringLiteral("settings"), &SettingsManager::instance());
    ctx->setContextProperty(QStringLiteral("feedListModel"), m_feedListModel);
    ctx->setContextProperty(QStringLiteral("articleListModel"), m_articleListModel);
    ctx->setContextProperty(QStringLiteral("categoryListModel"), m_categoryListModel);
}
