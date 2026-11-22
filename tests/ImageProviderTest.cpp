// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QBuffer>
#include <QDir>
#include <QGuiApplication>
#include <QImage>
#include <QImageReader>
#include <QMutex>
#include <QMutexLocker>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickTextureFactory>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QTimer>
#include <atomic>
#include <memory>

#include "../src/KirifluxImageProvider.h"
#include "../src/core/DatabaseManager.h"
#include "../src/core/NetworkManager.h"

namespace {

QByteArray makePngBytes(int w = 4, int h = 4, Qt::GlobalColor color = Qt::blue)
{
    QImage img(w, h, QImage::Format_RGBA8888);
    img.fill(color);
    QByteArray out;
    QBuffer buf(&out);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");
    return out;
}

class LoopbackImageServer : public QObject
{
    Q_OBJECT
public:
    explicit LoopbackImageServer(QObject *parent = nullptr)
        : QObject(parent)
        , m_pngBytes(makePngBytes(4, 4, Qt::darkGreen))
    {
        connect(&m_server, &QTcpServer::newConnection, this, &LoopbackImageServer::onNewConnection);
    }

    bool start()
    {
        return m_server.listen(QHostAddress::LocalHost, 0);
    }

    QString urlString(const QString& pathAndQuery) const
    {
        return QStringLiteral("http://127.0.0.1:%1%2").arg(m_server.serverPort()).arg(pathAndQuery);
    }

    QUrl url(const QString& pathAndQuery) const
    {
        return QUrl(urlString(pathAndQuery));
    }

    int okHitCount = 0;
    QString lastEncodedTarget;
    QStringList receivedTargets;

private slots:
    void onNewConnection()
    {
        while (QTcpSocket *socket = m_server.nextPendingConnection()) {
            auto buffer = std::make_shared<QByteArray>();
            auto handled = std::make_shared<bool>(false);
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            connect(socket, &QTcpSocket::readyRead, this, [this, socket, buffer, handled]() {
                if (*handled) {
                    return;
                }
                buffer->append(socket->readAll());
                const int headerEnd = buffer->indexOf("\r\n\r\n");
                if (headerEnd == -1) {
                    return;
                }
                *handled = true;

                const int lineEnd = buffer->indexOf("\r\n");
                const QByteArray requestLine = buffer->left(lineEnd);
                const QList<QByteArray> parts = requestLine.split(' ');
                const QString target = (parts.size() >= 2) ? QString::fromUtf8(parts.at(1)) : QString();
                receivedTargets.append(target);

                auto sendResponse = [socket](const QByteArray& status, const QByteArray& contentType, const QByteArray& body, const QByteArray& extraHeaders = {}) {
                    if (socket->state() != QAbstractSocket::ConnectedState) {
                        return;
                    }
                    QByteArray resp;
                    resp += "HTTP/1.1 " + status + "\r\n";
                    resp += "Content-Type: " + contentType + "\r\n";
                    resp += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
                    resp += "Connection: close\r\n";
                    resp += extraHeaders;
                    resp += "\r\n";
                    resp += body;
                    socket->write(resp);
                    socket->disconnectFromHost();
                };

                if (target == QLatin1String("/ok.png")) {
                    okHitCount++;
                    sendResponse("200 OK", "image/png", m_pngBytes, "Cache-Control: max-age=3600\r\n");
                } else if (target.startsWith(QLatin1String("/fast/"))) {
                    sendResponse("200 OK", "image/png", m_pngBytes);
                } else if (target.startsWith(QLatin1String("/encoded/"))) {
                    lastEncodedTarget = target;
                    sendResponse("200 OK", "image/png", m_pngBytes);
                } else if (target.startsWith(QLatin1String("/slow/"))) {
                    QPointer<QTcpSocket> sockPtr(socket);
                    QTimer::singleShot(35, this, [sendResponse, sockPtr, body = m_pngBytes]() {
                        if (sockPtr && sockPtr->state() == QAbstractSocket::ConnectedState) {
                            sendResponse("200 OK", "image/png", body);
                        }
                    });
                } else if (target == QLatin1String("/missing.png")) {
                    sendResponse("404 Not Found", "text/plain", "not found");
                } else if (target == QLatin1String("/corrupt.png")) {
                    sendResponse("200 OK", "image/png", "not-a-valid-png");
                } else {
                    sendResponse("404 Not Found", "text/plain", "unknown");
                }
            });
        }
    }

private:
    QTcpServer m_server;
    QByteArray m_pngBytes;
};

struct ResponseResult
{
    bool finished = false;
    int finishCount = 0;
    bool hasFactory = false;
    QSize imageSize;
    QString errorString;
};

} // namespace

