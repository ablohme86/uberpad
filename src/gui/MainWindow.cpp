#include "MainWindow.h"
#include "GoToLineDialog.h"
#include "../core/Config.h"
#include "../core/SyntaxManager.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QTabBar>
#include <QVBoxLayout>
#include "RemoteConnectDialog.h"
#include <QApplication>
#include <QShortcut>
#include <QClipboard>
#include <QIcon>

namespace UberPad {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_tabWidget(new QTabWidget(this))
    , m_searchBar(new SearchReplaceBar(this))
{
    setWindowTitle(QStringLiteral("UberPad"));

    QIcon appIcon(QStringLiteral(":/icons/uberpad-256.png"));
    appIcon.addFile(QStringLiteral(":/icons/uberpad-64.png"), QSize(64, 64));
    appIcon.addFile(QStringLiteral(":/icons/uberpad-32.png"), QSize(32, 32));
    appIcon.addFile(QStringLiteral(":/icons/uberpad.png"));
    setWindowIcon(appIcon);

    resize(1150, 750);
    setAcceptDrops(true);

    // Central widget container holding tabs and search bar
    QWidget *centralContainer = new QWidget(this);
    QVBoxLayout *containerLayout = new QVBoxLayout(centralContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);

    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    containerLayout->addWidget(m_tabWidget, 1);
    containerLayout->addWidget(m_searchBar);

    setCentralWidget(centralContainer);

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    createDocks();

    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::onCloseTab);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onCurrentTabChanged);
    connect(m_tabWidget, &QTabWidget::customContextMenuRequested, this, &MainWindow::onTabContextMenu);

    // Start with a new empty file
    onNewFile();

    // Apply saved theme
    applyTheme(Config::instance().themeName());
}

CodeEditor* MainWindow::currentEditor() const {
    return qobject_cast<CodeEditor*>(m_tabWidget->currentWidget());
}

CodeEditor* MainWindow::createEditorTab(const QString &title) {
    CodeEditor *editor = new CodeEditor(this);

    connect(editor, &CodeEditor::cursorPositionChanged, this, &MainWindow::updateStatusBar);
    connect(editor->document(), &QTextDocument::modificationChanged, this, &MainWindow::onEditorModificationChanged);
    connect(editor, &CodeEditor::languageChanged, this, &MainWindow::updateStatusBar);
    connect(editor, &CodeEditor::lineEndingChanged, this, &MainWindow::updateStatusBar);

    int idx = m_tabWidget->addTab(editor, title);
    m_tabWidget->setCurrentIndex(idx);
    editor->setFocus();
    return editor;
}

void MainWindow::onNewFile() {
    createEditorTab(tr("new 1"));
    updateStatusBar();
}

void MainWindow::onOpenFile() {
    QStringList files = QFileDialog::getOpenFileNames(this, tr("Open File"), QString(),
                                                      tr("All Files (*);;C/C++ Files (*.cpp *.c *.h *.hpp);;Python (*.py);;Web (*.html *.css *.js *.ts *.json)"));
    openFiles(files);
}

void MainWindow::openFiles(const QStringList &filePaths) {
    for (const QString &filePath : filePaths) {
        if (filePath.isEmpty()) continue;

        // Check if already open
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            CodeEditor *ed = qobject_cast<CodeEditor*>(m_tabWidget->widget(i));
            if (ed && ed->filePath() == filePath) {
                m_tabWidget->setCurrentIndex(i);
                return;
            }
        }

        // If only 1 tab that is untitled and empty, reuse it
        CodeEditor *targetEditor = nullptr;
        if (m_tabWidget->count() == 1) {
            CodeEditor *first = currentEditor();
            if (first && first->isUntitled() && !first->document()->isModified() && first->toPlainText().isEmpty()) {
                targetEditor = first;
            }
        }

        if (!targetEditor) {
            targetEditor = createEditorTab(QFileInfo(filePath).fileName());
        }

        if (targetEditor->loadFromFile(filePath)) {
            int idx = m_tabWidget->indexOf(targetEditor);
            m_tabWidget->setTabText(idx, targetEditor->fileName());
            m_tabWidget->setTabToolTip(idx, filePath);
            Config::instance().addRecentFile(filePath);
            updateRecentFilesMenu();
            statusBar()->showMessage(tr("Loaded %1").arg(filePath), 3000);
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Could not open file:\n%1").arg(filePath));
        }
    }
    updateStatusBar();
}

void MainWindow::onOpenFolder() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Folder as Workspace"));
    if (!dir.isEmpty()) {
        m_fileTree->setRootPath(dir);
        m_workspaceDock->show();
        m_workspaceDock->raise();
    }
}

void MainWindow::onOpenRemoteWorkspace() {
    m_remoteDock->show();
    m_remoteDock->raise();
    RemoteConnectDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        m_remoteWorkspace->connectToServer(dlg.config());
    }
}

