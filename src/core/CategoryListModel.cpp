// SPDX-License-Identifier: GPL-3.0-or-later
#include "CategoryListModel.h"

#include "SyncEngine.h"

CategoryListModel::CategoryListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&SyncEngine::instance(), &SyncEngine::articlesUpdated, this, &CategoryListModel::refresh);
    
    m_unreadUpdateTimer.setInterval(50);
    m_unreadUpdateTimer.setSingleShot(true);
    connect(&DatabaseManager::instance(), &DatabaseManager::unreadCountsChanged, &m_unreadUpdateTimer, static_cast<void(QTimer::*)()>(&QTimer::start));
    connect(&m_unreadUpdateTimer, &QTimer::timeout, this, &CategoryListModel::updateUnreadCounts);
    
    refresh();
}

void CategoryListModel::updateUnreadCounts()
{
    auto newCategories = DatabaseManager::instance().getAllCategories();
    for (int i = 0; i < m_categories.size(); ++i) {
        for (const auto& newCategory : newCategories) {
            if (m_categories[i].id == newCategory.id) {
                if (m_categories[i].unreadCount != newCategory.unreadCount) {
                    m_categories[i].unreadCount = newCategory.unreadCount;
                    QModelIndex qidx = index(i, 0);
                    emit dataChanged(qidx, qidx, {UnreadCountRole});
                }
                break;
            }
        }
    }
}

int CategoryListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_categories.count();
}

QVariant CategoryListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_categories.count())
        return QVariant();

    const auto &category = m_categories.at(index.row());

    switch (role) {
    case IdRole: return qlonglong(category.id);
    case TitleRole: return category.title;
    case UnreadCountRole: return category.unreadCount;
    }

    return QVariant();
}

QHash<int, QByteArray> CategoryListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[TitleRole] = "title";
    roles[UnreadCountRole] = "unreadCount";
    return roles;
}

void CategoryListModel::refresh()
{
    beginResetModel();
    m_categories = DatabaseManager::instance().getAllCategories();
    endResetModel();
}
