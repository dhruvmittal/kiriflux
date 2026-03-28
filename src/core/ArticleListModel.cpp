// SPDX-License-Identifier: GPL-3.0-or-later
#include "ArticleListModel.h"

ArticleListModel::ArticleListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    refresh();
}

void ArticleListModel::setCurrentFeedId(long id)
{
    if (m_currentFeedId == id) return;
    m_currentFeedId = id;
    refresh();
    emit currentFeedIdChanged();
}

void ArticleListModel::markAsRead(int index, bool read)
{
    if (index < 0 || index >= m_articles.count()) return;
    
    long id = m_articles.at(index).id;
    DatabaseManager::instance().markAsRead(id, read);
    
    // Update local data and notify view
    m_articles[index].status = read ? QStringLiteral("read") : QStringLiteral("unread");
    QModelIndex qidx = this->index(index, 0);
    emit dataChanged(qidx, qidx, {StatusRole});
}

void ArticleListModel::refresh()
{
    beginResetModel();
    if (m_currentFeedId == -1) {
        m_articles = DatabaseManager::instance().getAllArticles();
    } else {
        m_articles = DatabaseManager::instance().getArticlesForFeed(m_currentFeedId);
    }
    endResetModel();
}

int ArticleListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_articles.count();
}

QVariant ArticleListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_articles.count()) return {};

    const auto &article = m_articles.at(index.row());
    switch (role) {
    case IdRole: return qlonglong(article.id);
    case FeedIdRole: return qlonglong(article.feedId);
    case TitleRole: return article.title;
    case AuthorRole: return article.author;
    case ContentRole: return article.content;
    case UrlRole: return article.url;
    case PublishedAtRole: return article.publishedAt;
    case StatusRole: return article.status;
    case StarredRole: return article.starred;
    case Qt::DisplayRole: return article.title;
    }
    return {};
}

QHash<int, QByteArray> ArticleListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[FeedIdRole] = "feedId";
    roles[TitleRole] = "title";
    roles[AuthorRole] = "author";
    roles[ContentRole] = "content";
    roles[UrlRole] = "url";
    roles[PublishedAtRole] = "publishedAt";
    roles[StatusRole] = "status";
    roles[StarredRole] = "starred";
    return roles;
}