void MainWindow::onRemoteFileOpened(const QString &remotePath, const QByteArray &data, const RemoteConfig &config) {
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        CodeEditor *ed = qobject_cast<CodeEditor*>(m_tabWidget->widget(i));
        if (ed && ed->isRemote() && ed->remotePath() == remotePath && ed->remoteConfig().host == config.host) {
            m_tabWidget->setCurrentIndex(i);
            return;
        }
    }

    CodeEditor *editor = nullptr;
    if (m_tabWidget->count() == 1) {
        CodeEditor *first = qobject_cast<CodeEditor*>(m_tabWidget->widget(0));
        if (first && first->isUntitled() && !first->document()->isModified() && first->toPlainText().isEmpty()) {
            editor = first;
        }
    }

    if (!editor) {
        editor = createEditorTab();
    }

    editor->setPlainText(QString::fromUtf8(data));
    editor->setRemoteInfo(remotePath, config);
    editor->document()->setModified(false);

    int idx = m_tabWidget->indexOf(editor);
    m_tabWidget->setTabText(idx, editor->fileName());
    m_tabWidget->setTabToolTip(idx, QStringLiteral("%1://%2:%3%4")
                                        .arg(config.protocolString())
                                        .arg(config.host)
                                        .arg(config.port)
                                        .arg(remotePath));
    m_tabWidget->setCurrentWidget(editor);
    updateStatusBar();
}

bool MainWindow::saveRemoteFile(CodeEditor *editor) {
    if (!editor || !editor->isRemote()) return false;

    QByteArray data = editor->toPlainText().toUtf8();
    QString error;
    RemoteClient client(editor->remoteConfig());

    QApplication::setOverrideCursor(Qt::WaitCursor);
    bool ok = client.uploadFile(editor->remotePath(), data, error);
    QApplication::restoreOverrideCursor();

    if (ok) {
        editor->document()->setModified(false);
        int idx = m_tabWidget->indexOf(editor);
        if (idx >= 0) {
            m_tabWidget->setTabText(idx, editor->fileName());
        }
        statusBar()->showMessage(tr("Saved remote file %1 (%2)").arg(editor->remotePath(), editor->remoteConfig().host), 3000);
        updateStatusBar();
        return true;
    }

    QMessageBox::critical(this, tr("Remote Save Failed"),
                          tr("Could not save remote file '%1' to %2:\n%3")
                              .arg(editor->remotePath(), editor->remoteConfig().host, error));
    return false;
}

bool MainWindow::onSaveFile() {
    CodeEditor *editor = currentEditor();
    if (!editor) return false;

    if (editor->isRemote()) {
        return saveRemoteFile(editor);
    }

    if (editor->isUntitled()) {
        return onSaveFileAs();
    }

    if (editor->saveToFile()) {
        int idx = m_tabWidget->currentIndex();
        m_tabWidget->setTabText(idx, editor->fileName());
        statusBar()->showMessage(tr("Saved %1").arg(editor->filePath()), 3000);
        return true;
    }

    QMessageBox::warning(this, tr("Error"), tr("Could not save file:\n%1").arg(editor->filePath()));
    return false;
}

bool MainWindow::onSaveFileAs() {
    CodeEditor *editor = currentEditor();
    if (!editor) return false;

    QString initial = editor->isUntitled() ? editor->fileName() : editor->filePath();
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save File As"), initial, tr("All Files (*)"));
    if (filePath.isEmpty()) return false;

    if (editor->saveToFile(filePath)) {
        int idx = m_tabWidget->currentIndex();
        m_tabWidget->setTabText(idx, editor->fileName());
        m_tabWidget->setTabToolTip(idx, filePath);
        Config::instance().addRecentFile(filePath);
        updateRecentFilesMenu();
        statusBar()->showMessage(tr("Saved %1").arg(filePath), 3000);
        updateStatusBar();
        return true;
    }

    QMessageBox::warning(this, tr("Error"), tr("Could not save file:\n%1").arg(filePath));
    return false;
}

bool MainWindow::onSaveAll() {
    bool allOk = true;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        CodeEditor *ed = qobject_cast<CodeEditor*>(m_tabWidget->widget(i));
        if (ed && ed->document()->isModified()) {
            if (ed->isRemote()) {
                if (!saveRemoteFile(ed)) allOk = false;
            } else if (ed->isUntitled()) {
                m_tabWidget->setCurrentIndex(i);
                if (!onSaveFileAs()) allOk = false;
            } else {
                if (!ed->saveToFile()) allOk = false;
                else m_tabWidget->setTabText(i, ed->fileName());
            }
        }
    }
    return allOk;
}

