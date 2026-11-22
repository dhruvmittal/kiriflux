// SPDX-License-Identifier: GPL-3.0-or-later
#include "ArticleListModel.h"
#include "SyncEngine.h"
#include "kiriflux.h"
#include <KLocalizedString>
#include <QSet>

static void decodeFilterMode(const QString& mode, ArticleStatus& status, bool& starred)
{
    if (mode == QLatin1String("unread")) {
        status = ArticleStatus::Unread;
        starred = false;
    } else if (mode == QLatin1String("starred")) {
        status = ArticleStatus::All;
        starred = true;
    } else {
        status = ArticleStatus::All;
        starred = false;
    }
}

static QString statusToString(ArticleStatus status)
{
    switch (status) {
    case ArticleStatus::Read: return QStringLiteral("read");
    case ArticleStatus::Unread: return QStringLiteral("unread");
    case ArticleStatus::Removed: return QStringLiteral("removed");
    default: return QStringLiteral("unknown");
    }
}

static QVariantMap articleToMap(const Article& article)
{
    QVariantMap res;
    res[QStringLiteral("id")] = qlonglong(article.id);
    res[QStringLiteral("feedId")] = qlonglong(article.feedId);
    res[QStringLiteral("title")] = article.title;
    res[QStringLiteral("author")] = article.author;
    res[QStringLiteral("content")] = article.content;
    res[QStringLiteral("url")] = article.url;
    res[QStringLiteral("publishedAt")] = article.publishedAt;
    res[QStringLiteral("status")] = statusToString(article.status);
    res[QStringLiteral("starred")] = article.starred;
    res[QStringLiteral("isRead")] = (article.status == ArticleStatus::Read);
    res[QStringLiteral("feedTitle")] = article.feedTitle;
    res[QStringLiteral("feedSiteUrl")] = article.siteUrl;
    res[QStringLiteral("feedIconId")] = qlonglong(article.feedIconId);
    res[QStringLiteral("feedHasIcon")] = article.feedHasIcon;
    return res;
}

ArticleListModel::ArticleListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    const QString savedMode = Settings::self()->filterMode();
    if (savedMode == QLatin1String("unread") || savedMode == QLatin1String("starred")) {
        m_filterMode = savedMode;
    } else {
        m_filterMode = QStringLiteral("all");
    }
    
    connect(&SyncEngine::instance(), &SyncEngine::articlesUpdated, this, &ArticleListModel::refresh);
    connect(&SyncEngine::instance(), &SyncEngine::articleContentUpdated, this, &ArticleListModel::onArticleContentUpdated);
    connect(&SyncEngine::instance(), &SyncEngine::feedIconUpdated, this, &ArticleListModel::onFeedIconUpdated);

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
    bool starred = false;
    decodeFilterMode(m_filterMode, status, starred);

    QList<Article> moreArticles = DatabaseManager::instance().getArticles(
        m_currentFeedId, m_currentCategoryId, status, starred, batchSize, offset);

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

void ArticleListModel::markReadUpTo(int endIndex)
{
    const int limit = qMin(endIndex, static_cast<int>(m_articles.size()));
    if (limit <= 0) return;

    QList<qint64> unreadIds;
    for (int i = 0; i < limit; ++i) {
        if (m_articles.at(i).status == ArticleStatus::Unread) {
            unreadIds.append(m_articles.at(i).id);
        }
    }
    if (!unreadIds.isEmpty()) {
        markArticlesAsReadByIds(unreadIds, true);
    }
}

