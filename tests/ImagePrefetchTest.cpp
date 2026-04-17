#include <QtTest>
#include "../src/core/SyncEngine.h"
#include "../src/core/DatabaseManager.h"
#include <QSignalSpy>

// We need to access private methods for testing logic, or just test the public side.
// Since SyncEngine is a singleton, we can test it by performing a sync with mock data.
// But it's easier to just test the URL extraction if we make it public or use a friend class.

class ImagePrefetchTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {
        QCoreApplication::setApplicationName("ImagePrefetchTest");
    }

    void testHtmlParsing() {
        // This test verifies that we correctly identify <img> tags in HTML
        QString html = "<html><body>"
                       "<p>Hello world</p>"
                       "<img src=\"https://example.com/image1.jpg\">"
                       "<img class='test' src='https://example.com/image2.png' />"
                       "<a href='test'><img src=\"/relative/image3.gif\"></a>"
                       "</body></html>";
        
        // Match logic from SyncEngine.cpp:
        // QRegularExpression re(QStringLiteral("<img[^>]+src=[\"']([^\"']+)[\"']"), QRegularExpression::CaseInsensitiveOption);
        QRegularExpression re(QStringLiteral("<img[^>]+src=[\"']([^\"']+)[\"']"), QRegularExpression::CaseInsensitiveOption);
        
        QStringList detected;
        auto it = re.globalMatch(html);
        while (it.hasNext()) {
            detected << it.next().captured(1);
        }
        
        QCOMPARE(detected.count(), 3);
        QCOMPARE(detected.at(0), QString("https://example.com/image1.jpg"));
        QCOMPARE(detected.at(1), QString("https://example.com/image2.png"));
        QCOMPARE(detected.at(2), QString("/relative/image3.gif"));
    }

    void testUrlResolution() {
        QUrl base("https://blog.example.com/post/1");
        QString relative = "/static/img.png";
        QString absolute = "https://other.com/photo.jpg";
        
        QUrl res1 = base.resolved(QUrl(relative));
        QUrl res2 = base.resolved(QUrl(absolute));
        
        QCOMPARE(res1.toString(), QString("https://blog.example.com/static/img.png"));
        QCOMPARE(res2.toString(), QString("https://other.com/photo.jpg"));
    }
};

QTEST_GUILESS_MAIN(ImagePrefetchTest)
#include "ImagePrefetchTest.moc"
