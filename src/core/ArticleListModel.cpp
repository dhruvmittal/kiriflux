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

bool ArticleListModel::canFetchMore(const QModelIndex &parent) const
{
    if (parent.isValid()) return false;
    if (!Settings::self()->lazyLoadingEnabled()) return false;
    if (m_totalAvailableCount == -1) return false;
    return m_articles.size() < m_totalAvailableCount;
}

void ArticleListModel::fetchMore(const QModelIndex &parent)
{
    if (parent.isValid()) return;
    if (!canFetchMore(parent)) return;

    int batchSize = Settings::self()->lazyLoadingBatchSize();
    int offset = m_articles.size();
    
    ArticleStatus status = ArticleStatus::All;
    if (!m_filterStarred) {
        if (m_filterStatus == "unread") status = ArticleStatus::Unread;
        else if (m_filterStatus == "read") status = ArticleStatus::Read;
    }

    QList<Article> moreArticles;
    if (m_currentFeedId != -1) {
        moreArticles = DatabaseManager::instance().getArticlesForFeed(m_currentFeedId, status, m_filterStarred, batchSize, offset);
    } else if (m_currentCategoryId != -1) {
        moreArticles = DatabaseManager::instance().getArticlesForCategory(m_currentCategoryId, status, m_filterStarred, batchSize, offset);
    } else {
        moreArticles = DatabaseManager::instance().getAllArticles(status, m_filterStarred, batchSize, offset);
    }

    if (moreArticles.isEmpty()) return;

    beginInsertRows(QModelIndex(), m_articles.size(), m_articles.size() + moreArticles.size() - 1);
    m_articles.append(moreArticles);
    endInsertRows();
    
    emit countChanged();
}

void ArticleListModel::setCurrentFeedId(qint64 id)
{
    if (m_currentFeedId == id) return;
    m_currentFeedId = id;
    m_currentCategoryId = -1;
    refresh(true);
    emit currentFeedIdChanged();
    emit currentCategoryIdChanged();
    emit currentNameChanged();
}

void ArticleListModel::setCurrentCategoryId(qint64 id)
{
    if (m_currentCategoryId == id) return;
    m_currentCategoryId = id;
    m_currentFeedId = -1;
    refresh(true);
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
    markArticlesAsReadByIds({id}, read);
}

void ArticleListModel::markArticlesAsReadByIds(const QList<qint64>& ids, bool read)
{
    if (ids.isEmpty()) return;

    DatabaseManager::instance().markArticlesAsRead(ids, read);

    if (read) {
        for (qint64 id : ids) {
            SyncEngine::instance().enqueueMarkAsRead(id);
        }
    } else {
        SyncEngine::instance().updateArticlesStatus(ids, QStringLiteral("unread"));
    }

    QSet<qint64> idSet(ids.begin(), ids.end());
    ArticleStatus targetStatus = read ? ArticleStatus::Read : ArticleStatus::Unread;
    for (int i = 0; i < m_articles.count(); ++i) {
        if (idSet.contains(m_articles.at(i).id)) {
            m_articles[i].status = targetStatus;
            QModelIndex qidx = this->index(i, 0);
            emit dataChanged(qidx, qidx, {StatusRole});
        }
    }
}

