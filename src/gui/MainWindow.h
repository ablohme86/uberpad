#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QDockWidget>
#include <QLabel>
#include <QPushButton>

#include "CodeEditor.h"
#include "SearchReplaceBar.h"
#include "FileTreeWidget.h"
#include "TerminalWidget.h"

namespace UberPad {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

    void openFiles(const QStringList &filePaths);

protected:
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    // File actions
    void onNewFile();
    void onOpenFile();
    void onOpenFolder();
    bool onSaveFile();
    bool onSaveFileAs();
    bool onSaveAll();
    bool onCloseTab(int index);
    bool onCloseAllTabs();
    void onRecentFileTriggered();

    // Edit actions
    void onGoToLine();
    void onConvertLineEndingLF();
    void onConvertLineEndingCRLF();

    // Tab management
    void onCurrentTabChanged(int index);
    void onTabContextMenu(const QPoint &pos);
    void onEditorModificationChanged(bool modified);

    // View & Settings
    void onToggleWordWrap(bool checked);
    void onToggleLineNumbers(bool checked);
    void onToggleHighlightLine(bool checked);
    void onThemeSelected(const QString &themeName);
    void onLanguageSelected(const QString &langName);
    void onAutoDetectLanguage();

    // Status bar updates
    void updateStatusBar();

    // About
    void onAbout();

private:
    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void createDocks();
    void updateRecentFilesMenu();
    void applyTheme(const QString &themeName);

    CodeEditor* currentEditor() const;
    CodeEditor* createEditorTab(const QString &title = QStringLiteral("Untitled"));
    bool maybeSave(CodeEditor *editor);

    QTabWidget *m_tabWidget;
    SearchReplaceBar *m_searchBar;

    // Docks
    QDockWidget *m_workspaceDock;
    FileTreeWidget *m_fileTree;

    QDockWidget *m_terminalDock;
    TerminalWidget *m_terminal;

    // Status Bar items
    QLabel *m_statusPosLabel;
    QLabel *m_statusDocInfoLabel;
    QPushButton *m_statusLineEndingBtn;
    QLabel *m_statusEncodingLabel;
    QPushButton *m_statusLangBtn;
    QLabel *m_statusInsLabel;

    // Menus
    QMenu *m_fileMenu;
    QMenu *m_recentFilesMenu;
    QMenu *m_editMenu;
    QMenu *m_searchMenu;
    QMenu *m_viewMenu;
    QMenu *m_languageMenu;
    QMenu *m_themeMenu;
    QMenu *m_terminalMenu;
    QMenu *m_helpMenu;

    // Actions
    QAction *m_saveAct;
    QAction *m_saveAsAct;
    QAction *m_saveAllAct;
    QAction *m_closeAct;
    QAction *m_closeAllAct;
    QAction *m_undoAct;
    QAction *m_redoAct;
    QAction *m_cutAct;
    QAction *m_copyAct;
    QAction *m_pasteAct;
    QAction *m_findAct;
    QAction *m_replaceAct;
    QAction *m_wordWrapAct;
    QAction *m_lineNumbersAct;
    QAction *m_highlightLineAct;
    QAction *m_toggleTerminalAct;
    QAction *m_toggleWorkspaceAct;
};

} // namespace UberPad
