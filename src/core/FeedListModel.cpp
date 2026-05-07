// SPDX-License-Identifier: GPL-3.0-or-later
#include "FeedListModel.h"

#include "SyncEngine.h"

FeedListModel::FeedListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&SyncEngine::instance(), &SyncEngine::articlesUpdated, this, &FeedListModel::refresh);
    
    m_unreadUpdateTimer.setInterval(50);
    m_unreadUpdateTimer.setSingleShot(true);
    connect(&DatabaseManager::instance(), &DatabaseManager::unreadCountsChanged, &m_unreadUpdateTimer, static_cast<void(QTimer::*)()>(&QTimer::start));
    connect(&m_unreadUpdateTimer, &QTimer::timeout, this, &FeedListModel::updateUnreadCounts);
    
    refresh();
}

void FeedListModel::refresh()
{
    beginResetModel();
    m_feeds = DatabaseManager::instance().getAllFeeds();
    endResetModel();
}

void FeedListModel::updateUnreadCounts()
{
    auto newFeeds = DatabaseManager::instance().getAllFeeds();
    for (int i = 0; i < m_feeds.size(); ++i) {
        for (const auto& newFeed : newFeeds) {
            if (m_feeds[i].id == newFeed.id) {
                if (m_feeds[i].unreadCount != newFeed.unreadCount) {
                    m_feeds[i].unreadCount = newFeed.unreadCount;
                    QModelIndex qidx = index(i, 0);
                    emit dataChanged(qidx, qidx, {UnreadCountRole});
                }
                break;
            }
        }
    }
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
    case IdRole: return qint64(feed.id);
    case TitleRole: return feed.title;
    case SiteUrlRole: return feed.siteUrl;
    case FeedUrlRole: return feed.feedUrl;
    case CategoryIdRole: return qint64(feed.categoryId);
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