bool MainWindow::maybeSave(CodeEditor *editor) {
    if (!editor || !editor->document()->isModified()) return true;

    QMessageBox::StandardButton ret = QMessageBox::warning(
        this, tr("Save Changes"),
        tr("The file '%1' has unsaved changes.\nDo you want to save your changes?").arg(editor->fileName()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
    );

    if (ret == QMessageBox::Save) {
        m_tabWidget->setCurrentWidget(editor);
        return onSaveFile();
    } else if (ret == QMessageBox::Cancel) {
        return false;
    }
    return true;
}

bool MainWindow::onCloseTab(int index) {
    CodeEditor *editor = qobject_cast<CodeEditor*>(m_tabWidget->widget(index));
    if (!editor) return true;

    if (!maybeSave(editor)) return false;

    m_tabWidget->removeTab(index);
    delete editor;

    if (m_tabWidget->count() == 0) {
        onNewFile();
    }
    return true;
}

bool MainWindow::onCloseAllTabs() {
    while (m_tabWidget->count() > 0) {
        if (!onCloseTab(0)) return false;
    }
    return true;
}

void MainWindow::onRecentFileTriggered() {
    QAction *act = qobject_cast<QAction*>(sender());
    if (act) {
        openFiles({ act->data().toString() });
    }
}

void MainWindow::updateRecentFilesMenu() {
    m_recentFilesMenu->clear();
    QStringList recents = Config::instance().recentFiles();
    for (const QString &f : recents) {
        QAction *act = m_recentFilesMenu->addAction(QFileInfo(f).fileName());
        act->setToolTip(f);
        act->setData(f);
        connect(act, &QAction::triggered, this, &MainWindow::onRecentFileTriggered);
    }
    if (!recents.isEmpty()) {
        m_recentFilesMenu->addSeparator();
        m_recentFilesMenu->addAction(tr("Clear Recent Files"), []() {
            Config::instance().clearRecentFiles();
        });
    }
}

void MainWindow::onCurrentTabChanged(int index) {
    CodeEditor *editor = currentEditor();
    if (editor) {
        m_searchBar->setEditor(editor);
        editor->setFocus();
    }
    updateStatusBar();
}

void MainWindow::onTabContextMenu(const QPoint &pos) {
    int idx = m_tabWidget->tabBar()->tabAt(pos);
    if (idx < 0) return;

    CodeEditor *ed = qobject_cast<CodeEditor*>(m_tabWidget->widget(idx));
    if (!ed) return;

    QMenu menu(this);
    menu.addAction(tr("Close Tab"), [this, idx]() { onCloseTab(idx); });
    menu.addAction(tr("Close Other Tabs"), [this, idx]() {
        for (int i = m_tabWidget->count() - 1; i >= 0; --i) {
            if (i != idx) onCloseTab(i);
        }
    });
    menu.addAction(tr("Close Tabs to the Right"), [this, idx]() {
        for (int i = m_tabWidget->count() - 1; i > idx; --i) {
            onCloseTab(i);
        }
    });
    menu.addAction(tr("Close All"), this, &MainWindow::onCloseAllTabs);
    menu.addSeparator();

    if (ed->isRemote()) {
        menu.addAction(tr("Copy Remote Path"), [ed]() {
            QApplication::clipboard()->setText(ed->remotePath());
        });
        menu.addAction(tr("Copy Remote URL"), [ed]() {
            QString url = QStringLiteral("%1://%2:%3%4")
                              .arg(ed->remoteConfig().protocolString())
                              .arg(ed->remoteConfig().host)
                              .arg(ed->remoteConfig().port)
                              .arg(ed->remotePath());
            QApplication::clipboard()->setText(url);
        });
        menu.addSeparator();
    } else if (!ed->isUntitled()) {
        menu.addAction(tr("Copy Full Path"), [ed]() {
            QApplication::clipboard()->setText(ed->filePath());
        });
        menu.addAction(tr("Open Containing Folder"), [ed]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(ed->filePath()).absolutePath()));
        });
        menu.addSeparator();
    }

    menu.exec(m_tabWidget->mapToGlobal(pos));
}

void MainWindow::onEditorModificationChanged(bool modified) {
    CodeEditor *editor = qobject_cast<CodeEditor*>(sender()->parent());
    if (!editor) editor = currentEditor();
    if (!editor) return;

    int idx = m_tabWidget->indexOf(editor);
    if (idx >= 0) {
        QString title = editor->fileName();
        if (modified) title.prepend(QLatin1Char('*'));
        m_tabWidget->setTabText(idx, title);
    }

    if (editor == currentEditor()) {
        QString title = editor->fileName();
        if (modified) title.prepend(QLatin1Char('*'));
        setWindowTitle(QStringLiteral("%1 - UberPad").arg(title));
    }
}

void MainWindow::onGoToLine() {
    CodeEditor *editor = currentEditor();
    if (!editor) return;

    int currentLine = editor->textCursor().blockNumber() + 1;
    int maxLines = editor->blockCount();

    GoToLineDialog dlg(currentLine, maxLines, this);
    if (dlg.exec() == QDialog::Accepted) {
        int targetLine = dlg.selectedLine() - 1;
        QTextBlock block = editor->document()->findBlockByNumber(targetLine);
        if (block.isValid()) {
            QTextCursor cur(block);
            editor->setTextCursor(cur);
            editor->centerCursor();
            editor->setFocus();
        }
    }
}

