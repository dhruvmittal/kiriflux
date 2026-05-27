// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QQmlContext>
#include <QQmlApplicationEngine>

class FeedListModel;
class ArticleListModel;
class CategoryListModel;

class AppController : public QObject
{
    Q_OBJECT
public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController() override;

    bool init();
    void registerContextProperties(QQmlApplicationEngine* engine);

    FeedListModel* feedListModel() const { return m_feedListModel; }
    ArticleListModel* articleListModel() const { return m_articleListModel; }
    CategoryListModel* categoryListModel() const { return m_categoryListModel; }

private:
    FeedListModel *m_feedListModel{nullptr};
    ArticleListModel *m_articleListModel{nullptr};
    CategoryListModel *m_categoryListModel{nullptr};
};
