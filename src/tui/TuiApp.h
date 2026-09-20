#pragma once

#include "TuiBuffer.h"
#include "TuiHighlighter.h"

#include <QString>
#include <vector>

namespace UberPad {

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
    void recomputeStates();

    QString prompt(const QString &msg, const QString &initial = QString());
    bool confirm(const QString &msg);

    void onSave();
    void onFind();
    void onGoToLine();

    TuiBuffer m_buffer;
    TuiHighlighter m_highlighter;

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
