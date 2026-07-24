#include "../inc/Slide.h"

#include <QTextCursor>
#include <QTextCharFormat>

Slide::Slide(QObject* parent, bool isFirst)
    : QObject(parent)
    , m_document(std::make_unique<QTextDocument>(this))
    , m_font("BebasNeue-Regular", 72)
    , firstSlide(isFirst)
{
    applyDefaultFormat();
    connect(m_document.get(), &QTextDocument::contentsChanged,
            this, &Slide::contentChanged);
}

Slide::Slide(const QString& text, QObject* parent, bool isFirst)
    : QObject(parent)
    , m_document(std::make_unique<QTextDocument>(this))
    , m_font("BebasNeue-Regular", 72)
    , firstSlide(isFirst)
{
    applyDefaultFormat();
    m_document->setPlainText(text);
    connect(m_document.get(), &QTextDocument::contentsChanged,
            this, &Slide::contentChanged);
}

// ─── Plain / Rich Text Access ───────────────────────────────────────────────

QString Slide::plainText() const {
    return m_document->toPlainText();
}

void Slide::setPlainText(const QString& text) {
    m_document->setPlainText(text);
    applyDefaultFormat();
}

QString Slide::html() const {
    return m_document->toHtml();
}

void Slide::setHtml(const QString& html) {
    m_document->setHtml(html);
}

// ─── Formatting ─────────────────────────────────────────────────────────────

void Slide::setFont(const QFont& font) {
    m_font = font;
    applyDefaultFormat();
}

void Slide::setFontFamily(const QString& family) {
    m_font.setFamily(family);
    applyDefaultFormat();
}

void Slide::setFontSize(int pointSize) {
    m_font.setPointSize(pointSize);
    applyDefaultFormat();
}

void Slide::setBold(bool bold) {
    m_font.setBold(bold);
    applyDefaultFormat();
}

void Slide::setItalic(bool italic) {
    m_font.setItalic(italic);
    applyDefaultFormat();
}

void Slide::setTextColor(const QColor& color) {
    m_textColor = color;
    applyDefaultFormat();
}

void Slide::setIsFirst(bool isFirst)
{
    firstSlide = isFirst;
}

QFont Slide::font() const {
    return m_font;
}

QColor Slide::textColor() const {
    return m_textColor;
}

// ─── Internal ───────────────────────────────────────────────────────────────

void Slide::applyDefaultFormat() {
    m_document->setDefaultFont(m_font);

    // Apply the text color to all existing content
    QTextCursor cursor(m_document.get());
    cursor.select(QTextCursor::Document);

    QTextCharFormat fmt;
    fmt.setForeground(m_textColor);
    cursor.mergeCharFormat(fmt);
}
