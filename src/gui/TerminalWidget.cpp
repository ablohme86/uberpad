#include "TerminalWidget.h"
#include "../core/Config.h"

#include <pty.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <signal.h>

#include <QPainter>
#include <QFontMetrics>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolButton>
#include <QLabel>

namespace UberPad {

const QColor TerminalView::s_ansiColors[16] = {
    QColor(0x28, 0x2a, 0x36), // 0: Black
    QColor(0xff, 0x55, 0x55), // 1: Red
    QColor(0x50, 0xfa, 0x7b), // 2: Green
    QColor(0xf1, 0xfa, 0x8c), // 3: Yellow
    QColor(0xbd, 0x93, 0xf9), // 4: Blue
    QColor(0xff, 0x79, 0xc6), // 5: Magenta
    QColor(0x8b, 0xe9, 0xfd), // 6: Cyan
    QColor(0xbf, 0xbf, 0xbf), // 7: White
    QColor(0x4d, 0x4d, 0x4d), // 8: Bright Black
    QColor(0xff, 0x6e, 0x6e), // 9: Bright Red
    QColor(0x69, 0xff, 0x94), // 10: Bright Green
    QColor(0xff, 0xff, 0xa5), // 11: Bright Yellow
    QColor(0xd6, 0xac, 0xff), // 12: Bright Blue
    QColor(0xff, 0x92, 0xdf), // 13: Bright Magenta
    QColor(0xa4, 0xff, 0xff), // 14: Bright Cyan
    QColor(0xff, 0xff, 0xff)  // 15: Bright White
};

QColor TerminalView::colorFrom256(int idx) {
    if (idx < 0) idx = 0;
    if (idx < 16) return s_ansiColors[idx];
    if (idx >= 16 && idx <= 231) {
        int val = idx - 16;
        int r = (val / 36) * 51;
        int g = ((val / 6) % 6) * 51;
        int b = (val % 6) * 51;
        return QColor(r, g, b);
    }
    // Grayscale
    int gray = 8 + (idx - 232) * 10;
    return QColor(gray, gray, gray);
}

TerminalView::TerminalView(QWidget *parent)
    : QWidget(parent)
    , m_defaultFg(QColor(230, 230, 230))
    , m_defaultBg(QColor(24, 24, 30))
{
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent);

    m_currentFg = m_defaultFg;
    m_currentBg = m_defaultBg;

    QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    f.setPointSize(10);
    setFont(f);

    QFontMetrics fm(f);
    m_charWidth = std::max(6, fm.horizontalAdvance(QLatin1Char('M')));
    m_charHeight = std::max(12, fm.height());
    m_charAscent = fm.ascent();

    m_blinkTimer = new QTimer(this);
    connect(m_blinkTimer, &QTimer::timeout, this, &TerminalView::onBlinkTimer);
    m_blinkTimer->start(600);

    resetGrid();
    startShell();
}

TerminalView::~TerminalView() {
    stopShell();
}

void TerminalView::setScrollBar(QScrollBar *scrollBar) {
    m_scrollBar = scrollBar;
    connect(m_scrollBar, &QScrollBar::valueChanged, this, &TerminalView::onScrollValueChanged);
}

void TerminalView::onScrollValueChanged(int value) {
    m_scrollOffset = m_scrollBar->maximum() - value;
    update();
}

void TerminalView::resetGrid() {
    m_screen.assign(m_rows, std::vector<TermCell>(m_cols));
    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            m_screen[r][c].fg = m_defaultFg;
            m_screen[r][c].bg = m_defaultBg;
            m_screen[r][c].ch = ' ';
        }
    }
    m_curX = 0;
    m_curY = 0;
}

void TerminalView::resizeGrid(int cols, int rows) {
    cols = std::max(10, cols);
    rows = std::max(3, rows);
    if (cols == m_cols && rows == m_rows) return;

    std::vector<std::vector<TermCell>> newScreen(rows, std::vector<TermCell>(cols));
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (r < (int)m_screen.size() && c < (int)m_screen[r].size()) {
                newScreen[r][c] = m_screen[r][c];
            } else {
                newScreen[r][c].fg = m_defaultFg;
                newScreen[r][c].bg = m_defaultBg;
                newScreen[r][c].ch = ' ';
            }
        }
    }

    m_cols = cols;
    m_rows = rows;
    m_screen = std::move(newScreen);
    m_curX = std::min(m_curX, m_cols - 1);
    m_curY = std::min(m_curY, m_rows - 1);

    updatePtySize();
    update();
}

