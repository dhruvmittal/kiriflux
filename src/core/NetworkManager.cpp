// SPDX-License-Identifier: GPL-3.0-or-later
#include "NetworkManager.h"
#include "kiriflux.h"
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
#include <QNetworkReply>
#include <QNetworkRequest>

NetworkManager& NetworkManager::instance()
{
    static NetworkManager *inst = nullptr;
    if (!inst) {
        inst = new NetworkManager(nullptr);
        qAddPostRoutine([]() {
            delete inst;
            inst = nullptr;
        });
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
    
    connect(Settings::self(), &Settings::CacheSizeMBChanged, this, &NetworkManager::updateCacheSettings);
}

void NetworkManager::updateCacheSettings()
{
    // Convert MB to bytes
    m_cache->setMaximumCacheSize(static_cast<qint64>(Settings::self()->cacheSizeMB()) * 1024 * 1024);
}

void NetworkManager::clearCache()
{
    m_cache->clear();
}

void NetworkManager::prefetchUrl(const QUrl& url)
{
    const bool validScheme = (url.scheme() == QLatin1String("http") || url.scheme() == QLatin1String("https"));
    if (!url.isValid() || !validScheme || url.host().isEmpty()) {
        return;
    }
    if (m_prefetchQueue.contains(url) || m_inFlightPrefetches.contains(url)) {
        return;
    }

    m_prefetchQueue.append(url);
    processPrefetchQueue();
}

void NetworkManager::processPrefetchQueue()
{
    constexpr int kConcurrencyLimit = 6;

    while (m_inFlightPrefetches.size() < kConcurrencyLimit && !m_prefetchQueue.isEmpty()) {
        const QUrl url = m_prefetchQueue.takeFirst();

        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Kiriflux/1.0"));
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
        request.setPriority(QNetworkRequest::LowPriority);
        request.setTransferTimeout(60000);

        m_inFlightPrefetches.insert(url);
        QNetworkReply *reply = m_nam->get(request);

        connect(reply, &QNetworkReply::finished, this, [this, reply, url]() {
            m_inFlightPrefetches.remove(url);
            reply->deleteLater();
            processPrefetchQueue();
        });
    }
}
