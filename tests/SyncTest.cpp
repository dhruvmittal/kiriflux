#include <QtTest>
#include <QSignalSpy>
#include "../src/core/SyncEngine.h"
#include "../src/core/DatabaseManager.h"
#include "secrets.h"

class SyncTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testRealConnection();
    void testEndToEndSync();

private:
};

void SyncTest::initTestCase()
{
    QVERIFY(DatabaseManager::instance().init());
}

void SyncTest::testRealConnection()
{
    // Skip if secrets are empty
    if (Secrets::API_KEY.isEmpty()) {
        QSKIP("No secrets provided, skipping real connection test.");
    }

    SyncEngine::instance().setApiUrl(Secrets::API_URL);
    SyncEngine::instance().setApiKey(Secrets::API_KEY);

    QSignalSpy spyStarted(&SyncEngine::instance(), &SyncEngine::syncStarted);
    QSignalSpy spyFinished(&SyncEngine::instance(), &SyncEngine::syncFinished);

    SyncEngine::instance().forceSync();
    
    QCOMPARE(spyStarted.count(), 1);
    
    // Wait up to 10 seconds for real network
    QVERIFY(spyFinished.wait(10000));
    
    bool success = spyFinished.at(0).at(0).toBool();
    QVERIFY2(success, "Sync failed despite valid credentials");
}

void SyncTest::testEndToEndSync()
{
    // Verify that database now has feeds
    auto feeds = DatabaseManager::instance().getAllFeeds();
    QVERIFY(feeds.count() > 0);
    
    // Verify articles
    auto articles = DatabaseManager::instance().getAllArticles();
    QVERIFY(articles.count() > 0);
}

QTEST_GUILESS_MAIN(SyncTest)
#include "SyncTest.moc"
