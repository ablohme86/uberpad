#include "CodeEditor.h"
#include "../core/Config.h"
#include "../core/SyntaxManager.h"

#include <QPainter>
#include <QTextBlock>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QScrollBar>
#include <QFontDatabase>
#include <QRegularExpression>
#include <cmath>

namespace UberPad {

// LineNumberArea Implementation
LineNumberArea::LineNumberArea(CodeEditor *editor)
    : QWidget(editor)
    , m_editor(editor)
{
}

QSize LineNumberArea::sizeHint() const {
    return QSize(m_editor->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event) {
    m_editor->lineNumberAreaPaintEvent(event);
}

void LineNumberArea::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        // Select the clicked line
        int y = event->position().toPoint().y();
        QTextBlock block = m_editor->firstVisibleBlock();
        int top = (int)m_editor->blockBoundingGeometry(block).translated(m_editor->contentOffset()).top();
        int bottom = top + (int)m_editor->blockBoundingRect(block).height();

        while (block.isValid() && top <= y) {
            if (y >= top && y <= bottom) {
                QTextCursor cursor(block);
                cursor.movePosition(QTextCursor::StartOfBlock);
                cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                m_editor->setTextCursor(cursor);
                break;
            }
            block = block.next();
            top = bottom;
            bottom = top + (int)m_editor->blockBoundingRect(block).height();
        }
    }
    QWidget::mousePressEvent(event);
}

// CodeEditor Implementation
CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_lineNumberArea(new LineNumberArea(this))
    , m_highlighter(new KSyntaxHighlighting::SyntaxHighlighter(document()))
{
    // Configure editor defaults
    const auto &cfg = Config::instance();
    m_baseFontSize = cfg.fontSize();

    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setFamily(cfg.fontFamily());
    font.setPointSize(m_baseFontSize);
    setFont(font);

    setTabStopDistance(fontMetrics().horizontalAdvance(QLatin1Char(' ')) * cfg.tabSize());
    setLineWrapMode(cfg.wordWrap() ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);

    // Initial syntax & theme
    m_theme = SyntaxManager::instance().theme(cfg.themeName());
    m_highlighter->setTheme(m_theme);
    setDefinition(SyntaxManager::instance().defaultDefinition());

    applyThemeColors();

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLineAndBrackets);
    connect(this, &CodeEditor::textChanged, this, &CodeEditor::onTextChanged);

    updateLineNumberAreaWidth(0);
    highlightCurrentLineAndBrackets();
}

int CodeEditor::lineNumberAreaWidth() {
    if (!Config::instance().showLineNumbers()) return 0;
    int digits = 1;
    int maxLines = std::max(1, blockCount());
    while (maxLines >= 10) {
        maxLines /= 10;
        ++digits;
    }
    digits = std::max(3, digits);
    int space = 18 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy) {
        m_lineNumberArea->scroll(0, dy);
    } else {
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}

void CodeEditor::resizeEvent(QResizeEvent *event) {
    QPlainTextEdit::resizeEvent(event);
    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event) {
    if (!Config::instance().showLineNumbers()) return;

    QPainter painter(m_lineNumberArea);

    QColor bgColor = m_theme.editorColor(KSyntaxHighlighting::Theme::IconBorder);
    if (!bgColor.isValid()) {
        bgColor = (qGray(m_theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor)) < 128)
                  ? QColor(32, 34, 40) : QColor(240, 240, 240);
    }
    painter.fillRect(event->rect(), bgColor);

    // Separator line on the right edge of line number gutter
    QColor sepColor = m_theme.editorColor(KSyntaxHighlighting::Theme::Separator);
    if (!sepColor.isValid()) {
        sepColor = bgColor.lighter(130);
    }
    painter.setPen(sepColor);
    painter.drawLine(m_lineNumberArea->width() - 1, event->rect().top(),
                     m_lineNumberArea->width() - 1, event->rect().bottom());

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int)blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int)blockBoundingRect(block).height();

    int currentLine = textCursor().blockNumber();

    QColor normalColor = m_theme.editorColor(KSyntaxHighlighting::Theme::LineNumbers);
    if (!normalColor.isValid()) {
        normalColor = QColor(120, 120, 130);
    }

    QColor activeColor = m_theme.editorColor(KSyntaxHighlighting::Theme::CurrentLineNumber);
    if (!activeColor.isValid()) {
        activeColor = (qGray(m_theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor)) < 128)
                      ? QColor(240, 240, 100) : QColor(30, 30, 30);
    }

    QFont normalFont = font();
    QFont boldFont = font();
    boldFont.setBold(true);

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            bool isCurrent = (blockNumber == currentLine);

            painter.setPen(isCurrent ? activeColor : normalColor);
            painter.setFont(isCurrent ? boldFont : normalFont);
            painter.drawText(0, top, m_lineNumberArea->width() - 8, fontMetrics().height(),
                             Qt::AlignRight | Qt::AlignVCenter, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int)blockBoundingRect(block).height();
        ++blockNumber;
    }
}

