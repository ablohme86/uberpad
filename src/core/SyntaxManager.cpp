#include "SyntaxManager.h"
#include <algorithm>

namespace UberPad {

SyntaxManager& SyntaxManager::instance() {
    static SyntaxManager mgr;
    return mgr;
}

SyntaxManager::SyntaxManager() {
}

KSyntaxHighlighting::Definition SyntaxManager::definitionForFileName(const QString &fileName) const {
    return m_repo.definitionForFileName(fileName);
}

KSyntaxHighlighting::Definition SyntaxManager::definitionForName(const QString &name) const {
    return m_repo.definitionForName(name);
}

KSyntaxHighlighting::Definition SyntaxManager::definitionForMimeType(const QString &mimeType) const {
    return m_repo.definitionForMimeType(mimeType);
}

KSyntaxHighlighting::Definition SyntaxManager::defaultDefinition() const {
    return m_repo.definitionForName(QStringLiteral("None"));
}

KSyntaxHighlighting::Theme SyntaxManager::theme(const QString &name) const {
    KSyntaxHighlighting::Theme th = m_repo.theme(name);
    if (!th.isValid()) {
        th = defaultDarkTheme();
    }
    return th;
}

KSyntaxHighlighting::Theme SyntaxManager::defaultDarkTheme() const {
    // Try Dracula, Breeze Dark, Monokai, then fallback
    auto th = m_repo.theme(QStringLiteral("Dracula"));
    if (th.isValid()) return th;
    th = m_repo.theme(QStringLiteral("Breeze Dark"));
    if (th.isValid()) return th;
    return m_repo.defaultTheme(KSyntaxHighlighting::Repository::DarkTheme);
}

KSyntaxHighlighting::Theme SyntaxManager::defaultLightTheme() const {
    auto th = m_repo.theme(QStringLiteral("Breeze Light"));
    if (th.isValid()) return th;
    return m_repo.defaultTheme(KSyntaxHighlighting::Repository::LightTheme);
}

QList<KSyntaxHighlighting::Theme> SyntaxManager::themes() const {
    return m_repo.themes();
}

QList<KSyntaxHighlighting::Definition> SyntaxManager::allDefinitions() const {
    auto defs = m_repo.definitions();
    std::sort(defs.begin(), defs.end(), [](const KSyntaxHighlighting::Definition &a, const KSyntaxHighlighting::Definition &b) {
        return a.name().compare(b.name(), Qt::CaseInsensitive) < 0;
    });
    return defs;
}

QMap<QString, QList<KSyntaxHighlighting::Definition>> SyntaxManager::definitionsBySection() const {
    QMap<QString, QList<KSyntaxHighlighting::Definition>> sections;
    for (const auto &def : allDefinitions()) {
        if (def.isHidden()) continue;
        QString sec = def.section();
        if (sec.isEmpty()) sec = QStringLiteral("Other");
        sections[sec].append(def);
    }
    return sections;
}

} // namespace UberPad
