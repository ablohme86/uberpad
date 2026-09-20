#pragma once

#include <QString>
#include <QStringList>
#include <vector>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/State>

namespace UberPad {

class TuiBuffer {
public:
    TuiBuffer();

    bool loadFromFile(const QString &filePath);
    bool saveToFile(const QString &filePath = QString());

    const std::vector<QString>& lines() const { return m_lines; }
    int lineCount() const { return (int)m_lines.size(); }
    QString line(int index) const;

    int cursorRow() const { return m_cursorRow; }
    int cursorCol() const { return m_cursorCol; }
    void setCursor(int row, int col);

    QString filePath() const { return m_filePath; }
    QString fileName() const;
    bool isModified() const { return m_modified; }
    void setModified(bool mod) { m_modified = mod; }

    KSyntaxHighlighting::Definition definition() const { return m_definition; }
    QString languageName() const;

    // Editing
    void insertChar(QChar ch);
    void insertNewline();
    void backspace();
    void deleteChar();
    void insertTab();

    // Navigation
    void moveUp();
    void moveDown();
    void moveLeft();
    void moveRight();
    void moveHome();
    void moveEnd();
    void movePageUp(int pageSize);
    void movePageDown(int pageSize);

    // Search
    bool find(const QString &query, bool forward = true);

    const std::vector<KSyntaxHighlighting::State>& states() const { return m_states; }
    void setStates(const std::vector<KSyntaxHighlighting::State> &st) { m_states = st; }

private:
    void ensureCursorValid();
    void updateSyntaxDefinition();

    std::vector<QString> m_lines;
    std::vector<KSyntaxHighlighting::State> m_states;
    int m_cursorRow = 0;
    int m_cursorCol = 0;

    QString m_filePath;
    bool m_modified = false;
    bool m_crlf = false;
    KSyntaxHighlighting::Definition m_definition;
};

} // namespace UberPad
