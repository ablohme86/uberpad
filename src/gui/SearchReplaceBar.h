#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>

namespace UberPad {

class CodeEditor;

class SearchReplaceBar : public QWidget {
    Q_OBJECT

public:
    explicit SearchReplaceBar(QWidget *parent = nullptr);
    ~SearchReplaceBar() override = default;

    void setEditor(CodeEditor *editor);
    void openFind();
    void openReplace();

signals:
    void closed();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onFindNext();
    void onFindPrev();
    void onReplace();
    void onReplaceAll();
    void onSearchTextChanged(const QString &text);
    void onOptionsChanged();

private:
    void updateMatchCount();
    bool doFind(bool forward);

    CodeEditor *m_editor = nullptr;

    QLineEdit *m_findEdit;
    QLineEdit *m_replaceEdit;
    QLabel *m_replaceLabel;
    QCheckBox *m_caseCheckBox;
    QCheckBox *m_wordCheckBox;
    QCheckBox *m_regexCheckBox;
    QLabel *m_statusLabel;

    QPushButton *m_findNextBtn;
    QPushButton *m_findPrevBtn;
    QPushButton *m_replaceBtn;
    QPushButton *m_replaceAllBtn;
    QPushButton *m_closeBtn;

    bool m_replaceMode = false;
};

} // namespace UberPad
