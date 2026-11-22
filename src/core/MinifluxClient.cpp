// SPDX-License-Identifier: GPL-3.0-or-later
#include "MinifluxClient.h"
#include "KirifluxLogging.h"
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>

MinifluxClient::MinifluxClient(QObject *parent)
    : QObject(parent)
{
}

std::optional<QNetworkRequest> MinifluxClient::makeRequest(const QString& url, const QString& endpoint, const QString& apiKey, int timeoutMs, bool emitError)
{
    QString base = url.trimmed();
    while (base.endsWith(QLatin1Char('/'))) {
        base.chop(1);
    }

    const QUrl qurl(base + endpoint);
    const bool validScheme = (qurl.scheme() == QLatin1String("http") || qurl.scheme() == QLatin1String("https"));
    if (!qurl.isValid() || !validScheme || qurl.host().isEmpty()) {
        if (emitError) {
            emit errorOccurred(tr("Invalid API URL."));
        }
        return std::nullopt;
    }

    // SEC-1: Warn about insecure connections but allow them for local/tailscale setups
    if (qurl.scheme() != QLatin1String("https")) {
        static bool warned = false;
        if (!warned) {
            qCWarning(KIRIFLUX_SYNC) << "Sending API key over insecure connection:" << qurl.scheme();
            warned = true;
        }
    }

    QNetworkRequest request(qurl);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Kiriflux/1.0"));
    request.setRawHeader("X-Auth-Token", apiKey.trimmed().toUtf8());
    request.setTransferTimeout(timeoutMs);
    return request;
}

void MinifluxClient::fetchFeeds(const QString& url, const QString& apiKey)
{
    auto request = makeRequest(url, QStringLiteral("/v1/feeds"), apiKey);
    if (!request) return;

    QNetworkReply *reply = NetworkManager::instance().nam()->get(*request);
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
                0,
                obj["icon"].toObject()["icon_id"].toVariant().toLongLong(),
                false
            });
        }
        emit feedsFetched(feeds);
    });
}

void MinifluxClient::fetchCategories(const QString& url, const QString& apiKey)
{
    auto request = makeRequest(url, QStringLiteral("/v1/categories"), apiKey);
    if (!request) return;

    QNetworkReply *reply = NetworkManager::instance().nam()->get(*request);
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
    // Validate base URL once before starting parallel requests so SyncEngine never hangs
    if (!makeRequest(url, QStringLiteral("/v1/entries"), apiKey)) {
        return;
    }

    // SYNC-3: Parallel fetching maximizes throughput on stable connections.
    const QStringList endpoints = {
        QStringLiteral("/v1/entries?status=unread&order=published_at&direction=desc&limit=1000"),
        QStringLiteral("/v1/entries?starred=1&order=published_at&direction=desc&limit=1000")
    };

    QSharedPointer<int> pendingRequests(new int(endpoints.size()));
    QSharedPointer<QList<Article>> allArticles(new QList<Article>());
    QSharedPointer<bool> hasError(new bool(false));

    for (const auto& endpoint : endpoints) {
        auto request = makeRequest(url, endpoint, apiKey);
        if (!request) {
            *hasError = true;
            (*pendingRequests)--;
            continue;
        }

        QNetworkReply *reply = NetworkManager::instance().nam()->get(*request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, pendingRequests, allArticles, hasError]() {
            reply->deleteLater();
            (*pendingRequests)--;
            
            if (reply->error() != QNetworkReply::NoError) {
                if (!*hasError) {
                    *hasError = true;
                    const QByteArray errorBody = reply->readAll();
                    if (!errorBody.isEmpty()) {
                        qCWarning(KIRIFLUX_SYNC) << "Article fetch error:" << reply->errorString() << "Body:" << errorBody;
                    } else {
                        qCWarning(KIRIFLUX_SYNC) << "Article fetch error:" << reply->errorString();
                    }
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
                    a.status = (obj["status"].toString() == QLatin1String("read")) ? ArticleStatus::Read : ArticleStatus::Unread;
                    a.starred = obj["starred"].toBool();
                    allArticles->append(a);
                }
            }
            
            if (*pendingRequests == 0) {
                if (*hasError) {
                    return;
                }
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
    auto request = makeRequest(url, QStringLiteral("/v1/entries/%1/fetch-content?update_content=true").arg(entryId), apiKey);
    if (!request) return;

    QNetworkReply *reply = NetworkManager::instance().nam()->get(*request);
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

void MinifluxClient::updateArticlesStatus(const QString& url, const QString& apiKey, const QList<qint64>& entryIds, const QString& status)
{
    if (entryIds.isEmpty()) return;
    auto request = makeRequest(url, QStringLiteral("/v1/entries"), apiKey);
    if (!request) return;
    request->setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QJsonObject json;
    QJsonArray entryIdsArray;
    for (qint64 id : entryIds) entryIdsArray.append(qlonglong(id));
    json["entry_ids"] = entryIdsArray;
    json["status"] = status;

    QNetworkReply *reply = NetworkManager::instance().nam()->put(*request, QJsonDocument(json).toJson());
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
    auto request = makeRequest(url, QStringLiteral("/v1/entries/%1/bookmark").arg(entryId), apiKey);
    if (!request) return;
    
    QNetworkReply *reply = NetworkManager::instance().nam()->put(*request, QByteArray("{}"));
    connect(reply, &QNetworkReply::finished, this, [this, reply, entryId]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            emit articleBookmarkToggled(entryId);
        } else {
            emit errorOccurred(reply->errorString());
        }
    });
}

void MinifluxClient::fetchFeedIcon(const QString& url, const QString& apiKey, qint64 feedId)
{
    auto request = makeRequest(url, QStringLiteral("/v1/feeds/%1/icon").arg(feedId), apiKey, 30000, false);
    if (!request) {
        emit feedIconFailed(feedId);
        return;
    }

    QNetworkReply *reply = NetworkManager::instance().nam()->get(*request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, feedId]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            const QString data = doc.object()["data"].toString();
            if (!data.isEmpty()) {
                emit feedIconFetched(feedId, data);
                return;
            }
        }
        emit feedIconFailed(feedId);
    });
}
