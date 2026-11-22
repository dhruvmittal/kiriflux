// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QQuickAsyncImageProvider>
#include <QImage>
#include <memory>

class KirifluxImageResponse : public QQuickImageResponse
{
    Q_OBJECT
public:
    KirifluxImageResponse(const QString& id, const QSize& requestedSize);
    ~KirifluxImageResponse() override;

    QQuickTextureFactory *textureFactory() const override;
    QString errorString() const override;
    void cancel() override;

private:
    struct State;
    static void startOnMainThread(const std::shared_ptr<State>& state, const QString& id, const QSize& requestedSize);
    static void completeState(const std::shared_ptr<State>& state, QImage image, const QString& errorString);
    void emitFinished();

    std::shared_ptr<State> m_state;
};

class KirifluxImageProvider : public QQuickAsyncImageProvider
{
public:
    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;
};
