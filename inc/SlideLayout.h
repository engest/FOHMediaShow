#pragma once

#include <QString>
#include <QColor>
#include <QString>
#include <QRect>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include <QJsonDocument>
#include <QList>

struct SlideLayoutTimer {
    QString timerId; // The ID of the timer this element binds to
    QRect bounds;
    QString fontFamily;
    int fontSize;
    QColor fontColor;
    bool isBold;
    bool isItalic;
    Qt::Alignment alignment;
    
    QJsonObject toJson() const;
    static SlideLayoutTimer fromJson(const QJsonObject& json);
};

class SlideLayout {
public:
    SlideLayout();
    ~SlideLayout() = default;

    QString name;
    QString targetScreen; // Name of the Screen this layout targets (e.g. "Audience")
    int width;
    int height;
    QColor backgroundColor;
    QString backgroundImage;
    
    // Main text attributes
    QRect textBounds;
    QString fontFamily;
    int fontSize;
    QColor fontColor;
    bool isBold;
    bool isItalic;
    bool allCaps;
    Qt::Alignment alignment;

    // Next text attributes
    bool hasNextText;
    QRect nextTextBounds;
    QString nextFontFamily;
    int nextFontSize;
    QColor nextFontColor;
    bool nextIsBold;
    bool nextIsItalic;
    bool nextAllCaps;
    Qt::Alignment nextAlignment;

    QList<SlideLayoutTimer> timers;

    QJsonObject toJson() const;
    static SlideLayout fromJson(const QJsonObject& json);
};
