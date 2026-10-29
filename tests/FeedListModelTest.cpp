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
    QStandardPaths::setTestModeEnabled(true);
    DatabaseManager::instance().initWithPath(QStringLiteral(":memory:"));
    DatabaseManager::instance().clearAllData();
    
    Feed f1{1, "F1", "u1", "f1", 10, 0};
    DatabaseManager::instance().saveFeeds({f1});

    Article a1; a1.id = 1; a1.feedId = 1; a1.status = ArticleStatus::Unread;
    Article a2; a2.id = 2; a2.feedId = 1; a2.status = ArticleStatus::Unread;
    DatabaseManager::instance().saveArticles({a1, a2});
}

void FeedListModelTest::testModel()
{
    FeedListModel model;
    QCOMPARE(model.rowCount(), 1);
    
    QModelIndex validIdx = model.index(0, 0);
    QVERIFY(validIdx.isValid());
    
    // Check all roles
    QCOMPARE(model.data(validIdx, FeedListModel::IdRole).toLongLong(), 1);
    QCOMPARE(model.data(validIdx, FeedListModel::TitleRole).toString(), QString("F1"));
    QCOMPARE(model.data(validIdx, FeedListModel::SiteUrlRole).toString(), QString("u1"));
    QCOMPARE(model.data(validIdx, FeedListModel::FeedUrlRole).toString(), QString("f1"));
    QCOMPARE(model.data(validIdx, FeedListModel::CategoryIdRole).toLongLong(), 10);
    QCOMPARE(model.data(validIdx, FeedListModel::UnreadCountRole).toInt(), 2);
    QCOMPARE(model.data(validIdx, Qt::DisplayRole).toString(), QString("F1"));
    
    // Invalid index
    QVERIFY(!model.data(QModelIndex(), FeedListModel::TitleRole).isValid());
    QVERIFY(!model.data(model.index(999, 0), FeedListModel::TitleRole).isValid());
    
    // Invalid role
    QVERIFY(!model.data(validIdx, 9999).isValid());
    
    // rowCount with valid parent should be 0 (list model)
    QCOMPARE(model.rowCount(validIdx), 0);
}

QTEST_GUILESS_MAIN(FeedListModelTest)
#include "FeedListModelTest.moc"
