#include "TuiBuffer.h"
#include "../core/SyntaxManager.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <algorithm>

namespace UberPad {

TuiBuffer::TuiBuffer() {
    m_lines.push_back(QString());
    m_definition = SyntaxManager::instance().defaultDefinition();
}

QString TuiBuffer::line(int index) const {
    if (index >= 0 && index < (int)m_lines.size()) {
        return m_lines[index];
    }
    return QString();
}

QString TuiBuffer::fileName() const {
    if (m_filePath.isEmpty()) return QStringLiteral("[Untitled]");
    return QFileInfo(m_filePath).fileName();
}

QString TuiBuffer::languageName() const {
    if (m_definition.isValid() && !m_definition.name().isEmpty()) {
        return m_definition.name();
    }
    return QStringLiteral("Plain Text");
}

int TuiBuffer::characterCount() const {
    int total = 0;
    for (const auto &l : m_lines) {
        total += l.size() + 1;
    }
    return std::max(0, total - 1);
}

void TuiBuffer::updateSyntaxDefinition() {
    if (!m_filePath.isEmpty()) {
        m_definition = SyntaxManager::instance().definitionForFileName(m_filePath);
    }
    if (!m_definition.isValid()) {
        m_definition = SyntaxManager::instance().defaultDefinition();
    }
}

bool TuiBuffer::loadFromFile(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    m_crlf = data.contains("\r\n");
    QString content = QString::fromUtf8(data);

    m_lines.clear();
    QStringList rawLines = content.split(QLatin1Char('\n'));
    for (int i = 0; i < rawLines.size(); ++i) {
        QString l = rawLines[i];
        if (l.endsWith(QLatin1Char('\r'))) {
            l.chop(1);
        }
        m_lines.push_back(l);
    }
    if (m_lines.empty()) {
        m_lines.push_back(QString());
    }

    m_filePath = filePath;
    m_modified = false;
    m_cursorRow = 0;
    m_cursorCol = 0;

    updateSyntaxDefinition();
    return true;
}

bool TuiBuffer::saveToFile(const QString &filePath) {
    QString target = filePath.isEmpty() ? m_filePath : filePath;
    if (target.isEmpty()) return false;

    QFile file(target);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    QString eol = m_crlf ? QStringLiteral("\r\n") : QStringLiteral("\n");
    for (size_t i = 0; i < m_lines.size(); ++i) {
        file.write(m_lines[i].toUtf8());
        if (i + 1 < m_lines.size()) {
            file.write(eol.toUtf8());
        }
    }
    file.close();

    m_filePath = target;
    m_modified = false;
    updateSyntaxDefinition();
    return true;
}

void TuiBuffer::setCursor(int row, int col) {
    m_cursorRow = std::clamp(row, 0, (int)m_lines.size() - 1);
    m_cursorCol = std::clamp(col, 0, (int)m_lines[m_cursorRow].size());
}

void TuiBuffer::ensureCursorValid() {
    m_cursorRow = std::clamp(m_cursorRow, 0, (int)m_lines.size() - 1);
    m_cursorCol = std::clamp(m_cursorCol, 0, (int)m_lines[m_cursorRow].size());
}

void TuiBuffer::insertChar(QChar ch) {
    ensureCursorValid();
    m_lines[m_cursorRow].insert(m_cursorCol, ch);
    m_cursorCol++;
    m_modified = true;
}

void TuiBuffer::insertTab() {
    ensureCursorValid();
    m_lines[m_cursorRow].insert(m_cursorCol, QStringLiteral("    "));
    m_cursorCol += 4;
    m_modified = true;
}

void TuiBuffer::insertNewline() {
    ensureCursorValid();
    QString curLine = m_lines[m_cursorRow];
    QString left = curLine.left(m_cursorCol);
    QString right = curLine.mid(m_cursorCol);

    // Auto-indent
    int spaces = 0;
    while (spaces < left.size() && left[spaces].isSpace()) {
        ++spaces;
    }
    QString indent = left.left(spaces);

    QString trimmed = left.trimmed();
    if (!trimmed.isEmpty() && (trimmed.back() == '{' || trimmed.back() == ':')) {
        indent += QStringLiteral("    ");
    }

    m_lines[m_cursorRow] = left;
    m_lines.insert(m_lines.begin() + m_cursorRow + 1, indent + right);

    m_cursorRow++;
    m_cursorCol = indent.size();
    m_modified = true;
}

void TuiBuffer::backspace() {
    ensureCursorValid();
    if (m_cursorCol > 0) {
        m_lines[m_cursorRow].remove(m_cursorCol - 1, 1);
        m_cursorCol--;
        m_modified = true;
    } else if (m_cursorRow > 0) {
        // Merge with previous line
        int prevLen = m_lines[m_cursorRow - 1].size();
        m_lines[m_cursorRow - 1] += m_lines[m_cursorRow];
        m_lines.erase(m_lines.begin() + m_cursorRow);
        m_cursorRow--;
        m_cursorCol = prevLen;
        m_modified = true;
    }
}

void TuiBuffer::deleteChar() {
    ensureCursorValid();
    if (m_cursorCol < m_lines[m_cursorRow].size()) {
        m_lines[m_cursorRow].remove(m_cursorCol, 1);
        m_modified = true;
    } else if (m_cursorRow + 1 < (int)m_lines.size()) {
        // Merge with next line
        m_lines[m_cursorRow] += m_lines[m_cursorRow + 1];
        m_lines.erase(m_lines.begin() + m_cursorRow + 1);
        m_modified = true;
    }
}

void TuiBuffer::moveUp() {
    if (m_cursorRow > 0) {
        m_cursorRow--;
        m_cursorCol = std::min(m_cursorCol, (int)m_lines[m_cursorRow].size());
    }
}

void TuiBuffer::moveDown() {
    if (m_cursorRow + 1 < (int)m_lines.size()) {
        m_cursorRow++;
        m_cursorCol = std::min(m_cursorCol, (int)m_lines[m_cursorRow].size());
    }
}

void TuiBuffer::moveLeft() {
    if (m_cursorCol > 0) {
        m_cursorCol--;
    } else if (m_cursorRow > 0) {
        m_cursorRow--;
        m_cursorCol = m_lines[m_cursorRow].size();
    }
}

void TuiBuffer::moveRight() {
    if (m_cursorCol < m_lines[m_cursorRow].size()) {
        m_cursorCol++;
    } else if (m_cursorRow + 1 < (int)m_lines.size()) {
        m_cursorRow++;
        m_cursorCol = 0;
    }
}

void TuiBuffer::moveHome() {
    m_cursorCol = 0;
}

void TuiBuffer::moveEnd() {
    ensureCursorValid();
    m_cursorCol = m_lines[m_cursorRow].size();
}

void TuiBuffer::movePageUp(int pageSize) {
    m_cursorRow = std::max(0, m_cursorRow - pageSize);
    m_cursorCol = std::min(m_cursorCol, (int)m_lines[m_cursorRow].size());
}

void TuiBuffer::movePageDown(int pageSize) {
    m_cursorRow = std::min((int)m_lines.size() - 1, m_cursorRow + pageSize);
    m_cursorCol = std::min(m_cursorCol, (int)m_lines[m_cursorRow].size());
}

bool TuiBuffer::find(const QString &query, bool forward) {
    if (query.isEmpty() || m_lines.empty()) return false;

    int totalLines = (int)m_lines.size();
    int startRow = m_cursorRow;
    int startCol = forward ? m_cursorCol + 1 : m_cursorCol - 1;

    for (int step = 0; step < totalLines; ++step) {
        int r = (forward ? (startRow + step) : (startRow - step + totalLines)) % totalLines;
        const QString &l = m_lines[r];

        int foundIdx = -1;
        if (r == startRow && step == 0) {
            if (forward) {
                if (startCol < l.size()) {
                    foundIdx = l.indexOf(query, startCol, Qt::CaseInsensitive);
                }
            } else {
                if (startCol >= 0) {
                    foundIdx = l.lastIndexOf(query, startCol, Qt::CaseInsensitive);
                }
            }
        } else {
            foundIdx = forward ? l.indexOf(query, 0, Qt::CaseInsensitive)
                               : l.lastIndexOf(query, -1, Qt::CaseInsensitive);
        }

        if (foundIdx >= 0) {
            m_cursorRow = r;
            m_cursorCol = foundIdx;
            return true;
        }
    }
    return false;
}

void TuiBuffer::duplicateLine() {
    ensureCursorValid();
    QString cur = m_lines[m_cursorRow];
    m_lines.insert(m_lines.begin() + m_cursorRow + 1, cur);
    m_cursorRow++;
    m_modified = true;
}

void TuiBuffer::toggleComment() {
    ensureCursorValid();
    QString &cur = m_lines[m_cursorRow];
    QString lang = languageName().toLower();
    QString prefix = QStringLiteral("// ");
    if (lang.contains("python") || lang.contains("bash") || lang.contains("shell")
        || lang.contains("ruby") || lang.contains("perl") || lang.contains("yaml")
        || lang.contains("dockerfile") || lang.contains("r") || lang.contains("makefile")
        || lang.contains("cmake") || lang.contains("toml")) {
        prefix = QStringLiteral("# ");
    } else if (lang.contains("sql") || lang.contains("lua") || lang.contains("haskell")) {
        prefix = QStringLiteral("-- ");
    } else if (lang.contains("ini") || lang.contains("asm")) {
        prefix = QStringLiteral("; ");
    }

    QString trimmedPrefix = prefix.trimmed();
    QString trimmedLine = cur.trimmed();

    if (trimmedLine.startsWith(trimmedPrefix)) {
        // Uncomment
        int idx = cur.indexOf(trimmedPrefix);
        if (idx >= 0) {
            int len = trimmedPrefix.size();
            if (idx + len < cur.size() && cur[idx + len] == ' ') {
                ++len;
            }
            cur.remove(idx, len);
            m_cursorCol = std::max(0, m_cursorCol - len);
        }
    } else {
        // Comment
        int spaces = 0;
        while (spaces < cur.size() && cur[spaces].isSpace()) {
            ++spaces;
        }
        cur.insert(spaces, prefix);
        m_cursorCol += prefix.size();
    }
    m_modified = true;
}

} // namespace UberPad
