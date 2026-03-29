// SPDX-License-Identifier: GPL-3.0-or-later
#include "DatabaseManager.h"

#include <QStandardPaths>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager inst;
    return inst;
}

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
}

bool DatabaseManager::init()
{
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir(dataDir).mkpath(QStringLiteral("."));
    const QString dbPath = dataDir + QStringLiteral("/kiriflux.db");

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "Failed to open database:" << m_db.lastError().text();
        return false;
    }

    qDebug() << "Database opened successfully at:" << dbPath;

    QSqlQuery query;
    // Create feeds table
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS feeds ("
            "id INTEGER PRIMARY KEY, "
            "title TEXT, "
            "site_url TEXT, "
            "feed_url TEXT, "
            "category_id INTEGER"
            ")"))) {
        qWarning() << "Failed to create feeds table:" << query.lastError().text();
    }

    // Create categories table
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS categories ("
            "id INTEGER PRIMARY KEY, "
            "title TEXT"
            ")"))) {
        qWarning() << "Failed to create categories table:" << query.lastError().text();
    }

    // Create articles table
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS articles ("
            "id INTEGER PRIMARY KEY, "
            "feed_id INTEGER, "
            "title TEXT, "
            "author TEXT, "
            "content TEXT, "
            "url TEXT, "
            "published_at TEXT, "
            "status TEXT, "
            "starred BOOLEAN, "
            "FOREIGN KEY(feed_id) REFERENCES feeds(id)"
            ")"))) {
        qWarning() << "Failed to create articles table:" << query.lastError().text();
    }

    // Add index for faster sorting by time
    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_articles_published_at ON articles(published_at DESC)"));

    // Seed with welcome article if empty
    query.exec(QStringLiteral("SELECT count(*) FROM articles"));
    if (query.next() && query.value(0).toInt() == 0) {
        Article welcome;
        welcome.id = 1;
        welcome.feedId = -1;
        welcome.title = QStringLiteral("Welcome to Kiriflux!");
        welcome.author = QStringLiteral("Kiriflux Team");
        welcome.content = QStringLiteral("<h1>Welcome!</h1><p>Kiriflux is a desktop-first RSS reader for KDE Plasma 6.</p><p>Go to settings to configure your Miniflux API details and start syncing!</p><p><b>Shortcuts:</b><br/><b>J/K</b>: Navigation<br/><b>M</b>: Mark Read<br/><b>R</b>: Sync</p>");
        welcome.status = QStringLiteral("unread");
        welcome.starred = false;
        welcome.publishedAt = QDateTime::currentDateTime().toString(Qt::ISODate);
        saveArticles({welcome});
    }

    return true;
}

void DatabaseManager::saveFeeds(const QList<Feed>& feeds)
{
    m_db.transaction();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO feeds (id, title, site_url, feed_url, category_id) "
        "VALUES (?, ?, ?, ?, ?)"));

    for (const auto& feed : feeds) {
        query.addBindValue(qlonglong(feed.id));
        query.addBindValue(feed.title);
        query.addBindValue(feed.siteUrl);
        query.addBindValue(feed.feedUrl);
        query.addBindValue(qlonglong(feed.categoryId));
        query.exec();
    }
    m_db.commit();
}

QList<Feed> DatabaseManager::getAllFeeds() const
{
    QList<Feed> feeds;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT f.*, (SELECT COUNT(*) FROM articles a WHERE a.feed_id = f.id AND a.status = 'unread') as unread_count "
        "FROM feeds f ORDER BY f.title"));
    
    if (query.exec()) {
        while (query.next()) {
            feeds.append({
                query.value("id").toLongLong(),
                query.value("title").toString(),
                query.value("site_url").toString(),
                query.value("feed_url").toString(),
                query.value("category_id").toLongLong(),
                query.value("unread_count").toInt()
            });
        }
    }
    return feeds;
}

void DatabaseManager::saveCategories(const QList<Category>& categories)
{
    m_db.transaction();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO categories (id, title) "
        "VALUES (?, ?)"));

    for (const auto& category : categories) {
        query.addBindValue(qlonglong(category.id));
        query.addBindValue(category.title);
        query.exec();
    }
    m_db.commit();
}

QList<Category> DatabaseManager::getAllCategories() const
{
    QList<Category> categories;
    QSqlQuery query(m_db);
    // Aggregate unread count from all feeds in the category
    query.prepare(QStringLiteral(
        "SELECT c.id, c.title, "
        "(SELECT COUNT(*) FROM articles a "
        " JOIN feeds f ON a.feed_id = f.id "
        " WHERE f.category_id = c.id AND a.status = 'unread') as unread_count "
        "FROM categories c ORDER BY c.title"));
    
    if (query.exec()) {
        while (query.next()) {
            categories.append({
                query.value("id").toLongLong(),
                query.value("title").toString(),
                query.value("unread_count").toInt()
            });
        }
    }
    return categories;
}

