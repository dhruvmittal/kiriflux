// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QTimer>
#include <QSet>
#include <QUrl>
#include "MinifluxClient.h"

class SyncEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isSyncing READ isSyncing NOTIFY syncStatusChanged)
    Q_PROPERTY(QString lastSyncTimeText READ lastSyncTimeText NOTIFY syncFinished)

public:
    static SyncEngine& instance();
    static QList<QUrl> extractImageUrls(const QString& html, const QUrl& baseUrl);

    bool isSyncing() const { return m_isSyncing; }
    QString lastSyncTimeText() const;

public slots:
    void startIntervalSync();
    void forceSync();
    void fetchFullContent(qint64 entryId);
    void updateArticlesStatus(const QList<qint64>& entryIds, const QString& status);
    void toggleArticleBookmark(qint64 entryId);
    
    // Efficient status batching
    void enqueueMarkAsRead(qint64 articleId);

signals:
    void syncStarted();
    void syncFinished(bool success);
    void syncStatusChanged();
    void articlesUpdated(bool isManual);
    void articleContentUpdated(qint64 articleId);
    void articleStatusUpdated(qint64 entryId, const QString& status);
    void articleBookmarkToggled(qint64 entryId);
    void feedIconUpdated(qint64 feedId);
    void syncError(const QString& message);

private:
    explicit SyncEngine(QObject *parent = nullptr);
    ~SyncEngine() override = default;

    void performSync();
    void prefetchImages(const QList<Article>& articles);
    void fetchMissingFeedIcons();
    void processIconQueue();
    void processStatusBatch();
    void handleError(const QString& error);

    MinifluxClient *m_client;
    QTimer *m_syncTimer;
    QTimer *m_statusBatchTimer;
    QTimer *m_errorThrottleTimer;
    
    QSet<qint64> m_pendingReadIds;
    QList<qint64> m_pendingIconFeedIds;
    QSet<qint64> m_inFlightIconFeedIds;
    QString m_lastErrorMessage;
    bool m_isSyncing = false;
    bool m_isManualSync = false;
    bool m_statusRequestInFlight = false;
};
