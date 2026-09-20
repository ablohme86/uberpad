#include "SearchReplaceBar.h"
#include "CodeEditor.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRegularExpression>
#include <QKeyEvent>
#include <QTextCursor>

namespace UberPad {

SearchReplaceBar::SearchReplaceBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("SearchReplaceBar");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 6, 8, 6);
    mainLayout->setSpacing(4);

    // Row 1: Find Row
    QHBoxLayout *findRow = new QHBoxLayout();
    findRow->setSpacing(6);

    QLabel *findLabel = new QLabel(tr("Find:"), this);
    m_findEdit = new QLineEdit(this);
    m_findEdit->setPlaceholderText(tr("Search query..."));
    m_findEdit->setClearButtonEnabled(true);

    m_findNextBtn = new QPushButton(tr("Find Next"), this);
    m_findPrevBtn = new QPushButton(tr("Find Prev"), this);

    m_caseCheckBox = new QCheckBox(tr("Match Case"), this);
    m_wordCheckBox = new QCheckBox(tr("Whole Word"), this);
    m_regexCheckBox = new QCheckBox(tr("Regex"), this);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: #888888; margin-left: 8px; font-size: 11px;");

    m_closeBtn = new QPushButton(QStringLiteral("✕"), this);
    m_closeBtn->setFixedSize(22, 22);
    m_closeBtn->setFlat(true);

    findRow->addWidget(findLabel);
    findRow->addWidget(m_findEdit, 1);
    findRow->addWidget(m_findNextBtn);
    findRow->addWidget(m_findPrevBtn);
    findRow->addWidget(m_caseCheckBox);
    findRow->addWidget(m_wordCheckBox);
    findRow->addWidget(m_regexCheckBox);
    findRow->addWidget(m_statusLabel);
    findRow->addStretch();
    findRow->addWidget(m_closeBtn);

    // Row 2: Replace Row
    QHBoxLayout *replaceRow = new QHBoxLayout();
    replaceRow->setSpacing(6);

    m_replaceLabel = new QLabel(tr("Replace:"), this);
    m_replaceEdit = new QLineEdit(this);
    m_replaceEdit->setPlaceholderText(tr("Replace with..."));
    m_replaceEdit->setClearButtonEnabled(true);

    m_replaceBtn = new QPushButton(tr("Replace"), this);
    m_replaceAllBtn = new QPushButton(tr("Replace All"), this);

    replaceRow->addWidget(m_replaceLabel);
    replaceRow->addWidget(m_replaceEdit, 1);
    replaceRow->addWidget(m_replaceBtn);
    replaceRow->addWidget(m_replaceAllBtn);
    replaceRow->addStretch();

    mainLayout->addLayout(findRow);
    mainLayout->addLayout(replaceRow);

    // Style the bar with a subtle border and background
    setStyleSheet(
        "#SearchReplaceBar {"
        "  background-color: palette(window);"
        "  border-top: 1px solid palette(mid);"
        "}"
        "QLineEdit {"
        "  padding: 3px 6px;"
        "  border-radius: 3px;"
        "  min-width: 200px;"
        "}"
        "QPushButton {"
        "  padding: 3px 8px;"
        "  border-radius: 3px;"
        "}"
    );

    connect(m_findEdit, &QLineEdit::textChanged, this, &SearchReplaceBar::onSearchTextChanged);
    connect(m_findEdit, &QLineEdit::returnPressed, this, &SearchReplaceBar::onFindNext);
    connect(m_replaceEdit, &QLineEdit::returnPressed, this, &SearchReplaceBar::onReplace);
    connect(m_findNextBtn, &QPushButton::clicked, this, &SearchReplaceBar::onFindNext);
    connect(m_findPrevBtn, &QPushButton::clicked, this, &SearchReplaceBar::onFindPrev);
    connect(m_replaceBtn, &QPushButton::clicked, this, &SearchReplaceBar::onReplace);
    connect(m_replaceAllBtn, &QPushButton::clicked, this, &SearchReplaceBar::onReplaceAll);
    connect(m_closeBtn, &QPushButton::clicked, this, [this]() {
        if (m_editor) {
            m_editor->clearSearchHighlight();
            m_editor->setFocus();
        }
        hide();
        emit closed();
    });

    connect(m_caseCheckBox, &QCheckBox::toggled, this, &SearchReplaceBar::onOptionsChanged);
    connect(m_wordCheckBox, &QCheckBox::toggled, this, &SearchReplaceBar::onOptionsChanged);
    connect(m_regexCheckBox, &QCheckBox::toggled, this, &SearchReplaceBar::onOptionsChanged);

    hide();
}

void SearchReplaceBar::setEditor(CodeEditor *editor) {
    if (m_editor && m_editor != editor) {
        m_editor->clearSearchHighlight();
    }
    m_editor = editor;
    if (isVisible() && m_editor && !m_findEdit->text().isEmpty()) {
        onSearchTextChanged(m_findEdit->text());
    }
}

void SearchReplaceBar::openFind() {
    m_replaceMode = false;
    m_replaceLabel->hide();
    m_replaceEdit->hide();
    m_replaceBtn->hide();
    m_replaceAllBtn->hide();
    show();

    if (m_editor && m_editor->textCursor().hasSelection()) {
        m_findEdit->setText(m_editor->textCursor().selectedText());
    }
    m_findEdit->selectAll();
    m_findEdit->setFocus();
    updateMatchCount();
}

