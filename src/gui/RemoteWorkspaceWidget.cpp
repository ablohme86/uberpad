#include "RemoteWorkspaceWidget.h"
#include "RemoteConnectDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QClipboard>
#include <QApplication>
#include <QStyle>
#include <QMenu>
#include <thread>

namespace UberPad {

static QString formatFileSize(qint64 bytes) {
    if (bytes < 1024) return QStringLiteral("%1 B").arg(bytes);
    if (bytes < 1024 * 1024) return QStringLiteral("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    if (bytes < 1024LL * 1024 * 1024) return QStringLiteral("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    return QStringLiteral("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
}

RemoteWorkspaceWidget::RemoteWorkspaceWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void RemoteWorkspaceWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    // Header / Toolbar
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(2);

    m_statusLabel = new QLabel(tr("Disconnected"), this);
    m_statusLabel->setStyleSheet(QStringLiteral("font-size: 11px; color: palette(placeholder-text);"));
    m_statusLabel->setWordWrap(false);

    m_connectBtn = new QToolButton(this);
    m_connectBtn->setText(tr("⚡"));
    m_connectBtn->setToolTip(tr("Connect to Remote Server (SFTP / FTP)"));

    m_disconnectBtn = new QToolButton(this);
    m_disconnectBtn->setText(tr("🔌"));
    m_disconnectBtn->setToolTip(tr("Disconnect"));
    m_disconnectBtn->setEnabled(false);

    m_refreshBtn = new QToolButton(this);
    m_refreshBtn->setText(tr("🔄"));
    m_refreshBtn->setToolTip(tr("Refresh Remote Directory"));
    m_refreshBtn->setEnabled(false);

    m_newFileBtn = new QToolButton(this);
    m_newFileBtn->setText(tr("📄"));
    m_newFileBtn->setToolTip(tr("New Remote File..."));
    m_newFileBtn->setEnabled(false);

    m_newFolderBtn = new QToolButton(this);
    m_newFolderBtn->setText(tr("📁"));
    m_newFolderBtn->setToolTip(tr("New Remote Directory..."));
    m_newFolderBtn->setEnabled(false);

    m_deleteBtn = new QToolButton(this);
    m_deleteBtn->setText(tr("🗑"));
    m_deleteBtn->setToolTip(tr("Delete Remote File or Directory"));
    m_deleteBtn->setEnabled(false);

    headerLayout->addWidget(m_statusLabel, 1);
    headerLayout->addWidget(m_connectBtn);
    headerLayout->addWidget(m_disconnectBtn);
    headerLayout->addWidget(m_refreshBtn);
    headerLayout->addWidget(m_newFileBtn);
    headerLayout->addWidget(m_newFolderBtn);
    headerLayout->addWidget(m_deleteBtn);

    mainLayout->addLayout(headerLayout);

    // Tree Widget
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderLabels({ tr("Remote Workspace"), tr("Size") });
    m_treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeWidget->setAnimated(true);

    mainLayout->addWidget(m_treeWidget, 1);

    connect(m_connectBtn, &QToolButton::clicked, this, &RemoteWorkspaceWidget::onConnectClicked);
    connect(m_disconnectBtn, &QToolButton::clicked, this, &RemoteWorkspaceWidget::disconnectFromServer);
    connect(m_refreshBtn, &QToolButton::clicked, this, &RemoteWorkspaceWidget::refreshCurrentDirectory);
    connect(m_newFileBtn, &QToolButton::clicked, this, &RemoteWorkspaceWidget::onNewFile);
    connect(m_newFolderBtn, &QToolButton::clicked, this, &RemoteWorkspaceWidget::onNewFolder);
    connect(m_deleteBtn, &QToolButton::clicked, this, &RemoteWorkspaceWidget::onDeleteItem);

    connect(m_treeWidget, &QTreeWidget::itemExpanded, this, &RemoteWorkspaceWidget::onItemExpanded);
    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked, this, &RemoteWorkspaceWidget::onItemDoubleClicked);
    connect(m_treeWidget, &QTreeWidget::customContextMenuRequested, this, &RemoteWorkspaceWidget::onContextMenu);
}

void RemoteWorkspaceWidget::onConnectClicked() {
    RemoteConnectDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        connectToServer(dlg.config());
    }
}

