// SPDX-License-Identifier: GPL-3.0-or-later
#include "MinifluxClient.h"
#include "KirifluxLogging.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrlQuery>
#include <QSet>

static QUrl sanitizeAndVerify(const QString& url, const QString& endpoint)
{
    QString base = url.trimmed();
    if (base.endsWith(QLatin1Char('/'))) {
        base.chop(1);
    }
    QUrl qurl(base + endpoint);
    
    // SEC-1: Warn about insecure connections but allow them for local/tailscale setups
    if (qurl.scheme() != QStringLiteral("https")) {
        static bool warned = false;
        if (!warned) {
            qCWarning(KIRIFLUX_SYNC) << "Sending API key over insecure connection:" << qurl.scheme();
            warned = true;
        }
    }
    return qurl;
}

MinifluxClient::MinifluxClient(QObject *parent)
    : QObject(parent), m_nam(new QNetworkAccessManager(this))
{
}

void MinifluxClient::fetchFeeds(const QString& url, const QString& apiKey)
{
    QUrl qurl = sanitizeAndVerify(url, QStringLiteral("/v1/feeds"));
    if (!qurl.isValid()) {
        emit errorOccurred(tr("Invalid API URL."));
        return;
    }
    
    QNetworkRequest request(qurl);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Kiriflux/1.0"));
    request.setRawHeader("X-Auth-Token", apiKey.trimmed().toUtf8());
    request.setTransferTimeout(120000); // 120s timeout for robust fetching

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
                0
            });
        }
        emit feedsFetched(feeds);
    });
}

void MinifluxClient::fetchCategories(const QString& url, const QString& apiKey)
{
    QUrl qurl = sanitizeAndVerify(url, QStringLiteral("/v1/categories"));
    if (!qurl.isValid()) return;
    
    QNetworkRequest request(qurl);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Kiriflux/1.0"));
    request.setRawHeader("X-Auth-Token", apiKey.trimmed().toUtf8());
    request.setTransferTimeout(120000);

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
                0
            });
        }
        emit categoriesFetched(categories);
    });
}

void MinifluxClient::fetchArticles(const QString& url, const QString& apiKey)
{
    // SYNC-3: Restored parallel fetching now that server availability is confirmed.
    // This maximizes throughput on stable connections.
    QStringList endpoints = {
        QStringLiteral("/v1/entries?status=unread&order=published_at&direction=desc&limit=500"),
        QStringLiteral("/v1/entries?starred=1&order=published_at&direction=desc&limit=100")
    };

    QSharedPointer<int> pendingRequests(new int(endpoints.size()));
    QSharedPointer<QList<Article>> allArticles(new QList<Article>());
    QSharedPointer<bool> hasError(new bool(false));

    for (const auto& endpoint : endpoints) {
        QUrl qurl = sanitizeAndVerify(url, endpoint);
        if (!qurl.isValid()) {
            (*pendingRequests)--;
            continue;
        }

        QNetworkRequest request(qurl);
        request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Kiriflux/1.0"));
        request.setRawHeader("X-Auth-Token", apiKey.trimmed().toUtf8());
        request.setTransferTimeout(120000);

        QNetworkReply *reply = m_nam->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, pendingRequests, allArticles, hasError]() {
            reply->deleteLater();
            (*pendingRequests)--;
            
            if (reply->error() != QNetworkReply::NoError) {
                if (!*hasError) {
                    *hasError = true;
                    qCWarning(KIRIFLUX_SYNC) << "Article fetch error:" << reply->errorString();
                    emit errorOccurred(reply->errorString());
                }
            } else {
                const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
                const QJsonObject root = doc.object();
                const QJsonArray array = root["entries"].toArray();
                for (const auto& value : array) {
                    const QJsonObject obj = value.toObject();
                    Article a;
                    a.id = obj["id"].toVariant().toLongLong();
                    a.feedId = obj["feed_id"].toVariant().toLongLong();
                    a.title = obj["title"].toString();
                    a.author = obj["author"].toString();
                    a.content = obj["content"].toString();
                    a.url = obj["url"].toString();
                    a.publishedAt = QDateTime::fromString(obj["published_at"].toString(), Qt::ISODate);
                    a.status = (obj["status"].toString() == "read") ? ArticleStatus::Read : ArticleStatus::Unread;
                    a.starred = obj["starred"].toBool();
                    allArticles->append(a);
                }
            }
            
            if (*pendingRequests == 0) {
                QList<Article> deduped;
                QSet<qint64> seen;
                for (const auto& a : *allArticles) {
                    if (!seen.contains(a.id)) {
                        deduped.append(a);
                        seen.insert(a.id);
                    }
                }
                emit articlesFetched(deduped);
            }
        });
    }
}

