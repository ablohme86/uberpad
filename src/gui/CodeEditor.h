#pragma once

#include <QPlainTextEdit>
#include <QWidget>
#include <QString>
#include <QColor>
#include <QFileSystemWatcher>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Theme>
#include <KSyntaxHighlighting/SyntaxHighlighter>
#include "../core/RemoteClient.h"

namespace UberPad {

class CodeEditor;

class LineNumberArea : public QWidget {
    Q_OBJECT
public:
    explicit LineNumberArea(CodeEditor *editor);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    CodeEditor *m_editor;
};

class CodeEditor : public QPlainTextEdit {
    Q_OBJECT
    friend class LineNumberArea;

public:
    enum class LineEnding {
        LF,
        CRLF
    };

    explicit CodeEditor(QWidget *parent = nullptr);
    ~CodeEditor() override = default;

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

    // File operations
    bool loadFromFile(const QString &filePath);
    bool saveToFile(const QString &filePath = QString());
    QString filePath() const { return m_filePath; }
    QString fileName() const;
    bool isUntitled() const { return m_filePath.isEmpty() && !m_isRemote; }

    // Remote operations
    bool isRemote() const { return m_isRemote; }
    QString remotePath() const { return m_remotePath; }
    RemoteConfig remoteConfig() const { return m_remoteConfig; }
    void setRemoteInfo(const QString &remotePath, const RemoteConfig &config);

    // Line Endings
    LineEnding lineEnding() const { return m_lineEnding; }
    void setLineEnding(LineEnding le);
    QString lineEndingName() const;

    // Syntax Highlighting
    void setDefinition(const KSyntaxHighlighting::Definition &def);
    KSyntaxHighlighting::Definition definition() const { return m_definition; }
    QString languageName() const;
    void setTheme(const KSyntaxHighlighting::Theme &theme);
    KSyntaxHighlighting::Theme currentTheme() const { return m_theme; }

    // Editing operations
    void duplicateLineOrSelection();
    void moveLineUp();
    void moveLineDown();
    void toggleComment();
    void indentSelection();
    void unindentSelection();

    // Zooming
    void zoomIn(int range = 1);
    void zoomOut(int range = 1);
    void resetZoom();

    // Search highlights
    void setSearchHighlight(const QString &text, bool caseSensitive, bool wholeWord, bool isRegex);
    void clearSearchHighlight();

signals:
    void fileSaved(const QString &filePath);
    void languageChanged(const QString &languageName);
    void lineEndingChanged(const QString &lineEndingName);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLineAndBrackets();
    void updateLineNumberArea(const QRect &rect, int dy);
    void onTextChanged();

private:
    void applyThemeColors();
    void matchParentheses();
    QString commentPrefix() const;

    QWidget *m_lineNumberArea;
    KSyntaxHighlighting::SyntaxHighlighter *m_highlighter;
    KSyntaxHighlighting::Definition m_definition;
    KSyntaxHighlighting::Theme m_theme;

    QString m_filePath;
    bool m_isRemote = false;
    QString m_remotePath;
    RemoteConfig m_remoteConfig;
    LineEnding m_lineEnding = LineEnding::LF;
    int m_baseFontSize = 11;
    int m_currentZoomLevel = 0;

    // Search matches extra selections
    QList<QTextEdit::ExtraSelection> m_searchSelections;
    QList<QTextEdit::ExtraSelection> m_cursorSelections;
};

} // namespace UberPad
