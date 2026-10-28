#include <QtTest>
#include "../src/core/CategoryListModel.h"
#include "../src/core/DatabaseManager.h"

class CategoryListModelTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testModel();

private:
};

void CategoryListModelTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    DatabaseManager::instance().initWithPath(QStringLiteral(":memory:"));
    
    Category c1{1, "C1", 10};
    Category c2{2, "C2", 0};
    DatabaseManager::instance().saveCategories({c1, c2});
}

void CategoryListModelTest::testModel()
{
    CategoryListModel model;
    QCOMPARE(model.rowCount(), 2);
    
    QModelIndex validIdx = model.index(0, 0);
    QVERIFY(validIdx.isValid());
    
    // Check all roles
    QCOMPARE(model.data(validIdx, CategoryListModel::IdRole).toLongLong(), 1);
    QCOMPARE(model.data(validIdx, CategoryListModel::TitleRole).toString(), QString("C1"));
    QCOMPARE(model.data(validIdx, CategoryListModel::UnreadCountRole).toInt(), 0);
    
    // Invalid index
    QVERIFY(!model.data(QModelIndex(), CategoryListModel::TitleRole).isValid());
    QVERIFY(!model.data(model.index(999, 0), CategoryListModel::TitleRole).isValid());
    
    // Invalid role
    QVERIFY(!model.data(validIdx, 9999).isValid());
    
    // rowCount with valid parent should be 0 (list model)
    QCOMPARE(model.rowCount(validIdx), 0);
}

QTEST_GUILESS_MAIN(CategoryListModelTest)
#include "CategoryListModelTest.moc"
