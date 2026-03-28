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
    Q_PROPERTY(bool isSyncing READ isSyncing NOTIFY syncStatusChanged)

public:
    static SyncEngine& instance();

    bool isSyncing() const { return m_isSyncing; }

    QString apiUrl() const;
    void setApiUrl(const QString &url);

    QString apiKey() const;
    void setApiKey(const QString &key);

    int syncInterval() const;
    void setSyncInterval(int interval);

public slots:
    void startIntervalSync();
    void forceSync();
    void saveSettings();

signals:
    void settingsChanged();
    void syncStarted();
    void syncFinished(bool success);
    void syncStatusChanged();
    void articlesUpdated();

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
};
