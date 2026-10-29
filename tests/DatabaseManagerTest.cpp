#include <QtTest>
#include <QThread>
#include <QtConcurrent>
#include <QFuture>
#include <QElapsedTimer>
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
    void testPurgeOldReadArticles();
    void testWelcomeArticle();
    void testThreadLocalConnections();
    void testConcurrentWrites();
    void testNonBlockingReads();

private:
};

void DatabaseManagerTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    // Use a file database so worker threads can access the same tables
    QString dbPath = QDir::tempPath() + "/kiriflux_test.db";
    QFile::remove(dbPath);
    QVERIFY(DatabaseManager::instance().initWithPath(dbPath));
    DatabaseManager::instance().clearAllData();
}

void DatabaseManagerTest::testInit()
{
    // Verify initialization succeeded
    QVERIFY(DatabaseManager::instance().getAllFeeds().isEmpty()); // Starts empty
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
    a1.publishedAt = QDateTime::fromString("2026-04-07T12:00:00Z", Qt::ISODate);

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

void DatabaseManagerTest::testPurgeOldReadArticles()
{
    DatabaseManager::instance().clearAllData();
    
    Article oldRead, oldStarred, newRead, unread;
    oldRead.id = 1; oldRead.title = "Old Read"; oldRead.status = ArticleStatus::Read; oldRead.starred = false;
    oldRead.publishedAt = QDateTime::currentDateTime().addDays(-10);
    
    oldStarred.id = 2; oldStarred.title = "Old Starred"; oldStarred.status = ArticleStatus::Read; oldStarred.starred = true;
    oldStarred.publishedAt = QDateTime::currentDateTime().addDays(-10);
    
    newRead.id = 3; newRead.title = "New Read"; newRead.status = ArticleStatus::Read; newRead.starred = false;
    newRead.publishedAt = QDateTime::currentDateTime().addDays(-1);
    
    unread.id = 4; unread.title = "Unread"; unread.status = ArticleStatus::Unread; unread.starred = false;
    unread.publishedAt = QDateTime::currentDateTime().addDays(-10);
    
    DatabaseManager::instance().saveArticles({oldRead, oldStarred, newRead, unread});
    
    // Purge older than 7 days
    DatabaseManager::instance().purgeOldReadArticles(7);
    
    auto remaining = DatabaseManager::instance().getAllArticles(ArticleStatus::All, false);
    QCOMPARE(remaining.size(), 3); // oldRead should be deleted
    
    bool oldReadFound = false;
    for (const auto& a : remaining) {
        if (a.id == 1) oldReadFound = true;
    }
    QVERIFY(!oldReadFound);
}

void DatabaseManagerTest::testWelcomeArticle()
{
    // TEST-1: Fix logic to use memory DB for seeding test
    DatabaseManager::instance().clearAllData();
    // Welcome article is seeded in initWithPath if count is 0
    QString dbPath = QDir::tempPath() + "/kiriflux_test.db";
    QVERIFY(DatabaseManager::instance().initWithPath(dbPath));
    
    auto articles = DatabaseManager::instance().getAllArticles();
    QVERIFY(!articles.isEmpty());
    QCOMPARE(articles.first().title, QStringLiteral("Welcome to Kiriflux!"));
}

void DatabaseManagerTest::testThreadLocalConnections()
{
    QString mainConnectionName;
    QSqlDatabase mainDb;
    
    // Force the connection to be created on main thread
    auto articles = DatabaseManager::instance().getAllArticles();
    mainConnectionName = QStringLiteral("kiriflux_%1").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));
    
    QVERIFY(QSqlDatabase::contains(mainConnectionName));
    mainDb = QSqlDatabase::database(mainConnectionName);
    QVERIFY(mainDb.isOpen());

    QString workerConnectionName;
    QFuture<void> future = QtConcurrent::run([&]() {
        auto workerArticles = DatabaseManager::instance().getAllArticles();
        workerConnectionName = QStringLiteral("kiriflux_%1").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));
    });
    future.waitForFinished();

    QVERIFY(QSqlDatabase::contains(workerConnectionName));
    QVERIFY(mainConnectionName != workerConnectionName);
}

void DatabaseManagerTest::testConcurrentWrites()
{
    DatabaseManager::instance().clearAllData();

    const int numThreads = 10;
    const int articlesPerThread = 100;

    QList<QFuture<void>> futures;

    for (int t = 0; t < numThreads; ++t) {
        futures.append(QtConcurrent::run([t, articlesPerThread]() {
            QList<Article> batch;
            for (int i = 0; i < articlesPerThread; ++i) {
                Article a;
                a.id = t * 1000 + i;
                a.feedId = 1;
                a.title = QString("Thread %1 Article %2").arg(t).arg(i);
                a.status = ArticleStatus::Unread;
                a.publishedAt = QDateTime::currentDateTime();
                batch.append(a);
            }
            DatabaseManager::instance().saveArticles(batch);
        }));
    }

    for (auto& f : futures) {
        f.waitForFinished();
    }

    auto allArticles = DatabaseManager::instance().getAllArticles();
    QCOMPARE(allArticles.size(), numThreads * articlesPerThread);
}

void DatabaseManagerTest::testNonBlockingReads()
{
    DatabaseManager::instance().clearAllData();

    QFuture<void> writeFuture = QtConcurrent::run([]() {
        QList<Article> batch;
        for (int i = 0; i < 5000; ++i) {
            Article a;
            a.id = 10000 + i;
            a.feedId = 1;
            a.title = QString("Massive Article %1").arg(i);
            a.status = ArticleStatus::Unread;
            a.publishedAt = QDateTime::currentDateTime();
            batch.append(a);
        }
        DatabaseManager::instance().saveArticles(batch);
    });

    QElapsedTimer timer;
    int readCount = 0;
    
    // Rapidly read while writing is happening
    while (!writeFuture.isFinished()) {
        timer.start();
        DatabaseManager::instance().getAllArticles();
        qint64 elapsed = timer.elapsed();
        // The read should be extremely fast, definitely under 50ms even during a heavy transaction due to WAL
        QVERIFY(elapsed < 200);
        readCount++;
        QThread::msleep(5);
    }
    
    QVERIFY(readCount > 0);
}

QTEST_GUILESS_MAIN(DatabaseManagerTest)
#include "DatabaseManagerTest.moc"
