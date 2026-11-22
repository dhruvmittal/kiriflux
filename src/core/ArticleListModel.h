// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QAbstractListModel>
#include "DatabaseManager.h"

class ArticleListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(qint64 currentFeedId READ currentFeedId WRITE setCurrentFeedId NOTIFY currentFeedIdChanged)
    Q_PROPERTY(qint64 currentCategoryId READ currentCategoryId WRITE setCurrentCategoryId NOTIFY currentCategoryIdChanged)
    Q_PROPERTY(QString filterMode READ filterMode WRITE setFilterMode NOTIFY filterChanged)
    Q_PROPERTY(QString currentName READ currentName NOTIFY currentNameChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        FeedIdRole,
        TitleRole,
        AuthorRole,
        ContentRole,
        UrlRole,
        PublishedAtRole,
        StatusRole,
        StarredRole,
        FeedTitleRole,
        FeedSiteUrlRole,
        FeedIconIdRole,
        FeedHasIconRole
    };
    Q_ENUM(Roles)

    explicit ArticleListModel(QObject *parent = nullptr);

    Q_INVOKABLE int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

    qint64 currentFeedId() const { return m_currentFeedId; }
    void setCurrentFeedId(qint64 id);

    qint64 currentCategoryId() const { return m_currentCategoryId; }
    void setCurrentCategoryId(qint64 id);

    QString filterMode() const { return m_filterMode; }
    void setFilterMode(const QString &mode);

    QString currentName() const;

public slots:
    void refresh(bool forceReset = false);
    void onArticleContentUpdated(qint64 id);
    void onFeedIconUpdated(qint64 feedId);
    void markAsRead(int index, bool read);
    void markAsReadById(qint64 id, bool read);
    void markArticlesAsReadByIds(const QList<qint64>& ids, bool read = true);
    void markReadUpTo(int endIndex);
    void markAllAsRead();
    void toggleStarred(int index);
    void toggleStarredById(qint64 id);
    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE QVariantMap getById(qint64 id) const;
    Q_INVOKABLE qint64 idAt(int row) const;

signals:
    void currentFeedIdChanged();
    void currentCategoryIdChanged();
    void filterChanged();
    void currentNameChanged();
    void countChanged();

private:
    QList<Article> m_articles;
    qint64 m_currentFeedId = -1; // -1 for "All Articles"
    qint64 m_currentCategoryId = -1;
    QString m_filterMode = QStringLiteral("all"); // "all", "unread", or "starred"
    int m_totalAvailableCount = 0;
};
