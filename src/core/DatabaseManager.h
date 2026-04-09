// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QVariantList>

struct Feed {
    long id;
    QString title;
    QString siteUrl;
    QString feedUrl;
    long categoryId;
    int unreadCount;
};

struct Category {
    long id;
    QString title;
    int unreadCount;
};

struct Article {
    long id;
    long feedId;
    QString title;
    QString author;
    QString content;
    QString url;
    QString publishedAt;
    QString status;
    bool starred;
    QString feedTitle;
    QString feedSiteUrl;
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
    QString getFeedTitle(long feedId) const;

    // Categories
    void saveCategories(const QList<Category>& categories);
    QList<Category> getAllCategories() const;
    QString getCategoryTitle(long categoryId) const;

    // Articles
    void saveArticles(const QList<Article>& articles);
    QList<Article> getArticlesForFeed(long feedId) const;
    QList<Article> getArticlesForCategory(long categoryId) const;
    Q_INVOKABLE void clearAllData();
    QList<Article> getAllArticles() const;
    std::optional<Article> getArticleById(long id) const;
    
    void markAsRead(long articleId, bool read);
    void toggleStar(long articleId, bool starred);
    void updateArticleContent(long articleId, const QString& content);

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager() = default;
    
    QSqlDatabase m_db;
};
