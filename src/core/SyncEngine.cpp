// SPDX-License-Identifier: GPL-3.0-or-later
#include "SyncEngine.h"
#include "DatabaseManager.h"
#include "NetworkManager.h"
#include "kiriflux.h"
#include <QCoreApplication>
#include <QLocale>

#include "KirifluxLogging.h"
#include <QRegularExpression>

SyncEngine& SyncEngine::instance()
{
    static SyncEngine* inst = nullptr;
    if (!inst) {
        DatabaseManager::instance();
        NetworkManager::instance();
        inst = new SyncEngine(nullptr);
        qAddPostRoutine([]() {
            delete inst;
            inst = nullptr;
        });
    }
    return *inst;
}

SyncEngine::SyncEngine(QObject *parent)
    : QObject(parent)
    , m_client(new MinifluxClient(this))
    , m_syncTimer(new QTimer(this))
    , m_statusBatchTimer(new QTimer(this))
    , m_errorThrottleTimer(new QTimer(this))
{
    m_statusBatchTimer->setInterval(1000); // 1-second batch window
    m_statusBatchTimer->setSingleShot(true);
    connect(m_statusBatchTimer, &QTimer::timeout, this, &SyncEngine::processStatusBatch);

    m_errorThrottleTimer->setSingleShot(true);
    m_errorThrottleTimer->setInterval(5000); // 5-second error throttle

    connect(m_syncTimer, &QTimer::timeout, this, [this]() {
        m_isManualSync = false;
        performSync();
    });

    connect(Settings::self(), &Settings::SyncIntervalChanged, this, [this]() {
        if (m_syncTimer->isActive()) {
            m_isManualSync = false;
            startIntervalSync();
        }
    });

    connect(m_client, &MinifluxClient::categoriesFetched, this, [this](const QList<Category>& categories) {
        DatabaseManager::instance().saveCategories(categories);
        m_client->fetchFeeds(Settings::self()->apiUrl(), Settings::self()->apiKey());
    });

    connect(m_client, &MinifluxClient::feedsFetched, this, [this](const QList<Feed>& feeds) {
        DatabaseManager::instance().saveFeeds(feeds);
        fetchMissingFeedIcons();
        m_client->fetchArticles(Settings::self()->apiUrl(), Settings::self()->apiKey());
    });

    connect(m_client, &MinifluxClient::articlesFetched, this, [this](const QList<Article>& articles) {
        // Find all IDs of fetched unread articles
        QSet<qint64> fetchedUnreadIds;
        int fetchedUnreadCount = 0;
        for (const auto& a : articles) {
            if (a.status == ArticleStatus::Unread) {
                fetchedUnreadIds.insert(a.id);
                fetchedUnreadCount++;
            }
        }

        // Clean up local unread articles that are no longer unread on the server.
        // We only do this if we didn't hit the 1000 limit, meaning we received ALL unread articles.
        if (fetchedUnreadCount < 1000) {
            const QList<qint64> localUnreadIds = DatabaseManager::instance().getUnreadArticleIds(-1, -1, false);
            QList<qint64> toMarkRead;
            for (qint64 id : localUnreadIds) {
                if (!fetchedUnreadIds.contains(id)) {
                    toMarkRead.append(id);
                }
            }
            if (!toMarkRead.isEmpty()) {
                DatabaseManager::instance().markArticlesAsRead(toMarkRead, true);
            }
        }

        DatabaseManager::instance().saveArticles(articles);

        m_isSyncing = false;
        if (Settings::self()->preFetchImages() && Settings::self()->useReaderMode()) {
            prefetchImages(articles);
        }

        Settings::self()->setLastSyncTime(QDateTime::currentDateTime());
        Settings::self()->save();
        emit syncStatusChanged();
        emit syncFinished(true);
        emit articlesUpdated(m_isManualSync);
        m_isManualSync = false;
    });

    connect(m_client, &MinifluxClient::contentFetched, this, [this](qint64 entryId, const QString& content) {
        DatabaseManager::instance().updateArticleContent(entryId, content);
        emit articleContentUpdated(entryId);
    });

    connect(m_client, &MinifluxClient::articleStatusUpdated, this, [this](qint64 entryId, const QString& status) {
        m_statusRequestInFlight = false;
        if (!m_pendingReadIds.isEmpty()) {
            m_statusBatchTimer->start();
        }
        emit articleStatusUpdated(entryId, status);
    });

    connect(m_client, &MinifluxClient::articleBookmarkToggled, this, [this](qint64 entryId) {
        emit articleBookmarkToggled(entryId);
    });

    connect(m_client, &MinifluxClient::feedIconFetched, this, [this](qint64 feedId, const QString& iconData) {
        DatabaseManager::instance().saveFeedIcon(feedId, iconData);
        m_inFlightIconFeedIds.remove(feedId);
        emit feedIconUpdated(feedId);
        processIconQueue();
    });

    connect(m_client, &MinifluxClient::feedIconFailed, this, [this](qint64 feedId) {
        m_inFlightIconFeedIds.remove(feedId);
        processIconQueue();
    });

    connect(m_client, &MinifluxClient::errorOccurred, this, &SyncEngine::handleError);
}

