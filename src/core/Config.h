#pragma once

#include <QString>
#include <QStringList>
#include <QSettings>
#include <QFont>

namespace UberPad {

class Config {
public:
    static Config& instance();

    // Editor settings
    QString fontFamily() const;
    void setFontFamily(const QString &family);

    int fontSize() const;
    void setFontSize(int size);

    int tabSize() const;
    void setTabSize(int size);

    bool useSpacesForTabs() const;
    void setUseSpacesForTabs(bool spaces);

    bool wordWrap() const;
    void setWordWrap(bool wrap);

    bool showLineNumbers() const;
    void setShowLineNumbers(bool show);

    bool highlightCurrentLine() const;
    void setHighlightCurrentLine(bool highlight);

    bool autoIndent() const;
    void setAutoIndent(bool autoIndent);

    bool bracketMatching() const;
    void setBracketMatching(bool match);

    QString themeName() const;
    void setThemeName(const QString &theme);

    // Recent files
    QStringList recentFiles() const;
    void addRecentFile(const QString &filePath);
    void clearRecentFiles();

    // Last opened workspace
    QString lastWorkspacePath() const;
    void setLastWorkspacePath(const QString &path);

    // Terminal shell
    QString defaultShell() const;

private:
    Config();
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    QSettings m_settings;
};

} // namespace UberPad