void RemoteWorkspaceWidget::connectToServer(const RemoteConfig &config) {
    m_config = config;
    m_client.setConfig(config);

    m_treeWidget->clear();
    m_statusLabel->setText(tr("Connecting to %1...").arg(config.host));
    m_statusLabel->setToolTip(config.host);

    m_connectBtn->setEnabled(false);
    m_disconnectBtn->setEnabled(false);
    m_refreshBtn->setEnabled(false);

    QPointer<RemoteWorkspaceWidget> self(this);
    std::thread([self, cfg = config]() {
        RemoteClient client(cfg);
        QList<RemoteItem> items;
        QString error;
        bool ok = client.listDirectory(cfg.rootDir, items, error);

        QMetaObject::invokeMethod(self, [self, ok, items, error, cfg]() {
            if (!self) return;
            self->m_connectBtn->setEnabled(true);

            if (ok) {
                self->m_isConnected = true;
                self->m_disconnectBtn->setEnabled(true);
                self->m_refreshBtn->setEnabled(true);
                self->m_newFileBtn->setEnabled(true);
                self->m_newFolderBtn->setEnabled(true);
                self->m_deleteBtn->setEnabled(true);

                QString rootDisplay = QStringLiteral("%1://%2:%3%4")
                                          .arg(cfg.protocolString())
                                          .arg(cfg.host)
                                          .arg(cfg.port)
                                          .arg(cfg.rootDir);
                self->m_statusLabel->setText(cfg.host);
                self->m_statusLabel->setToolTip(rootDisplay);

                // Root item
                QTreeWidgetItem *rootItem = new QTreeWidgetItem(self->m_treeWidget);
                rootItem->setText(0, cfg.rootDir);
                rootItem->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_DriveNetIcon));
                rootItem->setData(0, Qt::UserRole, cfg.rootDir);
                rootItem->setData(0, Qt::UserRole + 1, true); // isDir
                rootItem->setData(0, Qt::UserRole + 2, true); // loaded

                self->populateChildren(rootItem, items);
                rootItem->setExpanded(true);

                emit self->connectionStateChanged(true);
            } else {
                self->m_isConnected = false;
                self->m_statusLabel->setText(tr("Connection failed"));
                QMessageBox::critical(self, tr("Connection Failed"),
                                      tr("Could not connect to %1:%2:\n%3").arg(cfg.host).arg(cfg.port).arg(error));
                emit self->connectionStateChanged(false);
            }
        });
    }).detach();
}

void RemoteWorkspaceWidget::disconnectFromServer() {
    m_isConnected = false;
    m_treeWidget->clear();
    m_statusLabel->setText(tr("Disconnected"));
    m_statusLabel->setToolTip(QString());

    m_disconnectBtn->setEnabled(false);
    m_refreshBtn->setEnabled(false);
    m_newFileBtn->setEnabled(false);
    m_newFolderBtn->setEnabled(false);
    m_deleteBtn->setEnabled(false);

    emit connectionStateChanged(false);
}

void RemoteWorkspaceWidget::refreshCurrentDirectory() {
    if (!m_isConnected) return;

    QTreeWidgetItem *current = m_treeWidget->currentItem();
    if (!current) {
        if (m_treeWidget->topLevelItemCount() > 0) {
            current = m_treeWidget->topLevelItem(0);
        }
    }

    if (!current) {
        connectToServer(m_config);
        return;
    }

    bool isDir = current->data(0, Qt::UserRole + 1).toBool();
    QTreeWidgetItem *targetItem = isDir ? current : current->parent();
    if (!targetItem) targetItem = m_treeWidget->topLevelItem(0);
    if (!targetItem) return;

    QString path = targetItem->data(0, Qt::UserRole).toString();
    fetchDirectory(targetItem, path);
}

void RemoteWorkspaceWidget::fetchDirectory(QTreeWidgetItem *parentItem, const QString &path) {
    if (!parentItem) return;

    parentItem->setData(0, Qt::UserRole + 2, true); // Mark loaded
    // Remove all children
    qDeleteAll(parentItem->takeChildren());

    // Show temporary loading item
    QTreeWidgetItem *loading = new QTreeWidgetItem(parentItem);
    loading->setText(0, tr("Loading..."));

    QPointer<RemoteWorkspaceWidget> self(this);
    std::thread([self, parentItem, path, cfg = m_config]() {
        RemoteClient client(cfg);
        QList<RemoteItem> items;
        QString error;
        bool ok = client.listDirectory(path, items, error);

        QMetaObject::invokeMethod(self, [self, parentItem, ok, items, error, path]() {
            if (!self) return;
            qDeleteAll(parentItem->takeChildren());

            if (ok) {
                self->populateChildren(parentItem, items);
            } else {
                QTreeWidgetItem *errItem = new QTreeWidgetItem(parentItem);
                errItem->setText(0, tr("[Error: %1]").arg(error));
            }
        });
    }).detach();
}

