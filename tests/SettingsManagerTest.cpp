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
    }

    void testPropertyChanges() {
        SettingsManager settings;
        QSignalSpy spy(&settings, &SettingsManager::cacheSizeMBChanged);

        int newSize = 1024;
        settings.setCacheSizeMB(newSize);
        QCOMPARE(settings.cacheSizeMB(), newSize);
        QCOMPARE(spy.count(), 1);
    }

    void testPersistence() {
        SettingsManager settings;
        QString testUrl = "https://test.miniflux.app";
        settings.setApiUrl(testUrl);
        settings.save();

        SettingsManager settings2;
        settings2.load();
        QCOMPARE(settings2.apiUrl(), testUrl);
    }
};

QTEST_GUILESS_MAIN(SettingsManagerTest)
#include "SettingsManagerTest.moc"
