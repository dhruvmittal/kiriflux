#include <QtTest>
#include "../src/core/AppController.h"
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "../src/core/DatabaseManager.h"
#include "../src/core/SyncEngine.h"
#include "kiriflux.h"

class AppControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setApplicationName("AppControllerTest");
        DatabaseManager::instance().initWithPath(QStringLiteral(":memory:"));
    }

    void testInit() {
        AppController controller;
        // Verify init succeeds
        QVERIFY(controller.init());
        
        // Verify models are instantiated
        QVERIFY(controller.feedListModel() != nullptr);
        QVERIFY(controller.articleListModel() != nullptr);
        QVERIFY(controller.categoryListModel() != nullptr);
    }

    void testContextRegistration() {
        AppController controller;
        controller.init();
        QQmlApplicationEngine engine;
        
        controller.registerContextProperties(&engine);
        
        QQmlContext* ctx = engine.rootContext();
        QVERIFY(ctx != nullptr);
        
        // Check if context properties are correctly set
        QVERIFY(ctx->contextProperty("syncEngine").value<QObject*>() != nullptr);
        QVERIFY(ctx->contextProperty("networkManager").value<QObject*>() != nullptr);
        QVERIFY(ctx->contextProperty("settings").value<QObject*>() != nullptr);
        QVERIFY(ctx->contextProperty("appAboutData").isValid());
        QVERIFY(ctx->contextProperty("feedListModel").value<QObject*>() != nullptr);
        QVERIFY(ctx->contextProperty("articleListModel").value<QObject*>() != nullptr);
        QVERIFY(ctx->contextProperty("categoryListModel").value<QObject*>() != nullptr);
    }
};

QTEST_GUILESS_MAIN(AppControllerTest)
#include "AppControllerTest.moc"