void CodeEditor::applyThemeColors() {
    QPalette pal = palette();

    QColor bg = m_theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor);
    if (!bg.isValid()) bg = QColor(30, 30, 35);
    pal.setColor(QPalette::Base, bg);

    QColor fg = m_theme.textColor(KSyntaxHighlighting::Theme::TextStyle::Normal);
    if (!fg.isValid()) fg = QColor(230, 230, 230);
    pal.setColor(QPalette::Text, fg);

    QColor selBg = m_theme.editorColor(KSyntaxHighlighting::Theme::TextSelection);
    if (!selBg.isValid()) selBg = QColor(50, 75, 120);
    pal.setColor(QPalette::Highlight, selBg);

    setPalette(pal);
    m_lineNumberArea->update();
    highlightCurrentLineAndBrackets();
}

void CodeEditor::setTheme(const KSyntaxHighlighting::Theme &theme) {
    m_theme = theme;
    m_highlighter->setTheme(m_theme);
    applyThemeColors();
    m_highlighter->rehighlight();
}

void CodeEditor::setDefinition(const KSyntaxHighlighting::Definition &def) {
    m_definition = def;
    m_highlighter->setDefinition(m_definition);
    m_highlighter->rehighlight();
    emit languageChanged(languageName());
}

QString CodeEditor::languageName() const {
    if (m_definition.isValid() && !m_definition.name().isEmpty()) {
        return m_definition.translatedName().isEmpty() ? m_definition.name() : m_definition.translatedName();
    }
    return QStringLiteral("Plain Text");
}

QString CodeEditor::fileName() const {
    if (m_filePath.isEmpty()) return QStringLiteral("Untitled");
    return QFileInfo(m_filePath).fileName();
}

QString CodeEditor::lineEndingName() const {
    return (m_lineEnding == LineEnding::CRLF) ? QStringLiteral("Windows (CRLF)") : QStringLiteral("Unix (LF)");
}

void CodeEditor::setLineEnding(LineEnding le) {
    if (m_lineEnding != le) {
        m_lineEnding = le;
        document()->setModified(true);
        emit lineEndingChanged(lineEndingName());
    }
}

bool CodeEditor::loadFromFile(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QByteArray rawData = file.readAll();
    file.close();

    // Detect line endings
    if (rawData.contains("\r\n")) {
        m_lineEnding = LineEnding::CRLF;
    } else {
        m_lineEnding = LineEnding::LF;
    }

    QString text = QString::fromUtf8(rawData);
    setPlainText(text);
    document()->setModified(false);
    m_filePath = filePath;

    // Auto-detect language
    auto def = SyntaxManager::instance().definitionForFileName(filePath);
    if (!def.isValid()) {
        def = SyntaxManager::instance().defaultDefinition();
    }
    setDefinition(def);

    emit lineEndingChanged(lineEndingName());
    return true;
}

bool CodeEditor::saveToFile(const QString &filePath) {
    QString target = filePath.isEmpty() ? m_filePath : filePath;
    if (target.isEmpty()) return false;

    QFile file(target);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    QString text = toPlainText();
    if (m_lineEnding == LineEnding::CRLF) {
        // Ensure CRLF
        text.replace(QLatin1String("\r\n"), QLatin1String("\n"));
        text.replace(QLatin1Char('\n'), QLatin1String("\r\n"));
    } else {
        // Ensure LF
        text.replace(QLatin1String("\r\n"), QLatin1String("\n"));
    }

    QByteArray data = text.toUtf8();
    file.write(data);
    file.close();

    m_filePath = target;
    document()->setModified(false);

    // Refresh syntax definition on save in case file extension was added/changed
    auto def = SyntaxManager::instance().definitionForFileName(target);
    if (def.isValid()) {
        setDefinition(def);
    }

    emit fileSaved(m_filePath);
    return true;
}

void CodeEditor::onTextChanged() {
    // Keep modified status in sync
}

