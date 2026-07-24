#include "../inc/Library.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include "../inc/ShowSerializer.h"
#include "../inc/SlideLayout.h"

namespace Library {

QString rootDir() {
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/FOHMedia/library";
}

QString showsDir() {
    QString path = rootDir() + "/shows";
    QDir().mkpath(path);
    return path;
}

QString slidedecksDir() {
    return QDir(rootDir()).filePath("slidedecks");
}

QString layoutsDir() {
    return QDir(rootDir()).filePath("layouts");
}

QString mediaDir() {
    return QDir(rootDir()).filePath("media");
}

void ensureDirectories() {
    QDir root(rootDir());
    if (!root.exists()) {
        root.mkpath(".");
    }
    
    QDir shows(showsDir());
    if (!shows.exists()) {
        shows.mkpath(".");
    }
    
    QDir decks(slidedecksDir());
    if (!decks.exists()) {
        decks.mkpath(".");
    }

    QDir layouts(layoutsDir());
    if (!layouts.exists()) {
        layouts.mkpath(".");
        
        // Generate Default.fohl
        QString defaultLayoutFile = layouts.filePath("Default.fohl");
        if (!QFile::exists(defaultLayoutFile)) {
            SlideLayout defaultLayout;
            defaultLayout.name = "Default";
            defaultLayout.width = 1920;
            defaultLayout.height = 1080;
            defaultLayout.fontFamily = "Bebas Neue";
            defaultLayout.fontSize = 90;
            defaultLayout.fontColor = Qt::white;
            defaultLayout.backgroundColor = Qt::black;
            defaultLayout.alignment = Qt::AlignCenter;
            defaultLayout.textBounds = QRect(0, 0, 1920, 1080);
            
            ShowSerializer::saveLayoutToFile(defaultLayout, defaultLayoutFile);
        }
    }

    QDir media(mediaDir());
    if (!media.exists()) {
        media.mkpath(".");
    }
    
    QString defaultStage = layouts.filePath("StageDefault.fohl");
    if (!QFile::exists(defaultStage)) {
        SlideLayout l;
        l.name = "StageDefault";
        l.targetScreen = "Stage";
        l.fontColor = QColor("#FFFF00"); // high contrast yellow
        l.backgroundColor = QColor("#000000"); // black background
        l.textBounds = QRect(100, 100, 1720, 440); // Top half
        ShowSerializer::saveLayoutToFile(l, defaultStage);
    }
}

QStringList listShowFiles() {
    QDir dir(showsDir());
    return dir.entryList({"*.fohs", "*.yaml"}, QDir::Files, QDir::Name);
}

QStringList listSlidedeckFiles() {
    QDir dir(slidedecksDir());
    return dir.entryList({"*.fohd", "*.json"}, QDir::Files, QDir::Name);
}

QStringList listLayoutFiles() {
    QDir dir(layoutsDir());
    return dir.entryList({"*.fohl", "*.json"}, QDir::Files, QDir::Name);
}

QString getShowTitle(const QString& filename) {
    QFile file(showsDir() + "/" + filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return filename;
    
    QByteArray data = file.readAll();
    
    // Try parsing as JSON first
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isObject()) {
        QJsonObject root = doc.object();
        if (root.contains("show_title")) {
            return root["show_title"].toString();
        }
    }
    
    // Fallback to line-by-line parsing for older formats
    QTextStream in(data);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith("show_title:")) {
            int firstQuote = static_cast<int>(line.indexOf('"'));
            int lastQuote = static_cast<int>(line.lastIndexOf('"'));
            if (firstQuote != -1 && lastQuote != -1 && lastQuote > firstQuote) {
                return line.mid(firstQuote + 1, lastQuote - firstQuote - 1);
            }
            return line.mid(11).trimmed();
        }
    }
    return filename;
}

QString getDeckTitle(const QString& filename) {
    QFile file(slidedecksDir() + "/" + filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return filename;
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isObject()) {
        QJsonObject root = doc.object();
        if (root.contains("metadata") && root["metadata"].isObject()) {
            QJsonObject meta = root["metadata"].toObject();
            if (meta.contains("title")) {
                return meta["title"].toString();
            }
        }
    }
    return filename;
}

