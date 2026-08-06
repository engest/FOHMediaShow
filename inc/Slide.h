#pragma once

#include <QTextDocument>
#include <QFont>
#include <QColor>
#include <QString>
#include <memory>

class Slide : public QObject {
    Q_OBJECT

public:
    explicit Slide(QObject* parent = nullptr, bool isFirst = true);
    explicit Slide(const QString& text, QObject* parent = nullptr, bool isFirst = true);
    ~Slide() override = default;

    /// Access the underlying document (e.g. to bind to a QTextEdit)
    QTextDocument* document() const { return m_document.get(); }

    /// Get/set the plain text content (convenience over the document)
    QString plainText() const;
    void setPlainText(const QString& text);

    /// Get/set the rich text (HTML) content
    QString html() const;
    void setHtml(const QString& html);

    /// Apply formatting to the entire document
    void setFont(const QFont& font);
    void setFontFamily(const QString& family);
    void setFontSize(int pointSize);
    void setBold(bool bold);
    void setItalic(bool italic);
    void setTextColor(const QColor& color);
    void setIsFirst(bool isFirst);
    bool isFirst() const { return firstSlide; }

    /// Read back current formatting (from the document's default)
    QFont font() const;
    QColor textColor() const;

    const QMap<QString, QString>& layouts() const { return m_layouts; }
    QString layoutForScreen(const QString& screenName) const { return m_layouts.value(screenName); }
    void setLayoutForScreen(const QString& screenName, const QString& layoutFile) { m_layouts[screenName] = layoutFile; }
    void setLayouts(const QMap<QString, QString>& layouts) { m_layouts = layouts; }

signals:
    void contentChanged();

private:
    void applyDefaultFormat();

    std::unique_ptr<QTextDocument> m_document;
    QFont  m_font;
    QColor m_textColor = Qt::white;
    bool firstSlide;
    QMap<QString, QString> m_layouts;
};