void CodeEditor::highlightCurrentLineAndBrackets() {
    m_cursorSelections.clear();

    if (!isReadOnly() && Config::instance().highlightCurrentLine()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = m_theme.editorColor(KSyntaxHighlighting::Theme::CurrentLine);
        if (!lineColor.isValid()) {
            lineColor = (qGray(m_theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor)) < 128)
                        ? QColor(45, 48, 58) : QColor(235, 238, 245);
        }
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        m_cursorSelections.append(selection);
    }

    if (Config::instance().bracketMatching()) {
        matchParentheses();
    }

    // Combine cursor selections and search selections
    QList<QTextEdit::ExtraSelection> all = m_cursorSelections;
    all.append(m_searchSelections);
    setExtraSelections(all);

    m_lineNumberArea->update();
}

void CodeEditor::matchParentheses() {
    QTextCursor cur = textCursor();
    int pos = cur.position();
    QString docText = toPlainText();
    if (docText.isEmpty()) return;

    auto isBracket = [](QChar c) {
        return c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']';
    };

    int matchPos = -1;
    int bracketPos = -1;

    // Check character before or after cursor
    if (pos > 0 && isBracket(docText[pos - 1])) {
        bracketPos = pos - 1;
    } else if (pos < docText.size() && isBracket(docText[pos])) {
        bracketPos = pos;
    }

    if (bracketPos < 0) return;

    QChar ch = docText[bracketPos];
    QChar target;
    int step = 1;

    if (ch == '(') { target = ')'; step = 1; }
    else if (ch == ')') { target = '('; step = -1; }
    else if (ch == '{') { target = '}'; step = 1; }
    else if (ch == '}') { target = '{'; step = -1; }
    else if (ch == '[') { target = ']'; step = 1; }
    else if (ch == ']') { target = '['; step = -1; }

    int depth = 1;
    for (int i = bracketPos + step; i >= 0 && i < docText.size(); i += step) {
        if (docText[i] == ch) ++depth;
        else if (docText[i] == target) {
            --depth;
            if (depth == 0) {
                matchPos = i;
                break;
            }
        }
    }

    if (matchPos >= 0) {
        QColor matchColor = m_theme.editorColor(KSyntaxHighlighting::Theme::BracketMatching);
        if (!matchColor.isValid()) {
            matchColor = QColor(100, 160, 220);
        }

        auto makeSelection = [this, &matchColor](int p) {
            QTextEdit::ExtraSelection sel;
            sel.cursor = textCursor();
            sel.cursor.setPosition(p);
            sel.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            sel.format.setBackground(matchColor.lighter(130));
            sel.format.setFontWeight(QFont::Bold);
            return sel;
        };

        m_cursorSelections.append(makeSelection(bracketPos));
        m_cursorSelections.append(makeSelection(matchPos));
    }
}

void CodeEditor::setSearchHighlight(const QString &text, bool caseSensitive, bool wholeWord, bool isRegex) {
    m_searchSelections.clear();
    if (text.isEmpty()) {
        highlightCurrentLineAndBrackets();
        return;
    }

    QColor searchColor = m_theme.editorColor(KSyntaxHighlighting::Theme::SearchHighlight);
    if (!searchColor.isValid()) {
        searchColor = QColor(255, 215, 0, 120);
    }

    QString pattern = text;
    if (!isRegex) {
        pattern = QRegularExpression::escape(text);
    }
    if (wholeWord) {
        pattern = QStringLiteral("\\b%1\\b").arg(pattern);
    }

    QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
    if (!caseSensitive) {
        options |= QRegularExpression::CaseInsensitiveOption;
    }

    QRegularExpression rx(pattern, options);
    if (!rx.isValid()) return;

    QString docText = toPlainText();
    auto it = rx.globalMatch(docText);
    while (it.hasNext()) {
        auto match = it.next();
        QTextEdit::ExtraSelection sel;
        sel.cursor = textCursor();
        sel.cursor.setPosition(match.capturedStart());
        sel.cursor.setPosition(match.capturedEnd(), QTextCursor::KeepAnchor);
        sel.format.setBackground(searchColor);
        m_searchSelections.append(sel);
    }

    highlightCurrentLineAndBrackets();
}

void CodeEditor::clearSearchHighlight() {
    m_searchSelections.clear();
    highlightCurrentLineAndBrackets();
}

