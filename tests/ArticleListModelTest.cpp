#include <QtTest>
#include "../src/core/ArticleListModel.h"
#include "../src/core/DatabaseManager.h"
#include "../src/core/SyncEngine.h"

class ArticleListModelTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testInitialState();
    void testFiltering();
    void testMarkAsRead();
    void testToggleStarred();
    void testGetMap();
    void testFeedSwitching();

private:
    void seedData();
};

void ArticleListModelTest::initTestCase()
{
    // Initialize DB in memory
    DatabaseManager::instance().initWithPath(QStringLiteral(":memory:"));
    seedData();
}

void ArticleListModelTest::seedData()
{
    DatabaseManager::instance().clearAllData();
    
    Feed f10{10, "F10", "u10", "f10", 1, 0};
    Feed f20{20, "F20", "u20", "f20", 1, 0};
    DatabaseManager::instance().saveFeeds({f10, f20});

    Article a1;
    a1.id = 1; a1.feedId = 10; a1.title = "A1"; a1.status = "unread"; a1.starred = false;
    Article a2;
    a2.id = 2; a2.feedId = 10; a2.title = "A2"; a2.status = "read"; a2.starred = true;
    Article a3;
    a3.id = 3; a3.feedId = 20; a3.title = "A3"; a3.status = "unread"; a3.starred = true;

    DatabaseManager::instance().saveArticles({a1, a2, a3});
}

void ArticleListModelTest::testInitialState()
{
    ArticleListModel model;
    model.setFilterStatus("all");
    model.setFilterStarred(false);
    QCOMPARE(model.rowCount(), 3);
}

void ArticleListModelTest::testFiltering()
{
    ArticleListModel model;
    model.setFilterStatus("unread");
    model.setFilterStarred(false);
    QCOMPARE(model.rowCount(), 2); // A1, A3

    model.setFilterStarred(true);
    QCOMPARE(model.rowCount(), 1); // Only A3 is unread AND starred

    model.setFilterStatus("all");
    QCOMPARE(model.rowCount(), 2); // A2, A3 are starred
}

void ArticleListModelTest::testMarkAsRead()
{
    ArticleListModel model;
    model.setFilterStatus("all");
    model.setFilterStarred(false);
    QSignalSpy spy(&model, &ArticleListModel::dataChanged);
    
    // Find A1 (id 1)
    int a1Idx = -1;
    for (int i=0; i<model.rowCount(); ++i) {
        if (model.data(model.index(i, 0), ArticleListModel::IdRole).toLongLong() == 1) {
            a1Idx = i;
            break;
        }
    }
    QVERIFY(a1Idx != -1);

    model.markAsRead(a1Idx, true);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(model.data(model.index(a1Idx, 0), ArticleListModel::StatusRole).toString(), QString("read"));
    
    auto dbArticle = DatabaseManager::instance().getArticleById(1);
    QCOMPARE(dbArticle->status, QString("read"));
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

void ArticleListModelTest::testFeedSwitching()
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

QTEST_GUILESS_MAIN(ArticleListModelTest)
#include "ArticleListModelTest.moc"
