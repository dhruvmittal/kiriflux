// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QSharedPointer>

class NetworkManager : public QObject
{
    Q_OBJECT
public:
    static NetworkManager& instance();

    QNetworkAccessManager* nam() const { return m_nam; }
    QNetworkDiskCache* cache() const { return m_cache; }

    void clearCache();

private:
    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager() = default;

    void updateCacheSettings();

    QNetworkAccessManager* m_nam;
    QNetworkDiskCache* m_cache;
};
