// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>
#include "DatabaseManager.h"

class ArticleListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(long currentFeedId READ currentFeedId WRITE setCurrentFeedId NOTIFY currentFeedIdChanged)
    Q_PROPERTY(QString filterStatus READ filterStatus WRITE setFilterStatus NOTIFY filterChanged)
    Q_PROPERTY(bool filterStarred READ filterStarred WRITE setFilterStarred NOTIFY filterChanged)

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
    void setCurrentFeedId(long id);

    QString filterStatus() const { return m_filterStatus; }
    void setFilterStatus(const QString &status);

    bool filterStarred() const { return m_filterStarred; }
    void setFilterStarred(bool starred);

public slots:
    void refresh();
    void markAsRead(int index, bool read);
    void markAllAsRead();
    void toggleStarred(int index);

signals:
    void currentFeedIdChanged();
    void filterChanged();

private:
    QList<Article> m_articles;
    long m_currentFeedId = -1; // -1 for "All Articles"
    QString m_filterStatus = "all"; // "all" or "unread"
    bool m_filterStarred = false;
};
