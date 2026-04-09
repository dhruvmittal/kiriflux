#include <QtTest>
#include "../src/core/FeedListModel.h"
#include "../src/core/DatabaseManager.h"

class FeedListModelTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testModel();

private:
};

void FeedListModelTest::initTestCase()
{
    DatabaseManager::instance().initWithPath(QStringLiteral(":memory:"));
    DatabaseManager::instance().clearAllData();
    
    Feed f1{1, "F1", "u1", "f1", 10, 0};
    DatabaseManager::instance().saveFeeds({f1});

    Article a1; a1.id = 1; a1.feedId = 1; a1.status = "unread";
    Article a2; a2.id = 2; a2.feedId = 1; a2.status = "unread";
    DatabaseManager::instance().saveArticles({a1, a2});
}

void FeedListModelTest::testModel()
{
    FeedListModel model;
    QCOMPARE(model.rowCount(), 1);
    
    QCOMPARE(model.data(model.index(0, 0), FeedListModel::TitleRole).toString(), QString("F1"));
    QCOMPARE(model.data(model.index(0, 0), FeedListModel::UnreadCountRole).toInt(), 2);
}

QTEST_GUILESS_MAIN(FeedListModelTest)
#include "FeedListModelTest.moc"
