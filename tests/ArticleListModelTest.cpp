#include <QtTest>
#include "../src/core/ArticleListModel.h"
#include "../src/core/DatabaseManager.h"
#include "../src/core/SyncEngine.h"
#include "../src/core/kiriflux.h"

class ArticleListModelTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void testInitialState();
    void testFilter();
    void testCurrentFeed();
    void testMarkRead();
    void testToggleStarred();
    void testGetMap();
    void testMarkAllAsRead();
    void testMarkAllAsReadLazyLoading();
    void testMarkArticlesAsReadByIds();
    void testGetByIdEdgeCases();
    void testCategoryFilter();
    void testArticleContentUpdated();
    void testDataRoles();
    void testWiggleRoom();
    void testMarkReadUpToAndIdAt();
    void testRefreshDuplicateSafety();
    void testFeedIconUpdated();

private:
    void seedData();
};

void ArticleListModelTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    // Initialize DB in memory
    DatabaseManager::instance().initWithPath(QStringLiteral(":memory:"));
}

void ArticleListModelTest::init()
{
    seedData();
}

void ArticleListModelTest::seedData()
{
    DatabaseManager::instance().clearAllData();
    
    Feed f10{10, "F10", "u10", "f10", 1, 0};
    Feed f20{20, "F20", "u20", "f20", 1, 0};
    DatabaseManager::instance().saveFeeds({f10, f20});

    Article a1;
    a1.id = 1; a1.feedId = 10; a1.title = "A1"; a1.status = ArticleStatus::Unread; a1.starred = false;
    Article a2;
    a2.id = 2; a2.feedId = 10; a2.title = "A2"; a2.status = ArticleStatus::Read; a2.starred = true;
    Article a3;
    a3.id = 3; a3.feedId = 20; a3.title = "A3"; a3.status = ArticleStatus::Unread; a3.starred = true;

    DatabaseManager::instance().saveArticles({a1, a2, a3});
}

void ArticleListModelTest::testInitialState()
{
    ArticleListModel model;
    model.setFilterMode("all");
    QCOMPARE(model.rowCount(), 3);
}

void ArticleListModelTest::testFilter()
{
    ArticleListModel model;
    model.setFilterMode("unread");
    QCOMPARE(model.rowCount(), 2); // A1, A3

    model.setFilterMode("starred");
    QCOMPARE(model.rowCount(), 2); // A2, A3

    model.setFilterMode("all");
    QCOMPARE(model.rowCount(), 3);
}

void ArticleListModelTest::testMarkRead()
{
    ArticleListModel model;
    model.setFilterMode("all");
    QSignalSpy spy(&model, &ArticleListModel::dataChanged);
    
    // Find A1 (id 1)
    int a1Idx = -1;
    for (int i=0; i<model.rowCount(); ++i) {
        if (model.data(model.index(i, 0), ArticleListModel::IdRole).toLongLong() == qint64(1)) {
            a1Idx = i;
            break;
        }
    }
    QVERIFY(a1Idx != -1);

    model.markAsRead(a1Idx, true);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(model.data(model.index(a1Idx, 0), ArticleListModel::StatusRole).toString(), QString("read"));
    
    auto dbArticle = DatabaseManager::instance().getArticleById(1);
    QCOMPARE(dbArticle->status, ArticleStatus::Read);
}

void ArticleListModelTest::testToggleStarred()
{
    ArticleListModel model;
    model.setFilterMode("all");
    QSignalSpy spy(&model, &ArticleListModel::dataChanged);
    
    model.toggleStarred(0); // Toggle whoever is first
    QCOMPARE(spy.count(), 1);
}

void ArticleListModelTest::testGetMap()
{
    ArticleListModel model;
    model.setFilterMode("all");
    QVariantMap map = model.get(0);
    QVERIFY(map.contains("id"));
    QVERIFY(map.contains("title"));
}

