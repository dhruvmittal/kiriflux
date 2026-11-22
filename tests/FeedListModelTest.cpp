#include <QtTest>
#include <QSignalSpy>
#include "../src/core/FeedListModel.h"
#include "../src/core/DatabaseManager.h"

class FeedListModelTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testModel();
    void testIconRoleAndUpdate();
};

void FeedListModelTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    DatabaseManager::instance().initWithPath(QStringLiteral(":memory:"));
    DatabaseManager::instance().clearAllData();
    
    Feed f1{1, "F1", "u1", "f1", 10, 0, 77, false};
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
    
    QCOMPARE(model.data(validIdx, FeedListModel::IdRole).toLongLong(), 1);
    QCOMPARE(model.data(validIdx, FeedListModel::TitleRole).toString(), QString("F1"));
    QCOMPARE(model.data(validIdx, FeedListModel::SiteUrlRole).toString(), QString("u1"));
    QCOMPARE(model.data(validIdx, FeedListModel::FeedUrlRole).toString(), QString("f1"));
    QCOMPARE(model.data(validIdx, FeedListModel::CategoryIdRole).toLongLong(), 10);
    QCOMPARE(model.data(validIdx, FeedListModel::UnreadCountRole).toInt(), 2);
    QCOMPARE(model.data(validIdx, FeedListModel::IconIdRole).toLongLong(), 77);
    QCOMPARE(model.data(validIdx, FeedListModel::HasIconRole).toBool(), false);
    QCOMPARE(model.data(validIdx, Qt::DisplayRole).toString(), QString("F1"));
    
    QVERIFY(!model.data(QModelIndex(), FeedListModel::TitleRole).isValid());
    QVERIFY(!model.data(model.index(999, 0), FeedListModel::TitleRole).isValid());
    QVERIFY(!model.data(validIdx, 9999).isValid());
    QCOMPARE(model.rowCount(validIdx), 0);
}

void FeedListModelTest::testIconRoleAndUpdate()
{
    FeedListModel model;
    const QModelIndex idx = model.index(0, 0);
    QCOMPARE(model.data(idx, FeedListModel::HasIconRole).toBool(), false);

    QSignalSpy spy(&model, &FeedListModel::dataChanged);
    model.onFeedIconUpdated(1);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(model.data(idx, FeedListModel::HasIconRole).toBool(), true);
}

QTEST_GUILESS_MAIN(FeedListModelTest)
#include "FeedListModelTest.moc"