void MainWindow::onConvertLineEndingLF() {
    CodeEditor *ed = currentEditor();
    if (ed) ed->setLineEnding(CodeEditor::LineEnding::LF);
}

void MainWindow::onConvertLineEndingCRLF() {
    CodeEditor *ed = currentEditor();
    if (ed) ed->setLineEnding(CodeEditor::LineEnding::CRLF);
}

void MainWindow::onToggleWordWrap(bool checked) {
    Config::instance().setWordWrap(checked);
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        CodeEditor *ed = qobject_cast<CodeEditor*>(m_tabWidget->widget(i));
        if (ed) {
            ed->setLineWrapMode(checked ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
        }
    }
}

void MainWindow::onToggleLineNumbers(bool checked) {
    Config::instance().setShowLineNumbers(checked);
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        CodeEditor *ed = qobject_cast<CodeEditor*>(m_tabWidget->widget(i));
        if (ed) ed->update();
    }
}

void MainWindow::onToggleHighlightLine(bool checked) {
    Config::instance().setHighlightCurrentLine(checked);
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        CodeEditor *ed = qobject_cast<CodeEditor*>(m_tabWidget->widget(i));
        if (ed) ed->update();
    }
}

void MainWindow::applyTheme(const QString &themeName) {
    Config::instance().setThemeName(themeName);
    auto theme = SyntaxManager::instance().theme(themeName);

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        CodeEditor *ed = qobject_cast<CodeEditor*>(m_tabWidget->widget(i));
        if (ed) {
            ed->setTheme(theme);
        }
    }

    // Set overall app palette
    bool isDark = (qGray(theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor)) < 128);
    QPalette pal = qApp->palette();
    if (isDark) {
        pal.setColor(QPalette::Window, QColor(33, 34, 44));
        pal.setColor(QPalette::WindowText, QColor(240, 240, 240));
        pal.setColor(QPalette::Base, QColor(24, 25, 32));
        pal.setColor(QPalette::AlternateBase, QColor(36, 38, 48));
        pal.setColor(QPalette::Text, QColor(240, 240, 240));
        pal.setColor(QPalette::Button, QColor(40, 42, 54));
        pal.setColor(QPalette::ButtonText, QColor(240, 240, 240));
        pal.setColor(QPalette::Mid, QColor(60, 62, 75));
        pal.setColor(QPalette::Highlight, QColor(68, 71, 90));
    } else {
        pal.setColor(QPalette::Window, QColor(245, 245, 247));
        pal.setColor(QPalette::WindowText, QColor(30, 30, 30));
        pal.setColor(QPalette::Base, QColor(255, 255, 255));
        pal.setColor(QPalette::AlternateBase, QColor(240, 240, 240));
        pal.setColor(QPalette::Text, QColor(30, 30, 30));
        pal.setColor(QPalette::Button, QColor(235, 235, 238));
        pal.setColor(QPalette::ButtonText, QColor(30, 30, 30));
        pal.setColor(QPalette::Mid, QColor(200, 200, 200));
        pal.setColor(QPalette::Highlight, QColor(180, 210, 255));
    }
    qApp->setPalette(pal);
}

void MainWindow::onThemeSelected(const QString &themeName) {
    applyTheme(themeName);
}

void MainWindow::onLanguageSelected(const QString &langName) {
    CodeEditor *ed = currentEditor();
    if (!ed) return;

    auto def = SyntaxManager::instance().definitionForName(langName);
    if (def.isValid()) {
        ed->setDefinition(def);
        updateStatusBar();
    }
}

void MainWindow::onAutoDetectLanguage() {
    CodeEditor *ed = currentEditor();
    if (!ed) return;

    auto def = SyntaxManager::instance().definitionForFileName(ed->filePath());
    if (def.isValid()) {
        ed->setDefinition(def);
    } else {
        ed->setDefinition(SyntaxManager::instance().defaultDefinition());
    }
    updateStatusBar();
}

void MainWindow::updateStatusBar() {
    CodeEditor *ed = currentEditor();
    if (!ed) {
        m_statusPosLabel->setText(QStringLiteral("Ln : -   Col : -   Sel : 0"));
        m_statusDocInfoLabel->setText(QStringLiteral("Lines : 0   Length : 0"));
        m_statusLineEndingBtn->setText(QStringLiteral("Unix (LF)"));
        m_statusEncodingLabel->setText(QStringLiteral("UTF-8"));
        m_statusLangBtn->setText(QStringLiteral("Plain Text"));
        return;
    }

    QTextCursor cur = ed->textCursor();
    int line = cur.blockNumber() + 1;
    int col = cur.positionInBlock() + 1;
    int sel = cur.selectedText().size();

    m_statusPosLabel->setText(QStringLiteral("Ln : %1   Col : %2   Sel : %3").arg(line).arg(col).arg(sel));
    m_statusDocInfoLabel->setText(QStringLiteral("Lines : %1   Length : %2").arg(ed->blockCount()).arg(ed->toPlainText().size()));
    m_statusLineEndingBtn->setText(ed->lineEndingName());
    m_statusEncodingLabel->setText(QStringLiteral("UTF-8"));
    m_statusLangBtn->setText(ed->languageName());
    m_statusInsLabel->setText(ed->overwriteMode() ? QStringLiteral("OVR") : QStringLiteral("INS"));

    QString title = ed->fileName();
    if (ed->document()->isModified()) title.prepend(QLatin1Char('*'));
    setWindowTitle(QStringLiteral("%1 - UberPad").arg(title));
}

