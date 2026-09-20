#include <QApplication>
#include <QCommandLineParser>
#include <QStyleFactory>
#include <QIcon>
#include <iostream>
#include <cstdlib>

#include "gui/MainWindow.h"
#include "tui/TuiApp.h"

int main(int argc, char *argv[]) {
    // Check if user requested TUI or if headless terminal environment
    bool forceTui = false;
    bool forceGui = false;
    QStringList fileArgs;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-t" || arg == "--tui") {
            forceTui = true;
        } else if (arg == "-g" || arg == "--gui") {
            forceGui = true;
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "UberPad - Modern Notepad++ equivalent in C++/Qt6 with TUI support\n\n"
                      << "Usage: uberpad [options] [files...]\n\n"
                      << "Options:\n"
                      << "  -t, --tui      Run in Terminal User Interface (TUI) mode\n"
                      << "  -g, --gui      Force Graphical User Interface (GUI) mode\n"
                      << "  -v, --version  Show version information\n"
                      << "  -h, --help     Show this help message\n\n"
                      << "Features:\n"
                      << "  - 460+ syntax highlighters with auto-detection (KDE KF6SyntaxHighlighting)\n"
                      << "  - Notepad++ style tabbed multi-document interface & Folder as Workspace\n"
                      << "  - Integrated interactive PTY Terminal in GUI mode\n"
                      << "  - Full-featured standalone TUI mode for terminal and SSH editing\n"
                      << "  - Live search & replace with regex, bracket matching, smart indentation\n";
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "UberPad version 1.0.0 (C++20 / Qt6 / KF6SyntaxHighlighting)\n";
            return 0;
        } else if (!arg.empty() && arg[0] != '-') {
            fileArgs.append(QString::fromLocal8Bit(argv[i]));
        }
    }

    bool hasDisplay = (std::getenv("DISPLAY") != nullptr) || (std::getenv("WAYLAND_DISPLAY") != nullptr);

    if (forceTui || (!hasDisplay && !forceGui)) {
        // Run in TUI Mode
        QCoreApplication app(argc, argv);
        QCoreApplication::setApplicationName(QStringLiteral("UberPad"));
        QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0"));

        UberPad::TuiApp tui;
        return tui.run(fileArgs);
    }

    // Run in GUI Mode
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("UberPad"));
    app.setApplicationDisplayName(QStringLiteral("UberPad"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));
    app.setOrganizationName(QStringLiteral("UberPad"));

    QIcon appIcon(QStringLiteral(":/icons/uberpad-256.png"));
    appIcon.addFile(QStringLiteral(":/icons/uberpad-64.png"), QSize(64, 64));
    appIcon.addFile(QStringLiteral(":/icons/uberpad-32.png"), QSize(32, 32));
    appIcon.addFile(QStringLiteral(":/icons/uberpad.png"));
    app.setWindowIcon(appIcon);

    // Modern Fusion style
    if (QStyleFactory::keys().contains(QStringLiteral("Fusion"), Qt::CaseInsensitive)) {
        app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    }

    UberPad::MainWindow win;
    win.show();

    if (!fileArgs.isEmpty()) {
        win.openFiles(fileArgs);
    }

    return app.exec();
}
