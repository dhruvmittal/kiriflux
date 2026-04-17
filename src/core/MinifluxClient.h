// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include "DatabaseManager.h"
#include "NetworkManager.h"

class MinifluxClient : public QObject
{
    Q_OBJECT
public:
    explicit MinifluxClient(QObject *parent = nullptr);

    void fetchFeeds(const QString& url, const QString& apiKey);
    void fetchCategories(const QString& url, const QString& apiKey);
    void fetchArticles(const QString& url, const QString& apiKey);
    void fetchArticleContent(const QString& url, const QString& apiKey, qint64 entryId);
    void updateArticleStatus(const QString& url, const QString& apiKey, qint64 entryId, const QString& status);
    void updateArticlesStatus(const QString& url, const QString& apiKey, const QList<qint64>& entryIds, const QString& status);
    void toggleArticleBookmark(const QString& url, const QString& apiKey, qint64 entryId);
    void fetchVersion(const QString& url, const QString& apiKey);
    void prefetchUrl(const QUrl& url);

signals:
    void feedsFetched(const QList<Feed>& feeds);
    void categoriesFetched(const QList<Category>& categories);
    void versionFetched(const QString& version);
    void articlesFetched(const QList<Article>& articles);
    void articleUpdated(const Article& article);
    void contentFetched(qint64 entryId, const QString& content);
    void articleStatusUpdated(qint64 entryId, const QString& status);
    void articleBookmarkToggled(qint64 entryId);
    void errorOccurred(const QString& message);

private:
    QList<QUrl> m_prefetchQueue;
    int m_activePrefetches = 0;
    
    void processPrefetchQueue();
};
