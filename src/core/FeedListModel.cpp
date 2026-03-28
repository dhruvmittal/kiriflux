// SPDX-License-Identifier: GPL-3.0-or-later
#include "FeedListModel.h"

FeedListModel::FeedListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    refresh();
}

void FeedListModel::refresh()
{
    beginResetModel();
    m_feeds = DatabaseManager::instance().getAllFeeds();
    endResetModel();
}

int FeedListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_feeds.count();
}

QVariant FeedListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_feeds.count()) return {};

    const auto &feed = m_feeds.at(index.row());
    switch (role) {
    case IdRole: return qlonglong(feed.id);
    case TitleRole: return feed.title;
    case SiteUrlRole: return feed.siteUrl;
    case FeedUrlRole: return feed.feedUrl;
    case CategoryIdRole: return qlonglong(feed.categoryId);
    case UnreadCountRole: return feed.unreadCount;
    case Qt::DisplayRole: return feed.title;
    }
    return {};
}

QHash<int, QByteArray> FeedListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[TitleRole] = "title";
    roles[SiteUrlRole] = "siteUrl";
    roles[FeedUrlRole] = "feedUrl";
    roles[CategoryIdRole] = "categoryId";
    roles[UnreadCountRole] = "unreadCount";
    return roles;
}
