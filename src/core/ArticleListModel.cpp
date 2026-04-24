// SPDX-License-Identifier: GPL-3.0-or-later
#include "ArticleListModel.h"
#include "SyncEngine.h"
#include "kiriflux.h"
#include <KLocalizedString>

ArticleListModel::ArticleListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_filterStatus = Settings::self()->filterStatus();
    m_filterStarred = Settings::self()->filterStarred();
    
    connect(&SyncEngine::instance(), &SyncEngine::articlesUpdated, this, &ArticleListModel::refresh);
    connect(&SyncEngine::instance(), &SyncEngine::articleContentUpdated, this, &ArticleListModel::onArticleContentUpdated);

    refresh();
}

void ArticleListModel::setCurrentFeedId(qint64 id)
{
    if (m_currentFeedId == id) return;
    m_currentFeedId = id;
    m_currentCategoryId = -1;
    refresh();
    emit currentFeedIdChanged();
    emit currentCategoryIdChanged();
    emit currentNameChanged();
}

void ArticleListModel::setCurrentCategoryId(qint64 id)
{
    if (m_currentCategoryId == id) return;
    m_currentCategoryId = id;
    m_currentFeedId = -1;
    refresh();
    emit currentCategoryIdChanged();
    emit currentFeedIdChanged();
    emit currentNameChanged();
}

void ArticleListModel::markAsRead(int index, bool read)
{
    if (index < 0 || index >= m_articles.count()) return;
    markAsReadById(m_articles.at(index).id, read);
}

void ArticleListModel::markAsReadById(qint64 id, bool read)
{
    qDebug() << "ArticleListModel::markAsReadById" << id << read;
    DatabaseManager::instance().markAsRead(id, read);
    
    SyncEngine::instance().enqueueMarkAsRead(id);
    
    // Find and update local list for immediate UI feedback
    for (int i = 0; i < m_articles.count(); ++i) {
        if (m_articles.at(i).id == id) {
            m_articles[i].status = read ? ArticleStatus::Read : ArticleStatus::Unread;
            QModelIndex qidx = this->index(i, 0);
            emit dataChanged(qidx, qidx, {StatusRole});
            break;
        }
    }
}

void ArticleListModel::setFilterStatus(const QString &status)
{
    if (m_filterStatus == status) return;
    m_filterStatus = status;
    Settings::setFilterStatus(status);
    Settings::self()->save();
    refresh();
    emit filterChanged();
}

QString ArticleListModel::currentName() const
{
    if (m_currentFeedId != -1) {
        return DatabaseManager::instance().getFeedTitle(m_currentFeedId);
    } else if (m_currentCategoryId != -1) {
        return DatabaseManager::instance().getCategoryTitle(m_currentCategoryId);
    }
    return i18n("All Articles");
}

void ArticleListModel::setFilterStarred(bool starred)
{
    if (m_filterStarred == starred) return;
    m_filterStarred = starred;
    Settings::setFilterStarred(starred);
    Settings::self()->save();
    refresh();
    emit filterChanged();
}

void ArticleListModel::markAllAsRead()
{
    QList<qint64> unreadIds;
    for (const auto& a : m_articles) {
        if (a.status == ArticleStatus::Unread) {
            unreadIds.append(a.id);
        }
    }

    if (unreadIds.isEmpty()) return;

    DatabaseManager::instance().markArticlesAsRead(unreadIds, true);
    SyncEngine::instance().updateArticlesStatus(unreadIds, QStringLiteral("read"));

    // Update local state and notify UI
    for (int i = 0; i < m_articles.count(); ++i) {
        if (m_articles[i].status == ArticleStatus::Unread) {
            m_articles[i].status = ArticleStatus::Read;
            QModelIndex qidx = index(i, 0);
            emit dataChanged(qidx, qidx, {StatusRole});
        }
    }
}

