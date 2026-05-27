// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QQmlNetworkAccessManagerFactory>
#include "NetworkManager.h"

class KirifluxNAMFactory : public QQmlNetworkAccessManagerFactory
{
public:
    QNetworkAccessManager *create(QObject *parent) override
    {
        QNetworkAccessManager *nam = new QNetworkAccessManager(parent);
        QNetworkDiskCache *cache = new QNetworkDiskCache(nam);
        cache->setCacheDirectory(NetworkManager::instance().cache()->cacheDirectory());
        cache->setMaximumCacheSize(NetworkManager::instance().cache()->maximumCacheSize());
        nam->setCache(cache);
        return nam;
    }
};