void TerminalView::updatePtySize() {
    if (m_masterFd >= 0) {
        struct winsize ws;
        ws.ws_col = (unsigned short)m_cols;
        ws.ws_row = (unsigned short)m_rows;
        ws.ws_xpixel = 0;
        ws.ws_ypixel = 0;
        ioctl(m_masterFd, TIOCSWINSZ, &ws);
        if (m_childPid > 0) {
            kill(m_childPid, SIGWINCH);
        }
    }
}

bool TerminalView::startShell(const QString &workingDir) {
    stopShell();

    struct winsize ws;
    ws.ws_col = (unsigned short)m_cols;
    ws.ws_row = (unsigned short)m_rows;
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;

    int master;
    pid_t pid = forkpty(&master, nullptr, nullptr, &ws);
    if (pid < 0) {
        return false;
    }

    if (pid == 0) {
        // Child Process
        if (!workingDir.isEmpty()) {
            chdir(workingDir.toLocal8Bit().constData());
        }
        setenv("TERM", "xterm-256color", 1);
        setenv("COLORTERM", "truecolor", 1);

        QString shell = Config::instance().defaultShell();
        QByteArray shellBytes = shell.toLocal8Bit();
        execlp(shellBytes.constData(), shellBytes.constData(), nullptr);
        _exit(1);
    }

    // Parent Process
    m_masterFd = master;
    m_childPid = pid;

    // Set non-blocking
    int flags = fcntl(m_masterFd, F_GETFL, 0);
    fcntl(m_masterFd, F_SETFL, flags | O_NONBLOCK);

    m_notifier = new QSocketNotifier(m_masterFd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &TerminalView::onPtyRead);

    resetGrid();
    return true;
}

void TerminalView::stopShell() {
    if (m_notifier) {
        delete m_notifier;
        m_notifier = nullptr;
    }
    if (m_masterFd >= 0) {
        ::close(m_masterFd);
        m_masterFd = -1;
    }
    if (m_childPid > 0) {
        kill(m_childPid, SIGTERM);
        int status;
        waitpid(m_childPid, &status, WNOHANG);
        m_childPid = -1;
    }
}

void TerminalView::onPtyRead() {
    if (m_masterFd < 0) return;

    char buf[4096];
    while (true) {
        ssize_t n = read(m_masterFd, buf, sizeof(buf));
        if (n > 0) {
            processBytes(QByteArray(buf, n));
        } else {
            break;
        }
    }
    update();
}

void TerminalView::sendInput(const QByteArray &data) {
    if (m_masterFd >= 0 && !data.isEmpty()) {
        write(m_masterFd, data.constData(), data.size());
    }
}

void TerminalView::changeDirectory(const QString &dir) {
    if (!dir.isEmpty()) {
        QByteArray cmd = "cd \"" + dir.toUtf8() + "\"\n";
        sendInput(cmd);
    }
}

void TerminalView::clearScreen() {
    resetGrid();
    m_scrollback.clear();
    if (m_scrollBar) {
        m_scrollBar->setRange(0, 0);
    }
    update();
}

void TerminalView::scrollUp() {
    m_scrollback.push_back(m_screen[0]);
    if (m_scrollback.size() > 2000) {
        m_scrollback.pop_front();
    }

    for (int r = 0; r < m_rows - 1; ++r) {
        m_screen[r] = m_screen[r + 1];
    }

    // Blank new bottom row
    m_screen[m_rows - 1].assign(m_cols, TermCell{ ' ', m_defaultFg, m_defaultBg, false, false });

    if (m_scrollBar) {
        m_scrollBar->setRange(0, (int)m_scrollback.size());
        m_scrollBar->setValue((int)m_scrollback.size());
    }
}

