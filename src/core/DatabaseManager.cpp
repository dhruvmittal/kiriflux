// SPDX-License-Identifier: GPL-3.0-or-later
#include "DatabaseManager.h"

#include <QStandardPaths>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
#include <QDateTime>
#include "KirifluxLogging.h"

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
    return initWithPath(dataDir + QStringLiteral("/kiriflux.db"));
}

bool DatabaseManager::initWithPath(const QString &dbPath)
{
    QMutexLocker locker(&m_mutex);
    
    // DB-1: Use a stable connection name to ensure test data persistence within a session.
    const QString connectionName = QStringLiteral("kiriflux_main");
    
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qCWarning(KIRIFLUX_DB) << "Failed to open database:" << m_db.lastError().text();
        return false;
    }

    // Enable WAL mode for better concurrency during sync (only for file-based DBs)
    if (dbPath != QStringLiteral(":memory:")) {
        QSqlQuery walQuery(m_db);
        if (!walQuery.exec(QStringLiteral("PRAGMA journal_mode=WAL"))) {
            qCWarning(KIRIFLUX_DB) << "Failed to enable WAL mode:" << walQuery.lastError().text();
        }
    }

    qCDebug(KIRIFLUX_DB) << "Database opened successfully at:" << dbPath;

    QSqlQuery query(m_db);
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
    if (!query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_articles_published_at ON articles(published_at DESC)"))) {
        qWarning() << "Failed to create articles index:" << query.lastError().text();
    }

    // Compound indexes for common filters (Feed/Status)
    if (!query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_articles_feed_status ON articles(feed_id, status)"))) {
        qWarning() << "Failed to create feed_status index:" << query.lastError().text();
    }

    // Index for starred articles
    if (!query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_articles_starred ON articles(starred) WHERE starred = 1"))) {
        qWarning() << "Failed to create starred index:" << query.lastError().text();
    }

    // Seed with welcome article if empty
    if (!query.exec(QStringLiteral("SELECT count(*) FROM articles"))) {
        qWarning() << "Failed to check article count:" << query.lastError().text();
    }
    if (query.next() && query.value(0).toInt() == 0) {
        Article welcome;
        welcome.id = 1;
        welcome.feedId = -1;
        welcome.title = QStringLiteral("Welcome to Kiriflux!");
        welcome.author = QStringLiteral("Kiriflux Team");
        welcome.content = QStringLiteral("<h1>Welcome!</h1><p>Kiriflux is a desktop-first RSS reader for KDE Plasma 6.</p><p>Go to settings to configure your Miniflux API details and start syncing!</p><p><b>Shortcuts:</b><br/><b>J/K</b>: Navigation<br/><b>M</b>: Mark Read<br/><b>R</b>: Sync</p>");
        welcome.status = ArticleStatus::Unread;
        welcome.starred = false;
        welcome.publishedAt = QDateTime::currentDateTime();
        saveArticles({welcome});
    }

    return true;
}

void DatabaseManager::saveFeeds(const QList<Feed>& feeds)
{
    QMutexLocker locker(&m_mutex);
    if (!m_db.transaction()) {
        qWarning() << "Failed to start transaction for feeds:" << m_db.lastError().text();
        return;
    }

    QSqlQuery query(m_db); // DB-3: Use explicit DB connection
    query.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO feeds (id, title, site_url, feed_url, category_id) "
        "VALUES (?, ?, ?, ?, ?)"));

    bool allOk = true;
    for (const auto& feed : feeds) {
        query.addBindValue(feed.id);
        query.addBindValue(feed.title);
        query.addBindValue(feed.siteUrl);
        query.addBindValue(feed.feedUrl);
        query.addBindValue(feed.categoryId);
        if (!query.exec()) {
            qWarning() << "Failed to save feed:" << query.lastError().text();
            allOk = false;
            break;
        }
    }

    if (allOk) {
        m_db.commit();
    } else {
        m_db.rollback(); // DB-2: Rollback on failure
    }
}