void MainWindow::createActions() {
    // File
    connect(new QShortcut(QKeySequence::New, this), &QShortcut::activated, this, &MainWindow::onNewFile);
    connect(new QShortcut(QKeySequence::Open, this), &QShortcut::activated, this, &MainWindow::onOpenFile);
    connect(new QShortcut(QKeySequence::Save, this), &QShortcut::activated, this, &MainWindow::onSaveFile);
    connect(new QShortcut(QKeySequence::SaveAs, this), &QShortcut::activated, this, &MainWindow::onSaveFileAs);
    connect(new QShortcut(QKeySequence::Close, this), &QShortcut::activated, this, [this]() { onCloseTab(m_tabWidget->currentIndex()); });

    // Search
    m_findAct = new QAction(tr("&Find..."), this);
    m_findAct->setShortcut(QKeySequence::Find);
    connect(m_findAct, &QAction::triggered, m_searchBar, &SearchReplaceBar::openFind);

    m_replaceAct = new QAction(tr("&Replace..."), this);
    m_replaceAct->setShortcut(QKeySequence::Replace);
    connect(m_replaceAct, &QAction::triggered, m_searchBar, &SearchReplaceBar::openReplace);

    // Zoom
    connect(new QShortcut(QKeySequence::ZoomIn, this), &QShortcut::activated, this, [this]() {
        if (currentEditor()) currentEditor()->zoomIn();
    });
    connect(new QShortcut(QKeySequence::ZoomOut, this), &QShortcut::activated, this, [this]() {
        if (currentEditor()) currentEditor()->zoomOut();
    });
    connect(new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_0), this), &QShortcut::activated, this, [this]() {
        if (currentEditor()) currentEditor()->resetZoom();
    });

    // Terminal toggle shortcut (Ctrl+` or F12)
    connect(new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_AsciiTilde), this), &QShortcut::activated, this, [this]() {
        m_terminalDock->setVisible(!m_terminalDock->isVisible());
    });
    connect(new QShortcut(QKeySequence(Qt::Key_F12), this), &QShortcut::activated, this, [this]() {
        m_terminalDock->setVisible(!m_terminalDock->isVisible());
    });
}

