#include <QtTest>
#include <QSqlQuery>
#include "../src/core/DatabaseManager.h"

class DatabaseManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testInit();
    void testCategories();
    void testFeeds();
    void testFeedIconsAndMigration();
    void testArticles();
    void testUnreadArticleIds();
    void testArticleMutation();
    void testCategoryTitles();
    void testClearAllData();
    void testPurgeOldReadArticles();
    void testWelcomeArticle();
};

void DatabaseManagerTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QString dbPath = QDir::tempPath() + "/kiriflux_test.db";
    QFile::remove(dbPath);
    QVERIFY(DatabaseManager::instance().initWithPath(dbPath));
    DatabaseManager::instance().clearAllData();
}

void DatabaseManagerTest::testInit()
{
    QVERIFY(DatabaseManager::instance().getAllFeeds().isEmpty());

    QSqlQuery query(QSqlDatabase::database(QStringLiteral("kiriflux_main")));
    QVERIFY(query.exec(QStringLiteral("PRAGMA user_version")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 2);
}

void DatabaseManagerTest::testCategories()
{
    Category cat1{1, "Tech", 0};
    Category cat2{2, "News", 0};
    DatabaseManager::instance().saveCategories({cat1, cat2});

    auto cats = DatabaseManager::instance().getAllCategories();
    QCOMPARE(cats.size(), 2);
    QCOMPARE(cats[0].title, QString("News"));
    QCOMPARE(cats[1].title, QString("Tech"));
}

void DatabaseManagerTest::testFeeds()
{
    Feed f1{101, "The Verge", "https://theverge.com", "https://theverge.com/rss", 1, 0, 55, false};
    DatabaseManager::instance().saveFeeds({f1});

    auto feeds = DatabaseManager::instance().getAllFeeds();
    QCOMPARE(feeds.size(), 1);
    QCOMPARE(feeds[0].title, QString("The Verge"));
    QCOMPARE(feeds[0].categoryId, qint64(1));
    QCOMPARE(feeds[0].iconId, qint64(55));
    QCOMPARE(feeds[0].hasIcon, false);
}

void DatabaseManagerTest::testFeedIconsAndMigration()
{
    // Feed 101 was saved with iconId = 55 and no icon data yet
    QList<qint64> missing = DatabaseManager::instance().getFeedIdsMissingIcons();
    QCOMPARE(missing, QList<qint64>{101});

    const QString iconPayload = QStringLiteral("image/png;base64,iVBORw0KGgo=");
    DatabaseManager::instance().saveFeedIcon(101, iconPayload);
    QCOMPARE(DatabaseManager::instance().getFeedIconData(101), iconPayload);
    QVERIFY(DatabaseManager::instance().getFeedIdsMissingIcons().isEmpty());

    auto feeds = DatabaseManager::instance().getAllFeeds();
    QCOMPARE(feeds[0].hasIcon, true);

    // Re-saving feed with same iconId must preserve icon_data
    Feed sameIcon{101, "The Verge Updated", "https://theverge.com", "https://theverge.com/rss", 1, 0, 55, false};
    DatabaseManager::instance().saveFeeds({sameIcon});
    QCOMPARE(DatabaseManager::instance().getFeedIconData(101), iconPayload);

    // Re-saving feed with a new iconId must invalidate old icon_data
    Feed newIcon{101, "The Verge", "https://theverge.com", "https://theverge.com/rss", 1, 0, 99, false};
    DatabaseManager::instance().saveFeeds({newIcon});
    QVERIFY(DatabaseManager::instance().getFeedIconData(101).isEmpty());
    QCOMPARE(DatabaseManager::instance().getFeedIdsMissingIcons(), QList<qint64>{101});
}

void DatabaseManagerTest::testArticles()
{
    Article a1;
    a1.id = 1001;
    a1.feedId = 101;
    a1.title = "Article 1";
    a1.status = ArticleStatus::Unread;
    a1.starred = false;
    a1.publishedAt = QDateTime::fromString("2026-04-07T12:00:00Z", Qt::ISODate);

    DatabaseManager::instance().saveArticles({a1});

    auto articles = DatabaseManager::instance().getArticles(-1, -1, ArticleStatus::Unread);
    QCOMPARE(articles.size(), 1);
    QCOMPARE(articles[0].id, qint64(1001));
    QCOMPARE(articles[0].title, QString("Article 1"));
    QCOMPARE(articles[0].feedIconId, qint64(99));

    auto feedArticles = DatabaseManager::instance().getArticles(101);
    QCOMPARE(feedArticles.size(), 1);

    auto catArticles = DatabaseManager::instance().getArticles(-1, 1);
    QCOMPARE(catArticles.size(), 1);

    QCOMPARE(DatabaseManager::instance().countArticles(-1, -1, ArticleStatus::Unread), 1);
    QCOMPARE(DatabaseManager::instance().countArticles(101, -1, ArticleStatus::Unread), 1);
    QCOMPARE(DatabaseManager::instance().countArticles(-1, 1, ArticleStatus::Unread), 1);

    auto articleOpt = DatabaseManager::instance().getArticleById(1001);
    QVERIFY(articleOpt.has_value());
    QCOMPARE(articleOpt->title, QString("Article 1"));
}

void DatabaseManagerTest::testUnreadArticleIds()
{
    Article a2;
    a2.id = 1002;
    a2.feedId = 101;
    a2.title = "Article 2";
    a2.status = ArticleStatus::Read;
    a2.starred = true;
    a2.publishedAt = QDateTime::fromString("2026-04-07T11:00:00Z", Qt::ISODate);
    DatabaseManager::instance().saveArticles({a2});

    QCOMPARE(DatabaseManager::instance().getUnreadArticleIds(), QList<qint64>{1001});
    QCOMPARE(DatabaseManager::instance().getUnreadArticleIds(101), QList<qint64>{1001});
    QCOMPARE(DatabaseManager::instance().getUnreadArticleIds(-1, 1), QList<qint64>{1001});
    QVERIFY(DatabaseManager::instance().getUnreadArticleIds(-1, -1, true).isEmpty());
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
    QVERIFY(DatabaseManager::instance().getArticles(-1, -1, ArticleStatus::All).isEmpty());
    QVERIFY(DatabaseManager::instance().getAllFeeds().isEmpty());
    QVERIFY(DatabaseManager::instance().getAllCategories().isEmpty());
}

void DatabaseManagerTest::testPurgeOldReadArticles()
{
    DatabaseManager::instance().clearAllData();
    
    Article oldRead, oldStarred, newRead, unread;
    oldRead.id = 1; oldRead.title = "Old Read"; oldRead.status = ArticleStatus::Read; oldRead.starred = false;
    oldRead.publishedAt = QDateTime::currentDateTimeUtc().addDays(-10);
    
    oldStarred.id = 2; oldStarred.title = "Old Starred"; oldStarred.status = ArticleStatus::Read; oldStarred.starred = true;
    oldStarred.publishedAt = QDateTime::currentDateTimeUtc().addDays(-10);
    
    newRead.id = 3; newRead.title = "New Read"; newRead.status = ArticleStatus::Read; newRead.starred = false;
    newRead.publishedAt = QDateTime::currentDateTimeUtc().addDays(-1);
    
    unread.id = 4; unread.title = "Unread"; unread.status = ArticleStatus::Unread; unread.starred = false;
    unread.publishedAt = QDateTime::currentDateTimeUtc().addDays(-10);
    
    DatabaseManager::instance().saveArticles({oldRead, oldStarred, newRead, unread});
    
    DatabaseManager::instance().purgeOldReadArticles(7);
    
    auto remaining = DatabaseManager::instance().getArticles(-1, -1, ArticleStatus::All, false);
    QCOMPARE(remaining.size(), 3);
    
    bool oldReadFound = false;
    for (const auto& a : remaining) {
        if (a.id == 1) oldReadFound = true;
    }
    QVERIFY(!oldReadFound);
}

void DatabaseManagerTest::testWelcomeArticle()
{
    DatabaseManager::instance().clearAllData();
    QString dbPath = QDir::tempPath() + "/kiriflux_test.db";
    QVERIFY(DatabaseManager::instance().initWithPath(dbPath));
    
    auto articles = DatabaseManager::instance().getArticles(-1, -1, ArticleStatus::Unread);
    QVERIFY(!articles.isEmpty());
    QCOMPARE(articles.first().title, QStringLiteral("Welcome to Kiriflux!"));
}

QTEST_GUILESS_MAIN(DatabaseManagerTest)
#include "DatabaseManagerTest.moc"
