// SPDX-License-Identifier: GPL-3.0-or-later
#include "CategoryListModel.h"

CategoryListModel::CategoryListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    refresh();
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
