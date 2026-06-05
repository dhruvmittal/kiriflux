// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>
#include <QTimer>
#include "DatabaseManager.h"

class FeedListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        SiteUrlRole,
        FeedUrlRole,
        CategoryIdRole,
        UnreadCountRole
    };

    explicit FeedListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void refresh();
    void updateUnreadCounts();

    Q_INVOKABLE int getNextFeedId(int currentFeedId) const;
    Q_INVOKABLE int getPreviousFeedId(int currentFeedId) const;

private:
    QList<Feed> m_feeds;
    QTimer m_unreadUpdateTimer;
};