void TerminalView::processBytes(const QByteArray &data) {
    for (int i = 0; i < data.size(); ++i) {
        char b = data[i];

        if (m_parseState == ParseState::Escape) {
            if (b == '[') {
                m_parseState = ParseState::CSI;
                m_csiBuffer.clear();
            } else if (b == ']') {
                m_parseState = ParseState::OSC;
            } else {
                m_parseState = ParseState::Normal;
            }
            continue;
        } else if (m_parseState == ParseState::CSI) {
            // Check if final character
            if ((b >= '@' && b <= '~')) {
                m_csiBuffer.append(b);
                parseAnsi(m_csiBuffer);
                m_parseState = ParseState::Normal;
            } else {
                m_csiBuffer.append(b);
            }
            continue;
        } else if (m_parseState == ParseState::OSC) {
            // OSC ends with BEL (0x07) or ST (ESC \)
            if (b == '\x07' || b == '\x1b') {
                m_parseState = ParseState::Normal;
            }
            continue;
        }

        // Normal state
        if (b == '\x1b') {
            m_parseState = ParseState::Escape;
        } else if (b == '\r') {
            m_curX = 0;
        } else if (b == '\n') {
            m_curY++;
            if (m_curY >= m_rows) {
                m_curY = m_rows - 1;
                scrollUp();
            }
        } else if (b == '\b') {
            m_curX = std::max(0, m_curX - 1);
        } else if (b == '\t') {
            m_curX = (m_curX / 8 + 1) * 8;
            if (m_curX >= m_cols) {
                m_curX = 0;
                m_curY++;
                if (m_curY >= m_rows) {
                    m_curY = m_rows - 1;
                    scrollUp();
                }
            }
        } else if ((unsigned char)b >= 32) {
            if (m_curX >= m_cols) {
                m_curX = 0;
                m_curY++;
                if (m_curY >= m_rows) {
                    m_curY = m_rows - 1;
                    scrollUp();
                }
            }
            m_screen[m_curY][m_curX] = TermCell{
                QChar(QLatin1Char(b)),
                m_currentFg,
                m_currentBg,
                m_currentBold,
                m_currentUnderline
            };
            m_curX++;
        }
    }
}

void TerminalView::parseAnsi(const QByteArray &seq) {
    if (seq.isEmpty()) return;
    char command = seq.back();
    QByteArray params = seq.left(seq.size() - 1);

    auto parts = params.split(';');
    std::vector<int> args;
    for (const auto &p : parts) {
        if (!p.isEmpty()) {
            args.push_back(p.toInt());
        }
    }

    if (command == 'm') {
        // SGR formatting
        if (args.empty()) args.push_back(0);
        for (size_t i = 0; i < args.size(); ++i) {
            int code = args[i];
            if (code == 0) {
                m_currentFg = m_defaultFg;
                m_currentBg = m_defaultBg;
                m_currentBold = false;
                m_currentUnderline = false;
            } else if (code == 1) {
                m_currentBold = true;
            } else if (code == 4) {
                m_currentUnderline = true;
            } else if (code >= 30 && code <= 37) {
                m_currentFg = s_ansiColors[code - 30];
            } else if (code == 39) {
                m_currentFg = m_defaultFg;
            } else if (code >= 40 && code <= 47) {
                m_currentBg = s_ansiColors[code - 40];
            } else if (code == 49) {
                m_currentBg = m_defaultBg;
            } else if (code >= 90 && code <= 97) {
                m_currentFg = s_ansiColors[code - 90 + 8];
            } else if (code >= 100 && code <= 107) {
                m_currentBg = s_ansiColors[code - 100 + 8];
            } else if (code == 38 && i + 2 < args.size() && args[i + 1] == 5) {
                // 256 colors fg
                m_currentFg = colorFrom256(args[i + 2]);
                i += 2;
            } else if (code == 48 && i + 2 < args.size() && args[i + 1] == 5) {
                // 256 colors bg
                m_currentBg = colorFrom256(args[i + 2]);
                i += 2;
            } else if (code == 38 && i + 4 < args.size() && args[i + 1] == 2) {
                // Truecolor fg
                m_currentFg = QColor(args[i + 2], args[i + 3], args[i + 4]);
                i += 4;
            } else if (code == 48 && i + 4 < args.size() && args[i + 1] == 2) {
                // Truecolor bg
                m_currentBg = QColor(args[i + 2], args[i + 3], args[i + 4]);
                i += 4;
            }
        }
    } else if (command == 'H' || command == 'f') {
        int r = (args.size() > 0 && args[0] > 0) ? args[0] - 1 : 0;
        int c = (args.size() > 1 && args[1] > 0) ? args[1] - 1 : 0;
        m_curY = std::clamp(r, 0, m_rows - 1);
        m_curX = std::clamp(c, 0, m_cols - 1);
    } else if (command == 'A') {
        int n = (!args.empty() && args[0] > 0) ? args[0] : 1;
        m_curY = std::max(0, m_curY - n);
    } else if (command == 'B') {
        int n = (!args.empty() && args[0] > 0) ? args[0] : 1;
        m_curY = std::min(m_rows - 1, m_curY + n);
    } else if (command == 'C') {
        int n = (!args.empty() && args[0] > 0) ? args[0] : 1;
        m_curX = std::min(m_cols - 1, m_curX + n);
    } else if (command == 'D') {
        int n = (!args.empty() && args[0] > 0) ? args[0] : 1;
        m_curX = std::max(0, m_curX - n);
    } else if (command == 'K') {
        int mode = args.empty() ? 0 : args[0];
        if (mode == 0) {
            // From cursor to end of line
            for (int c = m_curX; c < m_cols; ++c) {
                m_screen[m_curY][c] = TermCell{ ' ', m_defaultFg, m_defaultBg, false, false };
            }
        } else if (mode == 1) {
            // From start of line to cursor
            for (int c = 0; c <= m_curX; ++c) {
                m_screen[m_curY][c] = TermCell{ ' ', m_defaultFg, m_defaultBg, false, false };
            }
        } else if (mode == 2) {
            // Whole line
            for (int c = 0; c < m_cols; ++c) {
                m_screen[m_curY][c] = TermCell{ ' ', m_defaultFg, m_defaultBg, false, false };
            }
        }
    } else if (command == 'J') {
        int mode = args.empty() ? 0 : args[0];
        if (mode == 2) {
            resetGrid();
        } else if (mode == 3) {
            resetGrid();
            m_scrollback.clear();
        }
    }
}