QTreeWidgetItem* RemoteWorkspaceWidget::createItem(const RemoteItem &item) {
    QTreeWidgetItem *treeItem = new QTreeWidgetItem();
    treeItem->setText(0, item.name);
    treeItem->setData(0, Qt::UserRole, item.fullPath);
    treeItem->setData(0, Qt::UserRole + 1, item.isDir);

    if (item.isDir) {
        treeItem->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_DirIcon));
        treeItem->setData(0, Qt::UserRole + 2, false); // Not loaded yet
        // Add dummy child to show expansion arrow
        QTreeWidgetItem *dummy = new QTreeWidgetItem(treeItem);
        dummy->setText(0, tr("..."));
    } else {
        treeItem->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_FileIcon));
        treeItem->setText(1, formatFileSize(item.size));
        treeItem->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
    }
    return treeItem;
}

void RemoteWorkspaceWidget::populateChildren(QTreeWidgetItem *parentItem, const QList<RemoteItem> &items) {
    for (const auto &it : items) {
        parentItem->addChild(createItem(it));
    }
}

void RemoteWorkspaceWidget::onItemExpanded(QTreeWidgetItem *item) {
    if (!item) return;
    bool isDir = item->data(0, Qt::UserRole + 1).toBool();
    bool loaded = item->data(0, Qt::UserRole + 2).toBool();
    if (isDir && !loaded) {
        QString path = item->data(0, Qt::UserRole).toString();
        fetchDirectory(item, path);
    }
}

void RemoteWorkspaceWidget::onItemDoubleClicked(QTreeWidgetItem *item, int column) {
    Q_UNUSED(column);
    if (!item) return;

    bool isDir = item->data(0, Qt::UserRole + 1).toBool();
    if (isDir) {
        item->setExpanded(!item->isExpanded());
        return;
    }

    QString remotePath = item->data(0, Qt::UserRole).toString();
    QString fileName = item->text(0);
    m_statusLabel->setText(tr("Downloading %1...").arg(fileName));

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QPointer<RemoteWorkspaceWidget> self(this);
    std::thread([self, remotePath, cfg = m_config]() {
        RemoteClient client(cfg);
        QByteArray data;
        QString error;
        bool ok = client.downloadFile(remotePath, data, error);

        QMetaObject::invokeMethod(self, [self, ok, remotePath, data, error, cfg]() {
            QApplication::restoreOverrideCursor();
            if (!self) return;

            if (ok) {
                self->m_statusLabel->setText(self->m_config.host);
                emit self->remoteFileOpened(remotePath, data, cfg);
            } else {
                self->m_statusLabel->setText(tr("Download failed"));
                QMessageBox::critical(self, tr("Download Error"),
                                      tr("Failed to download '%1':\n%2").arg(remotePath, error));
            }
        });
    }).detach();
}

QString RemoteWorkspaceWidget::selectedFolderPath() const {
    QTreeWidgetItem *item = m_treeWidget->currentItem();
    if (!item) {
        return m_config.rootDir;
    }
    bool isDir = item->data(0, Qt::UserRole + 1).toBool();
    if (isDir) {
        return item->data(0, Qt::UserRole).toString();
    }
    QTreeWidgetItem *parent = item->parent();
    if (parent) {
        return parent->data(0, Qt::UserRole).toString();
    }
    return m_config.rootDir;
}

void RemoteWorkspaceWidget::onNewFile() {
    if (!m_isConnected) return;

    QString folder = selectedFolderPath();
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("New Remote File"),
                                         tr("Create file in '%1':").arg(folder),
                                         QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    QString remotePath = folder;
    if (!remotePath.endsWith(QLatin1Char('/'))) remotePath.append(QLatin1Char('/'));
    remotePath.append(name.trimmed());

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QPointer<RemoteWorkspaceWidget> self(this);
    std::thread([self, remotePath, folder, cfg = m_config]() {
        RemoteClient client(cfg);
        QString error;
        bool ok = client.uploadFile(remotePath, QByteArray(), error);

        QMetaObject::invokeMethod(self, [self, ok, remotePath, folder, error, cfg]() {
            QApplication::restoreOverrideCursor();
            if (!self) return;

            if (ok) {
                self->refreshCurrentDirectory();
                emit self->remoteFileOpened(remotePath, QByteArray(), cfg);
            } else {
                QMessageBox::critical(self, tr("File Creation Failed"),
                                      tr("Could not create remote file '%1':\n%2").arg(remotePath, error));
            }
        });
    }).detach();
}

void RemoteWorkspaceWidget::onNewFolder() {
    if (!m_isConnected) return;

    QString folder = selectedFolderPath();
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("New Remote Directory"),
                                         tr("Create folder in '%1':").arg(folder),
                                         QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    QString remotePath = folder;
    if (!remotePath.endsWith(QLatin1Char('/'))) remotePath.append(QLatin1Char('/'));
    remotePath.append(name.trimmed());

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QPointer<RemoteWorkspaceWidget> self(this);
    std::thread([self, remotePath, error = QString(), cfg = m_config]() mutable {
        RemoteClient client(cfg);
        bool ok = client.createDirectory(remotePath, error);

        QMetaObject::invokeMethod(self, [self, ok, remotePath, error]() {
            QApplication::restoreOverrideCursor();
            if (!self) return;

            if (ok) {
                self->refreshCurrentDirectory();
            } else {
                QMessageBox::critical(self, tr("Directory Creation Failed"),
                                      tr("Could not create remote directory '%1':\n%2").arg(remotePath, error));
            }
        });
    }).detach();
}

