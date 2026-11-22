#include <QtTest>
#include "kiriflux.h"
#include <QSignalSpy>

class SettingsManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setApplicationName("SettingsManagerTest");
        QString testConfig = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/kirifluxrc");
        QFile::remove(testConfig);
        Settings::self()->setDefaults();
        Settings::self()->save();
    }

    void cleanupTestCase() {
        QString testConfig = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/kirifluxrc");
        QFile::remove(testConfig);
    }

    void testDefaultValues() {
        Settings* s = Settings::self();
        QVERIFY(s->syncInterval() > 0);
        QVERIFY(s->apiUrl().isEmpty() || s->apiUrl() == "https://miniflux.example.com");
        QVERIFY(s->titleLines() >= 1);
        QVERIFY(s->leftPaneWidth() > 0);
        QVERIFY(s->centerPaneWidth() > 0);
        QCOMPARE(s->filterMode(), QStringLiteral("all"));

        // Verify font defaults never have negative sizes
        QVERIFY(s->articleViewTitleFont().pointSizeF() > 0 || s->articleViewTitleFont().pixelSize() > 0);
        QVERIFY(s->articleViewInfoFont().pointSizeF() > 0 || s->articleViewInfoFont().pixelSize() > 0);
    }

    void testPropertyChanges() {
        Settings* s = Settings::self();

        QSignalSpy cacheSpy(s, &Settings::CacheSizeMBChanged);
        s->setCacheSizeMB(1024);
        QCOMPARE(s->cacheSizeMB(), 1024);
        QCOMPARE(cacheSpy.count(), 1);

        QSignalSpy apiKeySpy(s, &Settings::ApiKeyChanged);
        s->setApiKey("test_api_key_123");
        QCOMPARE(s->apiKey(), QString("test_api_key_123"));
        QCOMPARE(apiKeySpy.count(), 1);

        QSignalSpy syncIntervalSpy(s, &Settings::SyncIntervalChanged);
        s->setSyncInterval(15);
        QCOMPARE(s->syncInterval(), 15);
        QCOMPARE(syncIntervalSpy.count(), 1);

        QSignalSpy markOpenSpy(s, &Settings::MarkAsReadOnOpenChanged);
        bool origMarkOpen = s->markAsReadOnOpen();
        s->setMarkAsReadOnOpen(!origMarkOpen);
        QCOMPARE(s->markAsReadOnOpen(), !origMarkOpen);
        QCOMPARE(markOpenSpy.count(), 1);

        QSignalSpy useReaderSpy(s, &Settings::UseReaderModeChanged);
        bool origReader = s->useReaderMode();
        s->setUseReaderMode(!origReader);
        QCOMPARE(s->useReaderMode(), !origReader);
        QCOMPARE(useReaderSpy.count(), 1);
    }

    void testFontProperties() {
        Settings* s = Settings::self();
        QSignalSpy fontSpy(s, &Settings::ArticleListTitleFontChanged);

        QFont newFont("Comic Sans MS", 14, QFont::Bold);
        s->setArticleListTitleFont(newFont);
        QCOMPARE(s->articleListTitleFont(), newFont);
        QCOMPARE(fontSpy.count(), 1);
    }

    void testPersistenceAndLoadRevert() {
        Settings* s = Settings::self();
        const QString savedUrl = QStringLiteral("https://test.miniflux.app");
        s->setApiUrl(savedUrl);
        s->setLeftPaneWidth(450);
        s->save();

        // Simulate user editing in Preferences then clicking Cancel
        QSignalSpy urlSpy(s, &Settings::ApiUrlChanged);
        s->setApiUrl(QStringLiteral("https://unsaved.example.com"));
        QCOMPARE(s->apiUrl(), QStringLiteral("https://unsaved.example.com"));
        QCOMPARE(urlSpy.count(), 1);

        // Call Q_INVOKABLE load() via QMetaObject to verify QML invokability too
        bool invoked = QMetaObject::invokeMethod(s, "load");
        QVERIFY(invoked);
        QCOMPARE(s->apiUrl(), savedUrl);
        QCOMPARE(s->leftPaneWidth(), 450);
        QCOMPARE(urlSpy.count(), 2);
    }
};

QTEST_GUILESS_MAIN(SettingsManagerTest)
#include "SettingsManagerTest.moc"
