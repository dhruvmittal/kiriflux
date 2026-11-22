#include <QtTest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <functional>
#include "../src/core/MinifluxClient.h"
#include "../src/core/SyncEngine.h"
#include "../src/core/DatabaseManager.h"
#include "kiriflux.h"

namespace {
struct HttpRequest {
    QByteArray method;
    QByteArray target;
    QMap<QByteArray, QByteArray> headers;
    QByteArray body;
};

struct HttpResponse {
    int statusCode = 200;
    QByteArray body;
};

class LocalMinifluxStub : public QObject {
public:
    QTcpServer server;
    QList<HttpRequest> requests;
    std::function<HttpResponse(const HttpRequest&)> handler;

    bool start() {
        connect(&server, &QTcpServer::newConnection, this, [this]() {
            while (QTcpSocket* sock = server.nextPendingConnection()) {
                auto buf = std::make_shared<QByteArray>();
                connect(sock, &QTcpSocket::readyRead, sock, [this, sock, buf]() {
                    buf->append(sock->readAll());
                    const int hdrEnd = buf->indexOf("\r\n\r\n");
                    if (hdrEnd == -1) return;

                    const QList<QByteArray> lines = buf->left(hdrEnd).split('\n');
                    if (lines.isEmpty()) return;
                    const QList<QByteArray> parts = lines[0].trimmed().split(' ');

                    HttpRequest req;
                    req.method = parts.value(0);
                    req.target = parts.value(1);
                    int contentLength = 0;
                    for (int i = 1; i < lines.size(); ++i) {
                        const int colon = lines[i].indexOf(':');
                        if (colon > 0) {
                            const QByteArray key = lines[i].left(colon).trimmed().toLower();
                            const QByteArray val = lines[i].mid(colon + 1).trimmed();
                            req.headers.insert(key, val);
                            if (key == "content-length") {
                                contentLength = val.toInt();
                            }
                        }
                    }

                    if (buf->size() < hdrEnd + 4 + contentLength) return;
                    req.body = buf->mid(hdrEnd + 4, contentLength);
                    requests.append(req);

                    const HttpResponse resp = handler ? handler(req) : HttpResponse{200, "{}"};
                    const QByteArray statusText = (resp.statusCode == 200) ? "OK" : (resp.statusCode == 404 ? "Not Found" : "Error");
                    QByteArray raw = "HTTP/1.1 " + QByteArray::number(resp.statusCode) + " " + statusText + "\r\n";
                    raw += "Content-Type: application/json\r\n";
                    raw += "Content-Length: " + QByteArray::number(resp.body.size()) + "\r\n";
                    raw += "Connection: close\r\n\r\n";
                    raw += resp.body;
                    sock->write(raw);
                    sock->flush();
                    sock->disconnectFromHost();
                    sock->deleteLater();
                });
            }
        });
        return server.listen(QHostAddress::LocalHost, 0);
    }

    QString baseUrl() const {
        return QStringLiteral("http://127.0.0.1:%1").arg(server.serverPort());
    }
};
} // namespace

class BatchSyncTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testSyncBatching();
    void testFullSyncAndActions();
    void testSyncInvalidUrlAndErrorThrottling();
    void testClientHeadersAndInvalidUrl();
    void testPartialArticleFetchFailure();
    void testFullArticleFetchDeduplication();
    void testFetchFeedIcon();
};

void BatchSyncTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setApplicationName("BatchSyncTest");
    QVERIFY(DatabaseManager::instance().initWithPath(QStringLiteral(":memory:")));
    DatabaseManager::instance().clearAllData();
}

void BatchSyncTest::cleanupTestCase()
{
    const QString testConfig = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/kirifluxrc");
    QFile::remove(testConfig);
}

void BatchSyncTest::testSyncBatching()
{
    LocalMinifluxStub stub;
    QVERIFY(stub.start());
    stub.handler = [](const HttpRequest&) {
        return HttpResponse{200, "{}"};
    };

    Settings::self()->setApiUrl(stub.baseUrl());
    Settings::self()->setApiKey(QStringLiteral("batch-secret"));

    QSignalSpy spyStatus(&SyncEngine::instance(), &SyncEngine::articleStatusUpdated);

    SyncEngine::instance().enqueueMarkAsRead(201);
    SyncEngine::instance().enqueueMarkAsRead(202);
    SyncEngine::instance().enqueueMarkAsRead(203);
    QCOMPARE(spyStatus.count(), 0);

    QVERIFY(QTest::qWaitFor([&]() { return spyStatus.count() == 3; }, 3000));
    QCOMPARE(stub.requests.size(), 1);
    QCOMPARE(stub.requests[0].method, QByteArray("PUT"));
    QCOMPARE(stub.requests[0].target, QByteArray("/v1/entries"));
    QCOMPARE(stub.requests[0].headers.value("x-auth-token"), QByteArray("batch-secret"));

    const QJsonObject payload = QJsonDocument::fromJson(stub.requests[0].body).object();
    QCOMPARE(payload["status"].toString(), QStringLiteral("read"));
    QSet<qint64> sentIds;
    for (const auto& v : payload["entry_ids"].toArray()) {
        sentIds.insert(v.toInteger());
    }
    QCOMPARE(sentIds, (QSet<qint64>{201, 202, 203}));
}

