// SPDX-License-Identifier: GPL-3.0-or-later
#include "MinifluxClient.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrlQuery>

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

    // We fetch both UNREAD and STARRED articles to ensure sync is complete
    // Miniflux doesn't have a simple "all recent" if we want to be efficient,
    // so we combine two requests or just fetch unread and starred.
    QStringList endpoints = {
        QStringLiteral("/v1/entries?status=unread"),
        QStringLiteral("/v1/entries?starred=1")
    };

    for (const auto& endpoint : endpoints) {
        QUrl qurl(sanitizedUrl + endpoint);
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
}

void MinifluxClient::fetchArticleContent(const QString& url, const QString& apiKey, long entryId)
{
    QString sanitizedUrl = url.trimmed();
    if (sanitizedUrl.endsWith(QLatin1Char('/'))) {
        sanitizedUrl.chop(1);
    }

    QUrl qurl(sanitizedUrl + QStringLiteral("/v1/entries/%1/fetch-content").arg(entryId));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("update_content"), QStringLiteral("true"));
    qurl.setQuery(query);
    qDebug() << "Fetching content via GET from URL:" << qurl.toString();
    QNetworkRequest request(qurl);
    request.setRawHeader("X-Auth-Token", apiKey.trimmed().toUtf8());

    // Miniflux API: GET /v1/entries/{entryID}/fetch-content
    QNetworkReply *reply = m_nam->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, entryId]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            QString errorBody = reply->readAll();
            qWarning() << "API Error:" << reply->errorString() << "Body:" << errorBody;
            emit errorOccurred(reply->errorString() + ": " + errorBody);
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        const QJsonObject obj = doc.object();
        QString content = obj[QStringLiteral("content")].toString();
        if (!content.isEmpty()) {
            emit contentFetched(entryId, content);
        }
    });
}

void MinifluxClient::saveArticleToService(const QString& url, const QString& apiKey, long entryId)
{
    QString sanitizedUrl = url.trimmed();
    if (sanitizedUrl.endsWith(QLatin1Char('/'))) {
        sanitizedUrl.chop(1);
    }

    QUrl qurl(sanitizedUrl + QStringLiteral("/v1/entries/%1/save").arg(entryId));
    QNetworkRequest request(qurl);
    request.setRawHeader("X-Auth-Token", apiKey.trimmed().toUtf8());
    
    QNetworkReply *reply = m_nam->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply, entryId]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            return;
        }
        emit articleSavedToService(entryId);
    });
}

void MinifluxClient::fetchVersion(const QString& url, const QString& apiKey)
{
    QString sanitizedUrl = url.trimmed();
    if (sanitizedUrl.endsWith(QLatin1Char('/'))) {
        sanitizedUrl.chop(1);
    }

    QUrl qurl(sanitizedUrl + QStringLiteral("/v1/version"));
    QNetworkRequest request(qurl);
    request.setRawHeader("X-Auth-Token", apiKey.trimmed().toUtf8());
    
    QNetworkReply *reply = m_nam->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            return;
        }
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        emit versionFetched(doc.object()["version"].toString());
    });
}
