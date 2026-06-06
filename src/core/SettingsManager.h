// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QFont>
#include <QtQml>

class SettingsManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString apiUrl READ apiUrl WRITE setApiUrl NOTIFY apiUrlChanged)
    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)
    Q_PROPERTY(int syncInterval READ syncInterval WRITE setSyncInterval NOTIFY syncIntervalChanged)
    Q_PROPERTY(bool cleanSync READ cleanSync WRITE setCleanSync NOTIFY cleanSyncChanged)
    Q_PROPERTY(QDateTime lastSyncTime READ lastSyncTime WRITE setLastSyncTime NOTIFY lastSyncTimeChanged)
    Q_PROPERTY(bool showToolbarLabels READ showToolbarLabels WRITE setShowToolbarLabels NOTIFY showToolbarLabelsChanged)
    Q_PROPERTY(int titleLines READ titleLines WRITE setTitleLines NOTIFY titleLinesChanged)
    Q_PROPERTY(int leftPaneWidth READ leftPaneWidth WRITE setLeftPaneWidth NOTIFY leftPaneWidthChanged)
    Q_PROPERTY(int centerPaneWidth READ centerPaneWidth WRITE setCenterPaneWidth NOTIFY centerPaneWidthChanged)
    Q_PROPERTY(bool markAsReadOnOpen READ markAsReadOnOpen WRITE setMarkAsReadOnOpen NOTIFY markAsReadOnOpenChanged)
    Q_PROPERTY(bool markAsReadOnScroll READ markAsReadOnScroll WRITE setMarkAsReadOnScroll NOTIFY markAsReadOnScrollChanged)
    Q_PROPERTY(bool useReaderMode READ useReaderMode WRITE setUseReaderMode NOTIFY useReaderModeChanged)
    Q_PROPERTY(bool preFetchImages READ preFetchImages WRITE setPreFetchImages NOTIFY preFetchImagesChanged)
    Q_PROPERTY(int cacheSizeMB READ cacheSizeMB WRITE setCacheSizeMB NOTIFY cacheSizeMBChanged)
    Q_PROPERTY(bool lazyLoadingEnabled READ lazyLoadingEnabled WRITE setLazyLoadingEnabled NOTIFY lazyLoadingEnabledChanged)
    Q_PROPERTY(int lazyLoadingBatchSize READ lazyLoadingBatchSize WRITE setLazyLoadingBatchSize NOTIFY lazyLoadingBatchSizeChanged)
    Q_PROPERTY(bool autoPruneEnabled READ autoPruneEnabled WRITE setAutoPruneEnabled NOTIFY autoPruneEnabledChanged)
    Q_PROPERTY(int pruneAfterDays READ pruneAfterDays WRITE setPruneAfterDays NOTIFY pruneAfterDaysChanged)
    Q_PROPERTY(bool closeReadingPaneOnReset READ closeReadingPaneOnReset WRITE setCloseReadingPaneOnReset NOTIFY closeReadingPaneOnResetChanged)
    
    Q_PROPERTY(QFont leftPaneFont READ leftPaneFont WRITE setLeftPaneFont NOTIFY leftPaneFontChanged)
    Q_PROPERTY(QFont articleListTitleFont READ articleListTitleFont WRITE setArticleListTitleFont NOTIFY articleListTitleFontChanged)
    Q_PROPERTY(QFont articleListInfoFont READ articleListInfoFont WRITE setArticleListInfoFont NOTIFY articleListInfoFontChanged)
    Q_PROPERTY(QFont articleViewTitleFont READ articleViewTitleFont WRITE setArticleViewTitleFont NOTIFY articleViewTitleFontChanged)
    Q_PROPERTY(QFont articleViewInfoFont READ articleViewInfoFont WRITE setArticleViewInfoFont NOTIFY articleViewInfoFontChanged)
    Q_PROPERTY(QFont articleViewTextFont READ articleViewTextFont WRITE setArticleViewTextFont NOTIFY articleViewTextFontChanged)

public:
    static SettingsManager& instance();
    explicit SettingsManager(QObject *parent = nullptr);

    QString apiUrl() const;
    void setApiUrl(const QString& url);

    QString apiKey() const;
    void setApiKey(const QString& key);

    int syncInterval() const;
    void setSyncInterval(int interval);

    bool cleanSync() const;
    void setCleanSync(bool clean);

    QDateTime lastSyncTime() const;
    void setLastSyncTime(const QDateTime& time);

    bool showToolbarLabels() const;
    void setShowToolbarLabels(bool show);

    int titleLines() const;
    void setTitleLines(int lines);

    int leftPaneWidth() const;
    void setLeftPaneWidth(int width);

    int centerPaneWidth() const;
    void setCenterPaneWidth(int width);

    bool markAsReadOnOpen() const;
    void setMarkAsReadOnOpen(bool mark);

    bool markAsReadOnScroll() const;
    void setMarkAsReadOnScroll(bool mark);

    bool useReaderMode() const;
    void setUseReaderMode(bool use);

    bool preFetchImages() const;
    void setPreFetchImages(bool prefetch);

    int cacheSizeMB() const;
    void setCacheSizeMB(int size);

    bool lazyLoadingEnabled() const;
    void setLazyLoadingEnabled(bool enabled);

    int lazyLoadingBatchSize() const;
    void setLazyLoadingBatchSize(int size);

    bool autoPruneEnabled() const;
    void setAutoPruneEnabled(bool enabled);

    int pruneAfterDays() const;
    void setPruneAfterDays(int days);

    bool closeReadingPaneOnReset() const;
    void setCloseReadingPaneOnReset(bool close);

    QFont leftPaneFont() const;
    void setLeftPaneFont(const QFont& font);

    QFont articleListTitleFont() const;
    void setArticleListTitleFont(const QFont& font);

    QFont articleListInfoFont() const;
    void setArticleListInfoFont(const QFont& font);

    QFont articleViewTitleFont() const;
    void setArticleViewTitleFont(const QFont& font);

    QFont articleViewInfoFont() const;
    void setArticleViewInfoFont(const QFont& font);

    QFont articleViewTextFont() const;
    void setArticleViewTextFont(const QFont& font);

    Q_INVOKABLE void save();
    Q_INVOKABLE void clearCache();
    void load();

signals:
    void apiUrlChanged();
    void apiKeyChanged();
    void syncIntervalChanged();
    void cleanSyncChanged();
    void lastSyncTimeChanged();
    void showToolbarLabelsChanged();
    void titleLinesChanged();
    void leftPaneWidthChanged();
    void centerPaneWidthChanged();
    void markAsReadOnOpenChanged();
    void markAsReadOnScrollChanged();
    void useReaderModeChanged();
    void preFetchImagesChanged();
    void cacheSizeMBChanged();
    void clearCacheRequested();
    void lazyLoadingEnabledChanged();
    void lazyLoadingBatchSizeChanged();
    void autoPruneEnabledChanged();
    void pruneAfterDaysChanged();
    void closeReadingPaneOnResetChanged();
    void leftPaneFontChanged();
    void articleListTitleFontChanged();
    void articleListInfoFontChanged();
    void articleViewTitleFontChanged();
    void articleViewInfoFontChanged();
    void articleViewTextFontChanged();
    void settingsChanged();

private:
};

#endif // SETTINGSMANAGER_H