void BatchSyncTest::testFullSyncAndActions()
{
    LocalMinifluxStub stub;
    QVERIFY(stub.start());
    stub.handler = [](const HttpRequest& req) -> HttpResponse {
        if (req.target == "/v1/categories") {
            return {200, R"([{"id":1,"title":"Tech"}])"};
        }
        if (req.target == "/v1/feeds") {
            return {200, R"([{"id":10,"title":"Feed A","site_url":"https://a.example","feed_url":"https://a.example/rss","category":{"id":1},"icon":{"icon_id":7}}])"};
        }
        if (req.target == "/v1/feeds/10/icon") {
            return {200, R"({"id":7,"data":"image/png;base64,iVBORw0KGgo=","mime_type":"image/png"})"};
        }
        if (req.target.startsWith("/v1/entries?status=unread")) {
            return {200, R"({"entries":[{"id":500,"feed_id":10,"title":"Hello","author":"Auth","content":"<p>Body</p>","url":"https://a.example/1","published_at":"2026-04-07T12:00:00Z","status":"unread","starred":false}]})"};
        }
        if (req.target.startsWith("/v1/entries?starred=1")) {
            return {200, R"({"entries":[]})"};
        }
        if (req.target == "/v1/entries/500/fetch-content?update_content=true") {
            return {200, R"({"content":"<p>Full Body</p>"})"};
        }
        if (req.target == "/v1/entries/500/bookmark") {
            return {200, "{}"};
        }
        return {404, "{}"};
    };

    Settings::self()->setApiUrl(stub.baseUrl());
    Settings::self()->setApiKey(QStringLiteral("sync-secret"));

    QSignalSpy spyFinished(&SyncEngine::instance(), &SyncEngine::syncFinished);
    QSignalSpy spyIcon(&SyncEngine::instance(), &SyncEngine::feedIconUpdated);
    QSignalSpy spyContent(&SyncEngine::instance(), &SyncEngine::articleContentUpdated);
    QSignalSpy spyBookmark(&SyncEngine::instance(), &SyncEngine::articleBookmarkToggled);

    SyncEngine::instance().forceSync();
    QVERIFY(spyFinished.wait(3000));
    QCOMPARE(spyFinished.at(0).at(0).toBool(), true);
    QVERIFY(!SyncEngine::instance().isSyncing());
    QVERIFY(SyncEngine::instance().lastSyncTimeText() != QStringLiteral("Never"));

    if (spyIcon.isEmpty()) {
        QVERIFY(spyIcon.wait(2000));
    }
    QCOMPARE(spyIcon.at(0).at(0).toLongLong(), qint64(10));
    QVERIFY(!DatabaseManager::instance().getFeedIconData(10).isEmpty());

    SyncEngine::instance().fetchFullContent(500);
    QVERIFY(spyContent.wait(2000));
    QCOMPARE(DatabaseManager::instance().getArticleById(500)->content, QStringLiteral("<p>Full Body</p>"));

    SyncEngine::instance().toggleArticleBookmark(500);
    QVERIFY(spyBookmark.wait(2000));
}

void BatchSyncTest::testSyncInvalidUrlAndErrorThrottling()
{
    QSignalSpy spyError(&SyncEngine::instance(), &SyncEngine::syncError);
    QSignalSpy spyFinished(&SyncEngine::instance(), &SyncEngine::syncFinished);

    Settings::self()->setApiUrl(QStringLiteral("not-a-valid-url"));
    Settings::self()->setApiKey(QStringLiteral("test-key"));
    SyncEngine::instance().forceSync();

    QCOMPARE(spyError.count(), 1);
    QCOMPARE(spyFinished.count(), 1);
    QCOMPARE(spyFinished.at(0).at(0).toBool(), false);
    QVERIFY(!SyncEngine::instance().isSyncing());

    // Identical server errors within 5 seconds must be throttled to 1 signal
    LocalMinifluxStub stub;
    QVERIFY(stub.start());
    stub.handler = [](const HttpRequest&) {
        return HttpResponse{500, "Internal Server Error"};
    };
    Settings::self()->setApiUrl(stub.baseUrl());

    spyError.clear();
    SyncEngine::instance().forceSync();
    QVERIFY(QTest::qWaitFor([&]() { return !SyncEngine::instance().isSyncing(); }, 2000));
    QCOMPARE(spyError.count(), 1);

    SyncEngine::instance().forceSync();
    QVERIFY(QTest::qWaitFor([&]() { return !SyncEngine::instance().isSyncing(); }, 2000));
    QCOMPARE(spyError.count(), 1);
}

void BatchSyncTest::testClientHeadersAndInvalidUrl()
{
    MinifluxClient client;
    QSignalSpy spyError(&client, &MinifluxClient::errorOccurred);

    client.fetchCategories(QStringLiteral("not-a-url"), QStringLiteral("k"));
    client.fetchArticles(QStringLiteral("ftp://example.com"), QStringLiteral("k"));
    client.fetchFeeds(QString(), QStringLiteral("k"));
    client.fetchArticleContent(QStringLiteral("missing-scheme.com"), QStringLiteral("k"), 1);
    client.updateArticlesStatus(QStringLiteral("bad"), QStringLiteral("k"), {1}, QStringLiteral("read"));
    client.toggleArticleBookmark(QStringLiteral("bad"), QStringLiteral("k"), 1);
    QCOMPARE(spyError.count(), 6);

    LocalMinifluxStub stub;
    QVERIFY(stub.start());
    stub.handler = [](const HttpRequest&) {
        return HttpResponse{200, R"([{"id":22,"title":"F","site_url":"https://f.test","feed_url":"https://f.test/rss","category":{"id":3},"icon":{"icon_id":42}}])"};
    };

    QSignalSpy spyFeeds(&client, &MinifluxClient::feedsFetched);
    client.fetchFeeds(stub.baseUrl(), QStringLiteral("  my-token  "));
    QVERIFY(spyFeeds.wait(2000));
    QCOMPARE(stub.requests.size(), 1);
    QCOMPARE(stub.requests[0].headers.value("x-auth-token"), QByteArray("my-token"));

    const QList<Feed> feeds = spyFeeds.at(0).at(0).value<QList<Feed>>();
    QCOMPARE(feeds.size(), 1);
    QCOMPARE(feeds[0].iconId, qint64(42));
}

void BatchSyncTest::testPartialArticleFetchFailure()
{
    LocalMinifluxStub stub;
    QVERIFY(stub.start());
    stub.handler = [](const HttpRequest& req) -> HttpResponse {
        if (req.target.contains("status=unread")) {
            return {200, R"({"entries":[{"id":1,"feed_id":10,"title":"A","author":"","content":"","url":"","published_at":"2026-04-07T12:00:00Z","status":"unread","starred":false}]})"};
        }
        return {500, "Server error"};
    };

    MinifluxClient client;
    QSignalSpy spyArticles(&client, &MinifluxClient::articlesFetched);
    QSignalSpy spyError(&client, &MinifluxClient::errorOccurred);

    client.fetchArticles(stub.baseUrl(), QStringLiteral("token"));
    QVERIFY(spyError.wait(2000));
    QTest::qWait(150);
    QCOMPARE(spyArticles.count(), 0);
}

void BatchSyncTest::testFullArticleFetchDeduplication()
{
    LocalMinifluxStub stub;
    QVERIFY(stub.start());
    stub.handler = [](const HttpRequest& req) -> HttpResponse {
        if (req.target.contains("status=unread")) {
            return {200, R"({"entries":[{"id":77,"feed_id":10,"title":"Both","author":"","content":"","url":"","published_at":"2026-04-07T12:00:00Z","status":"unread","starred":true}]})"};
        }
        return {200, R"({"entries":[{"id":77,"feed_id":10,"title":"Both","author":"","content":"","url":"","published_at":"2026-04-07T12:00:00Z","status":"unread","starred":true},{"id":78,"feed_id":10,"title":"ReadStar","author":"","content":"","url":"","published_at":"2026-04-07T11:00:00Z","status":"read","starred":true}]})"};
    };

    MinifluxClient client;
    QSignalSpy spyArticles(&client, &MinifluxClient::articlesFetched);
    client.fetchArticles(stub.baseUrl(), QStringLiteral("token"));
    QVERIFY(spyArticles.wait(2000));

    const QList<Article> articles = spyArticles.at(0).at(0).value<QList<Article>>();
    QCOMPARE(articles.size(), 2);
}

void BatchSyncTest::testFetchFeedIcon()
{
    LocalMinifluxStub stub;
    QVERIFY(stub.start());
    stub.handler = [](const HttpRequest& req) -> HttpResponse {
        if (req.target == "/v1/feeds/5/icon") {
            return {200, R"({"id":1,"data":"image/png;base64,QUJD","mime_type":"image/png"})"};
        }
        return {404, R"({"error_message":"icon not found"})"};
    };

    MinifluxClient client;
    QSignalSpy spyOk(&client, &MinifluxClient::feedIconFetched);
    QSignalSpy spyFail(&client, &MinifluxClient::feedIconFailed);
    QSignalSpy spyError(&client, &MinifluxClient::errorOccurred);

    client.fetchFeedIcon(stub.baseUrl(), QStringLiteral("token"), 5);
    QVERIFY(spyOk.wait(2000));
    QCOMPARE(spyOk.at(0).at(0).toLongLong(), qint64(5));
    QCOMPARE(spyOk.at(0).at(1).toString(), QStringLiteral("image/png;base64,QUJD"));

    client.fetchFeedIcon(stub.baseUrl(), QStringLiteral("token"), 9);
    QVERIFY(spyFail.wait(2000));
    QCOMPARE(spyFail.at(0).at(0).toLongLong(), qint64(9));
    QCOMPARE(spyError.count(), 0);
}

QTEST_GUILESS_MAIN(BatchSyncTest)
#include "BatchSyncTest.moc"