void ArticleListModel::setFilterStatus(const QString &status)
{
    if (m_filterStatus == status) return;
    m_filterStatus = status;
    Settings::setFilterStatus(status);
    Settings::self()->save();
    refresh(true);
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
    refresh(true);
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

void ArticleListModel::refresh(bool forceReset)
{
    ArticleStatus status = ArticleStatus::All;
    if (!m_filterStarred) {
        if (m_filterStatus == "unread") status = ArticleStatus::Unread;
        else if (m_filterStatus == "read") status = ArticleStatus::Read;
    }

    bool lazy = Settings::self()->lazyLoadingEnabled();
    int batchSize = Settings::self()->lazyLoadingBatchSize();
    int limit = -1;
    
    if (lazy) {
        limit = (forceReset || m_articles.isEmpty()) ? batchSize : (m_articles.size() + batchSize);
        if (m_currentFeedId != -1) {
            m_totalAvailableCount = DatabaseManager::instance().countArticlesForFeed(m_currentFeedId, status, m_filterStarred);
        } else if (m_currentCategoryId != -1) {
            m_totalAvailableCount = DatabaseManager::instance().countArticlesForCategory(m_currentCategoryId, status, m_filterStarred);
        } else {
            m_totalAvailableCount = DatabaseManager::instance().countAllArticles(status, m_filterStarred);
        }
    } else {
        m_totalAvailableCount = -1;
    }

    QList<Article> newArticles;
    if (m_currentFeedId != -1) {
        newArticles = DatabaseManager::instance().getArticlesForFeed(m_currentFeedId, status, m_filterStarred, limit, 0);
    } else if (m_currentCategoryId != -1) {
        newArticles = DatabaseManager::instance().getArticlesForCategory(m_currentCategoryId, status, m_filterStarred, limit, 0);
    } else {
        newArticles = DatabaseManager::instance().getAllArticles(status, m_filterStarred, limit, 0);
    }

    if (forceReset || m_articles.isEmpty()) {
        beginResetModel();
        m_articles = newArticles;
        endResetModel();
        emit countChanged();
        return;
    }

    int oldIdx = 0;
    int newIdx = 0;

    while (oldIdx < m_articles.size() && newIdx < newArticles.size()) {
        if (m_articles[oldIdx].id == newArticles[newIdx].id) {
            if (m_articles[oldIdx] != newArticles[newIdx]) {
                m_articles[oldIdx] = newArticles[newIdx];
                emit dataChanged(index(oldIdx, 0), index(oldIdx, 0));
            }
            oldIdx++;
            newIdx++;
        } else {
            bool newIsFirst = false;
            if (newArticles[newIdx].publishedAt > m_articles[oldIdx].publishedAt) {
                newIsFirst = true;
            } else if (newArticles[newIdx].publishedAt == m_articles[oldIdx].publishedAt) {
                newIsFirst = (newArticles[newIdx].id > m_articles[oldIdx].id);
            }

            if (newIsFirst) {
                int startInsertIdx = newIdx;
                while (newIdx < newArticles.size() && 
                       ((newArticles[newIdx].publishedAt > m_articles[oldIdx].publishedAt) || 
                        (newArticles[newIdx].publishedAt == m_articles[oldIdx].publishedAt && newArticles[newIdx].id > m_articles[oldIdx].id))) {
                    newIdx++;
                }
                int count = newIdx - startInsertIdx;
                beginInsertRows(QModelIndex(), oldIdx, oldIdx + count - 1);
                for (int i = 0; i < count; ++i) {
                    m_articles.insert(oldIdx + i, newArticles[startInsertIdx + i]);
                }
                endInsertRows();
                oldIdx += count;
            } else {
                if (m_filterStatus == "unread" && m_articles[oldIdx].status == ArticleStatus::Read) {
                    oldIdx++; // Evaporation Guard: keep locally read items in the unread list until hard reset
                } else {
                    int startRemoveIdx = oldIdx;
                    while (oldIdx < m_articles.size() && 
                           ((m_articles[oldIdx].publishedAt > newArticles[newIdx].publishedAt) || 
                            (m_articles[oldIdx].publishedAt == newArticles[newIdx].publishedAt && m_articles[oldIdx].id > newArticles[newIdx].id))) {
                        if (m_filterStatus == "unread" && m_articles[oldIdx].status == ArticleStatus::Read) break;
                        oldIdx++;
                    }
                    int count = oldIdx - startRemoveIdx;
                    if (count > 0) {
                        beginRemoveRows(QModelIndex(), startRemoveIdx, startRemoveIdx + count - 1);
                        m_articles.erase(m_articles.begin() + startRemoveIdx, m_articles.begin() + oldIdx);
                        endRemoveRows();
                        oldIdx = startRemoveIdx;
                    }
                }
            }
        }
    }

    if (newIdx < newArticles.size()) {
        int count = newArticles.size() - newIdx;
        beginInsertRows(QModelIndex(), oldIdx, oldIdx + count - 1);
        while (newIdx < newArticles.size()) {
            m_articles.append(newArticles[newIdx]);
            newIdx++;
        }
        endInsertRows();
    }

    if (oldIdx < m_articles.size()) {
        // Need to respect Evaporation Guard for tail removals too
        while (oldIdx < m_articles.size()) {
            if (m_filterStatus == "unread" && m_articles[oldIdx].status == ArticleStatus::Read) {
                oldIdx++;
            } else {
                int startRemoveIdx = oldIdx;
                while (oldIdx < m_articles.size() && !(m_filterStatus == "unread" && m_articles[oldIdx].status == ArticleStatus::Read)) {
                    oldIdx++;
                }
                int count = oldIdx - startRemoveIdx;
                beginRemoveRows(QModelIndex(), startRemoveIdx, startRemoveIdx + count - 1);
                m_articles.erase(m_articles.begin() + startRemoveIdx, m_articles.begin() + oldIdx);
                endRemoveRows();
                oldIdx = startRemoveIdx;
            }
        }
    }

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
        res["status"] = (article->status == ArticleStatus::Read) ? QStringLiteral("read")
                      : (article->status == ArticleStatus::Removed) ? QStringLiteral("removed")
                      : QStringLiteral("unread");
        res["starred"] = article->starred;
        res["isRead"] = article->status == ArticleStatus::Read;
        res["feedTitle"] = article->feedTitle;
        res["feedSiteUrl"] = article->siteUrl;
        return res;
    }
    return {};
}
