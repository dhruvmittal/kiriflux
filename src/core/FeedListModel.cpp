// SPDX-License-Identifier: GPL-3.0-or-later
#include "FeedListModel.h"
#include "SyncEngine.h"
#include <QHash>

FeedListModel::FeedListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&SyncEngine::instance(), &SyncEngine::articlesUpdated, this, &FeedListModel::refresh);
    connect(&SyncEngine::instance(), &SyncEngine::feedIconUpdated, this, &FeedListModel::onFeedIconUpdated);
    
    m_unreadUpdateTimer.setInterval(50);
    m_unreadUpdateTimer.setSingleShot(true);
    connect(&DatabaseManager::instance(), &DatabaseManager::unreadCountsChanged, &m_unreadUpdateTimer, qOverload<>(&QTimer::start));
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
    const auto newFeeds = DatabaseManager::instance().getAllFeeds();
    QHash<qint64, int> unreadById;
    unreadById.reserve(newFeeds.size());
    for (const auto& newFeed : newFeeds) {
        unreadById.insert(newFeed.id, newFeed.unreadCount);
    }

    for (int i = 0; i < m_feeds.size(); ++i) {
        auto it = unreadById.constFind(m_feeds[i].id);
        if (it != unreadById.constEnd() && m_feeds[i].unreadCount != it.value()) {
            m_feeds[i].unreadCount = it.value();
            const QModelIndex qidx = index(i, 0);
            emit dataChanged(qidx, qidx, {UnreadCountRole});
        }
    }
}

void FeedListModel::onFeedIconUpdated(qint64 feedId)
{
    for (int i = 0; i < m_feeds.size(); ++i) {
        if (m_feeds[i].id == feedId && !m_feeds[i].hasIcon) {
            m_feeds[i].hasIcon = true;
            const QModelIndex qidx = index(i, 0);
            emit dataChanged(qidx, qidx, {HasIconRole});
            return;
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
    case IconIdRole: return qlonglong(feed.iconId);
    case HasIconRole: return feed.hasIcon;
    case Qt::DisplayRole: return feed.title;
    }
    return {};
}

QHash<int, QByteArray> FeedListModel::roleNames() const
{
    static const QHash<int, QByteArray> roles = {
        {IdRole, "id"},
        {TitleRole, "title"},
        {SiteUrlRole, "siteUrl"},
        {FeedUrlRole, "feedUrl"},
        {CategoryIdRole, "categoryId"},
        {UnreadCountRole, "unreadCount"},
        {IconIdRole, "iconId"},
        {HasIconRole, "hasIcon"}
    };
    return roles;
}
