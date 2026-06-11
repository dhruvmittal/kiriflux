#include <QtTest>
#include "../src/core/NetworkManager.h"
#include "../src/core/KirifluxNAMFactory.h"
#include <QNetworkAccessManager>
#include <QDir>

class NetworkManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {
        QCoreApplication::setApplicationName("NetworkManagerTest");
    }

    void testSingleton() {
        NetworkManager& n1 = NetworkManager::instance();
        NetworkManager& n2 = NetworkManager::instance();
        QCOMPARE(&n1, &n2);
        QVERIFY(n1.nam() != nullptr);
        QVERIFY(n1.cache() != nullptr);
    }

    void testCacheInitialization() {
        NetworkManager& nm = NetworkManager::instance();
        QString cacheDir = nm.cache()->cacheDirectory();
        QVERIFY(!cacheDir.isEmpty());
        QVERIFY(QDir(cacheDir).exists());
    }

    void testNAMFactory() {
        KirifluxNAMFactory factory;
        QObject parent;
        QNetworkAccessManager* nam = factory.create(&parent);
        
        QVERIFY(nam != nullptr);
        QVERIFY(nam->cache() != nullptr);
        
        // Verify that the factory sets the global cache directory on the newly created NAM's cache
        auto namCache = qobject_cast<QNetworkDiskCache*>(nam->cache());
        QVERIFY(namCache != nullptr);
        QCOMPARE(namCache->cacheDirectory(), NetworkManager::instance().cache()->cacheDirectory());
        QCOMPARE(namCache->maximumCacheSize(), NetworkManager::instance().cache()->maximumCacheSize());
    }

    void testClearCache() {
        NetworkManager& nm = NetworkManager::instance();
        // This is hard to test without real network activity but we can verify the call
        nm.clearCache();
        // If it didn't crash, it's a good sign.
    }
};

QTEST_GUILESS_MAIN(NetworkManagerTest)
#include "NetworkManagerTest.moc"
