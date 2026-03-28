#include <QtTest>
#include "../src/core/DatabaseManager.h"

class DatabaseTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testInit();
    void testSaveAndGetFeeds();
    void testSaveAndGetArticles();
    void testUnreadCount();

private:
};

void DatabaseTest::initTestCase()
{
    // Use an in-memory database for testing if possible, or a temp file
    // For now we just let it use the default location but we should probably
    // override the path in DatabaseManager for better isolation.
}

void DatabaseTest::cleanupTestCase()
{
}

void DatabaseTest::testInit()
{
    QVERIFY(DatabaseManager::instance().init());
}

void DatabaseTest::testSaveAndGetFeeds()
{
    Feed testFeed;
    testFeed.id = 123;
    testFeed.title = "Test Feed";
    testFeed.siteUrl = "https://example.com";
    
    DatabaseManager::instance().saveFeeds({testFeed});
    
    QList<Feed> feeds = DatabaseManager::instance().getAllFeeds();
    bool found = false;
    for (const auto& f : feeds) {
        if (f.id == 123) {
            QCOMPARE(f.title, QString("Test Feed"));
            found = true;
            break;
        }
    }
    QVERIFY(found);
}

void DatabaseTest::testSaveAndGetArticles()
{
    Article testArticle;
    testArticle.id = 456;
    testArticle.feedId = 123;
    testArticle.title = "Test Article";
    testArticle.status = "unread";
    
    DatabaseManager::instance().saveArticles({testArticle});
    
    QList<Article> articles = DatabaseManager::instance().getArticlesForFeed(123);
    QCOMPARE(articles.count(), 1);
    QCOMPARE(articles[0].title, QString("Test Article"));
}

void DatabaseTest::testUnreadCount()
{
    // Verify that the feed unread count is updated correctly
    QList<Feed> feeds = DatabaseManager::instance().getAllFeeds();
    for (const auto& f : feeds) {
        if (f.id == 123) {
            QCOMPARE(f.unreadCount, 1);
            break;
        }
    }
    
    DatabaseManager::instance().markAsRead(456, true);
    feeds = DatabaseManager::instance().getAllFeeds();
    for (const auto& f : feeds) {
        if (f.id == 123) {
            QCOMPARE(f.unreadCount, 0);
            break;
        }
    }
}

QTEST_GUILESS_MAIN(DatabaseTest)
#include "DatabaseTest.moc"
