#include "Config.h"
#include <cstdlib>

namespace UberPad {

Config& Config::instance() {
    static Config cfg;
    return cfg;
}

Config::Config()
    : m_settings("UberPad", "UberPad")
{
}

QString Config::fontFamily() const {
    return m_settings.value("editor/fontFamily", "Monospace").toString();
}

void Config::setFontFamily(const QString &family) {
    m_settings.setValue("editor/fontFamily", family);
}

int Config::fontSize() const {
    return m_settings.value("editor/fontSize", 11).toInt();
}

void Config::setFontSize(int size) {
    m_settings.setValue("editor/fontSize", size);
}

int Config::tabSize() const {
    return m_settings.value("editor/tabSize", 4).toInt();
}

void Config::setTabSize(int size) {
    m_settings.setValue("editor/tabSize", size);
}

bool Config::useSpacesForTabs() const {
    return m_settings.value("editor/useSpacesForTabs", true).toBool();
}

void Config::setUseSpacesForTabs(bool spaces) {
    m_settings.setValue("editor/useSpacesForTabs", spaces);
}

bool Config::wordWrap() const {
    return m_settings.value("editor/wordWrap", false).toBool();
}

void Config::setWordWrap(bool wrap) {
    m_settings.setValue("editor/wordWrap", wrap);
}

bool Config::showLineNumbers() const {
    return m_settings.value("editor/showLineNumbers", true).toBool();
}

void Config::setShowLineNumbers(bool show) {
    m_settings.setValue("editor/showLineNumbers", show);
}

bool Config::highlightCurrentLine() const {
    return m_settings.value("editor/highlightCurrentLine", true).toBool();
}

void Config::setHighlightCurrentLine(bool highlight) {
    m_settings.setValue("editor/highlightCurrentLine", highlight);
}

bool Config::autoIndent() const {
    return m_settings.value("editor/autoIndent", true).toBool();
}

void Config::setAutoIndent(bool autoIndent) {
    m_settings.setValue("editor/autoIndent", autoIndent);
}

bool Config::bracketMatching() const {
    return m_settings.value("editor/bracketMatching", true).toBool();
}

void Config::setBracketMatching(bool match) {
    m_settings.setValue("editor/bracketMatching", match);
}

QString Config::themeName() const {
    return m_settings.value("editor/themeName", "Dracula").toString();
}

void Config::setThemeName(const QString &theme) {
    m_settings.setValue("editor/themeName", theme);
}

QStringList Config::recentFiles() const {
    return m_settings.value("recentFiles").toStringList();
}

void Config::addRecentFile(const QString &filePath) {
    QStringList files = recentFiles();
    files.removeAll(filePath);
    files.prepend(filePath);
    while (files.size() > 15) {
        files.removeLast();
    }
    m_settings.setValue("recentFiles", files);
}

void Config::clearRecentFiles() {
    m_settings.remove("recentFiles");
}

QString Config::lastWorkspacePath() const {
    return m_settings.value("lastWorkspacePath", "").toString();
}

void Config::setLastWorkspacePath(const QString &path) {
    m_settings.setValue("lastWorkspacePath", path);
}

QString Config::defaultShell() const {
    const char *shell = std::getenv("SHELL");
    if (shell && *shell) {
        return QString::fromUtf8(shell);
    }
    return QStringLiteral("/bin/bash");
}

} // namespace UberPad
