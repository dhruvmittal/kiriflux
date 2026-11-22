#include <QtTest>
#include "../src/core/SyncEngine.h"

class ImagePrefetchTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setApplicationName("ImagePrefetchTest");
    }

    void testExtractImageUrls() {
        const QUrl baseUrl(QStringLiteral("https://blog.example.com/posts/hello"));
        const QString html = QStringLiteral(
            "<html><body>"
            "<img src=\"https://cdn.example.com/a.jpg\">"
            "<img class='hero' src='https://cdn.example.com/b.png' />"
            "<img data-src=\"https://ignored.example.com/lazy.jpg\" src=\"/images/c.gif\">"
            "<img src=\"//media.example.com/d.webp\">"
            "<img src=\"https://cdn.example.com/a.jpg\">"
            "<img src=\"data:image/png;base64,iVBORw0KGgo=\">"
            "<img src=\"javascript:alert(1)\">"
            "</body></html>");

        const QList<QUrl> urls = SyncEngine::extractImageUrls(html, baseUrl);
        QCOMPARE(urls.size(), 4);
        QCOMPARE(urls.at(0), QUrl(QStringLiteral("https://cdn.example.com/a.jpg")));
        QCOMPARE(urls.at(1), QUrl(QStringLiteral("https://cdn.example.com/b.png")));
        QCOMPARE(urls.at(2), QUrl(QStringLiteral("https://blog.example.com/images/c.gif")));
        QCOMPARE(urls.at(3), QUrl(QStringLiteral("https://media.example.com/d.webp")));
    }
};

QTEST_GUILESS_MAIN(ImagePrefetchTest)
#include "ImagePrefetchTest.moc"