void ArticleListModel::toggleStarred(int index)
{
    if (index < 0 || index >= m_articles.count()) return;
    toggleStarredById(m_articles.at(index).id);
}

void ArticleListModel::toggleStarredById(qint64 id)
{
    // Find current state
    bool currentStarred = false;
    bool foundInModel = false;
    int modelIndex = -1;
    
    for (int i = 0; i < m_articles.count(); ++i) {
        if (m_articles.at(i).id == id) {
            currentStarred = m_articles.at(i).starred;
            modelIndex = i;
            foundInModel = true;
            break;
        }
    }
    
    if (!foundInModel) {
        // Must check DB if not in current view
        auto article = DatabaseManager::instance().getArticleById(id);
        if (article) {
            currentStarred = article->starred;
        } else {
            return; // Not found in DB either
        }
    }
    
    bool nextStarred = !currentStarred;
    DatabaseManager::instance().toggleStar(id, nextStarred);
    SyncEngine::instance().toggleArticleBookmark(id);
    
    if (foundInModel) {
        m_articles[modelIndex].starred = nextStarred;
        QModelIndex qidx = index(modelIndex, 0);
        emit dataChanged(qidx, qidx, {StarredRole});
    }
}

void ArticleListModel::onArticleContentUpdated(qint64 id)
{
    // Find the item in our list and update its content from DB
    for (int i = 0; i < m_articles.count(); ++i) {
        if (m_articles.at(i).id == id) {
            // Re-fetch this article from DB to get NEW content
            auto article = DatabaseManager::instance().getArticleById(id);
            if (article) {
                m_articles[i] = *article;
                QModelIndex qidx = index(i, 0);
                // Emit dataChanged for all relevant roles (content, etc.)
                emit dataChanged(qidx, qidx); 
            }
            return;
        }
    }
}

void ArticleListModel::refresh()
{
    beginResetModel();
    ArticleStatus status = ArticleStatus::All;
    if (m_filterStatus == "unread") status = ArticleStatus::Unread;
    else if (m_filterStatus == "read") status = ArticleStatus::Read;

    if (m_currentFeedId != -1) {
        m_articles = DatabaseManager::instance().getArticlesForFeed(m_currentFeedId, status, m_filterStarred);
    } else if (m_currentCategoryId != -1) {
        m_articles = DatabaseManager::instance().getArticlesForCategory(m_currentCategoryId, status, m_filterStarred);
    } else {
        m_articles = DatabaseManager::instance().getAllArticles(status, m_filterStarred);
    }
    endResetModel();
    emit countChanged();
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
    case StatusRole: {
        switch (article.status) {
            case ArticleStatus::Read: return QStringLiteral("read");
            case ArticleStatus::Unread: return QStringLiteral("unread");
            case ArticleStatus::Removed: return QStringLiteral("removed");
            default: return QStringLiteral("unknown");
        }
    }
    case StarredRole: return article.starred;
    case FeedTitleRole: return article.feedTitle;
    case FeedSiteUrlRole: return article.siteUrl;
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
QVariantMap ArticleListModel::getById(qint64 id) const
{
    // Search cached list first
    for (int i = 0; i < m_articles.count(); ++i) {
        if (m_articles.at(i).id == id) {
            return get(i);
        }
    }
    
    // Fallback to database
    auto article = DatabaseManager::instance().getArticleById(id);
    if (article) {
        QVariantMap res;
        res["id"] = qlonglong(article->id);
        res["feedId"] = qlonglong(article->feedId);
        res["title"] = article->title;
        res["author"] = article->author;
        res["content"] = article->content;
        res["url"] = article->url;
        res["publishedAt"] = article->publishedAt;
        res["status"] = static_cast<int>(article->status);
        res["starred"] = article->starred;
        res["isRead"] = article->status == ArticleStatus::Read;
        res["feedTitle"] = article->feedTitle;
        res["feedSiteUrl"] = article->siteUrl;
        return res;
    }
    return {};
}