QList<Feed> DatabaseManager::getAllFeeds() const
{
    QMutexLocker locker(&m_mutex);
    QList<Feed> feeds;
    QSqlQuery query(m_db);
    // Optimized: Use LEFT JOIN and GROUP BY instead of correlated subquery
    query.prepare(QStringLiteral(
        "SELECT f.id AS id, f.title AS title, f.site_url AS site_url, f.feed_url AS feed_url, f.category_id AS category_id, "
        "COUNT(CASE WHEN a.status = 0 THEN 1 END) as unread_count "
        "FROM feeds f "
        "LEFT JOIN articles a ON f.id = a.feed_id "
        "GROUP BY f.id "
        "ORDER BY f.title COLLATE NOCASE"));
    
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

QString DatabaseManager::getFeedTitle(qint64 feedId) const
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT title FROM feeds WHERE id = ?"));
    query.addBindValue(feedId);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return {};
}

void DatabaseManager::saveCategories(const QList<Category>& categories)
{
    QMutexLocker locker(&m_mutex);
    if (!m_db.transaction()) {
        qWarning() << "Failed to start transaction for categories:" << m_db.lastError().text();
        return;
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO categories (id, title) "
        "VALUES (?, ?)"));

    bool allOk = true;
    for (const auto& category : categories) {
        query.addBindValue(category.id);
        query.addBindValue(category.title);
        if (!query.exec()) {
            qWarning() << "Failed to save category:" << query.lastError().text();
            allOk = false;
            break;
        }
    }

    if (allOk) {
        m_db.commit();
    } else {
        m_db.rollback();
    }
}

QList<Category> DatabaseManager::getAllCategories() const
{
    QMutexLocker locker(&m_mutex);
    QList<Category> categories;
    QSqlQuery query(m_db);
    // Optimized: Use LEFT JOIN and GROUP BY instead of correlated subquery
    query.prepare(QStringLiteral(
        "SELECT c.id AS id, c.title AS title, "
        "COUNT(CASE WHEN a.status = 0 THEN 1 END) as unread_count "
        "FROM categories c "
        "LEFT JOIN feeds f ON c.id = f.category_id "
        "LEFT JOIN articles a ON f.id = a.feed_id "
        "GROUP BY c.id "
        "ORDER BY c.title COLLATE NOCASE"));
    
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

QString DatabaseManager::getCategoryTitle(qint64 categoryId) const
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT title FROM categories WHERE id = ?"));
    query.addBindValue(categoryId);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return {};
}

