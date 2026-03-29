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

void MinifluxClient::fetchCategories(const QString& url, const QString& apiKey)
{
    QString sanitizedUrl = url.trimmed();
    if (sanitizedUrl.endsWith(QLatin1Char('/'))) {
        sanitizedUrl.chop(1);
    }
    
    QUrl qurl(sanitizedUrl + QStringLiteral("/v1/categories"));
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
        QList<Category> categories;
        for (const auto& value : array) {
            const QJsonObject obj = value.toObject();
            categories.append({
                obj["id"].toVariant().toLongLong(),
                obj["title"].toString(),
                0 // unreadCount
            });
        }
        emit categoriesFetched(categories);
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
        QStringLiteral("/v1/entries?status=unread&order=published_at&direction=desc&limit=500"),
        QStringLiteral("/v1/entries?starred=1&order=published_at&direction=desc&limit=100")
    };

    QSharedPointer<int> pendingRequests(new int(endpoints.size()));
    QSharedPointer<QList<Article>> allArticles(new QList<Article>());
    QSharedPointer<bool> hasError(new bool(false));

    for (const auto& endpoint : endpoints) {
        QUrl qurl(sanitizedUrl + endpoint);
        QNetworkRequest request(qurl);
        request.setRawHeader("X-Auth-Token", apiKey.trimmed().toUtf8());

        QNetworkReply *reply = m_nam->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, pendingRequests, allArticles, hasError, endpoint]() {
            reply->deleteLater();
            (*pendingRequests)--;
            
            if (reply->error() != QNetworkReply::NoError) {
                if (!*hasError) {
                    *hasError = true;
                    QString errorMsg = reply->errorString() + " Body: " + reply->readAll();
                    qWarning() << "Miniflux API Error:" << errorMsg;
                    emit errorOccurred(errorMsg);
                }
            } else {
                QByteArray body = reply->readAll();
                const QJsonDocument doc = QJsonDocument::fromJson(body);
                const QJsonObject root = doc.object();
                const QJsonArray array = root["entries"].toArray();
                qDebug() << "Miniflux API returned" << array.size() << "articles for endpoint:" << endpoint;
                for (const auto& value : array) {
                    const QJsonObject obj = value.toObject();
                    Article a;
                    a.id = obj["id"].toVariant().toLongLong();
                    a.feedId = obj["feed_id"].toVariant().toLongLong();
                    a.title = obj["title"].toString();
                    a.author = obj["author"].toString();
                    a.content = obj["content"].toString();
                    a.url = obj["url"].toString();
                    a.publishedAt = obj["published_at"].toString();
                    a.status = obj["status"].toString();
                    a.starred = obj["starred"].toBool();
                    allArticles->append(a);
                }
            }
            
            if (*pendingRequests == 0 && !*hasError) {
                qDebug() << "All requests finished. Emitting articlesFetched with" << allArticles->size() << "articles total.";
                emit articlesFetched(*allArticles);
            }
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

void MinifluxClient::updateArticleStatus(const QString& url, const QString& apiKey, long entryId, const QString& status)
{
    QString sanitizedUrl = url.trimmed();
    if (sanitizedUrl.endsWith(QLatin1Char('/'))) {
        sanitizedUrl.chop(1);
    }

    QUrl qurl(sanitizedUrl + QStringLiteral("/v1/entries"));
    QNetworkRequest request(qurl);
    request.setRawHeader("X-Auth-Token", apiKey.trimmed().toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    QJsonArray entryIds;
    entryIds.append(qlonglong(entryId));
    json["entry_ids"] = entryIds;
    json["status"] = status;

    QNetworkReply *reply = m_nam->put(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply, entryId, status]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            return;
        }
        emit articleStatusUpdated(entryId, status);
    });
}

void MinifluxClient::toggleArticleBookmark(const QString& url, const QString& apiKey, long entryId)
{
    QString sanitizedUrl = url.trimmed();
    if (sanitizedUrl.endsWith(QLatin1Char('/'))) {
        sanitizedUrl.chop(1);
    }

    QUrl qurl(sanitizedUrl + QStringLiteral("/v1/entries/%1/bookmark").arg(entryId));
    QNetworkRequest request(qurl);
    request.setRawHeader("X-Auth-Token", apiKey.trimmed().toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply *reply = m_nam->put(request, QByteArray("{}"));
    connect(reply, &QNetworkReply::finished, this, [this, reply, entryId]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            return;
        }
        emit articleBookmarkToggled(entryId);
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
