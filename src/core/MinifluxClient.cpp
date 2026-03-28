// SPDX-License-Identifier: GPL-3.0-or-later
#include "MinifluxClient.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

MinifluxClient::MinifluxClient(QObject *parent)
    : QObject(parent), m_nam(new QNetworkAccessManager(this))
{
}

void MinifluxClient::fetchFeeds(const QString& url, const QString& apiKey)
{
    QString sanitizedUrl = url.trimmed();
    if (sanitizedUrl.endsWith(QLatin1Char('/'))) {
        sanitizedUrl.chop(1);
    }
    
    QUrl qurl(sanitizedUrl + QStringLiteral("/v1/feeds"));
    QNetworkRequest request(qurl);
    request.setRawHeader("X-Auth-Token", apiKey.trimmed().toUtf8());

    QNetworkReply *reply = m_nam->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        const QJsonArray array = doc.array();
        QList<Feed> feeds;
        for (const auto& value : array) {
            const QJsonObject obj = value.toObject();
            feeds.append({
                obj["id"].toVariant().toLongLong(),
                obj["title"].toString(),
                obj["site_url"].toString(),
                obj["feed_url"].toString(),
                obj["category"].toObject()["id"].toVariant().toLongLong(),
                0 // unreadCount initialized to 0
            });
        }
        emit feedsFetched(feeds);
    });
}

void MinifluxClient::fetchArticles(const QString& url, const QString& apiKey)
{
    QString sanitizedUrl = url.trimmed();
    if (sanitizedUrl.endsWith(QLatin1Char('/'))) {
        sanitizedUrl.chop(1);
    }

    QUrl qurl(sanitizedUrl + QStringLiteral("/v1/entries?status=unread")); // Start with unread
    QNetworkRequest request(qurl);
    request.setRawHeader("X-Auth-Token", apiKey.trimmed().toUtf8());

    QNetworkReply *reply = m_nam->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        const QJsonObject root = doc.object();
        const QJsonArray array = root["entries"].toArray();
        QList<Article> articles;
        for (const auto& value : array) {
            const QJsonObject obj = value.toObject();
            articles.append({
                obj["id"].toVariant().toLongLong(),
                obj["feed_id"].toVariant().toLongLong(),
                obj["title"].toString(),
                obj["author"].toString(),
                obj["content"].toString(),
                obj["url"].toString(),
                obj["published_at"].toString(),
                obj["status"].toString(),
                obj["starred"].toBool()
            });
        }
        emit articlesFetched(articles);
    });
}