class ImageProviderTest : public QObject
{
    Q_OBJECT

private:
    KirifluxImageProvider m_provider;
    std::unique_ptr<LoopbackImageServer> m_server;

    static QString encodeImgId(const QString& urlStr)
    {
        return QStringLiteral("img/") + QString::fromUtf8(QUrl::toPercentEncoding(urlStr));
    }

    ResponseResult runSingleOnThread(QThread *thread, const QString& id, const QSize& requestedSize = QSize())
    {
        auto res = std::make_shared<ResponseResult>();
        auto done = std::make_shared<std::atomic<bool>>(false);
        auto mutex = std::make_shared<QMutex>();

        auto execute = [this, thread, id, requestedSize, res, done, mutex]() {
            QQuickImageResponse *resp = m_provider.requestImageResponse(id, requestedSize);
            QObject::connect(resp, &QQuickImageResponse::finished, resp, [resp, thread, res, done, mutex]() {
                const bool onExpectedThread = (QThread::currentThread() == thread);
                std::unique_ptr<QQuickTextureFactory> factory(resp->textureFactory());
                const QString err = resp->errorString();
                {
                    QMutexLocker lock(mutex.get());
                    res->finishCount++;
                    res->finished = onExpectedThread;
                    res->hasFactory = (factory != nullptr);
                    res->imageSize = factory ? factory->textureSize() : QSize();
                    res->errorString = err;
                }
                done->store(true);
                resp->deleteLater();
            });
        };

        if (thread == QThread::currentThread()) {
            execute();
        } else {
            QMetaObject::invokeMethod(thread->eventDispatcher(), execute, Qt::QueuedConnection);
        }

        const bool completed = QTest::qWaitFor([&done]() { return done->load(); }, 5000);
        if (!completed) {
            QMutexLocker lock(mutex.get());
            res->finished = false;
        }
        return *res;
    }

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setApplicationName(QStringLiteral("ImageProviderTest"));
        QVERIFY(DatabaseManager::instance().initWithPath(QStringLiteral(":memory:")));

        m_server = std::make_unique<LoopbackImageServer>();
        QVERIFY(m_server->start());

        // Seed feeds with valid PNG, valid SVG, corrupt, and empty icons
        DatabaseManager::instance().saveFeeds({
            Feed{101, QStringLiteral("PNG Feed"), QStringLiteral("https://example.com"), QStringLiteral("https://example.com/rss"), 1, 0, 7, false},
            Feed{102, QStringLiteral("SVG Feed"), QStringLiteral("https://example.org"), QStringLiteral("https://example.org/rss"), 1, 0, 8, false},
            Feed{103, QStringLiteral("Corrupt Feed"), QStringLiteral("https://corrupt.test"), QStringLiteral("https://corrupt.test/rss"), 1, 0, 9, false},
            Feed{104, QStringLiteral("Empty Feed"), QStringLiteral("https://empty.test"), QStringLiteral("https://empty.test/rss"), 1, 0, 10, false},
        });

        const QByteArray png6x6 = makePngBytes(6, 6, Qt::red);
        DatabaseManager::instance().saveFeedIcon(101, QStringLiteral("data:image/png;base64,") + QString::fromLatin1(png6x6.toBase64()));

        const QByteArray svg = QByteArrayLiteral("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16'><rect width='16' height='16' fill='blue'/></svg>");
        DatabaseManager::instance().saveFeedIcon(102, QStringLiteral("data:image/svg+xml;base64,") + QString::fromLatin1(svg.toBase64()));

