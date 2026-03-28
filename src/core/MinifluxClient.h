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
    void fetchArticles(const QString& url, const QString& apiKey);

signals:
    void feedsFetched(const QList<Feed>& feeds);
    void articlesFetched(const QList<Article>& articles);
    void errorOccurred(const QString& message);

private:
    QNetworkAccessManager *m_nam;
};
