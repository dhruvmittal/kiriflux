// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QAbstractListModel>
#include <QTimer>
#include "DatabaseManager.h"

class CategoryListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        UnreadCountRole
    };

    explicit CategoryListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void refresh();
    void updateUnreadCounts();

    Q_INVOKABLE qint64 getNextCategoryId(qint64 currentCategoryId) const;
    Q_INVOKABLE qint64 getPreviousCategoryId(qint64 currentCategoryId) const;

private:
    qint64 adjacentCategoryId(qint64 currentCategoryId, int delta) const;

    QList<Category> m_categories;
    QTimer m_unreadUpdateTimer;
};
