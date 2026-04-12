#include <QtTest>
#include "../src/core/DatabaseManager.h"

class DatabaseManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testInit();
    void testCategories();
    void testFeeds();
    void testArticles();
    void testArticleMutation();
    void testCategoryTitles();
    void testClearAllData();
    void testWelcomeArticle();

private:
};

void DatabaseManagerTest::initTestCase()
{
    // Use in-memory database for isolation
    QVERIFY(DatabaseManager::instance().initWithPath(QStringLiteral(":memory:")));
    DatabaseManager::instance().clearAllData();
}

void DatabaseManagerTest::testInit()
{
    // Verify initialization succeeded
    QVERIFY(DatabaseManager::instance().getAllFeeds().isEmpty()); // Memory DB starts empty
}

void DatabaseManagerTest::testCategories()
{
    Category cat1{1, "Tech", 0};
    Category cat2{2, "News", 0};
    DatabaseManager::instance().saveCategories({cat1, cat2});

    auto cats = DatabaseManager::instance().getAllCategories();
    QCOMPARE(cats.size(), 2);
    QCOMPARE(cats[0].title, QString("News")); // Alphabetical order
    QCOMPARE(cats[1].title, QString("Tech"));
}

void DatabaseManagerTest::testFeeds()
{
    Feed f1{101, "The Verge", "https://theverge.com", "https://theverge.com/rss", 1, 0};
    DatabaseManager::instance().saveFeeds({f1});

    auto feeds = DatabaseManager::instance().getAllFeeds();
    QCOMPARE(feeds.size(), 1);
    QCOMPARE(feeds[0].title, QString("The Verge"));
    QCOMPARE(feeds[0].categoryId, qint64(1));
}

void DatabaseManagerTest::testArticles()
{
    Article a1;
    a1.id = 1001;
    a1.feedId = 101;
    a1.title = "Article 1";
    a1.status = ArticleStatus::Unread;
    a1.starred = false;
    a1.publishedAt = QStringLiteral("2026-04-07T12:00:00Z");

    DatabaseManager::instance().saveArticles({a1});

    auto articles = DatabaseManager::instance().getAllArticles();
    QCOMPARE(articles.size(), 1);
    QCOMPARE(articles[0].id, qint64(1001));
    QCOMPARE(articles[0].title, QString("Article 1"));

    auto feedArticles = DatabaseManager::instance().getArticlesForFeed(101);
    QCOMPARE(feedArticles.size(), 1);

    auto catArticles = DatabaseManager::instance().getArticlesForCategory(1);
    QCOMPARE(catArticles.size(), 1);

    auto articleOpt = DatabaseManager::instance().getArticleById(1001);
    QVERIFY(articleOpt.has_value());
    QCOMPARE(articleOpt->title, QString("Article 1"));
}

void DatabaseManagerTest::testArticleMutation()
{
    DatabaseManager::instance().markAsRead(1001, true);
    auto a = DatabaseManager::instance().getArticleById(1001);
    QCOMPARE(a->status, ArticleStatus::Read);

    DatabaseManager::instance().toggleStar(1001, true);
    a = DatabaseManager::instance().getArticleById(1001);
    QVERIFY(a->starred);

    DatabaseManager::instance().updateArticleContent(1001, "New Content");
    a = DatabaseManager::instance().getArticleById(1001);
    QCOMPARE(a->content, QString("New Content"));
}

void DatabaseManagerTest::testCategoryTitles()
{
    QCOMPARE(DatabaseManager::instance().getCategoryTitle(1), QString("Tech"));
    QCOMPARE(DatabaseManager::instance().getFeedTitle(101), QString("The Verge"));
}

void DatabaseManagerTest::testClearAllData()
{
    DatabaseManager::instance().clearAllData();
    QVERIFY(DatabaseManager::instance().getAllArticles().isEmpty());
    QVERIFY(DatabaseManager::instance().getAllFeeds().isEmpty());
    QVERIFY(DatabaseManager::instance().getAllCategories().isEmpty());
}

void DatabaseManagerTest::testWelcomeArticle()
{
    // TEST-1: Fix logic to use memory DB for seeding test
    DatabaseManager::instance().clearAllData();
    // Welcome article is seeded in initWithPath if count is 0
    QVERIFY(DatabaseManager::instance().initWithPath(QStringLiteral(":memory:")));
    
    auto articles = DatabaseManager::instance().getAllArticles();
    QVERIFY(!articles.isEmpty());
    QCOMPARE(articles.first().title, QStringLiteral("Welcome to Kiriflux!"));
}

QTEST_GUILESS_MAIN(DatabaseManagerTest)
#include "DatabaseManagerTest.moc"
