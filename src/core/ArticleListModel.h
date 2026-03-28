// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QAbstractListModel>
#include "DatabaseManager.h"

class ArticleListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(long currentFeedId READ currentFeedId WRITE setCurrentFeedId NOTIFY currentFeedIdChanged)

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
        StarredRole
    };

    explicit ArticleListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    long currentFeedId() const { return m_currentFeedId; }
    void setCurrentFeedId(long id);

public slots:
    void refresh();
    void markAsRead(int index, bool read);

signals:
    void currentFeedIdChanged();

private:
    QList<Article> m_articles;
    long m_currentFeedId = -1; // -1 for "All Articles"
};