void MainWindow::createMenus() {
    // File Menu
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(tr("&New"), QKeySequence::New, this, &MainWindow::onNewFile);
    m_fileMenu->addAction(tr("&Open..."), QKeySequence::Open, this, &MainWindow::onOpenFile);
    m_fileMenu->addAction(tr("Open Folder as &Workspace..."), QKeySequence(Qt::CTRL | Qt::Key_K), this, &MainWindow::onOpenFolder);
    m_fileMenu->addAction(tr("Connect to &Remote (SFTP/FTP)..."), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R), this, &MainWindow::onOpenRemoteWorkspace);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(tr("&Save"), QKeySequence::Save, this, &MainWindow::onSaveFile);
    m_fileMenu->addAction(tr("Save &As..."), QKeySequence::SaveAs, this, &MainWindow::onSaveFileAs);
    m_fileMenu->addAction(tr("Save A&ll"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S), this, &MainWindow::onSaveAll);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(tr("&Close Tab"), QKeySequence::Close, this, [this]() { onCloseTab(m_tabWidget->currentIndex()); });
    m_fileMenu->addAction(tr("Close &All Tabs"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_W), this, &MainWindow::onCloseAllTabs);
    m_fileMenu->addSeparator();
    m_recentFilesMenu = m_fileMenu->addMenu(tr("&Recent Files"));
    updateRecentFilesMenu();
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(tr("E&xit"), QKeySequence::Quit, this, [this]() { close(); });

    // Edit Menu
    m_editMenu = menuBar()->addMenu(tr("&Edit"));
    m_editMenu->addAction(tr("&Undo"), QKeySequence::Undo, this, [this]() { if (currentEditor()) currentEditor()->undo(); });
    m_editMenu->addAction(tr("&Redo"), QKeySequence::Redo, this, [this]() { if (currentEditor()) currentEditor()->redo(); });
    m_editMenu->addSeparator();
    m_editMenu->addAction(tr("Cu&t"), QKeySequence::Cut, this, [this]() { if (currentEditor()) currentEditor()->cut(); });
    m_editMenu->addAction(tr("&Copy"), QKeySequence::Copy, this, [this]() { if (currentEditor()) currentEditor()->copy(); });
    m_editMenu->addAction(tr("&Paste"), QKeySequence::Paste, this, [this]() { if (currentEditor()) currentEditor()->paste(); });
    m_editMenu->addAction(tr("Select &All"), QKeySequence::SelectAll, this, [this]() { if (currentEditor()) currentEditor()->selectAll(); });
    m_editMenu->addSeparator();
    m_editMenu->addAction(tr("&Duplicate Line / Selection"), QKeySequence(Qt::CTRL | Qt::Key_D), this, [this]() { if (currentEditor()) currentEditor()->duplicateLineOrSelection(); });
    m_editMenu->addAction(tr("Toggle &Comment"), QKeySequence(Qt::CTRL | Qt::Key_Slash), this, [this]() { if (currentEditor()) currentEditor()->toggleComment(); });
    m_editMenu->addAction(tr("Move Line &Up"), QKeySequence(Qt::ALT | Qt::Key_Up), this, [this]() { if (currentEditor()) currentEditor()->moveLineUp(); });
    m_editMenu->addAction(tr("Move Line &Down"), QKeySequence(Qt::ALT | Qt::Key_Down), this, [this]() { if (currentEditor()) currentEditor()->moveLineDown(); });
    m_editMenu->addSeparator();
    QMenu *lineEndingsMenu = m_editMenu->addMenu(tr("EOL &Conversion"));
    lineEndingsMenu->addAction(tr("Convert to Unix (LF)"), this, &MainWindow::onConvertLineEndingLF);
    lineEndingsMenu->addAction(tr("Convert to Windows (CRLF)"), this, &MainWindow::onConvertLineEndingCRLF);

    // Search Menu
    m_searchMenu = menuBar()->addMenu(tr("&Search"));
    m_searchMenu->addAction(m_findAct);
    m_searchMenu->addAction(m_replaceAct);
    m_searchMenu->addAction(tr("Find &Next"), QKeySequence::FindNext, this, [this]() { m_searchBar->openFind(); });
    m_searchMenu->addAction(tr("Find &Previous"), QKeySequence::FindPrevious, this, [this]() { m_searchBar->openFind(); });
    m_searchMenu->addSeparator();
    m_searchMenu->addAction(tr("&Go to Line..."), QKeySequence(Qt::CTRL | Qt::Key_G), this, &MainWindow::onGoToLine);

    // View Menu
    m_viewMenu = menuBar()->addMenu(tr("&View"));
    m_wordWrapAct = m_viewMenu->addAction(tr("&Word Wrap"), this, &MainWindow::onToggleWordWrap);
    m_wordWrapAct->setCheckable(true);
    m_wordWrapAct->setChecked(Config::instance().wordWrap());
    m_wordWrapAct->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_W));

    m_lineNumbersAct = m_viewMenu->addAction(tr("&Line Numbers"), this, &MainWindow::onToggleLineNumbers);
    m_lineNumbersAct->setCheckable(true);
    m_lineNumbersAct->setChecked(Config::instance().showLineNumbers());

    m_highlightLineAct = m_viewMenu->addAction(tr("&Highlight Current Line"), this, &MainWindow::onToggleHighlightLine);
    m_highlightLineAct->setCheckable(true);
    m_highlightLineAct->setChecked(Config::instance().highlightCurrentLine());

    m_viewMenu->addSeparator();
    m_viewMenu->addAction(tr("Zoom &In"), QKeySequence::ZoomIn, this, [this]() { if (currentEditor()) currentEditor()->zoomIn(); });
    m_viewMenu->addAction(tr("Zoom &Out"), QKeySequence::ZoomOut, this, [this]() { if (currentEditor()) currentEditor()->zoomOut(); });
    m_viewMenu->addAction(tr("Reset &Zoom"), QKeySequence(Qt::CTRL | Qt::Key_0), this, [this]() { if (currentEditor()) currentEditor()->resetZoom(); });
    m_viewMenu->addSeparator();

    // Language Menu (Categorized 460+ languages)
    m_languageMenu = menuBar()->addMenu(tr("&Language"));
    m_languageMenu->addAction(tr("Auto-Detect Language"), this, &MainWindow::onAutoDetectLanguage);
    m_languageMenu->addSeparator();

    auto defs = SyntaxManager::instance().allDefinitions();
    QMenu *menuAC = m_languageMenu->addMenu(tr("A - C"));
    QMenu *menuDH = m_languageMenu->addMenu(tr("D - H"));
    QMenu *menuIP = m_languageMenu->addMenu(tr("I - P"));
    QMenu *menuQZ = m_languageMenu->addMenu(tr("Q - Z"));

    for (const auto &def : defs) {
        if (def.isHidden()) continue;
        QString name = def.name();
        if (name.isEmpty()) continue;
        QChar first = name[0].toUpper();

        QMenu *target = menuAC;
        if (first >= 'D' && first <= 'H') target = menuDH;
        else if (first >= 'I' && first <= 'P') target = menuIP;
        else if (first >= 'Q' && first <= 'Z') target = menuQZ;

        target->addAction(name, [this, name]() { onLanguageSelected(name); });
    }

    // Themes Menu
    m_themeMenu = menuBar()->addMenu(tr("&Theme"));
    QMenu *darkThemesMenu = m_themeMenu->addMenu(tr("Dark Themes"));
    QMenu *lightThemesMenu = m_themeMenu->addMenu(tr("Light Themes"));

    auto allThemes = SyntaxManager::instance().themes();
    for (const auto &th : allThemes) {
        QString name = th.name();
        bool isDark = (qGray(th.editorColor(KSyntaxHighlighting::Theme::BackgroundColor)) < 128);
        QMenu *target = isDark ? darkThemesMenu : lightThemesMenu;
        target->addAction(name, [this, name]() { onThemeSelected(name); });
    }

    // Terminal Menu
    m_terminalMenu = menuBar()->addMenu(tr("&Terminal"));
    m_terminalMenu->addAction(tr("Toggle Terminal Panel"), QKeySequence(Qt::Key_F12), this, [this]() {
        m_terminalDock->setVisible(!m_terminalDock->isVisible());
    });
    m_terminalMenu->addAction(tr("CD to Current File Folder"), this, [this]() {
        CodeEditor *ed = currentEditor();
        if (ed && !ed->isUntitled()) {
            m_terminal->changeDirectory(QFileInfo(ed->filePath()).absolutePath());
            m_terminalDock->show();
            m_terminalDock->raise();
        }
    });
    m_terminalMenu->addAction(tr("Clear Terminal"), this, [this]() {
        if (m_terminal) m_terminal->clear();
    });
    m_terminalMenu->addAction(tr("Restart Terminal Shell"), this, [this]() {
        if (m_terminal) m_terminal->restartShell();
    });

    // Help Menu
    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(tr("&About UberPad"), this, &MainWindow::onAbout);
}

