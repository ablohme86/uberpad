#include "TuiFileTree.h"
#include <ncurses.h>
#include <algorithm>

namespace UberPad {

TuiFileTree::TuiFileTree(const QString &rootPath) {
    if (!rootPath.isEmpty() && QDir(rootPath).exists()) {
        m_rootPath = rootPath;
    } else {
        m_rootPath = QDir::currentPath();
    }
    refresh();
}

void TuiFileTree::setRootPath(const QString &path) {
    if (!path.isEmpty() && QDir(path).exists()) {
        m_rootPath = path;
        m_selectedIndex = 0;
        m_scrollOffset = 0;
        refresh();
    }
}

void TuiFileTree::refresh() {
    populateItems();
}

void TuiFileTree::populateItems() {
    m_items.clear();

    // Add parent directory navigation
    QDir current(m_rootPath);
    if (!current.isRoot()) {
        TuiTreeItem up;
        up.name = QStringLiteral(".. (Parent Directory)");
        up.fullPath = current.absoluteFilePath(QStringLiteral(".."));
        up.isDir = true;
        up.depth = 0;
        m_items.push_back(up);
    }

    addDirectory(m_rootPath, 0);

    if (m_selectedIndex >= (int)m_items.size()) {
        m_selectedIndex = std::max(0, (int)m_items.size() - 1);
    }
}

void TuiFileTree::addDirectory(const QString &dirPath, int depth) {
    QDir dir(dirPath);
    QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot,
                                             QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);

    for (const QFileInfo &fi : entries) {
        if (fi.isDir()) {
            bool expanded = (m_expandedDirs.count(fi.absoluteFilePath()) > 0);
            TuiTreeItem item;
            item.name = fi.fileName();
            item.fullPath = fi.absoluteFilePath();
            item.isDir = true;
            item.isExpanded = expanded;
            item.depth = depth;
            m_items.push_back(item);

            if (expanded) {
                addDirectory(fi.absoluteFilePath(), depth + 1);
            }
        } else {
            TuiTreeItem item;
            item.name = fi.fileName();
            item.fullPath = fi.absoluteFilePath();
            item.isDir = false;
            item.depth = depth;
            m_items.push_back(item);
        }
    }
}

void TuiFileTree::moveUp() {
    if (m_selectedIndex > 0) {
        m_selectedIndex--;
    }
}

void TuiFileTree::moveDown() {
    if (m_selectedIndex + 1 < (int)m_items.size()) {
        m_selectedIndex++;
    }
}

QString TuiFileTree::activateCurrent() {
    if (m_selectedIndex < 0 || m_selectedIndex >= (int)m_items.size()) {
        return QString();
    }

    TuiTreeItem &item = m_items[m_selectedIndex];

    if (item.name.startsWith(QStringLiteral(".."))) {
        QDir d(m_rootPath);
        if (d.cdUp()) {
            setRootPath(d.absolutePath());
        }
        return QString();
    }

    if (item.isDir) {
        if (item.isExpanded) {
            m_expandedDirs.erase(item.fullPath);
        } else {
            m_expandedDirs.insert(item.fullPath);
        }
        populateItems();
        return QString();
    }

    return item.fullPath;
}

void TuiFileTree::render(int startRow, int startCol, int width, int height, bool hasFocus) {
    if (width <= 0 || height <= 0) return;

    // Header: Workspace folder name
    int headerAttr = hasFocus ? (COLOR_PAIR(11) | A_BOLD) : (COLOR_PAIR(12));
    attron(headerAttr);
    mvhline(startRow, startCol, ' ', width);
    QFileInfo rootFi(m_rootPath);
    QString header = QStringLiteral(" 📁 %1 ").arg(rootFi.fileName().isEmpty() ? m_rootPath : rootFi.fileName());
    mvaddnstr(startRow, startCol, header.toUtf8().constData(), width);
    attroff(headerAttr);

    int contentHeight = height - 1;
    if (contentHeight <= 0) return;

    // Adjust scroll offset
    if (m_selectedIndex < m_scrollOffset) {
        m_scrollOffset = m_selectedIndex;
    } else if (m_selectedIndex >= m_scrollOffset + contentHeight) {
        m_scrollOffset = m_selectedIndex - contentHeight + 1;
    }

    for (int i = 0; i < contentHeight; ++i) {
        int idx = m_scrollOffset + i;
        int y = startRow + 1 + i;

        mvhline(y, startCol, ' ', width);

        if (idx < (int)m_items.size()) {
            const TuiTreeItem &item = m_items[idx];
            bool isSelected = (idx == m_selectedIndex);

            int itemAttr = 1;
            if (isSelected) {
                itemAttr = hasFocus ? (COLOR_PAIR(10) | A_BOLD) : (COLOR_PAIR(12) | A_BOLD);
            } else if (item.isDir) {
                itemAttr = COLOR_PAIR(7) | A_BOLD; // Cyan for directories
            } else {
                itemAttr = COLOR_PAIR(1);          // Normal white for files
            }

            attron(itemAttr);

            QString prefix = QString(item.depth * 2, ' ');
            if (item.isDir) {
                if (item.name.startsWith(QStringLiteral(".."))) {
                    prefix += QStringLiteral("⮝ ");
                } else {
                    prefix += item.isExpanded ? QStringLiteral("▾ ") : QStringLiteral("▸ ");
                }
            } else {
                prefix += QStringLiteral("  ");
            }

            QString text = prefix + item.name;
            mvaddnstr(y, startCol, text.toUtf8().constData(), width);

            attroff(itemAttr);
        }
    }
}

} // namespace UberPad
