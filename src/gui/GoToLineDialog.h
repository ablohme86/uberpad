#pragma once

#include <QDialog>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>

namespace UberPad {

class GoToLineDialog : public QDialog {
    Q_OBJECT

public:
    explicit GoToLineDialog(int currentLine, int maxLines, QWidget *parent = nullptr);
    int selectedLine() const;

private:
    QSpinBox *m_lineSpinBox;
};

} // namespace UberPad