void RemoteWorkspaceWidget::onDeleteItem() {
    if (!m_isConnected) return;

    QTreeWidgetItem *item = m_treeWidget->currentItem();
    if (!item) return;

    QString path = item->data(0, Qt::UserRole).toString();
    bool isDir = item->data(0, Qt::UserRole + 1).toBool();
    QString name = item->text(0);

    // Cannot delete root
    if (path == m_config.rootDir || item->parent() == nullptr) {
        QMessageBox::warning(this, tr("Delete"), tr("Cannot delete the workspace root directory."));
        return;
    }

    int ret = QMessageBox::question(this, tr("Confirm Delete"),
                                    tr("Are you sure you want to delete remote %1:\n'%2'?")
                                        .arg(isDir ? tr("folder") : tr("file"), path),
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QPointer<RemoteWorkspaceWidget> self(this);
    std::thread([self, path, isDir, cfg = m_config]() {
        RemoteClient client(cfg);
        QString error;
        bool ok = isDir ? client.removeDirectory(path, error) : client.removeFile(path, error);

        QMetaObject::invokeMethod(self, [self, ok, path, error]() {
            QApplication::restoreOverrideCursor();
            if (!self) return;

            if (ok) {
                self->refreshCurrentDirectory();
            } else {
                QMessageBox::critical(self, tr("Delete Failed"),
                                      tr("Could not delete '%1':\n%2").arg(path, error));
            }
        });
    }).detach();
}

void RemoteWorkspaceWidget::onContextMenu(const QPoint &pos) {
    if (!m_isConnected) return;

    QTreeWidgetItem *item = m_treeWidget->itemAt(pos);
    QString path = item ? item->data(0, Qt::UserRole).toString() : m_config.rootDir;
    bool isDir = item ? item->data(0, Qt::UserRole + 1).toBool() : true;

    QMenu menu(this);
    if (item && !isDir) {
        menu.addAction(tr("Open in Editor"), [this, item]() {
            onItemDoubleClicked(item, 0);
        });
        menu.addSeparator();
    }

    menu.addAction(tr("New Remote File..."), this, &RemoteWorkspaceWidget::onNewFile);
    menu.addAction(tr("New Remote Directory..."), this, &RemoteWorkspaceWidget::onNewFolder);

    menu.addSeparator();
    menu.addAction(tr("Upload Local File Here..."), [this, path, isDir]() {
        QString targetDir = isDir ? path : selectedFolderPath();
        QString localFile = QFileDialog::getOpenFileName(this, tr("Select Local File to Upload"));
        if (localFile.isEmpty()) return;

        QFile file(localFile);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, tr("File Error"), tr("Could not read local file."));
            return;
        }
        QByteArray data = file.readAll();
        file.close();

        QFileInfo fi(localFile);
        QString remotePath = targetDir;
        if (!remotePath.endsWith(QLatin1Char('/'))) remotePath.append(QLatin1Char('/'));
        remotePath.append(fi.fileName());

        QApplication::setOverrideCursor(Qt::WaitCursor);
        QPointer<RemoteWorkspaceWidget> self(this);
        std::thread([self, remotePath, data, cfg = m_config]() {
            RemoteClient client(cfg);
            QString error;
            bool ok = client.uploadFile(remotePath, data, error);

            QMetaObject::invokeMethod(self, [self, ok, remotePath, error]() {
                QApplication::restoreOverrideCursor();
                if (!self) return;

                if (ok) {
                    self->refreshCurrentDirectory();
                    self->m_statusLabel->setText(tr("Uploaded %1").arg(remotePath));
                } else {
                    QMessageBox::critical(self, tr("Upload Failed"),
                                          tr("Could not upload to '%1':\n%2").arg(remotePath, error));
                }
            });
        }).detach();
    });

    menu.addSeparator();
    menu.addAction(tr("Copy Remote Path"), [path]() {
        QApplication::clipboard()->setText(path);
    });

    menu.addAction(tr("Refresh"), this, &RemoteWorkspaceWidget::refreshCurrentDirectory);

    if (item && item->parent() != nullptr) {
        menu.addSeparator();
        menu.addAction(tr("Delete"), this, &RemoteWorkspaceWidget::onDeleteItem);
    }

    menu.exec(m_treeWidget->viewport()->mapToGlobal(pos));
}

} // namespace UberPad
