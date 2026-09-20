#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QLabel>
#include <QToolButton>
#include <QPointer>
#include "../core/RemoteClient.h"

namespace UberPad {

class RemoteWorkspaceWidget : public QWidget {
    Q_OBJECT

public:
    explicit RemoteWorkspaceWidget(QWidget *parent = nullptr);
    ~RemoteWorkspaceWidget() override = default;

    bool isConnected() const { return m_isConnected; }
    RemoteConfig currentConfig() const { return m_config; }

public slots:
    void connectToServer(const RemoteConfig &config);
    void disconnectFromServer();
    void refreshCurrentDirectory();

signals:
    void remoteFileOpened(const QString &remotePath, const QByteArray &data, const RemoteConfig &config);
    void connectionStateChanged(bool connected);

private slots:
    void onConnectClicked();
    void onItemExpanded(QTreeWidgetItem *item);
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);
    void onContextMenu(const QPoint &pos);
    void onNewFile();
    void onNewFolder();
    void onDeleteItem();

private:
    void setupUi();
    void fetchDirectory(QTreeWidgetItem *parentItem, const QString &path);
    void populateChildren(QTreeWidgetItem *parentItem, const QList<RemoteItem> &items);
    QTreeWidgetItem* createItem(const RemoteItem &item);
    QString selectedFolderPath() const;

    RemoteConfig m_config;
    RemoteClient m_client;
    bool m_isConnected = false;

    QLabel *m_statusLabel;
    QToolButton *m_connectBtn;
    QToolButton *m_disconnectBtn;
    QToolButton *m_refreshBtn;
    QToolButton *m_newFileBtn;
    QToolButton *m_newFolderBtn;
    QToolButton *m_deleteBtn;
    QTreeWidget *m_treeWidget;
};

} // namespace UberPad