void MainWindow::createToolBars() {
    QToolBar *tb = addToolBar(tr("Main Toolbar"));
    tb->setMovable(false);
    tb->setIconSize(QSize(16, 16));

    tb->addAction(tr("📄 New"), this, &MainWindow::onNewFile)->setToolTip(tr("New File (Ctrl+N)"));
    tb->addAction(tr("📂 Open"), this, &MainWindow::onOpenFile)->setToolTip(tr("Open File (Ctrl+O)"));
    tb->addAction(tr("💾 Save"), this, &MainWindow::onSaveFile)->setToolTip(tr("Save File (Ctrl+S)"));
    tb->addAction(tr("💾💾 Save All"), this, &MainWindow::onSaveAll)->setToolTip(tr("Save All (Ctrl+Shift+S)"));
    tb->addAction(tr("✕ Close"), this, [this]() { onCloseTab(m_tabWidget->currentIndex()); })->setToolTip(tr("Close Tab (Ctrl+W)"));

    tb->addSeparator();
    tb->addAction(tr("✂ Cut"), this, [this]() { if (currentEditor()) currentEditor()->cut(); });
    tb->addAction(tr("📋 Copy"), this, [this]() { if (currentEditor()) currentEditor()->copy(); });
    tb->addAction(tr("📥 Paste"), this, [this]() { if (currentEditor()) currentEditor()->paste(); });
    tb->addAction(tr("↩ Undo"), this, [this]() { if (currentEditor()) currentEditor()->undo(); });
    tb->addAction(tr("↪ Redo"), this, [this]() { if (currentEditor()) currentEditor()->redo(); });

    tb->addSeparator();
    tb->addAction(tr("🔍 Find"), m_searchBar, &SearchReplaceBar::openFind)->setToolTip(tr("Find (Ctrl+F)"));
    tb->addAction(tr("🔄 Replace"), m_searchBar, &SearchReplaceBar::openReplace)->setToolTip(tr("Replace (Ctrl+H)"));
    tb->addAction(tr("📍 Go To Line"), this, &MainWindow::onGoToLine)->setToolTip(tr("Go to Line (Ctrl+G)"));

    tb->addSeparator();
    tb->addAction(tr("➕ Zoom In"), this, [this]() { if (currentEditor()) currentEditor()->zoomIn(); });
    tb->addAction(tr("➖ Zoom Out"), this, [this]() { if (currentEditor()) currentEditor()->zoomOut(); });

    tb->addSeparator();
    tb->addAction(tr("📁 Workspace"), this, [this]() {
        m_workspaceDock->setVisible(!m_workspaceDock->isVisible());
    })->setToolTip(tr("Toggle Workspace sidebar"));

    tb->addAction(tr("🌐 Remote"), this, [this]() {
        m_remoteDock->setVisible(!m_remoteDock->isVisible());
    })->setToolTip(tr("Toggle Remote SFTP/FTP Workspace sidebar"));

    tb->addAction(tr("🖥 Terminal"), this, [this]() {
        m_terminalDock->setVisible(!m_terminalDock->isVisible());
    })->setToolTip(tr("Toggle Integrated Terminal (F12)"));
}