QList<QUrl> SyncEngine::extractImageUrls(const QString& html, const QUrl& baseUrl)
{
    static const QRegularExpression imgRegex(QStringLiteral("<img\\b[^>]*?\\ssrc=[\"']([^\"']+)[\"']"), QRegularExpression::CaseInsensitiveOption);

    QList<QUrl> urls;
    QSet<QUrl> seen;
    auto it = imgRegex.globalMatch(html);
    while (it.hasNext()) {
        QString raw = it.next().captured(1).trimmed();
        if (raw.isEmpty()) continue;
        raw.replace(QLatin1String("&amp;"), QLatin1String("&"), Qt::CaseInsensitive);

        const QUrl rawUrl(raw);
        const QUrl resolved = baseUrl.isValid() ? baseUrl.resolved(rawUrl) : rawUrl;
        const bool validScheme = (resolved.scheme() == QLatin1String("http") || resolved.scheme() == QLatin1String("https"));
        if (resolved.isValid() && validScheme && !resolved.host().isEmpty() && !seen.contains(resolved)) {
            seen.insert(resolved);
            urls.append(resolved);
        }
    }
    return urls;
}

void SyncEngine::startIntervalSync()
{
    m_syncTimer->start(Settings::self()->syncInterval() * 60 * 1000);
}

void SyncEngine::forceSync()
{
    m_isManualSync = true;
    performSync();
}

void SyncEngine::performSync()
{
    if (m_isSyncing) return;

    const QString apiUrl = Settings::self()->apiUrl();
    const QString apiKey = Settings::self()->apiKey();

    if (apiUrl.isEmpty() || apiKey.isEmpty()) {
        qCWarning(KIRIFLUX_SYNC) << "Miniflux API URL or Key not set, skipping sync.";
        return;
    }

    qCDebug(KIRIFLUX_SYNC) << "Starting sync" << (Settings::self()->cleanSync() ? "[CLEAN SYNC]" : "");
    if (Settings::self()->cleanSync()) {
        DatabaseManager::instance().clearAllData();
    }
    m_isSyncing = true;
    emit syncStatusChanged();
    emit syncStarted();
    m_client->fetchCategories(apiUrl, apiKey);
}

void SyncEngine::handleError(const QString& error)
{
    qWarning() << "Sync error:" << error;
    m_isSyncing = false;
    m_statusRequestInFlight = false;
    emit syncStatusChanged();
    
    // Resume batching if needed
    if (!m_pendingReadIds.isEmpty()) {
        m_statusBatchTimer->start();
    }

    if (m_errorThrottleTimer->isActive() && error == m_lastErrorMessage) {
        return;
    }
    m_lastErrorMessage = error;
    m_errorThrottleTimer->start();
    
    emit syncFinished(false);
    emit syncError(error);
}

