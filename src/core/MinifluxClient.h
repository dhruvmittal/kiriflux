// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include "DatabaseManager.h"

class MinifluxClient : public QObject
{
    Q_OBJECT
public:
    explicit MinifluxClient(QObject *parent = nullptr);

    void fetchFeeds(const QString& url, const QString& apiKey);
    void fetchCategories(const QString& url, const QString& apiKey);
    void fetchArticles(const QString& url, const QString& apiKey);
    void fetchArticleContent(const QString& url, const QString& apiKey, long entryId);
    void saveArticleToService(const QString& url, const QString& apiKey, long entryId);
    void fetchVersion(const QString& url, const QString& apiKey);

signals:
    void feedsFetched(const QList<Feed>& feeds);
    void categoriesFetched(const QList<Category>& categories);
    void versionFetched(const QString& version);
    void articlesFetched(const QList<Article>& articles);
    void articleUpdated(const Article& article);
    void contentFetched(long entryId, const QString& content);
    void articleSavedToService(long entryId);
    void errorOccurred(const QString& message);

private:
    QNetworkAccessManager *m_nam;
};