QVariantMap getLayoutProperties(const QString& layoutName) {
    QVariantMap map;
    QString filename = layoutName;
    if (!filename.endsWith(".fohl") && !filename.endsWith(".json")) {
        filename += ".fohl";
    }
    QString path = layoutsDir() + "/" + filename;
    if (QFile::exists(path)) {
        SlideLayout layout = ShowSerializer::loadLayoutFile(path);
        map["name"] = layout.name;
        map["width"] = layout.width;
        map["height"] = layout.height;
        map["fontFamily"] = layout.fontFamily;
        map["fontSize"] = layout.fontSize;
        map["fontColor"] = layout.fontColor.name(QColor::HexArgb);
        map["isBold"] = layout.isBold;
        map["isItalic"] = layout.isItalic;
        map["allCaps"] = layout.allCaps;
        map["backgroundColor"] = layout.backgroundColor.name(QColor::HexArgb);
        map["alignment"] = static_cast<int>(layout.alignment);
        map["hasNextText"] = layout.hasNextText;
        map["textBoundsX"] = layout.textBounds.x();
        map["textBoundsY"] = layout.textBounds.y();
        map["textBoundsWidth"] = layout.textBounds.width();
        map["textBoundsHeight"] = layout.textBounds.height();
        map["nextTextBoundsX"] = layout.nextTextBounds.x();
        map["nextTextBoundsY"] = layout.nextTextBounds.y();
        map["nextTextBoundsWidth"] = layout.nextTextBounds.width();
        map["nextTextBoundsHeight"] = layout.nextTextBounds.height();
        map["nextFontFamily"] = layout.nextFontFamily;
        map["nextFontSize"] = layout.nextFontSize;
        map["nextFontColor"] = layout.nextFontColor.name(QColor::HexArgb);
        map["nextIsBold"] = layout.nextIsBold;
        map["nextIsItalic"] = layout.nextIsItalic;
        map["nextAllCaps"] = layout.nextAllCaps;
        map["nextAlignment"] = static_cast<int>(layout.nextAlignment);
        map["targetScreen"] = layout.targetScreen;
        map["backgroundImage"] = layout.backgroundImage;
        
        QVariantList timersList;
        for (const auto& timer : layout.timers) {
            timersList.append(timer.toJson().toVariantMap());
        }
        map["timers"] = timersList;
        
        if (layout.targetScreen != "Stage" && !layout.backgroundImage.isEmpty()) {
            QString bgPath = mediaDir() + "/" + layout.backgroundImage;
            if (QFile::exists(bgPath)) {
                map["backgroundUrl"] = QUrl::fromLocalFile(bgPath).toString();
            }
        }
    }
    return map;
}

QList<QPair<QString, QString>> listShowsWithTitles() {
    QList<QPair<QString, QString>> result;
    QString dir = showsDir();
    for (const QString& f : listShowFiles()) {
        result.append({getShowTitle(f), dir + "/" + f});
    }
    return result;
}

QList<QPair<QString, QString>> listDecksWithTitles() {
    QList<QPair<QString, QString>> result;
    QString dir = slidedecksDir();
    for (const QString& f : listSlidedeckFiles()) {
        result.append({getDeckTitle(f), dir + "/" + f});
    }
    return result;
}

QString uniqueDestPath(const QString& dir, const QString& filename) {
    QFileInfo info(filename);
    QString baseName = info.completeBaseName();
    QString extension = info.suffix();
    if (!extension.isEmpty()) extension = "." + extension;

    QString destPath = dir + "/" + filename;
    int count = 2;
    while (QFile::exists(destPath)) {
        destPath = QString("%1/%2 (%3)%4").arg(dir, baseName).arg(count++).arg(extension);
    }
    return destPath;
}

QString importSlidedeckFile(const QString& srcPath) {
    QFileInfo srcInfo(srcPath);
    QString destPath = uniqueDestPath(slidedecksDir(), srcInfo.fileName());
    if (QFile::copy(srcPath, destPath)) {
        return destPath;
    }
    return QString();
}

QString importShowFile(const QString& srcPath) {
    QFileInfo srcInfo(srcPath);
    QString destPath = uniqueDestPath(showsDir(), srcInfo.fileName());

    // Minimal YAML parsing to find referenced decks
    QFile file(srcPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QDir srcDir = srcInfo.dir();
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        // Look for "- deck: "HoneyInTheRock.fohd"" or similar
        if (line.startsWith("- deck:")) {
            QString deckName;
            int firstQuote = static_cast<int>(line.indexOf('"'));
            int lastQuote = static_cast<int>(line.lastIndexOf('"'));
            if (firstQuote != -1 && lastQuote != -1 && lastQuote > firstQuote) {
                deckName = line.mid(firstQuote + 1, lastQuote - firstQuote - 1);
            } else {
                deckName = line.mid(7).trimmed();
            }

            if (!deckName.isEmpty()) {
                QString srcDeckPath = srcDir.absoluteFilePath(deckName);
                if (QFile::exists(srcDeckPath)) {
                    // Import deck if it doesn't exist in library or just ensure it's there
                    // The requirement says "copies any not yet present in slidedecksDir()"
                    QString destDeckPath = slidedecksDir() + "/" + deckName;
                    if (!QFile::exists(destDeckPath)) {
                        QFile::copy(srcDeckPath, destDeckPath);
                    }
                }
            }
        }
    }
    file.close();

    if (QFile::copy(srcPath, destPath)) {
        return destPath;
    }
    return QString();
}

} // namespace Library