void ArticleListModelTest::testCurrentFeed()
{
    ArticleListModel model;
    model.setFilterMode("all");
    model.setCurrentFeedId(10);
    QCOMPARE(model.rowCount(), 2); // A1, A2

    model.setCurrentFeedId(20);
    QCOMPARE(model.rowCount(), 1); // A3

    model.setCurrentFeedId(-1);
    QCOMPARE(model.rowCount(), 3);
}

void ArticleListModelTest::testMarkAllAsRead()
{
    ArticleListModel model;
    model.setFilterMode("unread");
    QCOMPARE(model.rowCount(), 2);
    
    QSignalSpy spy(&model, &ArticleListModel::dataChanged);
    model.markAllAsRead();
    
    // Both unread articles should be marked read
    QCOMPARE(spy.count(), 2);
    
    // Now they shouldn't show up in unread filter if we refresh (force hard reset to bypass evaporation guard)
    model.refresh(true);
    QCOMPARE(model.rowCount(), 0);
}

void ArticleListModelTest::testMarkAllAsReadLazyLoading()
{
    bool prevLazy = Settings::self()->lazyLoadingEnabled();
    int prevBatch = Settings::self()->lazyLoadingBatchSize();

    // In seedData(), there are 2 unread articles (id 1, id 3)
    // Add 13 more unread articles so we have 15 unread in DB
    QList<Article> extraArticles;
    for (int i = 4; i <= 16; ++i) {
        Article a;
        a.id = i;
        a.feedId = 10;
        a.title = QString("A%1").arg(i);
        a.status = ArticleStatus::Unread;
        a.starred = false;
        extraArticles.append(a);
    }
    DatabaseManager::instance().saveArticles(extraArticles);

    Settings::self()->setLazyLoadingEnabled(true);
    Settings::self()->setLazyLoadingBatchSize(10);

    ArticleListModel model;
    model.setFilterMode("unread");
    // Initial batch loaded in memory is only 10 articles
    QCOMPARE(model.rowCount(), 10);
    QVERIFY(model.canFetchMore(QModelIndex()));

    // Mark all as read should mark all 15 articles in the database
    model.markAllAsRead();

    // Verify all articles in DB are now read
    QList<Article> unreadInDb = DatabaseManager::instance().getArticles(-1, -1, ArticleStatus::Unread);
    QCOMPARE(unreadInDb.size(), 0);

    // Verify in-memory articles were marked read
    for (int i = 0; i < model.rowCount(); ++i) {
        QCOMPARE(model.data(model.index(i, 0), ArticleListModel::StatusRole).toString(), QStringLiteral("read"));
    }

    // Refreshing should show 0 unread
    model.refresh(true);
    QCOMPARE(model.rowCount(), 0);

    // Restore settings
    Settings::self()->setLazyLoadingEnabled(prevLazy);
    Settings::self()->setLazyLoadingBatchSize(prevBatch);
}

void ArticleListModelTest::testMarkArticlesAsReadByIds()
{
    ArticleListModel model;
    model.setFilterMode("all");

    QSignalSpy spy(&model, &ArticleListModel::dataChanged);
    model.markArticlesAsReadByIds({1, 3}, true);

    // Both A1 (id 1) and A3 (id 3) should have had dataChanged emitted
    QCOMPARE(spy.count(), 2);

    auto dbA1 = DatabaseManager::instance().getArticleById(1);
    auto dbA3 = DatabaseManager::instance().getArticleById(3);
    QVERIFY(dbA1.has_value());
    QVERIFY(dbA3.has_value());
    QCOMPARE(dbA1->status, ArticleStatus::Read);
    QCOMPARE(dbA3->status, ArticleStatus::Read);
}