void ArticleListModel::setFilterMode(const QString &mode)
{
    const QString normalized = (mode == QLatin1String("unread") || mode == QLatin1String("starred"))
                                   ? mode
                                   : QStringLiteral("all");
    if (m_filterMode == normalized) return;
    m_filterMode = normalized;
    Settings::self()->setFilterMode(m_filterMode);
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

void ArticleListModel::markAllAsRead()
{
    const bool starredOnly = (m_filterMode == QLatin1String("starred"));
    const QList<qint64> dbUnreadIds = DatabaseManager::instance().getUnreadArticleIds(m_currentFeedId, m_currentCategoryId, starredOnly);

    QSet<qint64> unreadSet(dbUnreadIds.begin(), dbUnreadIds.end());
    for (const auto& a : m_articles) {
        if (a.status == ArticleStatus::Unread) {
            unreadSet.insert(a.id);
        }
    }

    if (unreadSet.isEmpty()) return;

    QList<qint64> unreadIds(unreadSet.begin(), unreadSet.end());
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

    if (Settings::self()->lazyLoadingEnabled() && m_filterMode == QLatin1String("unread")) {
        m_totalAvailableCount = m_articles.size();
    }
}

void ArticleListModel::toggleStarred(int index)
{
    if (index < 0 || index >= m_articles.count()) return;
    toggleStarredById(m_articles.at(index).id);
}

void ArticleListModel::toggleStarredById(qint64 id)
{
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
        auto article = DatabaseManager::instance().getArticleById(id);
        if (article) {
            currentStarred = article->starred;
        } else {
            return;
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
    for (int i = 0; i < m_articles.count(); ++i) {
        if (m_articles.at(i).id == id) {
            auto article = DatabaseManager::instance().getArticleById(id);
            if (article) {
                m_articles[i] = *article;
                QModelIndex qidx = index(i, 0);
                emit dataChanged(qidx, qidx); 
            }
            return;
        }
    }
}

void ArticleListModel::onFeedIconUpdated(qint64 feedId)
{
    for (int i = 0; i < m_articles.count(); ++i) {
        if (m_articles[i].feedId == feedId && !m_articles[i].feedHasIcon) {
            m_articles[i].feedHasIcon = true;
            const QModelIndex qidx = index(i, 0);
            emit dataChanged(qidx, qidx, {FeedHasIconRole});
        }
    }
}

void ArticleListModel::refresh(bool forceReset)
{
    ArticleStatus status = ArticleStatus::All;
    bool starred = false;
    decodeFilterMode(m_filterMode, status, starred);

    bool lazy = Settings::self()->lazyLoadingEnabled();
    int batchSize = Settings::self()->lazyLoadingBatchSize();
    int limit = -1;
    
    if (lazy) {
        limit = (forceReset || m_articles.isEmpty()) ? batchSize : (m_articles.size() + batchSize);
        m_totalAvailableCount = DatabaseManager::instance().countArticles(m_currentFeedId, m_currentCategoryId, status, starred);
    } else {
        m_totalAvailableCount = -1;
    }

    QList<Article> newArticles = DatabaseManager::instance().getArticles(m_currentFeedId, m_currentCategoryId, status, starred, limit, 0);

    if (forceReset || m_articles.isEmpty()) {
        beginResetModel();
        m_articles = newArticles;
        endResetModel();
        emit countChanged();
        return;
    }

    QSet<qint64> oldIds;
    oldIds.reserve(m_articles.size());
    for (const auto& a : m_articles) {
        oldIds.insert(a.id);
    }

    const bool guardUnread = (m_filterMode == QLatin1String("unread"));
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
                    if (oldIds.contains(newArticles[newIdx].id)) {
                        // Article timestamp/order shifted; reset cleanly instead of creating duplicate IDs
                        beginResetModel();
                        m_articles = newArticles;
                        endResetModel();
                        emit countChanged();
                        return;
                    }
                    newIdx++;
                }
                int count = newIdx - startInsertIdx;
                beginInsertRows(QModelIndex(), oldIdx, oldIdx + count - 1);
                for (int i = 0; i < count; ++i) {
                    m_articles.insert(oldIdx + i, newArticles[startInsertIdx + i]);
                    oldIds.insert(newArticles[startInsertIdx + i].id);
                }
                endInsertRows();
                oldIdx += count;
            } else {
                if (guardUnread && m_articles[oldIdx].status == ArticleStatus::Read) {
                    oldIdx++; // Evaporation Guard: keep locally read items in the unread list until hard reset
                } else {
                    int startRemoveIdx = oldIdx;
                    while (oldIdx < m_articles.size() && 
                           ((m_articles[oldIdx].publishedAt > newArticles[newIdx].publishedAt) || 
                            (m_articles[oldIdx].publishedAt == newArticles[newIdx].publishedAt && m_articles[oldIdx].id > newArticles[newIdx].id))) {
                        if (guardUnread && m_articles[oldIdx].status == ArticleStatus::Read) break;
                        oldIds.remove(m_articles[oldIdx].id);
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
        for (int k = newIdx; k < newArticles.size(); ++k) {
            if (oldIds.contains(newArticles[k].id)) {
                beginResetModel();
                m_articles = newArticles;
                endResetModel();
                emit countChanged();
                return;
            }
        }
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
            if (guardUnread && m_articles[oldIdx].status == ArticleStatus::Read) {
                oldIdx++;
            } else {
                int startRemoveIdx = oldIdx;
                while (oldIdx < m_articles.size() && !(guardUnread && m_articles[oldIdx].status == ArticleStatus::Read)) {
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
    case StatusRole: return statusToString(article.status);
    case StarredRole: return article.starred;
    case FeedTitleRole: return article.feedTitle;
    case FeedSiteUrlRole: return article.siteUrl;
    case FeedIconIdRole: return qlonglong(article.feedIconId);
    case FeedHasIconRole: return article.feedHasIcon;
    case Qt::DisplayRole: return article.title;
    }
    return {};
}

QHash<int, QByteArray> ArticleListModel::roleNames() const
{
    static const QHash<int, QByteArray> roles = {
        {IdRole, "id"},
        {FeedIdRole, "feedId"},
        {TitleRole, "title"},
        {AuthorRole, "author"},
        {ContentRole, "content"},
        {UrlRole, "url"},
        {PublishedAtRole, "publishedAt"},
        {StatusRole, "status"},
        {StarredRole, "starred"},
        {FeedTitleRole, "feedTitle"},
        {FeedSiteUrlRole, "feedSiteUrl"},
        {FeedIconIdRole, "feedIconId"},
        {FeedHasIconRole, "feedHasIcon"}
    };
    return roles;
}

QVariantMap ArticleListModel::get(int row) const
{
    if (row < 0 || row >= m_articles.count()) return {};
    return articleToMap(m_articles.at(row));
}

QVariantMap ArticleListModel::getById(qint64 id) const
{
    for (int i = 0; i < m_articles.count(); ++i) {
        if (m_articles.at(i).id == id) {
            return articleToMap(m_articles.at(i));
        }
    }
    
    auto article = DatabaseManager::instance().getArticleById(id);
    if (article) {
        return articleToMap(*article);
    }
    return {};
}

qint64 ArticleListModel::idAt(int row) const
{
    if (row < 0 || row >= m_articles.count()) return -1;
    return m_articles.at(row).id;
}