void MainWindow::createStatusBar() {
    QStatusBar *sb = statusBar();

    m_statusPosLabel = new QLabel(this);
    m_statusDocInfoLabel = new QLabel(this);
    m_statusLineEndingBtn = new QPushButton(this);
    m_statusEncodingLabel = new QLabel(this);
    m_statusLangBtn = new QPushButton(this);
    m_statusInsLabel = new QLabel(this);

    m_statusLineEndingBtn->setFlat(true);
    m_statusLineEndingBtn->setStyleSheet("text-align: center; padding: 0 4px; border: none;");
    connect(m_statusLineEndingBtn, &QPushButton::clicked, this, [this]() {
        CodeEditor *ed = currentEditor();
        if (!ed) return;
        if (ed->lineEnding() == CodeEditor::LineEnding::LF) {
            ed->setLineEnding(CodeEditor::LineEnding::CRLF);
        } else {
            ed->setLineEnding(CodeEditor::LineEnding::LF);
        }
        updateStatusBar();
    });

    m_statusLangBtn->setFlat(true);
    m_statusLangBtn->setStyleSheet("text-align: center; padding: 0 6px; font-weight: bold; border: none;");
    connect(m_statusLangBtn, &QPushButton::clicked, this, [this]() {
        m_languageMenu->exec(m_statusLangBtn->mapToGlobal(QPoint(0, -m_languageMenu->sizeHint().height())));
    });

    sb->addPermanentWidget(m_statusDocInfoLabel);
    sb->addPermanentWidget(m_statusPosLabel);
    sb->addPermanentWidget(m_statusLineEndingBtn);
    sb->addPermanentWidget(m_statusEncodingLabel);
    sb->addPermanentWidget(m_statusLangBtn);
    sb->addPermanentWidget(m_statusInsLabel);

    updateStatusBar();
}

void MainWindow::createDocks() {
    // Workspace Dock (Folder as Workspace)
    m_workspaceDock = new QDockWidget(tr("Workspace"), this);
    m_workspaceDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_fileTree = new FileTreeWidget(m_workspaceDock);
    m_workspaceDock->setWidget(m_fileTree);
    addDockWidget(Qt::LeftDockWidgetArea, m_workspaceDock);

    connect(m_fileTree, &FileTreeWidget::fileOpened, this, [this](const QString &path) {
        openFiles({ path });
    });

    // Remote Workspace Dock (SFTP / FTP)
    m_remoteDock = new QDockWidget(tr("Remote Workspace (SFTP / FTP)"), this);
    m_remoteDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_remoteWorkspace = new RemoteWorkspaceWidget(m_remoteDock);
    m_remoteDock->setWidget(m_remoteWorkspace);
    addDockWidget(Qt::LeftDockWidgetArea, m_remoteDock);
    tabifyDockWidget(m_workspaceDock, m_remoteDock);
    m_workspaceDock->raise();

    connect(m_remoteWorkspace, &RemoteWorkspaceWidget::remoteFileOpened, this, &MainWindow::onRemoteFileOpened);

    // Terminal Dock (Integrated PTY terminal)
    m_terminalDock = new QDockWidget(tr("Terminal"), this);
    m_terminalDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    m_terminal = new TerminalWidget(m_terminalDock);
    m_terminalDock->setWidget(m_terminal);
    addDockWidget(Qt::BottomDockWidgetArea, m_terminalDock);

    m_viewMenu->addAction(m_workspaceDock->toggleViewAction());
    m_viewMenu->addAction(m_remoteDock->toggleViewAction());
    m_viewMenu->addAction(m_terminalDock->toggleViewAction());
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (onCloseAllTabs()) {
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event) {
    QStringList files;
    for (const QUrl &url : event->mimeData()->urls()) {
        if (url.isLocalFile()) {
            files.append(url.toLocalFile());
        }
    }
    if (!files.isEmpty()) {
        openFiles(files);
        event->acceptProposedAction();
    }
}

void MainWindow::onAbout() {
    QMessageBox::about(this, tr("About UberPad"),
        tr("<h3>UberPad</h3>"
           "<p>A high-performance Notepad++ equivalent text and code editor built with C++20 and Qt6.</p>"
           "<ul>"
           "<li><b>Dual frontend:</b> Full GUI and standalone Terminal UI (TUI) mode</li>"
           "<li><b>Syntax Highlighting:</b> 460+ programming languages auto-detected via KDE KF6SyntaxHighlighting</li>"
           "<li><b>Features:</b> Tabbed interface, Line numbers gutter, Current line highlight, Bracket matching, Auto-indentation, Smart Tabs, Live Search & Replace with Regex, Folder as Workspace sidebar</li>"
           "<li><b>Integrated Terminal:</b> Real interactive pseudo-terminal (PTY) with ANSI truecolor support</li>"
           "</ul>"));
}

} // namespace UberPad
