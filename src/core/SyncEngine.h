// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QTimer>
#include "MinifluxClient.h"

class SyncEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString apiUrl READ apiUrl WRITE setApiUrl NOTIFY settingsChanged)
    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY settingsChanged)
    Q_PROPERTY(int syncInterval READ syncInterval WRITE setSyncInterval NOTIFY settingsChanged)
    Q_PROPERTY(QString lastSyncTimeText READ lastSyncTimeText NOTIFY lastSyncTimeChanged)
    Q_PROPERTY(bool showToolbarLabels READ showToolbarLabels WRITE setShowToolbarLabels NOTIFY showToolbarLabelsChanged)
    Q_PROPERTY(int titleLines READ titleLines WRITE setTitleLines NOTIFY titleLinesChanged)
    Q_PROPERTY(bool cleanSync READ cleanSync WRITE setCleanSync NOTIFY settingsChanged)
    Q_PROPERTY(bool markAsReadOnOpen READ markAsReadOnOpen WRITE setMarkAsReadOnOpen NOTIFY settingsChanged)
    Q_PROPERTY(bool markAsReadOnScroll READ markAsReadOnScroll WRITE setMarkAsReadOnScroll NOTIFY settingsChanged)
    Q_PROPERTY(bool isSyncing READ isSyncing NOTIFY syncStatusChanged)
    Q_PROPERTY(QDateTime lastSyncTime READ lastSyncTime NOTIFY lastSyncTimeChanged)

public:
    static SyncEngine& instance();

    bool isSyncing() const { return m_isSyncing; }
    QDateTime lastSyncTime() const { return m_lastSyncTime; }
    QString lastSyncTimeText() const;

    QString apiUrl() const;
    void setApiUrl(const QString &url);

    QString apiKey() const;
    void setApiKey(const QString &key);

    int syncInterval() const;
    void setSyncInterval(int interval);

    bool showToolbarLabels() const;
    void setShowToolbarLabels(bool show);

    int titleLines() const;
    void setTitleLines(int lines);

    bool cleanSync() const;
    void setCleanSync(bool clean);

    bool markAsReadOnOpen() const;
    void setMarkAsReadOnOpen(bool mark);

    bool markAsReadOnScroll() const;
    void setMarkAsReadOnScroll(bool mark);

public slots:
    void startIntervalSync();
    void forceSync();
    void fetchFullContent(long entryId);
    void updateArticleStatus(long entryId, const QString& status);
    void toggleArticleBookmark(long entryId);
    void saveSettings();

signals:
    void settingsChanged();
    void syncStarted();
    void syncFinished(bool success);
    void syncStatusChanged();
    void articlesUpdated();
    void lastSyncTimeChanged();
    void showToolbarLabelsChanged();
    void titleLinesChanged();
    void articleStatusUpdated(long entryId, const QString& status);
    void articleBookmarkToggled(long entryId);

private:
    explicit SyncEngine(QObject *parent = nullptr);
    ~SyncEngine() = default;

    void performSync();

    MinifluxClient *m_client;
    QTimer *m_syncTimer;
    bool m_isSyncing = false;

    // Cached values — used for sync instead of re-reading from KConfigXT
    // (KWallet Password entries may not be available synchronously after save)
    QString m_apiUrl;
    QString m_apiKey;
    int m_syncInterval = 30;
    bool m_showToolbarLabels = false;
    bool m_cleanSync = false;
    bool m_markAsReadOnOpen = false;
    bool m_markAsReadOnScroll = false;
    int m_titleLines = 2;
    QDateTime m_lastSyncTime;
};
