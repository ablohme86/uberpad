#pragma once

#include <QString>
#include <QDir>
#include <QFileInfo>
#include <vector>
#include <set>

namespace UberPad {

struct TuiTreeItem {
    QString name;
    QString fullPath;
    bool isDir = false;
    bool isExpanded = false;
    int depth = 0;
};

class TuiFileTree {
public:
    explicit TuiFileTree(const QString &rootPath = QString());

    void setRootPath(const QString &path);
    QString rootPath() const { return m_rootPath; }

    void refresh();
    void moveUp();
    void moveDown();

    // Returns file path if a file was selected to open, empty string otherwise
    QString activateCurrent();

    int itemCount() const { return (int)m_items.size(); }
    int selectedIndex() const { return m_selectedIndex; }

    void render(int startRow, int startCol, int width, int height, bool hasFocus);

private:
    void populateItems();
    void addDirectory(const QString &dirPath, int depth);

    QString m_rootPath;
    std::vector<TuiTreeItem> m_items;
    std::set<QString> m_expandedDirs;
    int m_selectedIndex = 0;
    int m_scrollOffset = 0;
};

} // namespace UberPad
