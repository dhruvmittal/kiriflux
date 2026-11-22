// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QSet>
#include <QUrl>

class NetworkManager : public QObject
{
    Q_OBJECT
public:
    static NetworkManager& instance();

    QNetworkAccessManager* nam() const { return m_nam; }
    QNetworkDiskCache* cache() const { return m_cache; }

    Q_INVOKABLE void clearCache();
    void prefetchUrl(const QUrl& url);
    int activePrefetchCount() const { return m_inFlightPrefetches.size(); }

private:
    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager() override = default;

    void updateCacheSettings();
    void processPrefetchQueue();

    QNetworkAccessManager* m_nam;
    QNetworkDiskCache* m_cache;
    QList<QUrl> m_prefetchQueue;
    QSet<QUrl> m_inFlightPrefetches;
};
