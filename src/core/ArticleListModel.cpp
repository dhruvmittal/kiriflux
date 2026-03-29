// SPDX-License-Identifier: GPL-3.0-or-later
#include "ArticleListModel.h"
#include "SyncEngine.h"

ArticleListModel::ArticleListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    refresh();
}

void ArticleListModel::setCurrentFeedId(long id)
{
    if (m_currentFeedId == id) return;
    m_currentFeedId = id;
    m_currentCategoryId = -1;
    refresh();
    emit currentFeedIdChanged();
    emit currentCategoryIdChanged();
}

void ArticleListModel::setCurrentCategoryId(long id)
{
    if (m_currentCategoryId == id) return;
    m_currentCategoryId = id;
    m_currentFeedId = -1;
    refresh();
    emit currentCategoryIdChanged();
    emit currentFeedIdChanged();
}

void ArticleListModel::markAsRead(int index, bool read)
{
    if (index < 0 || index >= m_articles.count()) return;
    
    long id = m_articles.at(index).id;
    DatabaseManager::instance().markAsRead(id, read);
    
    QString statusString = read ? QStringLiteral("read") : QStringLiteral("unread");
    SyncEngine::instance().updateArticleStatus(id, statusString);
    
    // Update local data and notify view
    m_articles[index].status = statusString;
    QModelIndex qidx = this->index(index, 0);
    emit dataChanged(qidx, qidx, {StatusRole});
}

void ArticleListModel::setFilterStatus(const QString &status)
{
    if (m_filterStatus == status) return;
    m_filterStatus = status;
    refresh();
    emit filterChanged();
}

void ArticleListModel::setFilterStarred(bool starred)
{
    if (m_filterStarred == starred) return;
    m_filterStarred = starred;
    refresh();
    emit filterChanged();
}

void ArticleListModel::markAllAsRead()
{
    for (int i = 0; i < m_articles.count(); ++i) {
        if (m_articles.at(i).status == QStringLiteral("unread")) {
            markAsRead(i, true);
        }
    }
}

void ArticleListModel::toggleStarred(int index)
{
    if (index < 0 || index >= m_articles.count()) return;
    
    Article article = m_articles.at(index);
    article.starred = !article.starred;
    DatabaseManager::instance().toggleStar(article.id, article.starred);
    
    SyncEngine::instance().toggleArticleBookmark(article.id);
    
    m_articles[index] = article;
    QModelIndex qidx = this->index(index, 0);
    emit dataChanged(qidx, qidx, {StarredRole});
}

void ArticleListModel::refresh()
{
    beginResetModel();
    if (m_currentFeedId != -1) {
        m_articles = DatabaseManager::instance().getArticlesForFeed(m_currentFeedId);
    } else if (m_currentCategoryId != -1) {
        m_articles = DatabaseManager::instance().getArticlesForCategory(m_currentCategoryId);
    } else {
        m_articles = DatabaseManager::instance().getAllArticles();
    }

    // Apply client-side filters (for now, eventually move to DB queries if performance allows)
    auto it = std::remove_if(m_articles.begin(), m_articles.end(), [this](const Article& a) {
        if (m_filterStatus == QStringLiteral("unread") && a.status != QStringLiteral("unread")) {
            return true;
        }
        if (m_filterStarred && !a.starred) {
            return true;
        }
        return false;
    });
    m_articles.erase(it, m_articles.end());

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
    case FeedTitleRole: return article.feedTitle;
    case FeedSiteUrlRole: return article.feedSiteUrl;
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
    roles[FeedTitleRole] = "feedTitle";
    roles[FeedSiteUrlRole] = "feedSiteUrl";
    return roles;
}
QVariantMap ArticleListModel::get(int row) const
{
    if (row < 0 || row >= m_articles.count()) return {};

    QVariantMap res;
    QHash<int, QByteArray> roles = roleNames();
    QHashIterator<int, QByteArray> i(roles);
    while (i.hasNext()) {
        i.next();
        res[QString::fromUtf8(i.value())] = data(index(row, 0), i.key());
    }
    return res;
}
