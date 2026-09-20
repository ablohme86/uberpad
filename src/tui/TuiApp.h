#pragma once

#include "TuiBuffer.h"
#include "TuiHighlighter.h"
#include "TuiFileTree.h"
#include "TuiMenuBar.h"

#include <QString>
#include <vector>
#include <memory>

namespace UberPad {

enum class TuiFocus {
    Editor,
    Sidebar
};

class TuiApp {
public:
    TuiApp();
    ~TuiApp();

    int run(const QStringList &files);

private:
    void initCurses();
    void cleanupCurses();
    void render();
    void handleInput(int ch);
    void executeAction(TuiAction act);

    // Dialogs / Prompts
    QString prompt(const QString &msg, const QString &initial = QString());
    bool confirm(const QString &msg);

    // Buffer / Tab operations
    TuiBuffer* currentBuffer();
    const TuiBuffer* currentBuffer() const;
    void openFile(const QString &filePath);
    void newBuffer();
    bool closeCurrentTab();

    // Editor actions
    void onSave();
    void onSaveAs();
    void onFind();
    void onGoToLine();
    void onAbout();

    std::vector<std::unique_ptr<TuiBuffer>> m_buffers;
    int m_activeBufferIndex = 0;

    TuiHighlighter m_highlighter;
    TuiFileTree m_fileTree;
    TuiMenuBar m_menuBar;

    TuiFocus m_focus = TuiFocus::Editor;
    bool m_showSidebar = true;
    bool m_showLineNumbers = true;
    int m_sidebarWidth = 26;

    int m_viewTopRow = 0;
    int m_viewLeftCol = 0;
    int m_termRows = 24;
    int m_termCols = 80;
    int m_gutterWidth = 6;

    bool m_running = true;
    QString m_searchQuery;
    QString m_statusMessage;
};

} // namespace UberPad
