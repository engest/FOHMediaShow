#include "../inc/SlideLayout.h"

SlideLayout::SlideLayout()
    : name("New Layout")
    , targetScreen("Audience")
    , width(1920)
    , height(1080)
    , backgroundColor(Qt::black)
    , textBounds(100, 100, 1720, 880)
    , fontFamily("Arial")
    , fontSize(72)
    , fontColor(Qt::white)
    , isBold(false)
    , isItalic(false)
    , allCaps(false)
    , alignment(Qt::AlignCenter)
    , hasNextText(false)
    , nextTextBounds(100, 800, 1720, 200)
    , nextFontFamily("Arial")
    , nextFontSize(48)
    , nextFontColor(QColor(Qt::gray))
    , nextIsBold(false)
    , nextIsItalic(false)
    , nextAllCaps(false)
    , nextAlignment(Qt::AlignCenter)
{
}

QJsonObject SlideLayoutTimer::toJson() const {
    QJsonObject json;
    json["timerId"] = timerId;
    
    QJsonObject boundsObj;
    boundsObj["x"] = bounds.x();
    boundsObj["y"] = bounds.y();
    boundsObj["width"] = bounds.width();
    boundsObj["height"] = bounds.height();
    json["bounds"] = boundsObj;

    json["fontFamily"] = fontFamily;
    json["fontSize"] = fontSize;
    json["fontColor"] = fontColor.name();
    json["isBold"] = isBold;
    json["isItalic"] = isItalic;
    json["alignment"] = static_cast<int>(alignment);
    
    return json;
}

SlideLayoutTimer SlideLayoutTimer::fromJson(const QJsonObject& json) {
    SlideLayoutTimer timer;
    timer.timerId = json["timerId"].toString();
    
    if (json.contains("bounds")) {
        QJsonObject boundsObj = json["bounds"].toObject();
        timer.bounds = QRect(
            boundsObj["x"].toInt(),
            boundsObj["y"].toInt(),
            boundsObj["width"].toInt(),
            boundsObj["height"].toInt()
        );
    }
    
    timer.fontFamily = json["fontFamily"].toString("Arial");
    timer.fontSize = json["fontSize"].toInt(72);
    timer.fontColor = QColor(json["fontColor"].toString("#FFFFFF"));
    timer.isBold = json["isBold"].toBool(false);
    timer.isItalic = json["isItalic"].toBool(false);
    timer.alignment = static_cast<Qt::Alignment>(json["alignment"].toInt(Qt::AlignCenter));
    
    return timer;
}

QJsonObject SlideLayout::toJson() const {
    QJsonObject json;
    json["name"] = name;
    json["targetScreen"] = targetScreen;
    json["width"] = width;
    json["height"] = height;
    json["backgroundColor"] = backgroundColor.name();
    json["backgroundImage"] = backgroundImage;
    
    QJsonObject bounds;
    bounds["x"] = textBounds.x();
    bounds["y"] = textBounds.y();
    bounds["width"] = textBounds.width();
    bounds["height"] = textBounds.height();
    json["textBounds"] = bounds;

    QJsonObject font;
    font["family"] = fontFamily;
    font["size"] = fontSize;
    font["color"] = fontColor.name();
    font["bold"] = isBold;
    font["italic"] = isItalic;
    font["allCaps"] = allCaps;
    json["font"] = font;

    json["alignment"] = static_cast<int>(alignment);
    
    json["hasNextText"] = hasNextText;

    QJsonObject nBounds;
    nBounds["x"] = nextTextBounds.x();
    nBounds["y"] = nextTextBounds.y();
    nBounds["width"] = nextTextBounds.width();
    nBounds["height"] = nextTextBounds.height();
    json["nextTextBounds"] = nBounds;

    QJsonObject nFont;
    nFont["family"] = nextFontFamily;
    nFont["size"] = nextFontSize;
    nFont["color"] = nextFontColor.name();
    nFont["bold"] = nextIsBold;
    nFont["italic"] = nextIsItalic;
    nFont["allCaps"] = nextAllCaps;
    json["nextFont"] = nFont;
    
    json["nextIsItalic"] = nextIsItalic;
    json["nextAllCaps"] = nextAllCaps;
    json["nextAlignment"] = static_cast<int>(nextAlignment);

    QJsonArray timersArr;
    for (const auto& timer : timers) {
        timersArr.append(timer.toJson());
    }
    json["timers"] = timersArr;

    return json;
}

SlideLayout SlideLayout::fromJson(const QJsonObject& json) {
    SlideLayout layout;
    layout.name = json["name"].toString("New Layout");
    
    // Backwards compatibility with old files
    if (json.contains("type")) {
        layout.targetScreen = (json["type"].toString("Audience") == "Stage") ? "Stage" : "Audience";
    } else {
        layout.targetScreen = json["targetScreen"].toString("Audience");
    }
    
    layout.width = json["width"].toInt(1920);
    layout.height = json["height"].toInt(1080);
    layout.backgroundColor = QColor(json["backgroundColor"].toString("#000000"));
    layout.backgroundImage = json["backgroundImage"].toString("");

    if (json.contains("textBounds") && json["textBounds"].isObject()) {
        QJsonObject bounds = json["textBounds"].toObject();
        layout.textBounds = QRect(bounds["x"].toInt(0), bounds["y"].toInt(0),
                                  bounds["width"].toInt(1920), bounds["height"].toInt(1080));
    }

    if (json.contains("font") && json["font"].isObject()) {
        QJsonObject font = json["font"].toObject();
        layout.fontFamily = font["family"].toString("Bebas Neue");
        layout.fontSize = font["size"].toInt(90);
        layout.fontColor = QColor(font["color"].toString("#ffffff"));
        layout.isBold = font["bold"].toBool(false);
        layout.isItalic = font["italic"].toBool(false);
        layout.allCaps = font["allCaps"].toBool(false);
    }

    if (json.contains("alignment")) {
        layout.alignment = static_cast<Qt::Alignment>(json["alignment"].toInt(static_cast<int>(Qt::AlignCenter)));
    }
    
    layout.hasNextText = json["hasNextText"].toBool(false);

    if (json.contains("nextTextBounds") && json["nextTextBounds"].isObject()) {
        QJsonObject bounds = json["nextTextBounds"].toObject();
        layout.nextTextBounds = QRect(bounds["x"].toInt(0), bounds["y"].toInt(0),
                                  bounds["width"].toInt(1920), bounds["height"].toInt(1080));
    }

    if (json.contains("nextFont") && json["nextFont"].isObject()) {
        QJsonObject font = json["nextFont"].toObject();
        layout.nextFontFamily = font["family"].toString("Bebas Neue");
        layout.nextFontSize = font["size"].toInt(70);
        layout.nextFontColor = QColor(font["color"].toString("#ffa500"));
        layout.nextIsBold = font["bold"].toBool(false);
        layout.nextIsItalic = font["italic"].toBool(false);
        layout.nextAllCaps = font["allCaps"].toBool(false);
    }

    if (json.contains("nextAlignment")) {
        layout.nextAlignment = static_cast<Qt::Alignment>(json["nextAlignment"].toInt(static_cast<int>(Qt::AlignCenter)));
    }

    if (json.contains("timers")) {
        QJsonArray timersArr = json["timers"].toArray();
        for (const QJsonValue& val : timersArr) {
            layout.timers.append(SlideLayoutTimer::fromJson(val.toObject()));
        }
    }

    return layout;
}
