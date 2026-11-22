// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QNetworkRequest>
#include <optional>
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
    void updateArticlesStatus(const QString& url, const QString& apiKey, const QList<qint64>& entryIds, const QString& status);
    void toggleArticleBookmark(const QString& url, const QString& apiKey, qint64 entryId);
    void fetchFeedIcon(const QString& url, const QString& apiKey, qint64 feedId);

signals:
    void feedsFetched(const QList<Feed>& feeds);
    void categoriesFetched(const QList<Category>& categories);
    void articlesFetched(const QList<Article>& articles);
    void contentFetched(qint64 entryId, const QString& content);
    void articleStatusUpdated(qint64 entryId, const QString& status);
    void articleBookmarkToggled(qint64 entryId);
    void feedIconFetched(qint64 feedId, const QString& iconData);
    void feedIconFailed(qint64 feedId);
    void errorOccurred(const QString& message);

private:
    std::optional<QNetworkRequest> makeRequest(const QString& url, const QString& endpoint, const QString& apiKey, int timeoutMs = 120000, bool emitError = true);
};
