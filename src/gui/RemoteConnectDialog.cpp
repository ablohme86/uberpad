#include "RemoteConnectDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>

namespace UberPad {

RemoteConnectDialog::RemoteConnectDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Connect to Remote Workspace (SFTP / FTP)"));
    resize(420, 260);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    QFormLayout *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);

    m_protocolCombo = new QComboBox(this);
    m_protocolCombo->addItem(QStringLiteral("SFTP (SSH File Transfer Protocol)"), (int)RemoteConfig::SFTP);
    m_protocolCombo->addItem(QStringLiteral("FTP (File Transfer Protocol)"), (int)RemoteConfig::FTP);
    m_protocolCombo->addItem(QStringLiteral("FTPS (FTP over SSL/TLS)"), (int)RemoteConfig::FTPS);

    m_hostEdit = new QLineEdit(this);
    m_hostEdit->setPlaceholderText(QStringLiteral("e.g. server.example.com or 192.168.1.100"));

    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(22);

    m_userEdit = new QLineEdit(this);
    m_userEdit->setPlaceholderText(QStringLiteral("Username"));

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(QStringLiteral("Password"));

    m_dirEdit = new QLineEdit(this);
    m_dirEdit->setText(QStringLiteral("/"));
    m_dirEdit->setPlaceholderText(QStringLiteral("Initial directory, e.g. /var/www or /home/user"));

    form->addRow(tr("Protocol:"), m_protocolCombo);
    form->addRow(tr("Host / Server:"), m_hostEdit);
    form->addRow(tr("Port:"), m_portSpin);
    form->addRow(tr("Username:"), m_userEdit);
    form->addRow(tr("Password:"), m_passwordEdit);
    form->addRow(tr("Remote Path:"), m_dirEdit);

    mainLayout->addLayout(form);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    btnBox->button(QDialogButtonBox::Ok)->setText(tr("Connect"));
    connect(btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addSpacing(10);
    mainLayout->addWidget(btnBox);

    connect(m_protocolCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RemoteConnectDialog::onProtocolChanged);
}

void RemoteConnectDialog::onProtocolChanged(int index) {
    auto proto = (RemoteConfig::Protocol)m_protocolCombo->itemData(index).toInt();
    if (proto == RemoteConfig::SFTP) {
        m_portSpin->setValue(22);
    } else {
        m_portSpin->setValue(21);
    }
}

RemoteConfig RemoteConnectDialog::config() const {
    RemoteConfig cfg;
    cfg.protocol = (RemoteConfig::Protocol)m_protocolCombo->currentData().toInt();
    cfg.host = m_hostEdit->text().trimmed();
    cfg.port = m_portSpin->value();
    cfg.username = m_userEdit->text().trimmed();
    cfg.password = m_passwordEdit->text();
    cfg.rootDir = m_dirEdit->text().trimmed();
    if (cfg.rootDir.isEmpty()) cfg.rootDir = QStringLiteral("/");
    return cfg;
}

} // namespace UberPad
