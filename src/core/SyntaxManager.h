#pragma once

#include <QString>
#include <QList>
#include <QMap>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Theme>

namespace UberPad {

class SyntaxManager {
public:
    static SyntaxManager& instance();

    const KSyntaxHighlighting::Repository& repository() const { return m_repo; }
    KSyntaxHighlighting::Repository& repository() { return m_repo; }

    KSyntaxHighlighting::Definition definitionForFileName(const QString &fileName) const;
    KSyntaxHighlighting::Definition definitionForName(const QString &name) const;
    KSyntaxHighlighting::Definition definitionForMimeType(const QString &mimeType) const;
    KSyntaxHighlighting::Definition defaultDefinition() const;

    KSyntaxHighlighting::Theme theme(const QString &name) const;
    KSyntaxHighlighting::Theme defaultDarkTheme() const;
    KSyntaxHighlighting::Theme defaultLightTheme() const;
    QList<KSyntaxHighlighting::Theme> themes() const;

    QList<KSyntaxHighlighting::Definition> allDefinitions() const;
    QMap<QString, QList<KSyntaxHighlighting::Definition>> definitionsBySection() const;

private:
    SyntaxManager();
    ~SyntaxManager() = default;
    SyntaxManager(const SyntaxManager&) = delete;
    SyntaxManager& operator=(const SyntaxManager&) = delete;

    KSyntaxHighlighting::Repository m_repo;
};

} // namespace UberPad