void CodeEditor::keyPressEvent(QKeyEvent *event) {
    // Smart Indentation on Enter
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) && Config::instance().autoIndent()) {
        QTextCursor cur = textCursor();
        QString blockText = cur.block().text();
        int curPosInBlock = cur.positionInBlock();
        QString textBeforeCursor = blockText.left(curPosInBlock);

        // Calculate current indentation
        int spaces = 0;
        while (spaces < textBeforeCursor.size() && textBeforeCursor[spaces].isSpace()) {
            ++spaces;
        }
        QString indent = textBeforeCursor.left(spaces);

        QString trimmed = textBeforeCursor.trimmed();
        bool addExtraIndent = false;
        if (!trimmed.isEmpty()) {
            QChar lastChar = trimmed.back();
            if (lastChar == '{' || lastChar == ':' || lastChar == '(' || lastChar == '[') {
                addExtraIndent = true;
            }
        }

        QPlainTextEdit::keyPressEvent(event);

        // Insert indentation
        if (addExtraIndent) {
            indent += Config::instance().useSpacesForTabs()
                      ? QString(Config::instance().tabSize(), ' ')
                      : QStringLiteral("\t");
        }
        if (!indent.isEmpty()) {
            insertPlainText(indent);
        }
        return;
    }

    // Tab / Shift+Tab indent/unindent
    if (event->key() == Qt::Key_Tab) {
        if (textCursor().hasSelection()) {
            indentSelection();
            return;
        } else if (Config::instance().useSpacesForTabs()) {
            insertPlainText(QString(Config::instance().tabSize(), ' '));
            return;
        }
    } else if (event->key() == Qt::Key_Backtab) {
        unindentSelection();
        return;
    }

    // Ctrl+D: Duplicate line or selection
    if (event->key() == Qt::Key_D && (event->modifiers() & Qt::ControlModifier)) {
        duplicateLineOrSelection();
        return;
    }

    // Ctrl+/: Toggle comment
    if (event->key() == Qt::Key_Slash && (event->modifiers() & Qt::ControlModifier)) {
        toggleComment();
        return;
    }

    // Alt+Up / Alt+Down: Move line up/down
    if ((event->modifiers() & Qt::AltModifier) && event->key() == Qt::Key_Up) {
        moveLineUp();
        return;
    }
    if ((event->modifiers() & Qt::AltModifier) && event->key() == Qt::Key_Down) {
        moveLineDown();
        return;
    }

    QPlainTextEdit::keyPressEvent(event);
}

void CodeEditor::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        int numDegrees = event->angleDelta().y() / 8;
        int numSteps = numDegrees / 15;
        if (numSteps > 0) {
            zoomIn(numSteps);
        } else if (numSteps < 0) {
            zoomOut(-numSteps);
        }
        event->accept();
        return;
    }
    QPlainTextEdit::wheelEvent(event);
}

void CodeEditor::zoomIn(int range) {
    m_currentZoomLevel += range;
    QFont f = font();
    f.setPointSize(std::max(6, m_baseFontSize + m_currentZoomLevel));
    setFont(f);
    setTabStopDistance(fontMetrics().horizontalAdvance(QLatin1Char(' ')) * Config::instance().tabSize());
    updateLineNumberAreaWidth(0);
}

void CodeEditor::zoomOut(int range) {
    zoomIn(-range);
}

void CodeEditor::resetZoom() {
    m_currentZoomLevel = 0;
    QFont f = font();
    f.setPointSize(m_baseFontSize);
    setFont(f);
    setTabStopDistance(fontMetrics().horizontalAdvance(QLatin1Char(' ')) * Config::instance().tabSize());
    updateLineNumberAreaWidth(0);
}

void CodeEditor::duplicateLineOrSelection() {
    QTextCursor cur = textCursor();
    if (cur.hasSelection()) {
        QString sel = cur.selectedText();
        int end = cur.selectionEnd();
        cur.setPosition(end);
        cur.insertText(sel);
    } else {
        QTextBlock block = cur.block();
        QString line = block.text();
        cur.movePosition(QTextCursor::EndOfBlock);
        cur.insertText("\n" + line);
        setTextCursor(cur);
    }
}

void CodeEditor::moveLineUp() {
    QTextCursor cur = textCursor();
    int curLine = cur.blockNumber();
    if (curLine <= 0) return;

    cur.beginEditBlock();
    QTextBlock block = cur.block();
    QString text = block.text();
    int col = cur.positionInBlock();

    // Delete current line
    cur.movePosition(QTextCursor::StartOfBlock);
    cur.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
    if (!cur.hasSelection()) {
        // Last block
        cur.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    }
    cur.removeSelectedText();

    // Move to previous block
    cur.movePosition(QTextCursor::PreviousBlock);
    cur.movePosition(QTextCursor::StartOfBlock);
    cur.insertText(text + "\n");
    cur.movePosition(QTextCursor::PreviousBlock);
    cur.movePosition(QTextCursor::StartOfBlock);
    cur.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, col);

    cur.endEditBlock();
    setTextCursor(cur);
}

