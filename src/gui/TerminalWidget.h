#pragma once

#include <QWidget>
#include <QScrollBar>
#include <QSocketNotifier>
#include <QTimer>
#include <vector>
#include <deque>

namespace UberPad {

struct TermCell {
    QChar ch = ' ';
    QColor fg = QColor(220, 220, 220);
    QColor bg = QColor(24, 24, 28);
    bool bold = false;
    bool underline = false;
};

class TerminalView : public QWidget {
    Q_OBJECT

public:
    explicit TerminalView(QWidget *parent = nullptr);
    ~TerminalView() override;

    bool startShell(const QString &workingDir = QString());
    void stopShell();
    void sendInput(const QByteArray &data);
    void changeDirectory(const QString &dir);
    void clearScreen();

    int cols() const { return m_cols; }
    int rows() const { return m_rows; }

    void setScrollBar(QScrollBar *scrollBar);

signals:
    void shellExited(int exitCode);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void onPtyRead();
    void onBlinkTimer();
    void onScrollValueChanged(int value);

private:
    void resetGrid();
    void resizeGrid(int cols, int rows);
    void processBytes(const QByteArray &data);
    void parseAnsi(const QByteArray &seq);
    void scrollUp();
    void updatePtySize();

    static QColor colorFrom256(int index);

    int m_masterFd = -1;
    pid_t m_childPid = -1;
    QSocketNotifier *m_notifier = nullptr;

    int m_cols = 80;
    int m_rows = 24;
    int m_charWidth = 9;
    int m_charHeight = 18;
    int m_charAscent = 14;

    int m_curX = 0;
    int m_curY = 0;
    bool m_cursorVisible = true;
    bool m_cursorBlinkState = true;
    QTimer *m_blinkTimer = nullptr;

    // Current text attributes
    QColor m_currentFg;
    QColor m_currentBg;
    bool m_currentBold = false;
    bool m_currentUnderline = false;

    // Color definitions
    QColor m_defaultFg;
    QColor m_defaultBg;
    static const QColor s_ansiColors[16];

    // Grid & scrollback
    std::vector<std::vector<TermCell>> m_screen;
    std::deque<std::vector<TermCell>> m_scrollback;
    int m_scrollOffset = 0; // 0 = at bottom
    QScrollBar *m_scrollBar = nullptr;

    // Parser state
    enum class ParseState {
        Normal,
        Escape,
        CSI,
        OSC
    };
    ParseState m_parseState = ParseState::Normal;
    QByteArray m_csiBuffer;

    // Selection
    bool m_hasSelection = false;
    QPoint m_selectStart;
    QPoint m_selectEnd;
};

class TerminalWidget : public QWidget {
    Q_OBJECT

public:
    explicit TerminalWidget(QWidget *parent = nullptr);
    ~TerminalWidget() override = default;

    void changeDirectory(const QString &dir);
    void clear();
    void restartShell();

private:
    TerminalView *m_view;
    QScrollBar *m_scrollBar;
};

} // namespace UberPad