void TerminalView::onBlinkTimer() {
    m_cursorBlinkState = !m_cursorBlinkState;
    update();
}

void TerminalView::paintEvent(QPaintEvent * /* event */) {
    QPainter painter(this);
    painter.fillRect(rect(), m_defaultBg);

    int startRow = 0;
    int scrollCount = (int)m_scrollback.size();

    for (int r = 0; r < m_rows; ++r) {
        int actualIndex = r - m_scrollOffset;
        const std::vector<TermCell> *line = nullptr;

        if (actualIndex < 0) {
            int histIdx = scrollCount + actualIndex;
            if (histIdx >= 0 && histIdx < scrollCount) {
                line = &m_scrollback[histIdx];
            }
        } else if (actualIndex < m_rows) {
            line = &m_screen[actualIndex];
        }

        if (!line) continue;

        int y = r * m_charHeight;
        for (int c = 0; c < m_cols && c < (int)line->size(); ++c) {
            const TermCell &cell = (*line)[c];
            int x = c * m_charWidth;

            // Background
            if (cell.bg != m_defaultBg) {
                painter.fillRect(x, y, m_charWidth, m_charHeight, cell.bg);
            }

            // Foreground text
            if (cell.ch != ' ') {
                painter.setPen(cell.fg);
                QFont f = font();
                f.setBold(cell.bold);
                f.setUnderline(cell.underline);
                painter.setFont(f);
                painter.drawText(x, y + m_charAscent, QString(cell.ch));
            }
        }
    }

    // Draw cursor if at bottom and active
    if (m_scrollOffset == 0 && m_cursorVisible && m_cursorBlinkState && hasFocus()) {
        int cx = m_curX * m_charWidth;
        int cy = m_curY * m_charHeight;
        painter.fillRect(cx, cy, m_charWidth, m_charHeight, QColor(240, 240, 240, 180));
    }
}

void TerminalView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    int newCols = std::max(10, width() / m_charWidth);
    int newRows = std::max(3, height() / m_charHeight);
    resizeGrid(newCols, newRows);
}