void CodeEditor::moveLineDown() {
    QTextCursor cur = textCursor();
    int curLine = cur.blockNumber();
    if (curLine >= blockCount() - 1) return;

    cur.beginEditBlock();
    QTextBlock block = cur.block();
    QString text = block.text();
    int col = cur.positionInBlock();

    // Delete current line
    cur.movePosition(QTextCursor::StartOfBlock);
    cur.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
    cur.removeSelectedText();

    // Move past next block
    cur.movePosition(QTextCursor::EndOfBlock);
    cur.insertText("\n" + text);
    cur.movePosition(QTextCursor::StartOfBlock);
    cur.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, col);

    cur.endEditBlock();
    setTextCursor(cur);
}

void CodeEditor::indentSelection() {
    QTextCursor cur = textCursor();
    cur.beginEditBlock();
    int start = cur.selectionStart();
    int end = cur.selectionEnd();

    QTextBlock startBlock = document()->findBlock(start);
    QTextBlock endBlock = document()->findBlock(end);

    QString tab = Config::instance().useSpacesForTabs()
                  ? QString(Config::instance().tabSize(), ' ')
                  : QStringLiteral("\t");

    while (startBlock.isValid()) {
        QTextCursor lineCur(startBlock);
        lineCur.movePosition(QTextCursor::StartOfBlock);
        lineCur.insertText(tab);

        if (startBlock == endBlock) break;
        startBlock = startBlock.next();
    }
    cur.endEditBlock();
}

void CodeEditor::unindentSelection() {
    QTextCursor cur = textCursor();
    cur.beginEditBlock();
    int start = cur.selectionStart();
    int end = cur.selectionEnd();

    QTextBlock startBlock = document()->findBlock(start);
    QTextBlock endBlock = document()->findBlock(end);

    int tabSize = Config::instance().tabSize();

    while (startBlock.isValid()) {
        QString text = startBlock.text();
        int toRemove = 0;
        if (text.startsWith(QLatin1Char('\t'))) {
            toRemove = 1;
        } else {
            while (toRemove < tabSize && toRemove < text.size() && text[toRemove] == QLatin1Char(' ')) {
                ++toRemove;
            }
        }

        if (toRemove > 0) {
            QTextCursor lineCur(startBlock);
            lineCur.movePosition(QTextCursor::StartOfBlock);
            lineCur.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, toRemove);
            lineCur.removeSelectedText();
        }

        if (startBlock == endBlock) break;
        startBlock = startBlock.next();
    }
    cur.endEditBlock();
}

QString CodeEditor::commentPrefix() const {
    QString lang = m_definition.name().toLower();
    if (lang.contains("python") || lang.contains("bash") || lang.contains("shell")
        || lang.contains("ruby") || lang.contains("perl") || lang.contains("yaml")
        || lang.contains("dockerfile") || lang.contains("r") || lang.contains("makefile")
        || lang.contains("cmake") || lang.contains("toml")) {
        return QStringLiteral("# ");
    }
    if (lang.contains("sql") || lang.contains("lua") || lang.contains("haskell")
        || lang.contains("ada") || lang.contains("vhdl")) {
        return QStringLiteral("-- ");
    }
    if (lang.contains("ini") || lang.contains("assembly") || lang.contains("asm")) {
        return QStringLiteral("; ");
    }
    // Default C-style comment
    return QStringLiteral("// ");
}

void CodeEditor::toggleComment() {
    QTextCursor cur = textCursor();
    cur.beginEditBlock();
    int start = cur.selectionStart();
    int end = cur.selectionEnd();

    QTextBlock startBlock = document()->findBlock(start);
    QTextBlock endBlock = document()->findBlock(end);

    QString prefix = commentPrefix();
    QString trimmedPrefix = prefix.trimmed();

    // Determine if all lines are already commented
    bool allCommented = true;
    QTextBlock block = startBlock;
    while (block.isValid()) {
        QString t = block.text().trimmed();
        if (!t.isEmpty() && !t.startsWith(trimmedPrefix)) {
            allCommented = false;
            break;
        }
        if (block == endBlock) break;
        block = block.next();
    }

    block = startBlock;
    while (block.isValid()) {
        QTextCursor lineCur(block);
        lineCur.movePosition(QTextCursor::StartOfBlock);
        QString t = block.text();

        if (allCommented) {
            // Uncomment
            int idx = t.indexOf(trimmedPrefix);
            if (idx >= 0) {
                lineCur.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, idx);
                int len = trimmedPrefix.size();
                if (idx + len < t.size() && t[idx + len] == ' ') {
                    ++len;
                }
                lineCur.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, len);
                lineCur.removeSelectedText();
            }
        } else {
            // Comment
            lineCur.insertText(prefix);
        }

        if (block == endBlock) break;
        block = block.next();
    }
    cur.endEditBlock();
}

} // namespace UberPad
