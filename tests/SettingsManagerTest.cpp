#include <QtTest>
#include "../src/core/SettingsManager.h"
#include <QSignalSpy>

class SettingsManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {
        QCoreApplication::setApplicationName("SettingsManagerTest");
    }

    void testDefaultValues() {
        SettingsManager settings;
        // Verify some defaults from kcfg
        QVERIFY(settings.syncInterval() > 0);
        QVERIFY(!settings.apiUrl().isEmpty() || settings.apiUrl() == "https://miniflux.example.com");
        QVERIFY(settings.titleLines() >= 1);
        QVERIFY(settings.leftPaneWidth() > 0);
        QVERIFY(settings.centerPaneWidth() > 0);
    }

    void testPropertyChanges() {
        SettingsManager settings;
        
        // Cache size
        QSignalSpy cacheSpy(&settings, &SettingsManager::cacheSizeMBChanged);
        int newSize = 1024;
        settings.setCacheSizeMB(newSize);
        QCOMPARE(settings.cacheSizeMB(), newSize);
        QCOMPARE(cacheSpy.count(), 1);

        // API Key
        QSignalSpy apiKeySpy(&settings, &SettingsManager::apiKeyChanged);
        QString newKey = "test_api_key_123";
        settings.setApiKey(newKey);
        QCOMPARE(settings.apiKey(), newKey);
        QCOMPARE(apiKeySpy.count(), 1);

        // Sync interval
        QSignalSpy syncIntervalSpy(&settings, &SettingsManager::syncIntervalChanged);
        settings.setSyncInterval(15);
        QCOMPARE(settings.syncInterval(), 15);
        QCOMPARE(syncIntervalSpy.count(), 1);

        // Boolean properties
        QSignalSpy markOpenSpy(&settings, &SettingsManager::markAsReadOnOpenChanged);
        bool origMarkOpen = settings.markAsReadOnOpen();
        settings.setMarkAsReadOnOpen(!origMarkOpen);
        QCOMPARE(settings.markAsReadOnOpen(), !origMarkOpen);
        QCOMPARE(markOpenSpy.count(), 1);

        QSignalSpy useReaderSpy(&settings, &SettingsManager::useReaderModeChanged);
        bool origReader = settings.useReaderMode();
        settings.setUseReaderMode(!origReader);
        QCOMPARE(settings.useReaderMode(), !origReader);
        QCOMPARE(useReaderSpy.count(), 1);

        // Missing integer properties
        settings.setTitleLines(3);
        QCOMPARE(settings.titleLines(), 3);
        
        settings.setCenterPaneWidth(500);
        QCOMPARE(settings.centerPaneWidth(), 500);
        
        // Missing boolean properties
        settings.setCleanSync(true);
        QCOMPARE(settings.cleanSync(), true);

        settings.setShowToolbarLabels(true);
        QCOMPARE(settings.showToolbarLabels(), true);
        
        settings.setPreFetchImages(true);
        QCOMPARE(settings.preFetchImages(), true);
        
        // DateTime
        QDateTime now = QDateTime::currentDateTime();
        settings.setLastSyncTime(now);
        QCOMPARE(settings.lastSyncTime(), now);
    }

    void testFontProperties() {
        SettingsManager settings;
        QSignalSpy fontSpy(&settings, &SettingsManager::articleListTitleFontChanged);
        
        QFont newFont("Comic Sans MS", 14, QFont::Bold);
        settings.setArticleListTitleFont(newFont);
        QCOMPARE(settings.articleListTitleFont(), newFont);
        QCOMPARE(fontSpy.count(), 1);
        
        // Test other fonts to hit coverage
        settings.setArticleListInfoFont(newFont);
        QCOMPARE(settings.articleListInfoFont(), newFont);
        
        settings.setArticleViewTitleFont(newFont);
        QCOMPARE(settings.articleViewTitleFont(), newFont);
        
        settings.setArticleViewInfoFont(newFont);
        QCOMPARE(settings.articleViewInfoFont(), newFont);
        
        settings.setArticleViewTextFont(newFont);
        QCOMPARE(settings.articleViewTextFont(), newFont);
        
        settings.setLeftPaneFont(newFont);
        QCOMPARE(settings.leftPaneFont(), newFont);
    }

    void testPersistence() {
        QString testUrl = "https://test.miniflux.app";
        int testWidth = 450;
        
        {
            SettingsManager settings;
            settings.setApiUrl(testUrl);
            settings.setLeftPaneWidth(testWidth);
            settings.save();
        }

        {
            SettingsManager settings;
            settings.load();
            QCOMPARE(settings.apiUrl(), testUrl);
            QCOMPARE(settings.leftPaneWidth(), testWidth);
        }
    }
    
    void testClearCacheRequested() {
        SettingsManager settings;
        QSignalSpy spy(&settings, &SettingsManager::clearCacheRequested);
        settings.clearCache();
        QCOMPARE(spy.count(), 1);
    }
};

QTEST_GUILESS_MAIN(SettingsManagerTest)
#include "SettingsManagerTest.moc"
