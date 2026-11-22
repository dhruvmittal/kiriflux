// SPDX-License-Identifier: GPL-3.0-or-later
#include "DatabaseManager.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include "kiriflux.h"
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
#include <QDateTime>
#include "KirifluxLogging.h"

static const QString kConnectionName = QStringLiteral("kiriflux_main");

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager* inst = nullptr;
    if (!inst) {
        inst = new DatabaseManager(nullptr);
        qAddPostRoutine([]() {
            delete inst;
            inst = nullptr;
        });
    }
    return *inst;
}

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
}

DatabaseManager::~DatabaseManager()
{
    closeDb();
}

void DatabaseManager::closeDb()
{
    {
        QSqlDatabase db = QSqlDatabase::database(kConnectionName, false);
        if (db.isValid() && db.isOpen()) {
            db.close();
        }
    }
    if (QSqlDatabase::contains(kConnectionName)) {
        QSqlDatabase::removeDatabase(kConnectionName);
    }
}

bool DatabaseManager::init()
{
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir(dataDir).mkpath(QStringLiteral("."));
    return initWithPath(dataDir + QStringLiteral("/kiriflux.db"));
}

QSqlDatabase DatabaseManager::getDb() const
{
    if (QSqlDatabase::contains(kConnectionName)) {
        return QSqlDatabase::database(kConnectionName);
    }
    
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), kConnectionName);
    db.setDatabaseName(m_dbPath);

    if (!db.open()) {
        qCWarning(KIRIFLUX_DB) << "Failed to open database:" << db.lastError().text();
        return db;
    }

    if (m_dbPath != QStringLiteral(":memory:")) {
        QSqlQuery pragmaQuery(db);
        if (!pragmaQuery.exec(QStringLiteral("PRAGMA journal_mode=WAL"))) {
            qCWarning(KIRIFLUX_DB) << "Failed to enable WAL mode:" << pragmaQuery.lastError().text();
        }
        if (!pragmaQuery.exec(QStringLiteral("PRAGMA synchronous=NORMAL"))) {
            qCWarning(KIRIFLUX_DB) << "Failed to set synchronous=NORMAL:" << pragmaQuery.lastError().text();
        }
    }
    return db;
}

bool DatabaseManager::initWithPath(const QString &dbPath)
{
    closeDb();
    m_dbPath = dbPath;
    
    QSqlDatabase db = getDb();
    if (!db.isOpen()) {
        return false;
    }

    qCDebug(KIRIFLUX_DB) << "Database opened successfully at:" << dbPath;

    QSqlQuery query(db);
    // Create feeds table
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS feeds ("
            "id INTEGER PRIMARY KEY, "
            "title TEXT, "
            "site_url TEXT, "
            "feed_url TEXT, "
            "category_id INTEGER, "
            "icon_id INTEGER DEFAULT 0, "
            "icon_data TEXT DEFAULT ''"
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
    
    int schemaVersion = 0;
    if (query.exec(QStringLiteral("PRAGMA user_version")) && query.next()) {
        schemaVersion = query.value(0).toInt();
    }

    if (schemaVersion < 1) {
        // Migration v1: Convert legacy string statuses ('unread', 'read') to integer-based statuses ('0', '1')
        query.exec(QStringLiteral("UPDATE articles SET status = '0' WHERE status = 'unread'"));
        query.exec(QStringLiteral("UPDATE articles SET status = '1' WHERE status = 'read'"));
        query.exec(QStringLiteral("UPDATE articles SET status = '2' WHERE status = 'removed'"));
    }

    if (schemaVersion < 2) {
        // Migration v2: Ensure feeds table has icon_id and icon_data columns
        bool hasIconId = false;
        bool hasIconData = false;
        if (query.exec(QStringLiteral("PRAGMA table_info(feeds)"))) {
            while (query.next()) {
                const QString col = query.value(1).toString();
                if (col == QLatin1String("icon_id")) hasIconId = true;
                else if (col == QLatin1String("icon_data")) hasIconData = true;
            }
        }
        if (!hasIconId && !query.exec(QStringLiteral("ALTER TABLE feeds ADD COLUMN icon_id INTEGER DEFAULT 0"))) {
            qWarning() << "Failed to add icon_id column to feeds:" << query.lastError().text();
        }
        if (!hasIconData && !query.exec(QStringLiteral("ALTER TABLE feeds ADD COLUMN icon_data TEXT DEFAULT ''"))) {
            qWarning() << "Failed to add icon_data column to feeds:" << query.lastError().text();
        }
        if (!query.exec(QStringLiteral("PRAGMA user_version = 2"))) {
            qWarning() << "Failed to set PRAGMA user_version:" << query.lastError().text();
        }
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
        welcome.publishedAt = QDateTime::currentDateTimeUtc();
        saveArticles({welcome});
    }

    return true;
}

void DatabaseManager::saveFeeds(const QList<Feed>& feeds)
{
    QSqlDatabase db = getDb();
    if (!db.transaction()) {
        qWarning() << "Failed to start transaction for feeds:" << db.lastError().text();
        return;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT INTO feeds (id, title, site_url, feed_url, category_id, icon_id) "
        "VALUES (?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(id) DO UPDATE SET "
        "title = excluded.title, "
        "site_url = excluded.site_url, "
        "feed_url = excluded.feed_url, "
        "category_id = excluded.category_id, "
        "icon_id = excluded.icon_id, "
        "icon_data = CASE WHEN feeds.icon_id != excluded.icon_id THEN '' ELSE feeds.icon_data END"));

    bool allOk = true;
    for (const auto& feed : feeds) {
        query.bindValue(0, feed.id);
        query.bindValue(1, feed.title);
        query.bindValue(2, feed.siteUrl);
        query.bindValue(3, feed.feedUrl);
        query.bindValue(4, feed.categoryId);
        query.bindValue(5, feed.iconId);
        if (!query.exec()) {
            qWarning() << "Failed to save feed:" << query.lastError().text();
            allOk = false;
            break;
        }
    }

    if (allOk) {
        db.commit();
    } else {
        db.rollback();
    }
}

