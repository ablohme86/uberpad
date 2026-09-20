#pragma once

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include "../core/RemoteClient.h"

namespace UberPad {

class RemoteConnectDialog : public QDialog {
    Q_OBJECT

public:
    explicit RemoteConnectDialog(QWidget *parent = nullptr);

    RemoteConfig config() const;

private slots:
    void onProtocolChanged(int index);

private:
    QComboBox *m_protocolCombo;
    QLineEdit *m_hostEdit;
    QSpinBox *m_portSpin;
    QLineEdit *m_userEdit;
    QLineEdit *m_passwordEdit;
    QLineEdit *m_dirEdit;
};

} // namespace UberPad
