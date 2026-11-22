#include <QtTest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDir>
#include "../src/core/NetworkManager.h"

class NetworkManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setApplicationName("NetworkManagerTest");
    }

    void testSingleton() {
        NetworkManager& n1 = NetworkManager::instance();
        NetworkManager& n2 = NetworkManager::instance();
        QCOMPARE(&n1, &n2);
        QVERIFY(n1.nam() != nullptr);
        QVERIFY(n1.cache() != nullptr);
    }

    void testCacheInitialization() {
        NetworkManager& nm = NetworkManager::instance();
        QString cacheDir = nm.cache()->cacheDirectory();
        QVERIFY(!cacheDir.isEmpty());
        QVERIFY(QDir(cacheDir).exists());
    }

    void testPrefetchConcurrencyCap() {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));

        int totalServed = 0;
        int maxConcurrent = 0;
        NetworkManager& nm = NetworkManager::instance();

        connect(&server, &QTcpServer::newConnection, this, [&]() {
            while (QTcpSocket* sock = server.nextPendingConnection()) {
                maxConcurrent = qMax(maxConcurrent, nm.activePrefetchCount());
                QTimer::singleShot(25, sock, [sock, &totalServed]() {
                    sock->write("HTTP/1.1 200 OK\r\nContent-Length: 2\r\nCache-Control: no-store\r\nConnection: close\r\n\r\nok");
                    sock->flush();
                    sock->disconnectFromHost();
                    sock->deleteLater();
                    totalServed++;
                });
            }
        });

        const quint16 port = server.serverPort();
        const int totalRequests = 15;
        for (int i = 0; i < totalRequests; ++i) {
            const QUrl url(QStringLiteral("http://127.0.0.1:%1/img-%2.png").arg(port).arg(i));
            nm.prefetchUrl(url);
            maxConcurrent = qMax(maxConcurrent, nm.activePrefetchCount());
        }

        // Duplicate and non-HTTP URLs must be ignored
        nm.prefetchUrl(QUrl(QStringLiteral("http://127.0.0.1:%1/img-0.png").arg(port)));
        nm.prefetchUrl(QUrl(QStringLiteral("data:image/png;base64,iVBORw0KGgo=")));

        QVERIFY(QTest::qWaitFor([&]() {
            maxConcurrent = qMax(maxConcurrent, nm.activePrefetchCount());
            return totalServed == totalRequests && nm.activePrefetchCount() == 0;
        }, 5000));

        QCOMPARE(totalServed, totalRequests);
        QVERIFY(maxConcurrent <= 6);
        QVERIFY(maxConcurrent >= 2);
    }

    void testClearCache() {
        NetworkManager& nm = NetworkManager::instance();
        nm.clearCache();
    }
};

QTEST_GUILESS_MAIN(NetworkManagerTest)
#include "NetworkManagerTest.moc"