void ArticleListModelTest::testGetByIdEdgeCases()
{
    ArticleListModel model;
    model.setFilterMode("all");
    
    // Existing ID
    QVariantMap map = model.getById(1);
    QVERIFY(!map.isEmpty());
    QCOMPARE(map["title"].toString(), QString("A1"));
    QCOMPARE(map["status"].toString(), QStringLiteral("unread"));
    
    // Non-existent ID
    QVariantMap map2 = model.getById(999);
    QVERIFY(map2.isEmpty());
    
    // Fallback getById (simulate article not in model but in DB)
    model.setCurrentFeedId(20); // Model now only has A3 (ID=3)
    QVariantMap fallbackMap = model.getById(1); // A1 is in DB, not in model
    QCOMPARE(fallbackMap["title"].toString(), QStringLiteral("A1"));
    QCOMPARE(fallbackMap["isRead"].toBool(), false);
    QCOMPARE(fallbackMap["status"].toString(), QStringLiteral("unread"));

    QVariantMap fallbackReadMap = model.getById(2); // A2 is in DB, read
    QCOMPARE(fallbackReadMap["isRead"].toBool(), true);
    QCOMPARE(fallbackReadMap["status"].toString(), QStringLiteral("read"));
}

void ArticleListModelTest::testCategoryFilter()
{
    // Need to insert a category and associate it
    Category c100{100, "Tech", 1};
    DatabaseManager::instance().saveCategories({c100});
    // Associate F10 with Category 100
    Feed f10{10, "F10", "u10", "f10", 100, 1};
    DatabaseManager::instance().saveFeeds({f10});
    
    ArticleListModel model;
    
    QSignalSpy categorySpy(&model, &ArticleListModel::currentCategoryIdChanged);
    QSignalSpy nameSpy(&model, &ArticleListModel::currentNameChanged);
    
    model.setCurrentCategoryId(100);
    QCOMPARE(model.currentCategoryId(), 100);
    QCOMPARE(model.currentFeedId(), -1);
    QCOMPARE(categorySpy.count(), 1);
    QCOMPARE(nameSpy.count(), 1);
    
    // Check currentName fallback
    QCOMPARE(model.currentName(), QStringLiteral("Tech"));
    
    // Should show A1 and A2 which belong to F10 -> C100
    QCOMPARE(model.rowCount(), 2);
}

void ArticleListModelTest::testArticleContentUpdated()
{
    ArticleListModel model;
    QSignalSpy dataSpy(&model, &ArticleListModel::dataChanged);
    
    // Modify DB behind the model's back
    DatabaseManager::instance().markAsRead(1, true); // Change status or content
    
    model.onArticleContentUpdated(1); // Simulate SyncEngine emitting this for A1
    
    QCOMPARE(dataSpy.count(), 1);
}

void ArticleListModelTest::testDataRoles()
{
    ArticleListModel model;
    model.setFilterMode("all");
    
    QModelIndex idx;
    for (int i = 0; i < model.rowCount(); ++i) {
        if (model.data(model.index(i, 0), ArticleListModel::IdRole).toLongLong() == 1) {
            idx = model.index(i, 0);
            break;
        }
    }
    QVERIFY(idx.isValid());
    
    // Test Qt::DisplayRole
    QCOMPARE(model.data(idx, Qt::DisplayRole).toString(), QStringLiteral("A1"));
    
    // Test invalid role
    QVERIFY(!model.data(idx, 9999).isValid());
    
    // Test invalid index
    QVERIFY(!model.data(model.index(999, 0), Qt::DisplayRole).isValid());
}

void ArticleListModelTest::testWiggleRoom()
{
    // Seed 150 articles
    DatabaseManager::instance().clearAllData();
    Feed f1{1, "F1", "u1", "f1", 1, 0};
    DatabaseManager::instance().saveFeeds({f1});

    QList<Article> articles;
    for (int i = 1; i <= 150; ++i) {
        Article a;
        a.id = i;
        a.feedId = 1;
        a.title = QString("Article %1").arg(i);
        a.status = ArticleStatus::Unread;
        a.publishedAt = QDateTime::currentDateTime().addSecs(i); // ascending
        articles.append(a);
    }
    DatabaseManager::instance().saveArticles(articles);

    // Enable lazy loading
    Settings::self()->setLazyLoadingEnabled(true);
    Settings::self()->setLazyLoadingBatchSize(50);

    ArticleListModel model;
    model.setFilterMode("all");
    
    // Verify only 50 loaded
    QCOMPARE(model.rowCount(), 50);
    
    // Verify fetchMore loads next 50
    model.fetchMore(QModelIndex());
    QCOMPARE(model.rowCount(), 100);

    // Programmatically inject 10 new articles
    QList<Article> newArticles;
    for (int i = 151; i <= 160; ++i) {
        Article a;
        a.id = i;
        a.feedId = 1;
        a.title = QString("Article %1").arg(i);
        a.status = ArticleStatus::Unread;
        a.publishedAt = QDateTime::currentDateTime().addSecs(i + 1000); // newer
        newArticles.append(a);
    }
    DatabaseManager::instance().saveArticles(newArticles);

    // Trigger background sync (forceReset = false)
    model.refresh(false);

    // Model should now have 110 items (100 originally loaded + 10 new)
    QCOMPARE(model.rowCount(), 110);
    
    // Clean up
    Settings::self()->setLazyLoadingEnabled(false);
}

