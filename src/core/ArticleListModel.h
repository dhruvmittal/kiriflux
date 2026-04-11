// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>
#include "DatabaseManager.h"

class ArticleListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(qint64 currentFeedId READ currentFeedId WRITE setCurrentFeedId NOTIFY currentFeedIdChanged)
    Q_PROPERTY(qint64 currentCategoryId READ currentCategoryId WRITE setCurrentCategoryId NOTIFY currentCategoryIdChanged)
    Q_PROPERTY(QString filterStatus READ filterStatus WRITE setFilterStatus NOTIFY filterChanged)
    Q_PROPERTY(bool filterStarred READ filterStarred WRITE setFilterStarred NOTIFY filterChanged)
    Q_PROPERTY(QString currentName READ currentName NOTIFY currentNameChanged)

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
        FeedSiteUrlRole
    };
    Q_ENUM(Roles)

    explicit ArticleListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    long currentFeedId() const { return m_currentFeedId; }
    void setCurrentFeedId(qint64 id);

    long currentCategoryId() const { return m_currentCategoryId; }
    void setCurrentCategoryId(qint64 id);

    QString filterStatus() const { return m_filterStatus; }
    void setFilterStatus(const QString &status);

    bool filterStarred() const { return m_filterStarred; }
    void setFilterStarred(bool starred);

    QString currentName() const;

public slots:
    void refresh();
    void onArticleContentUpdated(qint64 id);
    void markAsRead(int index, bool read);
    void markAsReadById(qint64 id, bool read);
    void markAllAsRead();
    void toggleStarred(int index);
    void toggleStarredById(qint64 id);
    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE QVariantMap getById(qint64 id) const;

signals:
    void currentFeedIdChanged();
    void currentCategoryIdChanged();
    void filterChanged();
    void currentNameChanged();

private:
    QList<Article> m_articles;
    qint64 m_currentFeedId = -1; // -1 for "All Articles"
    qint64 m_currentCategoryId = -1;
    QString m_filterStatus = "all"; // "all" or "unread"
    bool m_filterStarred = false;
};
