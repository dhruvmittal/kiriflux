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
    DatabaseManager::instance().initWithPath(QStringLiteral(":memory:"));
    
    Category c1{1, "C1", 10};
    Category c2{2, "C2", 0};
    DatabaseManager::instance().saveCategories({c1, c2});
}

void CategoryListModelTest::testModel()
{
    CategoryListModel model;
    QCOMPARE(model.rowCount(), 2);
    
    QCOMPARE(model.data(model.index(0, 0), CategoryListModel::TitleRole).toString(), QString("C1"));
}

QTEST_GUILESS_MAIN(CategoryListModelTest)
#include "CategoryListModelTest.moc"
