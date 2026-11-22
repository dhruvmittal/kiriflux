// SPDX-License-Identifier: GPL-3.0-or-later
#include "CategoryListModel.h"
#include "SyncEngine.h"
#include <QHash>

CategoryListModel::CategoryListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&SyncEngine::instance(), &SyncEngine::articlesUpdated, this, &CategoryListModel::refresh);
    
    m_unreadUpdateTimer.setInterval(50);
    m_unreadUpdateTimer.setSingleShot(true);
    connect(&DatabaseManager::instance(), &DatabaseManager::unreadCountsChanged, &m_unreadUpdateTimer, qOverload<>(&QTimer::start));
    connect(&m_unreadUpdateTimer, &QTimer::timeout, this, &CategoryListModel::updateUnreadCounts);
    
    refresh();
}

void CategoryListModel::updateUnreadCounts()
{
    const auto newCategories = DatabaseManager::instance().getAllCategories();
    QHash<qint64, int> unreadById;
    unreadById.reserve(newCategories.size());
    for (const auto& newCategory : newCategories) {
        unreadById.insert(newCategory.id, newCategory.unreadCount);
    }

    for (int i = 0; i < m_categories.size(); ++i) {
        auto it = unreadById.constFind(m_categories[i].id);
        if (it != unreadById.constEnd() && m_categories[i].unreadCount != it.value()) {
            m_categories[i].unreadCount = it.value();
            const QModelIndex qidx = index(i, 0);
            emit dataChanged(qidx, qidx, {UnreadCountRole});
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
    static const QHash<int, QByteArray> roles = {
        {IdRole, "id"},
        {TitleRole, "title"},
        {UnreadCountRole, "unreadCount"}
    };
    return roles;
}

void CategoryListModel::refresh()
{
    beginResetModel();
    m_categories = DatabaseManager::instance().getAllCategories();
    endResetModel();
}

qint64 CategoryListModel::adjacentCategoryId(qint64 currentCategoryId, int delta) const
{
    if (m_categories.isEmpty()) return -1;
    if (currentCategoryId == -1) {
        return (delta > 0) ? m_categories.first().id : m_categories.last().id;
    }

    for (int i = 0; i < m_categories.size(); ++i) {
        if (m_categories[i].id == currentCategoryId) {
            const int target = i + delta;
            if (target >= 0 && target < m_categories.size()) {
                return m_categories[target].id;
            }
            return -1;
        }
    }
    return -1;
}

qint64 CategoryListModel::getNextCategoryId(qint64 currentCategoryId) const
{
    return adjacentCategoryId(currentCategoryId, 1);
}

qint64 CategoryListModel::getPreviousCategoryId(qint64 currentCategoryId) const
{
    return adjacentCategoryId(currentCategoryId, -1);
}