QList<Feed> DatabaseManager::getAllFeeds() const
{
    QList<Feed> feeds;
    QSqlQuery query(getDb());
    query.prepare(QStringLiteral(
        "SELECT f.id AS id, f.title AS title, f.site_url AS site_url, f.feed_url AS feed_url, f.category_id AS category_id, "
        "COALESCE(f.icon_id, 0) AS icon_id, "
        "(f.icon_data IS NOT NULL AND f.icon_data != '') AS has_icon, "
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
                query.value("unread_count").toInt(),
                query.value("icon_id").toLongLong(),
                query.value("has_icon").toBool()
            });
        }
    }
    return feeds;
}

QString DatabaseManager::getFeedTitle(qint64 feedId) const
{
    QSqlQuery query(getDb());
    query.prepare(QStringLiteral("SELECT title FROM feeds WHERE id = ?"));
    query.addBindValue(feedId);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return {};
}

QList<qint64> DatabaseManager::getFeedIdsMissingIcons() const
{
    QList<qint64> ids;
    QSqlQuery query(getDb());
    if (query.exec(QStringLiteral("SELECT id FROM feeds WHERE icon_id > 0 AND (icon_data IS NULL OR icon_data = '')"))) {
        while (query.next()) {
            ids.append(query.value(0).toLongLong());
        }
    }
    return ids;
}

void DatabaseManager::saveFeedIcon(qint64 feedId, const QString& iconData)
{
    QSqlQuery query(getDb());
    query.prepare(QStringLiteral("UPDATE feeds SET icon_data = ? WHERE id = ?"));
    query.addBindValue(iconData);
    query.addBindValue(feedId);
    if (!query.exec()) {
        qCWarning(KIRIFLUX_DB) << "Failed to save feed icon:" << feedId << query.lastError().text();
    }
}

QString DatabaseManager::getFeedIconData(qint64 feedId) const
{
    QSqlQuery query(getDb());
    query.prepare(QStringLiteral("SELECT icon_data FROM feeds WHERE id = ?"));
    query.addBindValue(feedId);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return {};
}

void DatabaseManager::saveCategories(const QList<Category>& categories)
{
    QSqlDatabase db = getDb();
    if (!db.transaction()) {
        qWarning() << "Failed to start transaction for categories:" << db.lastError().text();
        return;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO categories (id, title) "
        "VALUES (?, ?)"));

    bool allOk = true;
    for (const auto& category : categories) {
        query.bindValue(0, category.id);
        query.bindValue(1, category.title);
        if (!query.exec()) {
            qWarning() << "Failed to save category:" << query.lastError().text();
            allOk = false;
            break;
        }
    }

    if (allOk) {
        db.commit();
    } else {
        db.rollback();
    }
}

QList<Category> DatabaseManager::getAllCategories() const
{
    QList<Category> categories;
    QSqlQuery query(getDb());
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
    QSqlQuery query(getDb());
    query.prepare(QStringLiteral("SELECT title FROM categories WHERE id = ?"));
    query.addBindValue(categoryId);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return {};
}