void ArticleListModelTest::testMarkReadUpToAndIdAt()
{
    DatabaseManager::instance().clearAllData();
    Feed f1{1, "F1", "u1", "f1", 1, 0};
    DatabaseManager::instance().saveFeeds({f1});

    QList<Article> articles;
    const QDateTime base = QDateTime::currentDateTimeUtc();
    for (int i = 1; i <= 4; ++i) {
        Article a;
        a.id = i;
        a.feedId = 1;
        a.title = QString("A%1").arg(i);
        a.status = ArticleStatus::Unread;
        a.publishedAt = base.addSecs(i);
        articles.append(a);
    }
    DatabaseManager::instance().saveArticles(articles);

    ArticleListModel model;
    model.setFilterMode("all");
    QCOMPARE(model.rowCount(), 4);
    QCOMPARE(model.idAt(-1), qint64(-1));
    QCOMPARE(model.idAt(99), qint64(-1));
    QCOMPARE(model.idAt(0), qint64(4));

    model.markReadUpTo(2);
    QCOMPARE(DatabaseManager::instance().getArticleById(4)->status, ArticleStatus::Read);
    QCOMPARE(DatabaseManager::instance().getArticleById(3)->status, ArticleStatus::Read);
    QCOMPARE(DatabaseManager::instance().getArticleById(2)->status, ArticleStatus::Unread);
    QCOMPARE(DatabaseManager::instance().getArticleById(1)->status, ArticleStatus::Unread);
}

void ArticleListModelTest::testRefreshDuplicateSafety()
{
    DatabaseManager::instance().clearAllData();
    Feed f1{1, "F1", "u1", "f1", 1, 0};
    DatabaseManager::instance().saveFeeds({f1});

    const QDateTime base = QDateTime::currentDateTimeUtc();
    Article a1; a1.id = 1; a1.feedId = 1; a1.title = "One"; a1.status = ArticleStatus::Unread; a1.publishedAt = base.addSecs(10);
    Article a2; a2.id = 2; a2.feedId = 1; a2.title = "Two"; a2.status = ArticleStatus::Unread; a2.publishedAt = base.addSecs(20);
    DatabaseManager::instance().saveArticles({a1, a2});

    ArticleListModel model;
    model.setFilterMode("all");
    QCOMPARE(model.rowCount(), 2);

    // Move article 1 ahead of article 2 by updating its publishedAt in DB
    a1.publishedAt = base.addSecs(30);
    DatabaseManager::instance().saveArticles({a1});

    model.refresh(false);
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.idAt(0), qint64(1));
    QCOMPARE(model.idAt(1), qint64(2));
}

void ArticleListModelTest::testFeedIconUpdated()
{
    ArticleListModel model;
    model.setFilterMode("all");
    QSignalSpy spy(&model, &ArticleListModel::dataChanged);

    model.onFeedIconUpdated(10);
    QCOMPARE(spy.count(), 2); // A1 and A2 belong to feed 10
    QCOMPARE(model.getById(1)["feedHasIcon"].toBool(), true);
}

QTEST_GUILESS_MAIN(ArticleListModelTest)
#include "ArticleListModelTest.moc"