void TerminalView::keyPressEvent(QKeyEvent *event) {
    // Scroll to bottom on key press
    if (m_scrollOffset > 0) {
        m_scrollOffset = 0;
        if (m_scrollBar) m_scrollBar->setValue(m_scrollBar->maximum());
        update();
    }

    // Copy / Paste shortcuts
    if ((event->modifiers() & Qt::ControlModifier) && (event->modifiers() & Qt::ShiftModifier)) {
        if (event->key() == Qt::Key_C) {
            // Copy
            return;
        }
        if (event->key() == Qt::Key_V) {
            // Paste
            QString text = QApplication::clipboard()->text();
            sendInput(text.toUtf8());
            return;
        }
    }

    // Special Keys
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        sendInput("\r");
        return;
    }
    if (event->key() == Qt::Key_Backspace) {
        sendInput("\x7f");
        return;
    }
    if (event->key() == Qt::Key_Tab) {
        sendInput("\t");
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        sendInput("\x1b");
        return;
    }
    if (event->key() == Qt::Key_Up) {
        sendInput("\x1b[A");
        return;
    }
    if (event->key() == Qt::Key_Down) {
        sendInput("\x1b[B");
        return;
    }
    if (event->key() == Qt::Key_Right) {
        sendInput("\x1b[C");
        return;
    }
    if (event->key() == Qt::Key_Left) {
        sendInput("\x1b[D");
        return;
    }
    if (event->key() == Qt::Key_Home) {
        sendInput("\x1b[H");
        return;
    }
    if (event->key() == Qt::Key_End) {
        sendInput("\x1b[F");
        return;
    }
    if (event->key() == Qt::Key_PageUp) {
        sendInput("\x1b[5~");
        return;
    }
    if (event->key() == Qt::Key_PageDown) {
        sendInput("\x1b[6~");
        return;
    }
    if (event->key() == Qt::Key_Delete) {
        sendInput("\x1b[3~");
        return;
    }

    // Control key combinations (Ctrl+C, Ctrl+D, Ctrl+L, etc.)
    if (event->modifiers() & Qt::ControlModifier) {
        int k = event->key();
        if (k >= Qt::Key_A && k <= Qt::Key_Z) {
            char ctrl = (char)(k - Qt::Key_A + 1);
            sendInput(QByteArray(1, ctrl));
            return;
        }
    }

    // Normal text
    QString text = event->text();
    if (!text.isEmpty()) {
        sendInput(text.toUtf8());
        return;
    }

    QWidget::keyPressEvent(event);
}

void TerminalView::mousePressEvent(QMouseEvent *event) {
    setFocus();
    QWidget::mousePressEvent(event);
}

void TerminalView::mouseMoveEvent(QMouseEvent *event) {
    QWidget::mouseMoveEvent(event);
}

void TerminalView::wheelEvent(QWheelEvent *event) {
    int degrees = event->angleDelta().y() / 8;
    int steps = degrees / 15;
    if (steps != 0 && m_scrollBar) {
        m_scrollBar->setValue(m_scrollBar->value() - steps * 3);
    }
}

// TerminalWidget Implementation
TerminalWidget::TerminalWidget(QWidget *parent)
    : QWidget(parent)
    , m_view(new TerminalView(this))
    , m_scrollBar(new QScrollBar(Qt::Vertical, this))
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Mini toolbar
    QWidget *bar = new QWidget(this);
    bar->setStyleSheet("background-color: palette(window); border-bottom: 1px solid palette(mid);");
    QHBoxLayout *barLayout = new QHBoxLayout(bar);
    barLayout->setContentsMargins(6, 2, 6, 2);
    barLayout->setSpacing(6);

    QLabel *title = new QLabel(tr("🖥 Integrated Terminal"), bar);
    title->setStyleSheet("font-weight: bold; font-size: 11px;");

    QToolButton *clearBtn = new QToolButton(bar);
    clearBtn->setText(tr("Clear"));
    clearBtn->setToolTip(tr("Clear terminal screen"));
    connect(clearBtn, &QToolButton::clicked, this, &TerminalWidget::clear);

    QToolButton *restartBtn = new QToolButton(bar);
    restartBtn->setText(tr("Restart"));
    restartBtn->setToolTip(tr("Restart shell"));
    connect(restartBtn, &QToolButton::clicked, this, &TerminalWidget::restartShell);

    barLayout->addWidget(title);
    barLayout->addStretch();
    barLayout->addWidget(clearBtn);
    barLayout->addWidget(restartBtn);

    mainLayout->addWidget(bar);

    // Terminal View + ScrollBar
    QHBoxLayout *viewLayout = new QHBoxLayout();
    viewLayout->setContentsMargins(0, 0, 0, 0);
    viewLayout->setSpacing(0);

    m_view->setScrollBar(m_scrollBar);

    viewLayout->addWidget(m_view, 1);
    viewLayout->addWidget(m_scrollBar);

    mainLayout->addLayout(viewLayout, 1);
}

void TerminalWidget::changeDirectory(const QString &dir) {
    m_view->changeDirectory(dir);
}

void TerminalWidget::clear() {
    m_view->clearScreen();
}

void TerminalWidget::restartShell() {
    m_view->startShell();
}

} // namespace UberPad