        DatabaseManager::instance().saveFeedIcon(103, QStringLiteral("data:image/png;base64,!!!notbase64!!!"));
    }

    void cleanupTestCase()
    {
        NetworkManager::instance().clearCache();
        m_server.reset();
        const QString cfg = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/kirifluxrc");
        QFile::remove(cfg);
    }

    void testFeedIconMainAndWorkerThread()
    {
        // Main thread lookup with query string + requested scaling
        {
            const ResponseResult r = runSingleOnThread(QThread::currentThread(), QStringLiteral("feed-icon/101?v=7"), QSize(16, 16));
            QVERIFY(r.finished);
            QCOMPARE(r.finishCount, 1);
            QVERIFY(r.hasFactory);
            QCOMPARE(r.imageSize, QSize(16, 16));
            QVERIFY(r.errorString.isEmpty());
        }

        // Worker thread lookups (simulating QQuickPixmapReader)
        QThread worker;
        worker.start();
        QVERIFY(QTest::qWaitFor([&worker]() { return worker.eventDispatcher() != nullptr; }, 2000));

        {
            const ResponseResult r = runSingleOnThread(&worker, QStringLiteral("feed-icon/101?v=7"), QSize(12, 12));
            QVERIFY(r.finished);
            QCOMPARE(r.finishCount, 1);
            QVERIFY(r.hasFactory);
            QCOMPARE(r.imageSize, QSize(12, 12));
            QVERIFY(r.errorString.isEmpty());
        }

        if (QImageReader::supportedImageFormats().contains("svg")) {
            const ResponseResult r = runSingleOnThread(&worker, QStringLiteral("feed-icon/102?v=8"));
            QVERIFY(r.finished);
            QVERIFY(r.hasFactory);
            QVERIFY(r.errorString.isEmpty());
        }

        for (const QString& badId : {QStringLiteral("feed-icon/103"), QStringLiteral("feed-icon/104"), QStringLiteral("feed-icon/abc"), QStringLiteral("feed-icon/-5"), QStringLiteral("feed-icon/0")}) {
            const ResponseResult r = runSingleOnThread(&worker, badId);
            QVERIFY(r.finished);
            QCOMPARE(r.finishCount, 1);
            QVERIFY(!r.hasFactory);
            QVERIFY(!r.errorString.isEmpty());
        }

        worker.quit();
        QVERIFY(worker.wait(3000));
    }

    void testHttpImagesWorkerThreadStress()
    {
        QThread worker;
        worker.start();
        QVERIFY(QTest::qWaitFor([&worker]() { return worker.eventDispatcher() != nullptr; }, 2000));

        m_server->lastEncodedTarget.clear();

        struct Stats {
            std::atomic<int> completed{0};
            std::atomic<int> validCount{0};
            std::atomic<int> errorCount{0};
            std::atomic<int> wrongThread{0};
            std::atomic<int> doubleFinish{0};
        };
        auto stats = std::make_shared<Stats>();

        QStringList ids;
        for (int i = 0; i < 32; ++i) {
            ids.append(encodeImgId(m_server->urlString(QStringLiteral("/fast/%1.png").arg(i))));
        }
        const QString encodedPath = QStringLiteral("/encoded/a%20b.png?x=1&u=https%3A%2F%2Fcdn%2Ffoo.png");
        ids.append(encodeImgId(m_server->urlString(encodedPath)));
        ids.append(encodeImgId(m_server->urlString(encodedPath)));
        for (int i = 0; i < 3; ++i) {
            ids.append(encodeImgId(m_server->urlString(QStringLiteral("/missing.png"))));
        }
        for (int i = 0; i < 3; ++i) {
            ids.append(encodeImgId(m_server->urlString(QStringLiteral("/corrupt.png"))));
        }
        QCOMPARE(ids.size(), 40);

        QMetaObject::invokeMethod(worker.eventDispatcher(), [this, &worker, ids, stats]() {
            for (const QString& id : ids) {
                QQuickImageResponse *resp = m_provider.requestImageResponse(id, QSize());
                auto fired = std::make_shared<int>(0);
                QObject::connect(resp, &QQuickImageResponse::finished, resp, [resp, &worker, stats, fired]() {
                    (*fired)++;
                    if (*fired > 1) {
                        stats->doubleFinish++;
                    }
                    if (QThread::currentThread() != &worker) {
                        stats->wrongThread++;
                    }
                    std::unique_ptr<QQuickTextureFactory> factory(resp->textureFactory());
                    if (factory && factory->textureSize() == QSize(4, 4) && resp->errorString().isEmpty()) {
                        stats->validCount++;
                    } else if (!factory && !resp->errorString().isEmpty()) {
                        stats->errorCount++;
                    }
                    stats->completed++;
                    resp->deleteLater();
                });
            }
        }, Qt::QueuedConnection);

        QTRY_COMPARE_WITH_TIMEOUT(stats->completed.load(), 40, 10000);
        QTest::qWait(30); // Flush worker deleteLater

        worker.quit();
        QVERIFY(worker.wait(3000));

        QCOMPARE(stats->wrongThread.load(), 0);
        QCOMPARE(stats->doubleFinish.load(), 0);
        QCOMPARE(stats->validCount.load(), 34);
        QCOMPARE(stats->errorCount.load(), 6);
        QCOMPARE(m_server->lastEncodedTarget, encodedPath);
    }

    void testPrefetchCacheHit()
    {
        m_server->okHitCount = 0;
        const QUrl okUrl = m_server->url(QStringLiteral("/ok.png"));

        NetworkManager::instance().prefetchUrl(okUrl);
        QTRY_COMPARE_WITH_TIMEOUT(m_server->okHitCount, 1, 5000);
        // Allow QNetworkReply::finished + QNetworkDiskCache write to settle on main thread
        QTest::qWait(80);

        QThread worker;
        worker.start();
        QVERIFY(QTest::qWaitFor([&worker]() { return worker.eventDispatcher() != nullptr; }, 2000));

        const ResponseResult r = runSingleOnThread(&worker, encodeImgId(okUrl.toString()));
        QVERIFY(r.finished);
        QVERIFY(r.hasFactory);
        QCOMPARE(r.imageSize, QSize(4, 4));
        QCOMPARE(m_server->okHitCount, 1); // Served from NetworkManager disk cache

        worker.quit();
        QVERIFY(worker.wait(3000));
    }

    void testCancelAndDestroyRaces()
    {
        QThread worker;
        worker.start();
        QVERIFY(QTest::qWaitFor([&worker]() { return worker.eventDispatcher() != nullptr; }, 2000));

        auto cancelledFinished = std::make_shared<std::atomic<int>>(0);
        auto doubleFinished = std::make_shared<std::atomic<int>>(0);

        QMetaObject::invokeMethod(worker.eventDispatcher(), [this, cancelledFinished, doubleFinished]() {
            // 25 cancelled responses (12 immediate, 13 delayed)
            for (int i = 0; i < 25; ++i) {
                QQuickImageResponse *resp = m_provider.requestImageResponse(
                    encodeImgId(m_server->urlString(QStringLiteral("/slow/%1.png").arg(i))), QSize());
                auto fired = std::make_shared<int>(0);
                QObject::connect(resp, &QQuickImageResponse::finished, resp, [resp, cancelledFinished, doubleFinished, fired]() {
                    (*fired)++;
                    if (*fired > 1) {
                        doubleFinished->fetch_add(1);
                    }
                    cancelledFinished->fetch_add(1);
                    resp->deleteLater();
                });
                if (i < 12) {
                    resp->cancel();
                } else {
                    QPointer<QQuickImageResponse> ptr(resp);
                    QTimer::singleShot(10, [ptr]() {
                        if (ptr) {
                            ptr->cancel();
                        }
                    });
                }
            }

            // 25 destroyed mid-flight without waiting for finished (10 immediate, 15 delayed)
            for (int i = 25; i < 50; ++i) {
                const QString path = (i < 40) ? QStringLiteral("/slow/%1.png").arg(i) : QStringLiteral("/fast/%1.png").arg(i);
                QQuickImageResponse *resp = m_provider.requestImageResponse(encodeImgId(m_server->urlString(path)), QSize());
                if (i < 35) {
                    delete resp;
                } else {
                    QPointer<QQuickImageResponse> ptr(resp);
                    QTimer::singleShot(10, [ptr]() {
                        delete ptr.data();
                    });
                }
            }
        }, Qt::QueuedConnection);

        QTRY_COMPARE_WITH_TIMEOUT(cancelledFinished->load(), 25, 10000);
        QTest::qWait(150); // Let any in-flight /slow replies & aborts finish cleanly on main + worker threads
        QCOMPARE(doubleFinished->load(), 0);

        worker.quit();
        QVERIFY(worker.wait(3000));
        QTest::qWait(50);
    }

    void testRealQmlEngineImageAndRichText()
    {
        auto engine = std::make_unique<QQmlEngine>();
        engine->addImageProvider(QStringLiteral("kiriflux"), new KirifluxImageProvider());

        QQmlComponent component(engine.get());
        component.setData(QByteArrayLiteral(
            "import QtQuick\n"
            "Item {\n"
            "    property string imgUrl: \"\"\n"
            "    property string feedIconUrl: \"\"\n"
            "    property string htmlText: \"\"\n"
            "    readonly property int imgStatus: img.status\n"
            "    readonly property real imgWidth: img.implicitWidth\n"
            "    readonly property int feedStatus: feedImg.status\n"
            "    readonly property real feedWidth: feedImg.implicitWidth\n"
            "    Image {\n"
            "        id: img\n"
            "        asynchronous: true\n"
            "        cache: false\n"
            "        source: parent.imgUrl\n"
            "    }\n"
            "    Image {\n"
            "        id: feedImg\n"
            "        asynchronous: true\n"
            "        cache: false\n"
            "        source: parent.feedIconUrl\n"
            "    }\n"
            "    TextEdit {\n"
            "        id: richText\n"
            "        textFormat: TextEdit.RichText\n"
            "        text: parent.htmlText\n"
            "    }\n"
            "}\n"), QUrl(QStringLiteral("qrc:/TestImageProvider.qml")));

        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> item(component.create());
        QVERIFY(item != nullptr);

        // 1. Load HTTP image via real QQuickPixmapReader worker thread
        const QString okProviderUrl = QStringLiteral("image://kiriflux/") + encodeImgId(m_server->urlString(QStringLiteral("/ok.png")));
        item->setProperty("imgUrl", okProviderUrl);
        QTRY_COMPARE_WITH_TIMEOUT(item->property("imgStatus").toInt(), 1 /* Image.Ready */, 5000);
        QCOMPARE(item->property("imgWidth").toReal(), 4.0);

        // 2. Load Miniflux feed icon via real QQuickPixmapReader worker thread
        item->setProperty("feedIconUrl", QStringLiteral("image://kiriflux/feed-icon/101?v=7"));
        QTRY_COMPARE_WITH_TIMEOUT(item->property("feedStatus").toInt(), 1 /* Image.Ready */, 5000);
        QCOMPARE(item->property("feedWidth").toReal(), 6.0);

        // 3. Rapidly swap Image source + TextEdit.RichText HTML across slow & fast endpoints
        m_server->receivedTargets.clear();
        for (int i = 0; i < 20; ++i) {
            const QString target = (i % 2 == 0)
                ? QStringLiteral("/slow/%1.png").arg(i)
                : QStringLiteral("/fast/%1.png").arg(i);
            const QString providerUrl = QStringLiteral("image://kiriflux/") + encodeImgId(m_server->urlString(target));
            item->setProperty("imgUrl", providerUrl);
            item->setProperty("htmlText", QStringLiteral("<p><img src=\"%1\"></p>").arg(providerUrl));
            QTest::qWait(5);
        }

        // Wait for the final Image & TextEdit fetch to settle
        QTRY_COMPARE_WITH_TIMEOUT(item->property("imgStatus").toInt(), 1 /* Image.Ready */, 5000);
        QTRY_VERIFY_WITH_TIMEOUT(m_server->receivedTargets.contains(QStringLiteral("/fast/19.png")), 5000);

        // Swap to another slow request and immediately tear down item & engine mid-flight
        const QString finalSlow = QStringLiteral("image://kiriflux/") + encodeImgId(m_server->urlString(QStringLiteral("/slow/999.png")));
        item->setProperty("imgUrl", finalSlow);
        item->setProperty("htmlText", QStringLiteral("<p><img src=\"%1\"></p>").arg(finalSlow));
        QTest::qWait(5);

        item.reset();
        engine.reset();
        QTest::qWait(100);
    }

    void testInvalidRoutesAndSchemes()
    {
        for (const QString& badId : {
            QStringLiteral("img/file:///etc/passwd"),
            QStringLiteral("img/javascript:alert(1)"),
            QStringLiteral("img/"),
            QStringLiteral("unknown/foo")
        }) {
            const ResponseResult r = runSingleOnThread(QThread::currentThread(), badId);
            QVERIFY(r.finished);
            QCOMPARE(r.finishCount, 1);
            QVERIFY(!r.hasFactory);
            QVERIFY(!r.errorString.isEmpty());
        }
    }
};

QTEST_MAIN(ImageProviderTest)
#include "ImageProviderTest.moc"
