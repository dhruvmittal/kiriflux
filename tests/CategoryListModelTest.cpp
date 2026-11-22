#include <QtTest>
#include "../src/core/CategoryListModel.h"
#include "../src/core/DatabaseManager.h"

class CategoryListModelTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testModel();
    void testLargeIdNavigation();
};

void CategoryListModelTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    DatabaseManager::instance().initWithPath(QStringLiteral(":memory:"));
    DatabaseManager::instance().clearAllData();
    
    Category c1{1, "A-First", 10};
    Category c2{3000000000LL, "B-LargeId", 0};
    DatabaseManager::instance().saveCategories({c1, c2});
}

void CategoryListModelTest::testModel()
{
    CategoryListModel model;
    QCOMPARE(model.rowCount(), 2);
    
    QModelIndex validIdx = model.index(0, 0);
    QVERIFY(validIdx.isValid());
    
    QCOMPARE(model.data(validIdx, CategoryListModel::IdRole).toLongLong(), qint64(1));
    QCOMPARE(model.data(validIdx, CategoryListModel::TitleRole).toString(), QString("A-First"));
    QCOMPARE(model.data(validIdx, CategoryListModel::UnreadCountRole).toInt(), 0);
    
    QVERIFY(!model.data(QModelIndex(), CategoryListModel::TitleRole).isValid());
    QVERIFY(!model.data(model.index(999, 0), CategoryListModel::TitleRole).isValid());
    QVERIFY(!model.data(validIdx, 9999).isValid());
    QCOMPARE(model.rowCount(validIdx), 0);
}

void CategoryListModelTest::testLargeIdNavigation()
{
    CategoryListModel model;
    QCOMPARE(model.getNextCategoryId(-1), qint64(1));
    QCOMPARE(model.getNextCategoryId(1), qint64(3000000000LL));
    QCOMPARE(model.getNextCategoryId(3000000000LL), qint64(-1));

    QCOMPARE(model.getPreviousCategoryId(-1), qint64(3000000000LL));
    QCOMPARE(model.getPreviousCategoryId(3000000000LL), qint64(1));
    QCOMPARE(model.getPreviousCategoryId(1), qint64(-1));
}

QTEST_GUILESS_MAIN(CategoryListModelTest)
#include "CategoryListModelTest.moc"