void MinifluxClient::fetchArticleContent(const QString& url, const QString& apiKey, qint64 entryId)
{
    QUrl qurl = sanitizeAndVerify(url, QStringLiteral("/v1/entries/%1/fetch-content").arg(entryId));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("update_content"), QStringLiteral("true"));
    qurl.setQuery(query);
    
    QNetworkRequest request(qurl);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Kiriflux/1.0"));
    request.setRawHeader("X-Auth-Token", apiKey.trimmed().toUtf8());
    request.setTransferTimeout(120000);

    QNetworkReply *reply = m_nam->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, entryId]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            emit contentFetched(entryId, doc.object()["content"].toString());
        } else {
            emit errorOccurred(reply->errorString());
        }
    });
}

void MinifluxClient::updateArticleStatus(const QString& url, const QString& apiKey, qint64 entryId, const QString& status)
{
    updateArticlesStatus(url, apiKey, {entryId}, status);
}

void MinifluxClient::updateArticlesStatus(const QString& url, const QString& apiKey, const QList<qint64>& entryIds, const QString& status)
{
    if (entryIds.isEmpty()) return;
    QUrl qurl = sanitizeAndVerify(url, QStringLiteral("/v1/entries"));
    
    QNetworkRequest request(qurl);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Kiriflux/1.0"));
    request.setRawHeader("X-Auth-Token", apiKey.trimmed().toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(120000);

    QJsonObject json;
    QJsonArray entryIdsArray;
    for (qint64 id : entryIds) entryIdsArray.append(qlonglong(id));
    json["entry_ids"] = entryIdsArray;
    json["status"] = status;

    QNetworkReply *reply = m_nam->put(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply, entryIds, status]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            for (qint64 id : entryIds) emit articleStatusUpdated(id, status);
        } else {
            emit errorOccurred(reply->errorString());
        }
    });
}

void MinifluxClient::toggleArticleBookmark(const QString& url, const QString& apiKey, qint64 entryId)
{
    QUrl qurl = sanitizeAndVerify(url, QStringLiteral("/v1/entries/%1/bookmark").arg(entryId));
    QNetworkRequest request(qurl);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Kiriflux/1.0"));
    request.setRawHeader("X-Auth-Token", apiKey.trimmed().toUtf8());
    request.setTransferTimeout(120000);
    
    QNetworkReply *reply = m_nam->put(request, QByteArray("{}"));
    connect(reply, &QNetworkReply::finished, this, [this, reply, entryId]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            emit articleBookmarkToggled(entryId);
        } else {
            emit errorOccurred(reply->errorString());
        }
    });
}

void MinifluxClient::fetchVersion(const QString& url, const QString& apiKey)
{
    QUrl qurl = sanitizeAndVerify(url, QStringLiteral("/v1/version"));
    QNetworkRequest request(qurl);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Kiriflux/1.0"));
    request.setRawHeader("X-Auth-Token", apiKey.trimmed().toUtf8());
    request.setTransferTimeout(30000); // Version check can be faster
    
    QNetworkReply *reply = m_nam->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            emit versionFetched(doc.object()["version"].toString());
        }
    });
}
