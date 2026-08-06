#pragma once

#include <QString>
#include <QColor>
#include <QMap>

class ThemeManager {
public:
    static QString getGlobalStyleSheet();
    static void applyTheme();

    // ArrangementEditorWidget
    static QString getCloseButtonStyleSheet();
    static QMap<QString, QColor> getComponentBaseColorMap();
    static QString getSequenceChipStyleSheet(const QString& bgColor);

    // DraggableComponentBtn
    static QString getBaseStyle(QColor baseColor);
    static QString getHoverStyle(QColor hoverColor);

    // Emoji icon generator
    static class QIcon getEmojiIcon(const QString& emoji, int pointSize = 16);
};