void DatabaseManager::saveArticles(const QList<Article>& articles)
{
    QMutexLocker locker(&m_mutex);
    if (!m_db.transaction()) {
        qWarning() << "Failed to start transaction for articles:" << m_db.lastError().text();
        return;
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO articles (id, feed_id, title, author, content, url, published_at, status, starred) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"));

    bool allOk = true;
    for (const auto& article : articles) {
        query.addBindValue(article.id);
        query.addBindValue(article.feedId);
        query.addBindValue(article.title);
        query.addBindValue(article.author);
        query.addBindValue(article.content);
        query.addBindValue(article.url);
        query.addBindValue(article.publishedAt.toString(Qt::ISODate));
        query.addBindValue(static_cast<int>(article.status));
        query.addBindValue(article.starred ? 1 : 0);
        if (!query.exec()) {
            qWarning() << "Failed to save article:" << query.lastError().text();
            allOk = false;
            break;
        }
    }

    if (allOk) {
        m_db.commit();
    } else {
        m_db.rollback();
    }
}

static Article articleFromQuery(const QSqlQuery& query)
{
    Article a;
    a.id = query.value(0).toLongLong();
    a.feedId = query.value(1).toLongLong();
    a.title = query.value(2).toString();
    a.author = query.value(3).toString();
    a.content = query.value(4).toString();
    a.url = query.value(5).toString();
    a.publishedAt = QDateTime::fromString(query.value(6).toString(), Qt::ISODate);
    
    QVariant statusVar = query.value(7);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (statusVar.typeId() == QMetaType::Int || statusVar.typeId() == QMetaType::LongLong) {
#else
    if (statusVar.type() == QVariant::Int || statusVar.type() == QVariant::LongLong) {
#endif
        a.status = static_cast<ArticleStatus>(statusVar.toInt());
    } else {
        QString statusStr = statusVar.toString().toLower();
        if (statusStr == "read" || statusStr == "1") a.status = ArticleStatus::Read;
        else if (statusStr == "removed" || statusStr == "2") a.status = ArticleStatus::Removed;
        else a.status = ArticleStatus::Unread;
    }
    
    a.starred = query.value(8).toBool();
    a.feedTitle = query.value(9).toString();
    a.siteUrl = query.value(10).toString();
    
    return a;
}

QList<Article> DatabaseManager::getAllArticles(ArticleStatus filterStatus, bool filterStarred) const
{
    QMutexLocker locker(&m_mutex);
    QList<Article> articles;
    QSqlQuery query(m_db);
    
    QString sql = QStringLiteral(
        "SELECT a.id, a.feed_id, a.title, a.author, a.content, a.url, a.published_at, a.status, a.starred, "
        "f.title as feed_title, f.site_url "
        "FROM articles a "
        "LEFT JOIN feeds f ON a.feed_id = f.id "
        "WHERE 1=1 ");
    
    if (filterStatus != ArticleStatus::All) {
        sql += QStringLiteral("AND a.status = %1 ").arg(static_cast<int>(filterStatus));
    }
    if (filterStarred) {
        sql += QStringLiteral("AND a.starred = 1 ");
    }
    sql += QStringLiteral("ORDER BY a.published_at DESC, a.id DESC");

    query.prepare(sql);
    if (query.exec()) {
        while (query.next()) {
            articles.append(articleFromQuery(query));
        }
    }
    return articles;
}

std::optional<Article> DatabaseManager::getArticleById(qint64 id) const
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT a.id, a.feed_id, a.title, a.author, a.content, a.url, a.published_at, a.status, a.starred, "
        "f.title as feed_title, f.site_url "
        "FROM articles a "
        "LEFT JOIN feeds f ON a.feed_id = f.id "
        "WHERE a.id = ?"));
    query.addBindValue(id);
    if (query.exec() && query.next()) {
        return articleFromQuery(query);
    }
    return std::nullopt;
}

QList<Article> DatabaseManager::getArticlesForFeed(qint64 feedId, ArticleStatus filterStatus, bool filterStarred) const
{
    QMutexLocker locker(&m_mutex);
    QList<Article> articles;
    QSqlQuery query(m_db);
    
    QString sql = QStringLiteral(
        "SELECT a.id, a.feed_id, a.title, a.author, a.content, a.url, a.published_at, a.status, a.starred, "
        "f.title as feed_title, f.site_url "
        "FROM articles a "
        "LEFT JOIN feeds f ON a.feed_id = f.id "
        "WHERE a.feed_id = ? ");
    
    if (filterStatus != ArticleStatus::All) {
        sql += QStringLiteral("AND a.status = %1 ").arg(static_cast<int>(filterStatus));
    }
    if (filterStarred) {
        sql += QStringLiteral("AND a.starred = 1 ");
    }
    sql += QStringLiteral("ORDER BY a.published_at DESC, a.id DESC");

    query.prepare(sql);
    query.addBindValue(feedId);
    if (query.exec()) {
        while (query.next()) {
            articles.append(articleFromQuery(query));
        }
    }
    return articles;
}

void DatabaseManager::clearAllData()
{
    QMutexLocker locker(&m_mutex);
    if (!m_db.transaction()) return;
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("DELETE FROM articles"))) qWarning() << "Clear articles failed:" << query.lastError().text();
    if (!query.exec(QStringLiteral("DELETE FROM feeds"))) qWarning() << "Clear feeds failed:" << query.lastError().text();
    if (!query.exec(QStringLiteral("DELETE FROM categories"))) qWarning() << "Clear categories failed:" << query.lastError().text();
    m_db.commit();
}