void SearchReplaceBar::openReplace() {
    m_replaceMode = true;
    m_replaceLabel->show();
    m_replaceEdit->show();
    m_replaceBtn->show();
    m_replaceAllBtn->show();
    show();

    if (m_editor && m_editor->textCursor().hasSelection()) {
        m_findEdit->setText(m_editor->textCursor().selectedText());
    }
    m_findEdit->selectAll();
    m_findEdit->setFocus();
    updateMatchCount();
}

void SearchReplaceBar::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        if (m_editor) {
            m_editor->clearSearchHighlight();
            m_editor->setFocus();
        }
        hide();
        emit closed();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void SearchReplaceBar::onSearchTextChanged(const QString &text) {
    if (m_editor) {
        m_editor->setSearchHighlight(text, m_caseCheckBox->isChecked(),
                                    m_wordCheckBox->isChecked(), m_regexCheckBox->isChecked());
    }
    updateMatchCount();
}

void SearchReplaceBar::onOptionsChanged() {
    onSearchTextChanged(m_findEdit->text());
}

void SearchReplaceBar::updateMatchCount() {
    if (!m_editor || m_findEdit->text().isEmpty()) {
        m_statusLabel->clear();
        return;
    }

    QString text = m_findEdit->text();
    QString pattern = m_regexCheckBox->isChecked() ? text : QRegularExpression::escape(text);
    if (m_wordCheckBox->isChecked()) {
        pattern = QStringLiteral("\\b%1\\b").arg(pattern);
    }

    QRegularExpression::PatternOptions opts = QRegularExpression::NoPatternOption;
    if (!m_caseCheckBox->isChecked()) {
        opts |= QRegularExpression::CaseInsensitiveOption;
    }

    QRegularExpression rx(pattern, opts);
    if (!rx.isValid()) {
        m_statusLabel->setText(tr("Invalid Regex"));
        m_statusLabel->setStyleSheet("color: #ff5555;");
        return;
    }

    int count = 0;
    int currentMatch = 0;
    int cursorPosition = m_editor->textCursor().position();
    auto it = rx.globalMatch(m_editor->toPlainText());
    while (it.hasNext()) {
        auto match = it.next();
        ++count;
        if (match.capturedEnd() <= cursorPosition) {
            currentMatch = count;
        }
    }

    if (count == 0) {
        m_statusLabel->setText(tr("No match"));
        m_statusLabel->setStyleSheet("color: #ffaa00;");
    } else {
        m_statusLabel->setText(tr("%1 of %2 matches").arg(currentMatch).arg(count));
        m_statusLabel->setStyleSheet("color: #00aa00;");
    }
}

bool SearchReplaceBar::doFind(bool forward) {
    if (!m_editor) return false;
    QString text = m_findEdit->text();
    if (text.isEmpty()) return false;

    QTextDocument::FindFlags flags;
    if (!forward) flags |= QTextDocument::FindBackward;
    if (m_caseCheckBox->isChecked()) flags |= QTextDocument::FindCaseSensitively;
    if (m_wordCheckBox->isChecked()) flags |= QTextDocument::FindWholeWords;

    bool found = false;
    if (m_regexCheckBox->isChecked()) {
        QRegularExpression::PatternOptions opts = QRegularExpression::NoPatternOption;
        if (!m_caseCheckBox->isChecked()) opts |= QRegularExpression::CaseInsensitiveOption;
        QRegularExpression rx(text, opts);
        if (rx.isValid()) {
            found = m_editor->find(rx, flags);
            if (!found) {
                // Wrap around
                QTextCursor cur = m_editor->textCursor();
                cur.movePosition(forward ? QTextCursor::Start : QTextCursor::End);
                m_editor->setTextCursor(cur);
                found = m_editor->find(rx, flags);
            }
        }
    } else {
        found = m_editor->find(text, flags);
        if (!found) {
            // Wrap around
            QTextCursor cur = m_editor->textCursor();
            cur.movePosition(forward ? QTextCursor::Start : QTextCursor::End);
            m_editor->setTextCursor(cur);
            found = m_editor->find(text, flags);
        }
    }

    updateMatchCount();
    return found;
}

void SearchReplaceBar::onFindNext() {
    doFind(true);
}

void SearchReplaceBar::onFindPrev() {
    doFind(false);
}

void SearchReplaceBar::onReplace() {
    if (!m_editor) return;
    QTextCursor cur = m_editor->textCursor();
    if (cur.hasSelection()) {
        cur.insertText(m_replaceEdit->text());
    }
    doFind(true);
}

void SearchReplaceBar::onReplaceAll() {
    if (!m_editor || m_findEdit->text().isEmpty()) return;

    QString text = m_findEdit->text();
    QString replaceWith = m_replaceEdit->text();

    QString pattern = m_regexCheckBox->isChecked() ? text : QRegularExpression::escape(text);
    if (m_wordCheckBox->isChecked()) {
        pattern = QStringLiteral("\\b%1\\b").arg(pattern);
    }

    QRegularExpression::PatternOptions opts = QRegularExpression::NoPatternOption;
    if (!m_caseCheckBox->isChecked()) opts |= QRegularExpression::CaseInsensitiveOption;
    QRegularExpression rx(pattern, opts);
    if (!rx.isValid()) return;

    QTextCursor cur = m_editor->textCursor();
    cur.beginEditBlock();

    QString docText = m_editor->toPlainText();
    int count = 0;
    auto it = rx.globalMatch(docText);
    while (it.hasNext()) {
        it.next();
        ++count;
    }

    docText.replace(rx, replaceWith);
    m_editor->setPlainText(docText);

    cur.endEditBlock();

    m_statusLabel->setText(tr("Replaced %1 occurrences").arg(count));
    m_statusLabel->setStyleSheet("color: #00aa00;");
    onSearchTextChanged(m_findEdit->text());
}

} // namespace UberPad
