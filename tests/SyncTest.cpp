#include <QtTest>
#include <QSignalSpy>
#include "../src/core/SyncEngine.h"
#include "../src/core/DatabaseManager.h"
#include "../src/core/SettingsManager.h"
#include <QProcessEnvironment>

#ifdef EXISTS_SECRETS
#include "secrets.h"
#endif

class SyncTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testRealConnection();
    void testEndToEndSync();
    void testFetchFullContent();

private:
};

void SyncTest::initTestCase()
{
    QVERIFY(DatabaseManager::instance().init());
}

void SyncTest::testRealConnection()
{
    QString apiUrl;
    QString apiKey;

#ifdef EXISTS_SECRETS
    apiUrl = Secrets::API_URL;
    apiKey = Secrets::API_KEY;
#endif

    // SEC-2: Prioritize environment variables for security
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (env.contains("KIRIFLUX_TEST_URL")) apiUrl = env.value("KIRIFLUX_TEST_URL");
    if (env.contains("KIRIFLUX_TEST_KEY")) apiKey = env.value("KIRIFLUX_TEST_KEY");

    if (apiKey.isEmpty()) {
        QSKIP("No secrets provided (via secrets.h or environment), skipping real connection test.");
    }

    SettingsManager::instance().setApiUrl(apiUrl);
    SettingsManager::instance().setApiKey(apiKey);

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
    
    auto articles = DatabaseManager::instance().getAllArticles();
    QVERIFY(articles.count() > 0);
}

void SyncTest::testFetchFullContent()
{
    if (SettingsManager::instance().apiKey().isEmpty()) {
        QSKIP("No secrets provided, skipping full content test.");
    }

    auto articles = DatabaseManager::instance().getAllArticles();
    QVERIFY(!articles.isEmpty());
    qint64 testId = articles.first().id;

    QSignalSpy spyUpdated(&SyncEngine::instance(), &SyncEngine::articleContentUpdated);
    SyncEngine::instance().fetchFullContent(testId);

    // Wait for the update (Miniflux can be slow to reach upstream)
    QVERIFY(spyUpdated.wait(10000));

    auto updatedArticles = DatabaseManager::instance().getAllArticles();
    QString newContent;
    for (const auto& a : updatedArticles) {
        if (a.id == testId) {
            newContent = a.content;
            break;
        }
    }

    qDebug() << "Fetched content for ID" << testId << "length:" << newContent.length();
    QVERIFY(!newContent.isEmpty());
}

QTEST_GUILESS_MAIN(SyncTest)
#include "SyncTest.moc"
