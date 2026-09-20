#include "GoToLineDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>

namespace UberPad {

GoToLineDialog::GoToLineDialog(int currentLine, int maxLines, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Go to Line"));
    setFixedSize(280, 140);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *infoLabel = new QLabel(tr("You are here: line %1 of %2").arg(currentLine).arg(maxLines), this);
    QLabel *promptLabel = new QLabel(tr("Line number (1 - %1):").arg(maxLines), this);

    m_lineSpinBox = new QSpinBox(this);
    m_lineSpinBox->setRange(1, std::max(1, maxLines));
    m_lineSpinBox->setValue(currentLine);
    m_lineSpinBox->selectAll();

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Go"));

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    layout->addWidget(infoLabel);
    layout->addWidget(promptLabel);
    layout->addWidget(m_lineSpinBox);
    layout->addSpacing(10);
    layout->addWidget(buttons);
}

int GoToLineDialog::selectedLine() const {
    return m_lineSpinBox->value();
}

} // namespace UberPad
