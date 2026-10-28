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
    void testFeedSwitching();
    void testMarkAllAsRead();
    void testMarkArticlesAsReadByIds();
    void testGetByIdEdgeCases();
    void testCategoryFilter();
    void testArticleContentUpdated();
    void testDataRoles();
    void testWiggleRoom();

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
    model.setFilterStatus("all");
    model.setFilterStarred(false);
    QCOMPARE(model.rowCount(), 3);
}

void ArticleListModelTest::testFilter()
{
    ArticleListModel model;
    model.setFilterStatus("unread");
    model.setFilterStarred(false);
    QCOMPARE(model.rowCount(), 2); // A1, A3

    model.setFilterStarred(true);
    QCOMPARE(model.rowCount(), 2); // Starred filter ignores unread status, shows A2 and A3

    model.setFilterStatus("all");
    QCOMPARE(model.rowCount(), 2); // A2, A3 are starred
}

void ArticleListModelTest::testMarkRead()
{
    ArticleListModel model;
    model.setFilterStatus("all");
    model.setFilterStarred(false);
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
    model.setFilterStatus("all");
    model.setFilterStarred(false);
    QSignalSpy spy(&model, &ArticleListModel::dataChanged);
    
    model.toggleStarred(0); // Toggle whoever is first
    QCOMPARE(spy.count(), 1);
}

void ArticleListModelTest::testGetMap()
{
    ArticleListModel model;
    model.setFilterStatus("all");
    model.setFilterStarred(false);
    QVariantMap map = model.get(0);
    QVERIFY(map.contains("id"));
    QVERIFY(map.contains("title"));
}

void ArticleListModelTest::testCurrentFeed()
{
    ArticleListModel model;
    model.setFilterStatus("all");
    model.setFilterStarred(false);
    model.setCurrentFeedId(10);
    QCOMPARE(model.rowCount(), 2); // A1, A2

    model.setCurrentFeedId(20);
    QCOMPARE(model.rowCount(), 1); // A3

    model.setCurrentFeedId(-1);
    QCOMPARE(model.rowCount(), 3);
}

void ArticleListModelTest::testFeedSwitching()
{
    testCurrentFeed();
}

void ArticleListModelTest::testMarkAllAsRead()
{
    ArticleListModel model;
    model.setFilterStatus("unread");
    QCOMPARE(model.rowCount(), 2);
    
    QSignalSpy spy(&model, &ArticleListModel::dataChanged);
    model.markAllAsRead();
    
    // Both unread articles should be marked read
    QCOMPARE(spy.count(), 2);
    
    // Now they shouldn't show up in unread filter if we refresh (force hard reset to bypass evaporation guard)
    model.refresh(true);
    QCOMPARE(model.rowCount(), 0);
}

void ArticleListModelTest::testMarkArticlesAsReadByIds()
{
    ArticleListModel model;
    model.setFilterStatus("all");
    model.setFilterStarred(false);

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
    model.setFilterStatus("all");
    
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
    model.setFilterStatus("all");
    model.setFilterStarred(false);
    
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
    model.setFilterStatus("all");
    model.setFilterStarred(false);
    
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

QTEST_GUILESS_MAIN(ArticleListModelTest)
#include "ArticleListModelTest.moc"
