#pragma once

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QLabel>
#include <QToolButton>

namespace UberPad {

class FileTreeWidget : public QWidget {
    Q_OBJECT

public:
    explicit FileTreeWidget(QWidget *parent = nullptr);
    ~FileTreeWidget() override = default;

    void setRootPath(const QString &path);
    QString rootPath() const { return m_rootPath; }

signals:
    void fileOpened(const QString &filePath);

private slots:
    void onItemDoubleClicked(const QModelIndex &index);
    void onChangeFolder();
    void onRefresh();
    void onContextMenu(const QPoint &pos);

private:
    QString m_rootPath;
    QLabel *m_folderLabel;
    QTreeView *m_treeView;
    QFileSystemModel *m_model;
};

} // namespace UberPad
