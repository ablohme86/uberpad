cmake_minimum_required(VERSION 3.20)
project(UberPad VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(KF6SyntaxHighlighting REQUIRED)
find_package(Curses REQUIRED)

set(SOURCES
    src/main.cpp
    src/core/Config.h
    src/core/Config.cpp
    src/core/SyntaxManager.h
    src/core/SyntaxManager.cpp
    src/gui/MainWindow.h
    src/gui/MainWindow.cpp
    src/gui/CodeEditor.h
    src/gui/CodeEditor.cpp
    src/gui/SearchReplaceBar.h
    src/gui/SearchReplaceBar.cpp
    src/gui/FileTreeWidget.h
    src/gui/FileTreeWidget.cpp
    src/gui/TerminalWidget.h
    src/gui/TerminalWidget.cpp
    src/gui/GoToLineDialog.h
    src/gui/GoToLineDialog.cpp
    src/tui/TuiApp.h
    src/tui/TuiApp.cpp
    src/tui/TuiBuffer.h
    src/tui/TuiBuffer.cpp
    src/tui/TuiHighlighter.h
    src/tui/TuiHighlighter.cpp
)

add_executable(uberpad ${SOURCES})

target_include_directories(uberpad PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CURSES_INCLUDE_DIRS}
)

target_link_libraries(uberpad PRIVATE
    Qt6::Widgets
    KF6::SyntaxHighlighting
    ${CURSES_LIBRARIES}
    util
)

install(TARGETS uberpad DESTINATION bin)
