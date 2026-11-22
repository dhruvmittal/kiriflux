// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QDateTime>
#include <optional>

struct Feed {
    qint64 id;
    QString title;
    QString siteUrl;
    QString feedUrl;
    qint64 categoryId;
    int unreadCount = 0;
    qint64 iconId = 0;
    bool hasIcon = false;
};

struct Category {
    qint64 id;
    QString title;
    int unreadCount;
};

enum class ArticleStatus {
    Unread = 0,
    Read = 1,
    Removed = 2,
    All = 3
};

struct Article {
    qint64 id;
    qint64 feedId;
    QString title;
    QString author;
    QString content;
    QString url;
    QDateTime publishedAt;
    ArticleStatus status = ArticleStatus::Unread;
    bool starred = false;
    
    // UI fields
    QString feedTitle;
    QString siteUrl;
    qint64 feedIconId = 0;
    bool feedHasIcon = false;

    bool operator==(const Article& other) const = default;
};

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    static DatabaseManager& instance();

    bool init();
    bool initWithPath(const QString &dbPath);
    
    // Feeds
    void saveFeeds(const QList<Feed>& feeds);
    QList<Feed> getAllFeeds() const;
    QString getFeedTitle(qint64 feedId) const;
    QList<qint64> getFeedIdsMissingIcons() const;
    void saveFeedIcon(qint64 feedId, const QString& iconData);
    QString getFeedIconData(qint64 feedId) const;

    // Categories
    void saveCategories(const QList<Category>& categories);
    QList<Category> getAllCategories() const;
    QString getCategoryTitle(qint64 categoryId) const;

    // Articles
    void saveArticles(const QList<Article>& articles);
    QList<Article> getArticles(qint64 feedId = -1, qint64 categoryId = -1, ArticleStatus status = ArticleStatus::Unread, bool filterStarred = false, int limit = -1, int offset = 0) const;
    int countArticles(qint64 feedId = -1, qint64 categoryId = -1, ArticleStatus status = ArticleStatus::Unread, bool filterStarred = false) const;
    QList<qint64> getUnreadArticleIds(qint64 feedId = -1, qint64 categoryId = -1, bool filterStarred = false) const;
    Q_INVOKABLE void clearAllData();
    std::optional<Article> getArticleById(qint64 id) const;
    
    void markAsRead(qint64 articleId, bool read);
    void markArticlesAsRead(const QList<qint64>& articleIds, bool read);
    void toggleStar(qint64 articleId, bool starred);
    void updateArticleContent(qint64 articleId, const QString& content);
    void purgeOldReadArticles(int days = 7);
    void vacuum();
    Q_INVOKABLE void optimizeDatabase();

signals:
    void unreadCountsChanged();

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager() override;
    
    QSqlDatabase getDb() const;
    void closeDb();

    QString m_dbPath;
};
