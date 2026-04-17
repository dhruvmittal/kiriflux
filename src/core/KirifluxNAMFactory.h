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
        nam->setCache(NetworkManager::instance().cache());
        return nam;
    }
};