void SyncEngine::enqueueMarkAsRead(qint64 articleId)
{
    m_pendingReadIds.insert(articleId);
    if (!m_statusRequestInFlight) {
        m_statusBatchTimer->start();
    }
}

void SyncEngine::processStatusBatch()
{
    if (m_statusRequestInFlight || m_pendingReadIds.isEmpty()) return;

    const QString apiUrl = Settings::self()->apiUrl();
    const QString apiKey = Settings::self()->apiKey();
    if (apiUrl.isEmpty() || apiKey.isEmpty()) return;

    const QList<qint64> ids = m_pendingReadIds.values();
    m_pendingReadIds.clear();
    
    m_statusRequestInFlight = true;
    m_client->updateArticlesStatus(apiUrl, apiKey, ids, QStringLiteral("read"));
}

void SyncEngine::prefetchImages(const QList<Article>& articles)
{
    if (articles.isEmpty()) return;

    const int limit = qMin(articles.size(), 20);
    qCDebug(KIRIFLUX_SYNC) << "Starting image prefetch for" << limit << "recent articles";

    int count = 0;
    for (int i = 0; i < limit; ++i) {
        const auto& article = articles.at(i);
        const QList<QUrl> urls = extractImageUrls(article.content, QUrl(article.url));
        for (const QUrl& url : urls) {
            NetworkManager::instance().prefetchUrl(url);
            count++;
        }
    }
    qCDebug(KIRIFLUX_SYNC) << "Queued" << count << "images for prefetch";
}

void SyncEngine::fetchMissingFeedIcons()
{
    const QList<qint64> missingIds = DatabaseManager::instance().getFeedIdsMissingIcons();
    for (qint64 id : missingIds) {
        if (!m_pendingIconFeedIds.contains(id) && !m_inFlightIconFeedIds.contains(id)) {
            m_pendingIconFeedIds.append(id);
        }
    }
    processIconQueue();
}

void SyncEngine::processIconQueue()
{
    constexpr int kIconConcurrencyLimit = 4;
    const QString apiUrl = Settings::self()->apiUrl();
    const QString apiKey = Settings::self()->apiKey();
    if (apiUrl.isEmpty() || apiKey.isEmpty()) {
        m_pendingIconFeedIds.clear();
        return;
    }

    while (m_inFlightIconFeedIds.size() < kIconConcurrencyLimit && !m_pendingIconFeedIds.isEmpty()) {
        const qint64 feedId = m_pendingIconFeedIds.takeFirst();
        m_inFlightIconFeedIds.insert(feedId);
        m_client->fetchFeedIcon(apiUrl, apiKey, feedId);
    }
}

void SyncEngine::fetchFullContent(qint64 entryId)
{
    const QString apiUrl = Settings::self()->apiUrl();
    const QString apiKey = Settings::self()->apiKey();
    if (apiUrl.isEmpty() || apiKey.isEmpty()) return;
    m_client->fetchArticleContent(apiUrl, apiKey, entryId);
}

void SyncEngine::updateArticlesStatus(const QList<qint64>& entryIds, const QString& status)
{
    const QString apiUrl = Settings::self()->apiUrl();
    const QString apiKey = Settings::self()->apiKey();
    if (apiUrl.isEmpty() || apiKey.isEmpty() || entryIds.isEmpty()) return;
    m_client->updateArticlesStatus(apiUrl, apiKey, entryIds, status);
}

void SyncEngine::toggleArticleBookmark(qint64 entryId)
{
    const QString apiUrl = Settings::self()->apiUrl();
    const QString apiKey = Settings::self()->apiKey();
    if (apiUrl.isEmpty() || apiKey.isEmpty()) return;
    m_client->toggleArticleBookmark(apiUrl, apiKey, entryId);
}

QString SyncEngine::lastSyncTimeText() const
{
    const QDateTime lastSync = Settings::self()->lastSyncTime();
    if (!lastSync.isValid()) return tr("Never");
    return QLocale::system().toString(lastSync, QLocale::ShortFormat);
}
