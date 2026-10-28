#include <QtTest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSignalSpy>
#include <QQueue>

class NetworkConcurrencyTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
    }

    // This test simulates the CURRENT problematic behavior
    void testUnthrottledBurst() {
        QNetworkAccessManager nam;
        QList<QNetworkReply*> replies;
        
        qDebug() << "Firing 50 unthrottled requests...";
        for (int i = 0; i < 50; ++i) {
            // Using a real stable URL to see if we can hit stream limits
            QNetworkRequest req(QUrl("https://www.google.com/robots.txt"));
            replies.append(nam.get(req));
        }

        int maxConcurrent = 0;
        QElapsedTimer timer;
        timer.start();

        // Monitor concurrency for 2 seconds or until finished
        while (timer.elapsed() < 2000) {
            int current = 0;
            for (auto r : replies) {
                if (r->isRunning()) current++;
            }
            maxConcurrent = qMax(maxConcurrent, current);
            QCoreApplication::processEvents();
            if (current == 0 && replies.last()->isFinished()) break;
        }

        qDebug() << "Peak unthrottled concurrency:" << maxConcurrent;
        
        // This usually hits 50 or the system limit immediately
        QVERIFY(maxConcurrent > 10); 

        for (auto r : replies) r->deleteLater();
    }

    // This test simulates the PROPOSED throttled behavior
    void testThrottledQueue() {
        QNetworkAccessManager nam;
        QQueue<QUrl> queue;
        QList<QNetworkReply*> activeReplies;
        int maxConcurrentObserved = 0;
        int completedCount = 0;

        const int CONCURRENCY_LIMIT = 6;
        const int TOTAL_REQUESTS = 50;

        for (int i = 0; i < TOTAL_REQUESTS; ++i) {
            queue.enqueue(QUrl("https://www.google.com/robots.txt"));
        }

        auto processQueue = [&]() {
            while (activeReplies.size() < CONCURRENCY_LIMIT && !queue.isEmpty()) {
                QUrl url = queue.dequeue();
                QNetworkRequest req(url);
                QNetworkReply* reply = nam.get(req);
                activeReplies.append(reply);

                connect(reply, &QNetworkReply::finished, this, [&, reply]() {
                    activeReplies.removeAll(reply);
                    reply->deleteLater();
                    completedCount++;
                    // In real implementation, this would call processQueue again
                }, Qt::QueuedConnection);
            }
        };

        qDebug() << "Starting throttled queue (Limit:" << CONCURRENCY_LIMIT << ")...";
        
        QElapsedTimer timer;
        timer.start();

        while (completedCount < TOTAL_REQUESTS && timer.elapsed() < 10000) {
            processQueue();
            maxConcurrentObserved = qMax(maxConcurrentObserved, activeReplies.size());
            QCoreApplication::processEvents();
        }

        qDebug() << "Peak throttled concurrency observed:" << maxConcurrentObserved;
        qDebug() << "Total requests completed:" << completedCount;

        QCOMPARE(completedCount, TOTAL_REQUESTS);
        QVERIFY(maxConcurrentObserved <= CONCURRENCY_LIMIT);
    }
};

QTEST_GUILESS_MAIN(NetworkConcurrencyTest)
#include "NetworkConcurrencyTest.moc"