void DatabaseManager::saveArticles(const QList<Article>& articles)
{
    m_db.transaction();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO articles (id, feed_id, title, author, content, url, published_at, status, starred) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"));

    for (const auto& article : articles) {
        query.addBindValue(qlonglong(article.id));
        query.addBindValue(qlonglong(article.feedId));
        query.addBindValue(article.title);
        query.addBindValue(article.author);
        query.addBindValue(article.content);
        query.addBindValue(article.url);
        query.addBindValue(article.publishedAt);
        query.addBindValue(article.status);
        query.addBindValue(article.starred);
        query.exec();
    }
    m_db.commit();
}

QList<Article> DatabaseManager::getAllArticles() const
{
    QList<Article> articles;
    QSqlQuery query(QStringLiteral(
        "SELECT a.id, a.feed_id, a.title, a.author, a.content, a.url, a.published_at, a.status, a.starred, "
        "f.title as feed_title, f.site_url "
        "FROM articles a "
        "LEFT JOIN feeds f ON a.feed_id = f.id "
        "ORDER BY a.published_at DESC, a.id DESC"));
    while (query.next()) {
        articles.append({
            query.value(0).toLongLong(),
            query.value(1).toLongLong(),
            query.value(2).toString(),
            query.value(3).toString(),
            query.value(4).toString(),
            query.value(5).toString(),
            query.value(6).toString(),
            query.value(7).toString(),
            query.value(8).toBool(),
            query.value(9).toString(),  // feed_title
            query.value(10).toString()  // site_url
        });
    }
    return articles;
}

QList<Article> DatabaseManager::getArticlesForFeed(long feedId) const
{
    QList<Article> articles;
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT a.id, a.feed_id, a.title, a.author, a.content, a.url, a.published_at, a.status, a.starred, "
        "f.title as feed_title, f.site_url "
        "FROM articles a "
        "LEFT JOIN feeds f ON a.feed_id = f.id "
        "WHERE a.feed_id = ? "
        "ORDER BY a.published_at DESC, a.id DESC"));
    query.addBindValue(qlonglong(feedId));
    if (query.exec()) {
        while (query.next()) {
            articles.append({
                query.value(0).toLongLong(),
                query.value(1).toLongLong(),
                query.value(2).toString(),
                query.value(3).toString(),
                query.value(4).toString(),
                query.value(5).toString(),
                query.value(6).toString(),
                query.value(7).toString(),
                query.value(8).toBool(),
                query.value(9).toString(),  // feed_title
                query.value(10).toString()  // site_url
            });
        }
    }
    return articles;
}

QList<Article> DatabaseManager::getArticlesForCategory(long categoryId) const
{
    QList<Article> articles;
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT a.id, a.feed_id, a.title, a.author, a.content, a.url, a.published_at, a.status, a.starred, "
        "f.title as feed_title, f.site_url "
        "FROM articles a "
        "JOIN feeds f ON a.feed_id = f.id "
        "WHERE f.category_id = ? "
        "ORDER BY a.published_at DESC, a.id DESC"));
    query.addBindValue(qlonglong(categoryId));
    if (query.exec()) {
        while (query.next()) {
            articles.append({
                query.value(0).toLongLong(),
                query.value(1).toLongLong(),
                query.value(2).toString(),
                query.value(3).toString(),
                query.value(4).toString(),
                query.value(5).toString(),
                query.value(6).toString(),
                query.value(7).toString(),
                query.value(8).toBool(),
                query.value(9).toString(),  // feed_title
                query.value(10).toString()  // site_url
            });
        }
    }
    return articles;
}

void DatabaseManager::markAsRead(long articleId, bool read)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("UPDATE articles SET status = ? WHERE id = ?"));
    query.addBindValue(read ? QStringLiteral("read") : QStringLiteral("unread"));
    query.addBindValue(qlonglong(articleId));
    query.exec();
}

void DatabaseManager::toggleStar(long articleId, bool starred)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("UPDATE articles SET starred = ? WHERE id = ?"));
    query.addBindValue(starred);
    query.addBindValue(qlonglong(articleId));
    query.exec();
}

void DatabaseManager::updateArticleContent(long articleId, const QString& content)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("UPDATE articles SET content = ? WHERE id = ?"));
    query.addBindValue(content);
    query.addBindValue(qlonglong(articleId));
    query.exec();
}