void DatabaseManager::saveArticles(const QList<Article>& articles)
{
    // shortcut: synchronous SQLite writes on main thread; move to a dedicated worker QThread if single-sync writes exceed ~50ms on large feeds
    QSqlDatabase db = getDb();
    if (!db.transaction()) {
        qWarning() << "Failed to start transaction for articles:" << db.lastError().text();
        return;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO articles (id, feed_id, title, author, content, url, published_at, status, starred) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"));

    bool allOk = true;
    for (const auto& article : articles) {
        query.bindValue(0, article.id);
        query.bindValue(1, article.feedId);
        query.bindValue(2, article.title);
        query.bindValue(3, article.author);
        query.bindValue(4, article.content);
        query.bindValue(5, article.url);
        query.bindValue(6, article.publishedAt.toUTC().toString(Qt::ISODate));
        query.bindValue(7, static_cast<int>(article.status));
        query.bindValue(8, article.starred ? 1 : 0);
        if (!query.exec()) {
            qWarning() << "Failed to save article:" << query.lastError().text();
            allOk = false;
            break;
        }
    }

    if (allOk) {
        db.commit();
    } else {
        db.rollback();
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
    
    const QVariant statusVar = query.value(7);
    if (statusVar.typeId() == QMetaType::Int || statusVar.typeId() == QMetaType::LongLong) {
        a.status = static_cast<ArticleStatus>(statusVar.toInt());
    } else {
        const QString statusStr = statusVar.toString().toLower();
        if (statusStr == QLatin1String("read") || statusStr == QLatin1String("1")) a.status = ArticleStatus::Read;
        else if (statusStr == QLatin1String("removed") || statusStr == QLatin1String("2")) a.status = ArticleStatus::Removed;
        else a.status = ArticleStatus::Unread;
    }
    
    a.starred = query.value(8).toBool();
    a.feedTitle = query.value(9).toString();
    a.siteUrl = query.value(10).toString();
    a.feedIconId = query.value(11).toLongLong();
    a.feedHasIcon = query.value(12).toBool();
    
    return a;
}

static void appendArticleFilter(QString& sql, QList<QVariant>& binds, qint64 feedId, qint64 categoryId, ArticleStatus status, bool filterStarred)
{
    if (feedId != -1) {
        sql += QStringLiteral("AND a.feed_id = ? ");
        binds.append(feedId);
    } else if (categoryId != -1) {
        sql += QStringLiteral("AND f.category_id = ? ");
        binds.append(categoryId);
    }
    if (status != ArticleStatus::All) {
        sql += QStringLiteral("AND a.status = ? ");
        binds.append(static_cast<int>(status));
    }
    if (filterStarred) {
        sql += QStringLiteral("AND a.starred = 1 ");
    }
}

QList<Article> DatabaseManager::getArticles(qint64 feedId, qint64 categoryId, ArticleStatus filterStatus, bool filterStarred, int limit, int offset) const
{
    QList<Article> articles;
    QSqlQuery query(getDb());
    
    QString sql = QStringLiteral(
        "SELECT a.id, a.feed_id, a.title, a.author, a.content, a.url, a.published_at, a.status, a.starred, "
        "f.title as feed_title, f.site_url, "
        "COALESCE(f.icon_id, 0) as feed_icon_id, "
        "(f.icon_data IS NOT NULL AND f.icon_data != '') as feed_has_icon "
        "FROM articles a "
        "LEFT JOIN feeds f ON a.feed_id = f.id "
        "WHERE 1=1 ");
    
    QList<QVariant> binds;
    appendArticleFilter(sql, binds, feedId, categoryId, filterStatus, filterStarred);
    sql += QStringLiteral("ORDER BY a.published_at DESC, a.id DESC ");

    if (limit > 0) {
        sql += QStringLiteral("LIMIT ? OFFSET ?");
        binds.append(limit);
        binds.append(offset);
    }

    query.prepare(sql);
    for (const auto& b : binds) {
        query.addBindValue(b);
    }
    if (query.exec()) {
        while (query.next()) {
            articles.append(articleFromQuery(query));
        }
    }
    return articles;
}

int DatabaseManager::countArticles(qint64 feedId, qint64 categoryId, ArticleStatus filterStatus, bool filterStarred) const
{
    QSqlQuery query(getDb());
    QString sql = QStringLiteral(
        "SELECT COUNT(*) FROM articles a "
        "LEFT JOIN feeds f ON a.feed_id = f.id "
        "WHERE 1=1 ");
    QList<QVariant> binds;
    appendArticleFilter(sql, binds, feedId, categoryId, filterStatus, filterStarred);

    query.prepare(sql);
    for (const auto& b : binds) {
        query.addBindValue(b);
    }
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

QList<qint64> DatabaseManager::getUnreadArticleIds(qint64 feedId, qint64 categoryId, bool filterStarred) const
{
    QList<qint64> ids;
    QSqlQuery query(getDb());
    QString sql = QStringLiteral(
        "SELECT a.id FROM articles a "
        "LEFT JOIN feeds f ON a.feed_id = f.id "
        "WHERE 1=1 ");
    QList<QVariant> binds;
    appendArticleFilter(sql, binds, feedId, categoryId, ArticleStatus::Unread, filterStarred);

    query.prepare(sql);
    for (const auto& b : binds) {
        query.addBindValue(b);
    }
    if (query.exec()) {
        while (query.next()) {
            ids.append(query.value(0).toLongLong());
        }
    }
    return ids;
}

std::optional<Article> DatabaseManager::getArticleById(qint64 id) const
{
    QSqlQuery query(getDb());
    query.prepare(QStringLiteral(
        "SELECT a.id, a.feed_id, a.title, a.author, a.content, a.url, a.published_at, a.status, a.starred, "
        "f.title as feed_title, f.site_url, "
        "COALESCE(f.icon_id, 0) as feed_icon_id, "
        "(f.icon_data IS NOT NULL AND f.icon_data != '') as feed_has_icon "
        "FROM articles a "
        "LEFT JOIN feeds f ON a.feed_id = f.id "
        "WHERE a.id = ?"));
    query.addBindValue(id);
    if (query.exec() && query.next()) {
        return articleFromQuery(query);
    }
    return std::nullopt;
}

void DatabaseManager::clearAllData()
{
    QSqlDatabase db = getDb();
    if (!db.transaction()) return;
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("DELETE FROM articles"))) qWarning() << "Clear articles failed:" << query.lastError().text();
    if (!query.exec(QStringLiteral("DELETE FROM feeds"))) qWarning() << "Clear feeds failed:" << query.lastError().text();
    if (!query.exec(QStringLiteral("DELETE FROM categories"))) qWarning() << "Clear categories failed:" << query.lastError().text();
    db.commit();
}

void DatabaseManager::markAsRead(qint64 articleId, bool read)
{
    markArticlesAsRead({articleId}, read);
}

void DatabaseManager::markArticlesAsRead(const QList<qint64>& articleIds, bool read)
{
    if (articleIds.isEmpty()) return;

    QSqlDatabase db = getDb();
    if (!db.transaction()) return;
    
    QSqlQuery query(db);
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

    if (allOk) {
        db.commit();
        emit unreadCountsChanged();
    } else {
        db.rollback();
    }
}

void DatabaseManager::toggleStar(qint64 articleId, bool starred)
{
    QSqlQuery query(getDb());
    query.prepare(QStringLiteral("UPDATE articles SET starred = ? WHERE id = ?"));
    query.addBindValue(starred);
    query.addBindValue(articleId);
    if (!query.exec()) {
        qCWarning(KIRIFLUX_DB) << "Failed to toggle star:" << query.lastError().text();
    }
}

void DatabaseManager::updateArticleContent(qint64 articleId, const QString& content)
{
    QSqlQuery query(getDb());
    query.prepare(QStringLiteral("UPDATE articles SET content = ? WHERE id = ?"));
    query.addBindValue(content);
    query.addBindValue(articleId);
    if (!query.exec()) {
        qCWarning(KIRIFLUX_DB) << "Failed to update content:" << query.lastError().text();
    }
}

void DatabaseManager::purgeOldReadArticles(int days)
{
    QDateTime cutoff = QDateTime::currentDateTimeUtc().addDays(-days);
    
    QSqlQuery query(getDb());
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

void DatabaseManager::vacuum()
{
    QSqlQuery query(getDb());
    // VACUUM cannot run within a transaction
    if (!query.exec(QStringLiteral("VACUUM"))) {
        qCWarning(KIRIFLUX_DB) << "Failed to vacuum database:" << query.lastError().text();
    } else {
        qCDebug(KIRIFLUX_DB) << "Database vacuum successful.";
    }
}

void DatabaseManager::optimizeDatabase()
{
    // Used by UI to manually trigger an optimization
    purgeOldReadArticles(Settings::self()->pruneAfterDays());
    vacuum();
    Settings::self()->setLastVacuumDate(QDateTime::currentDateTime());
    Settings::self()->save();
}
