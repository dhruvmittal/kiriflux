// SPDX-License-Identifier: GPL-3.0-or-later
#include "KirifluxImageProvider.h"
#include "core/DatabaseManager.h"
#include "core/NetworkManager.h"
#include <QCoreApplication>
#include <QMetaObject>
#include <QMutex>
#include <QMutexLocker>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QUrl>

struct KirifluxImageResponse::State
{
    QMutex mutex;
    QImage image;
    QString errorString;
    bool cancelled = false;
    bool completed = false;
    KirifluxImageResponse *response = nullptr; // guarded by mutex; cleared in ~KirifluxImageResponse()
    QPointer<QNetworkReply> reply;             // ONLY touched on the main thread
};

KirifluxImageResponse::KirifluxImageResponse(const QString& id, const QSize& requestedSize)
    : m_state(std::make_shared<State>())
{
    m_state->response = this;

    QObject *app = QCoreApplication::instance();
    if (app && !QCoreApplication::closingDown()) {
        QMetaObject::invokeMethod(app, [state = m_state, id, requestedSize]() {
            startOnMainThread(state, id, requestedSize);
        }, Qt::QueuedConnection);
    } else {
        completeState(m_state, {}, QStringLiteral("Application shutting down"));
    }
}

KirifluxImageResponse::~KirifluxImageResponse()
{
    {
        QMutexLocker lock(&m_state->mutex);
        m_state->cancelled = true;
        m_state->response = nullptr;
    }

    // Never touch m_state->reply directly from the worker thread; always dispatch abort to main thread.
    QObject *app = QCoreApplication::instance();
    if (app && !QCoreApplication::closingDown()) {
        QMetaObject::invokeMethod(app, [state = m_state]() {
            if (state->reply) {
                state->reply->abort();
            }
        }, Qt::QueuedConnection);
    }
}

QQuickTextureFactory *KirifluxImageResponse::textureFactory() const
{
    QMutexLocker lock(&m_state->mutex);
    return m_state->image.isNull() ? nullptr : QQuickTextureFactory::textureFactoryForImage(m_state->image);
}

QString KirifluxImageResponse::errorString() const
{
    QMutexLocker lock(&m_state->mutex);
    return m_state->errorString;
}

void KirifluxImageResponse::cancel()
{
    {
        QMutexLocker lock(&m_state->mutex);
        if (m_state->cancelled) {
            return;
        }
        m_state->cancelled = true;
    }

    QObject *app = QCoreApplication::instance();
    if (app && !QCoreApplication::closingDown()) {
        QMetaObject::invokeMethod(app, [state = m_state]() {
            if (state->reply) {
                state->reply->abort();
            }
            completeState(state, {}, QString());
        }, Qt::QueuedConnection);
    } else {
        completeState(m_state, {}, QString());
    }
}

void KirifluxImageResponse::emitFinished()
{
    emit finished();
}

void KirifluxImageResponse::completeState(const std::shared_ptr<State>& state, QImage image, const QString& errorString)
{
    QMutexLocker lock(&state->mutex);
    if (state->completed) {
        return;
    }
    state->completed = true;
    if (!state->cancelled) {
        state->image = std::move(image);
        state->errorString = errorString;
    }
    if (state->response) {
        QMetaObject::invokeMethod(state->response, &KirifluxImageResponse::emitFinished, Qt::QueuedConnection);
    }
}

void KirifluxImageResponse::startOnMainThread(const std::shared_ptr<State>& state, const QString& id, const QSize& requestedSize)
{
    if (QCoreApplication::closingDown()) {
        completeState(state, {}, QStringLiteral("Application shutting down"));
        return;
    }

    {
        QMutexLocker lock(&state->mutex);
        if (state->cancelled || state->completed) {
            lock.unlock();
            completeState(state, {}, QString());
            return;
        }
    }

    if (id.startsWith(QLatin1String("feed-icon/"))) {
        QString rest = id.mid(10);
        const int qIdx = rest.indexOf(QLatin1Char('?'));
        if (qIdx != -1) {
            rest = rest.left(qIdx);
        }

        bool ok = false;
        const qint64 feedId = rest.toLongLong(&ok);
        if (!ok || feedId <= 0) {
            completeState(state, {}, QStringLiteral("Invalid feed icon ID"));
            return;
        }

        const QString data = DatabaseManager::instance().getFeedIconData(feedId);
        if (data.isEmpty()) {
            completeState(state, {}, QStringLiteral("No icon cached for feed"));
            return;
        }

        const int comma = data.indexOf(QLatin1Char(','));
        const QByteArray encoded = (comma != -1) ? data.mid(comma + 1).toLatin1() : data.toLatin1();
        QImage img = QImage::fromData(QByteArray::fromBase64(encoded));
        if (img.isNull()) {
            completeState(state, {}, QStringLiteral("Failed to decode feed icon"));
            return;
        }

        if (requestedSize.isValid() && !requestedSize.isEmpty()) {
            img = img.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        completeState(state, std::move(img), QString());
        return;
    }

    if (id.startsWith(QLatin1String("img/"))) {
        const QString raw = id.mid(4);
        const bool alreadyDecoded = raw.startsWith(QLatin1String("http://"), Qt::CaseInsensitive)
                                 || raw.startsWith(QLatin1String("https://"), Qt::CaseInsensitive);
        const QUrl url(alreadyDecoded ? raw : QUrl::fromPercentEncoding(raw.toUtf8()));
        const bool validScheme = (url.scheme() == QLatin1String("http") || url.scheme() == QLatin1String("https"));
        if (!url.isValid() || !validScheme || url.host().isEmpty()) {
            completeState(state, {}, QStringLiteral("Invalid image URL"));
            return;
        }

        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Kiriflux/1.0"));
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
        request.setTransferTimeout(60000);

        QNetworkReply *reply = NetworkManager::instance().nam()->get(request);
        state->reply = reply;

        // Connect with reply as context so readAll()/deleteLater() execute strictly on the main thread.
        QObject::connect(reply, &QNetworkReply::finished, reply, [state, reply, requestedSize]() {
            state->reply = nullptr;
            const auto error = reply->error();
            const QString errorStr = reply->errorString();
            const QByteArray payload = (error == QNetworkReply::NoError) ? reply->readAll() : QByteArray();
            reply->deleteLater();

            {
                QMutexLocker lock(&state->mutex);
                if (state->cancelled || state->completed) {
                    lock.unlock();
                    completeState(state, {}, QString());
                    return;
                }
            }

            if (error != QNetworkReply::NoError) {
                completeState(state, {}, errorStr);
                return;
            }

            QImage img = QImage::fromData(payload);
            if (img.isNull()) {
                completeState(state, {}, QStringLiteral("Invalid image data"));
                return;
            }

            if (requestedSize.isValid() && !requestedSize.isEmpty()) {
                img = img.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
            completeState(state, std::move(img), QString());
        });
        return;
    }

    completeState(state, {}, QStringLiteral("Unknown image provider route"));
}

QQuickImageResponse *KirifluxImageProvider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    return new KirifluxImageResponse(id, requestedSize);
}
