// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>
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

    Q_INVOKABLE int getNextCategoryId(int currentCategoryId) const;
    Q_INVOKABLE int getPreviousCategoryId(int currentCategoryId) const;

private:
    QList<Category> m_categories;
    QTimer m_unreadUpdateTimer;
};
