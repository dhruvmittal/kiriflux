// SPDX-License-Identifier: GPL-3.0-or-later
#include "SettingsManager.h"
#include "kiriflux.h"

SettingsManager& SettingsManager::instance()
{
    static SettingsManager* inst = nullptr;
    if (!inst) {
        inst = new SettingsManager(qApp);
    }
    return *inst;
}

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
{
    load();
}

QString SettingsManager::apiUrl() const { return Settings::apiUrl(); }
void SettingsManager::setApiUrl(const QString& url) {
    if (url == apiUrl()) return;
    Settings::setApiUrl(url);
    emit apiUrlChanged();
    emit settingsChanged();
}

QString SettingsManager::apiKey() const { return Settings::apiKey(); }
void SettingsManager::setApiKey(const QString& key) {
    if (key == apiKey()) return;
    Settings::setApiKey(key);
    emit apiKeyChanged();
    emit settingsChanged();
}

int SettingsManager::syncInterval() const { return Settings::syncInterval(); }
void SettingsManager::setSyncInterval(int interval) {
    if (interval == syncInterval()) return;
    Settings::setSyncInterval(interval);
    emit syncIntervalChanged();
    emit settingsChanged();
}

bool SettingsManager::cleanSync() const { return Settings::cleanSync(); }
void SettingsManager::setCleanSync(bool clean) {
    if (clean == cleanSync()) return;
    Settings::setCleanSync(clean);
    emit cleanSyncChanged();
    emit settingsChanged();
}

QDateTime SettingsManager::lastSyncTime() const { return Settings::lastSyncTime(); }
void SettingsManager::setLastSyncTime(const QDateTime& time) {
    if (time == lastSyncTime()) return;
    Settings::setLastSyncTime(time);
    emit lastSyncTimeChanged();
    emit settingsChanged();
}

bool SettingsManager::showToolbarLabels() const { return Settings::showToolbarLabels(); }
void SettingsManager::setShowToolbarLabels(bool show) {
    if (show == showToolbarLabels()) return;
    Settings::setShowToolbarLabels(show);
    emit showToolbarLabelsChanged();
    emit settingsChanged();
}

int SettingsManager::titleLines() const { return Settings::titleLines(); }
void SettingsManager::setTitleLines(int lines) {
    if (lines == titleLines()) return;
    Settings::setTitleLines(lines);
    emit titleLinesChanged();
    emit settingsChanged();
}

int SettingsManager::leftPaneWidth() const { return Settings::leftPaneWidth(); }
void SettingsManager::setLeftPaneWidth(int width) {
    if (width == leftPaneWidth()) return;
    Settings::setLeftPaneWidth(width);
    emit leftPaneWidthChanged();
    emit settingsChanged();
}

int SettingsManager::centerPaneWidth() const { return Settings::centerPaneWidth(); }
void SettingsManager::setCenterPaneWidth(int width) {
    if (width == centerPaneWidth()) return;
    Settings::setCenterPaneWidth(width);
    emit centerPaneWidthChanged();
    emit settingsChanged();
}

bool SettingsManager::markAsReadOnOpen() const { return Settings::markAsReadOnOpen(); }
void SettingsManager::setMarkAsReadOnOpen(bool mark) {
    if (mark == markAsReadOnOpen()) return;
    Settings::setMarkAsReadOnOpen(mark);
    emit markAsReadOnOpenChanged();
    emit settingsChanged();
}

bool SettingsManager::markAsReadOnScroll() const { return Settings::markAsReadOnScroll(); }
void SettingsManager::setMarkAsReadOnScroll(bool mark) {
    if (mark == markAsReadOnScroll()) return;
    Settings::setMarkAsReadOnScroll(mark);
    emit markAsReadOnScrollChanged();
    emit settingsChanged();
}

bool SettingsManager::useReaderMode() const { return Settings::useReaderMode(); }
void SettingsManager::setUseReaderMode(bool use) {
    if (use == useReaderMode()) return;
    Settings::setUseReaderMode(use);
    emit useReaderModeChanged();
    emit settingsChanged();
}

QFont SettingsManager::leftPaneFont() const { return Settings::leftPaneFont(); }
void SettingsManager::setLeftPaneFont(const QFont& font) {
    if (font == leftPaneFont()) return;
    Settings::setLeftPaneFont(font);
    emit leftPaneFontChanged();
    emit settingsChanged();
}

QFont SettingsManager::articleListTitleFont() const { return Settings::articleListTitleFont(); }
void SettingsManager::setArticleListTitleFont(const QFont& font) {
    if (font == articleListTitleFont()) return;
    Settings::setArticleListTitleFont(font);
    emit articleListTitleFontChanged();
    emit settingsChanged();
}

QFont SettingsManager::articleListInfoFont() const { return Settings::articleListInfoFont(); }
void SettingsManager::setArticleListInfoFont(const QFont& font) {
    if (font == articleListInfoFont()) return;
    Settings::setArticleListInfoFont(font);
    emit articleListInfoFontChanged();
    emit settingsChanged();
}

QFont SettingsManager::articleViewTitleFont() const { return Settings::articleViewTitleFont(); }
void SettingsManager::setArticleViewTitleFont(const QFont& font) {
    if (font == articleViewTitleFont()) return;
    Settings::setArticleViewTitleFont(font);
    emit articleViewTitleFontChanged();
    emit settingsChanged();
}

QFont SettingsManager::articleViewInfoFont() const { return Settings::articleViewInfoFont(); }
void SettingsManager::setArticleViewInfoFont(const QFont& font) {
    if (font == articleViewInfoFont()) return;
    Settings::setArticleViewInfoFont(font);
    emit articleViewInfoFontChanged();
    emit settingsChanged();
}

QFont SettingsManager::articleViewTextFont() const { return Settings::articleViewTextFont(); }
void SettingsManager::setArticleViewTextFont(const QFont& font) {
    if (font == articleViewTextFont()) return;
    Settings::setArticleViewTextFont(font);
    emit articleViewTextFontChanged();
    emit settingsChanged();
}

void SettingsManager::save()
{
    Settings::self()->save();
}

void SettingsManager::load()
{
    Settings::self()->load();
}
