#include "FileTreeWidget.h"
#include "../core/Config.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMenu>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QInputDialog>
#include <QMessageBox>
#include <QHeaderView>

namespace UberPad {

FileTreeWidget::FileTreeWidget(QWidget *parent)
    : QWidget(parent)
    , m_folderLabel(new QLabel(this))
    , m_treeView(new QTreeView(this))
    , m_model(new QFileSystemModel(this))
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    // Header toolbar
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(4);

    m_folderLabel->setStyleSheet("font-weight: bold; color: palette(text);");
    m_folderLabel->setText(tr("Workspace"));

    QToolButton *changeBtn = new QToolButton(this);
    changeBtn->setText(tr("📁"));
    changeBtn->setToolTip(tr("Open Folder as Workspace..."));

    QToolButton *refreshBtn = new QToolButton(this);
    refreshBtn->setText(tr("🔄"));
    refreshBtn->setToolTip(tr("Refresh Workspace"));

    QToolButton *collapseBtn = new QToolButton(this);
    collapseBtn->setText(tr("⮝"));
    collapseBtn->setToolTip(tr("Collapse All"));

    headerLayout->addWidget(m_folderLabel, 1);
    headerLayout->addWidget(changeBtn);
    headerLayout->addWidget(refreshBtn);
    headerLayout->addWidget(collapseBtn);

    mainLayout->addLayout(headerLayout);

    // Tree View
    m_model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    m_treeView->setModel(m_model);
    m_treeView->setHeaderHidden(true);
    // Hide size, type, date columns
    for (int i = 1; i < 4; ++i) {
        m_treeView->hideColumn(i);
    }
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    mainLayout->addWidget(m_treeView, 1);

    connect(changeBtn, &QToolButton::clicked, this, &FileTreeWidget::onChangeFolder);
    connect(refreshBtn, &QToolButton::clicked, this, &FileTreeWidget::onRefresh);
    connect(collapseBtn, &QToolButton::clicked, m_treeView, &QTreeView::collapseAll);
    connect(m_treeView, &QTreeView::doubleClicked, this, &FileTreeWidget::onItemDoubleClicked);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &FileTreeWidget::onContextMenu);

    QString lastPath = Config::instance().lastWorkspacePath();
    if (lastPath.isEmpty() || !QDir(lastPath).exists()) {
        lastPath = QDir::currentPath();
    }
    setRootPath(lastPath);
}

void FileTreeWidget::setRootPath(const QString &path) {
    if (path.isEmpty() || !QDir(path).exists()) return;

    m_rootPath = path;
    Config::instance().setLastWorkspacePath(path);

    m_model->setRootPath(path);
    m_treeView->setRootIndex(m_model->index(path));

    QFileInfo fi(path);
    m_folderLabel->setText(fi.fileName().isEmpty() ? path : fi.fileName());
    m_folderLabel->setToolTip(path);
}

void FileTreeWidget::onChangeFolder() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Workspace Folder"),
                                                    m_rootPath.isEmpty() ? QDir::currentPath() : m_rootPath);
    if (!dir.isEmpty()) {
        setRootPath(dir);
    }
}

void FileTreeWidget::onRefresh() {
    if (!m_rootPath.isEmpty()) {
        m_model->setRootPath(QString());
        m_model->setRootPath(m_rootPath);
        m_treeView->setRootIndex(m_model->index(m_rootPath));
    }
}

void FileTreeWidget::onItemDoubleClicked(const QModelIndex &index) {
    if (!index.isValid()) return;
    QString path = m_model->filePath(index);
    QFileInfo fi(path);
    if (fi.isFile()) {
        emit fileOpened(path);
    }
}

void FileTreeWidget::onContextMenu(const QPoint &pos) {
    QModelIndex index = m_treeView->indexAt(pos);
    QString path = index.isValid() ? m_model->filePath(index) : m_rootPath;
    QFileInfo fi(path);

    QMenu menu(this);
    if (fi.isFile()) {
        menu.addAction(tr("Open in Editor"), [this, path]() {
            emit fileOpened(path);
        });
        menu.addSeparator();
    }

    menu.addAction(tr("New File..."), [this, path, fi]() {
        QString dir = fi.isDir() ? path : fi.absolutePath();
        bool ok = false;
        QString name = QInputDialog::getText(this, tr("New File"), tr("File name:"), QLineEdit::Normal, QString(), &ok);
        if (ok && !name.isEmpty()) {
            QString full = QDir(dir).filePath(name);
            QFile f(full);
            if (f.open(QIODevice::WriteOnly)) {
                f.close();
                emit fileOpened(full);
            }
        }
    });

    menu.addAction(tr("New Folder..."), [this, path, fi]() {
        QString dir = fi.isDir() ? path : fi.absolutePath();
        bool ok = false;
        QString name = QInputDialog::getText(this, tr("New Folder"), tr("Folder name:"), QLineEdit::Normal, QString(), &ok);
        if (ok && !name.isEmpty()) {
            QDir(dir).mkdir(name);
        }
    });

    menu.addSeparator();
    menu.addAction(tr("Reveal in File Manager"), [path]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).isDir() ? path : QFileInfo(path).absolutePath()));
    });

    if (index.isValid()) {
        menu.addSeparator();
        menu.addAction(tr("Delete"), [this, path, fi]() {
            int ret = QMessageBox::question(this, tr("Confirm Delete"),
                                            tr("Are you sure you want to delete '%1'?").arg(fi.fileName()),
                                            QMessageBox::Yes | QMessageBox::No);
            if (ret == QMessageBox::Yes) {
                if (fi.isDir()) {
                    QDir(path).removeRecursively();
                } else {
                    QFile::remove(path);
                }
            }
        });
    }

    menu.exec(m_treeView->viewport()->mapToGlobal(pos));
}

} // namespace UberPad
