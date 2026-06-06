// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QVariantList>
#include <QMutex>
#include <QRecursiveMutex>
#include <QDateTime>

struct Feed {
    qint64 id;
    QString title;
    QString siteUrl;
    QString feedUrl;
    qint64 categoryId;
    int unreadCount;
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

    bool operator==(const Article& other) const {
        return id == other.id &&
               feedId == other.feedId &&
               title == other.title &&
               author == other.author &&
               content == other.content &&
               url == other.url &&
               publishedAt == other.publishedAt &&
               status == other.status &&
               starred == other.starred &&
               feedTitle == other.feedTitle &&
               siteUrl == other.siteUrl;
    }
    bool operator!=(const Article& other) const {
        return !(*this == other);
    }
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

    // Categories
    void saveCategories(const QList<Category>& categories);
    QList<Category> getAllCategories() const;
    QString getCategoryTitle(qint64 categoryId) const;

    // Articles
    void saveArticles(const QList<Article>& articles);
    QList<Article> getArticlesForFeed(qint64 feedId, ArticleStatus status = ArticleStatus::Unread, bool filterStarred = false) const;
    QList<Article> getArticlesForCategory(qint64 categoryId, ArticleStatus status = ArticleStatus::Unread, bool filterStarred = false) const;
    Q_INVOKABLE void clearAllData();
    QList<Article> getAllArticles(ArticleStatus status = ArticleStatus::Unread, bool filterStarred = false) const;
    std::optional<Article> getArticleById(qint64 id) const;
    
    void markAsRead(qint64 articleId, bool read);
    void markArticlesAsRead(const QList<qint64>& articleIds, bool read);
    void toggleStar(qint64 articleId, bool starred);
    void updateArticleStatus(qint64 articleId, ArticleStatus status);
    void updateArticleContent(qint64 articleId, const QString& content);
    void purgeOldReadArticles(int days = 7);

signals:
    void unreadCountsChanged();

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager() = default;
    
    QSqlDatabase getDb() const;

    mutable QRecursiveMutex m_mutex;
    QString m_dbPath;
};
