// SPDX-License-Identifier: GPL-3.0-or-later
#include "NetworkManager.h"
#include "SettingsManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>

NetworkManager& NetworkManager::instance()
{
    static NetworkManager *inst = nullptr;
    if (!inst) {
        inst = new NetworkManager();
    }
    return *inst;
}

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_cache(new QNetworkDiskCache(this))
{
    QString cachePath;
    if (QCoreApplication::instance()) {
        cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/images");
    } else {
        cachePath = QDir::tempPath() + QStringLiteral("/kiriflux-test-cache/images");
    }
    
    QDir().mkpath(cachePath);
    m_cache->setCacheDirectory(cachePath);
    
    updateCacheSettings();
    m_nam->setCache(m_cache);
    
    // Connect to settings changes
    connect(&SettingsManager::instance(), &SettingsManager::cacheSizeMBChanged, this, &NetworkManager::updateCacheSettings);
    connect(&SettingsManager::instance(), &SettingsManager::clearCacheRequested, this, &NetworkManager::clearCache);
}

void NetworkManager::updateCacheSettings()
{
    // Convert MB to bytes
    m_cache->setMaximumCacheSize(static_cast<qint64>(SettingsManager::instance().cacheSizeMB()) * 1024 * 1024);
}

void NetworkManager::clearCache()
{
    m_cache->clear();
}
