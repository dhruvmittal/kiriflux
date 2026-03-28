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
    
    // Feeds
    void saveFeeds(const QList<Feed>& feeds);
    QList<Feed> getAllFeeds() const;

    // Articles
    void saveArticles(const QList<Article>& articles);
    QList<Article> getArticlesForFeed(long feedId) const;
    QList<Article> getAllArticles() const;
    
    void markAsRead(long articleId, bool read);
    void toggleStar(long articleId, bool starred);
    void updateArticleContent(long articleId, const QString& content);

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager() = default;
    
    QSqlDatabase m_db;
};