QList<Article> DatabaseManager::getArticlesForCategory(qint64 categoryId, ArticleStatus filterStatus, bool filterStarred) const
{
    QMutexLocker locker(&m_mutex);
    QList<Article> articles;
    QSqlQuery query(m_db);
    
    QString sql = QStringLiteral(
        "SELECT a.id, a.feed_id, a.title, a.author, a.content, a.url, a.published_at, a.status, a.starred, "
        "f.title as feed_title, f.site_url "
        "FROM articles a "
        "JOIN feeds f ON a.feed_id = f.id "
        "WHERE f.category_id = ? ");
    
    if (filterStatus != ArticleStatus::All) {
        sql += QStringLiteral("AND a.status = %1 ").arg(static_cast<int>(filterStatus));
    }
    if (filterStarred) {
        sql += QStringLiteral("AND a.starred = 1 ");
    }
    sql += QStringLiteral("ORDER BY a.published_at DESC, a.id DESC");

    query.prepare(sql);
    query.addBindValue(categoryId);
    if (query.exec()) {
        while (query.next()) {
            articles.append(articleFromQuery(query));
        }
    }
    return articles;
}

void DatabaseManager::markAsRead(qint64 articleId, bool read)
{
    markArticlesAsRead({articleId}, read);
}

void DatabaseManager::markArticlesAsRead(const QList<qint64>& articleIds, bool read)
{
    if (articleIds.isEmpty()) return;

    QMutexLocker locker(&m_mutex);
    if (!m_db.transaction()) return;

    QSqlQuery query(m_db);
    // Optimization: For batch, we could use WHERE id IN (...) but SQLite has limits on params.
    // For simplicity, we loop within the transaction.
    query.prepare(QStringLiteral("UPDATE articles SET status = ? WHERE id = ?"));
    int statusInt = read ? static_cast<int>(ArticleStatus::Read) : static_cast<int>(ArticleStatus::Unread);

    bool allOk = true;
    for (qint64 id : articleIds) {
        query.bindValue(0, statusInt);
        query.bindValue(1, id);
        if (!query.exec()) {
            qCWarning(KIRIFLUX_DB) << "Failed to mark article as read:" << id << query.lastError().text();
            allOk = false;
            break;
        }
    }

    if (allOk) m_db.commit(); else m_db.rollback();
}

void DatabaseManager::toggleStar(qint64 articleId, bool starred)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE articles SET starred = ? WHERE id = ?"));
    query.addBindValue(starred);
    query.addBindValue(articleId);
    if (!query.exec()) {
        qCWarning(KIRIFLUX_DB) << "Failed to toggle star:" << query.lastError().text();
    }
}

void DatabaseManager::updateArticleStatus(qint64 articleId, ArticleStatus status)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE articles SET status = ? WHERE id = ?"));
    query.addBindValue(static_cast<int>(status));
    query.addBindValue(articleId);
    if (!query.exec()) {
        qCWarning(KIRIFLUX_DB) << "Failed to update article status:" << query.lastError().text();
    }
}

void DatabaseManager::updateArticleContent(qint64 articleId, const QString& content)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE articles SET content = ? WHERE id = ?"));
    query.addBindValue(content);
    query.addBindValue(articleId);
    if (!query.exec()) {
        qCWarning(KIRIFLUX_DB) << "Failed to update content:" << query.lastError().text();
    }
}

void DatabaseManager::purgeOldReadArticles(int days)
{
    QMutexLocker locker(&m_mutex);
    QDateTime cutoff = QDateTime::currentDateTime().addDays(-days);
    
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "DELETE FROM articles "
        "WHERE status = ? "
        "AND starred = 0 "
        "AND published_at < ?"));
    query.addBindValue(static_cast<int>(ArticleStatus::Read));
    query.addBindValue(cutoff.toString(Qt::ISODate));
    
    if (!query.exec()) {
        qCWarning(KIRIFLUX_DB) << "Failed to purge old articles:" << query.lastError().text();
    } else {
        int purged = query.numRowsAffected();
        if (purged > 0) {
            qCDebug(KIRIFLUX_DB) << "Purged" << purged << "old read articles.";
        }
    }
}
