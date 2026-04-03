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

void ArticleListModel::setCurrentFeedId(long id)
{
    if (m_currentFeedId == id) return;
    m_currentFeedId = id;
    m_currentCategoryId = -1;
    refresh();
    emit currentFeedIdChanged();
    emit currentCategoryIdChanged();
    emit currentNameChanged();
}

void ArticleListModel::setCurrentCategoryId(long id)
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

void ArticleListModel::markAsReadById(long id, bool read)
{
    qDebug() << "ArticleListModel::markAsReadById" << id << read;
    DatabaseManager::instance().markAsRead(id, read);
    
    QString statusString = read ? QStringLiteral("read") : QStringLiteral("unread");
    SyncEngine::instance().updateArticleStatus(id, statusString);
    
    // Find and update local list for immediate UI feedback
    for (int i = 0; i < m_articles.count(); ++i) {
        if (m_articles.at(i).id == id) {
            m_articles[i].status = statusString;
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
    for (int i = 0; i < m_articles.count(); ++i) {
        if (m_articles.at(i).status == QStringLiteral("unread")) {
            markAsRead(i, true);
        }
    }
}

void ArticleListModel::toggleStarred(int index)
{
    if (index < 0 || index >= m_articles.count()) return;
    toggleStarredById(m_articles.at(index).id);
}

void ArticleListModel::toggleStarredById(long id)
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
        auto all = DatabaseManager::instance().getAllArticles();
        for (const auto& a : all) {
            if (a.id == id) {
                currentStarred = a.starred;
                break;
            }
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

void ArticleListModel::onArticleContentUpdated(long id)
{
    // Find the item in our list and update its content from DB
    for (int i = 0; i < m_articles.count(); ++i) {
        if (m_articles.at(i).id == id) {
            // Re-fetch this article from DB to get NEW content
            auto all = DatabaseManager::instance().getAllArticles();
            for (const auto& a : all) {
                if (a.id == id) {
                    m_articles[i] = a;
                    QModelIndex qidx = index(i, 0);
                    // Emit dataChanged for all relevant roles (content, etc.)
                    emit dataChanged(qidx, qidx); 
                    return;
                }
            }
            break;
        }
    }
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
QVariantMap ArticleListModel::getById(long id) const
{
    // Search cached list first
    for (int i = 0; i < m_articles.count(); ++i) {
        if (m_articles.at(i).id == id) {
            return get(i);
        }
    }
    
    // Fallback to database
    auto all = DatabaseManager::instance().getAllArticles();
    for (const auto& a : all) {
        if (a.id == id) {
            QVariantMap res;
            res["id"] = qlonglong(a.id);
            res["feedId"] = qlonglong(a.feedId);
            res["title"] = a.title;
            res["author"] = a.author;
            res["content"] = a.content;
            res["url"] = a.url;
            res["publishedAt"] = a.publishedAt;
            res["status"] = a.status;
            res["starred"] = a.starred;
            res["feedTitle"] = a.feedTitle;
            res["feedSiteUrl"] = a.feedSiteUrl;
            return res;
        }
    }
    return {};
}
