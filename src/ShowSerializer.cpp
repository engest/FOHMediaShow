#include "../inc/ShowSerializer.h"
#include "../inc/Show.h"
#include "../inc/SlideDeck.h"
#include "../inc/Slide.h"
#include "../inc/SlideLayout.h"
#include "../inc/Library.h"

#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QMessageBox>
#include <QApplication>

Show* ShowSerializer::loadFohsFile(const QString& filePath, QObject* parentObj) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Fallback if file is inaccessible
        auto* show = new Show(QStringLiteral("New Show"), parentObj);
        show->appendDeck(QStringLiteral("Untitled Deck"))->addComponent("Slide 1", {QStringList{""}});
        return show;
    }

    QByteArray fileData = file.readAll();
    file.close();

    QString showName = QFileInfo(filePath).completeBaseName();
    auto* show = new Show(showName, parentObj);
    show->setSourceFile(filePath);

    // Try parsing as JSON first
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(fileData, &err);
    if (!doc.isNull() && doc.isObject()) {
        QJsonObject root = doc.object();
        if (root.contains("show_title")) show->setName(root["show_title"].toString());
        if (root.contains("default_transition_type")) show->setDefaultTransitionType(root["default_transition_type"].toString());
        if (root.contains("default_transition_duration")) show->setDefaultTransitionDurationMs(root["default_transition_duration"].toInt());
        
        if (root.contains("decks") && root["decks"].isArray()) {
            QDir baseDir(Library::slidedecksDir());
            QJsonArray decksArr = root["decks"].toArray();
            for (const QJsonValue& v : decksArr) {
                if (v.isObject()) {
                    QJsonObject deckObj = v.toObject();
                    QString deckFile = deckObj["deck"].toString();
                    QString arr = deckObj["arrangement"].toString();
                    if (!deckFile.isEmpty()) {
                        loadFohdFile(baseDir.absoluteFilePath(deckFile), show, arr);
                    }
                }
            }
        }
        return show;
    }

    // Fallback to legacy parsing
    QString content = QString::fromUtf8(fileData);
    QStringList sections = content.split(QStringLiteral("---\n"), Qt::SkipEmptyParts);
    QStringList showMetaDataLines = sections.size() >= 1 ? sections[0].split(QLatin1Char('\n'), Qt::SkipEmptyParts) : QStringList();
    for (const QString& line : showMetaDataLines) {
        QString trimmed = line.trimmed();
        if (trimmed.startsWith(QStringLiteral("show_title:"))) {
            int firstQuote = static_cast<int>(trimmed.indexOf(QLatin1Char('"')));
            int lastQuote = static_cast<int>(trimmed.lastIndexOf(QLatin1Char('"')));
            if (firstQuote != -1 && lastQuote != -1 && lastQuote > firstQuote) {
                show->setName(trimmed.mid(firstQuote + 1, lastQuote - firstQuote - 1));
            }
        } else if (trimmed.startsWith(QStringLiteral("default_transition_type:"))) {
            int firstQuote = static_cast<int>(trimmed.indexOf(QLatin1Char('"')));
            int lastQuote = static_cast<int>(trimmed.lastIndexOf(QLatin1Char('"')));
            if (firstQuote != -1 && lastQuote != -1 && lastQuote > firstQuote) {
                show->setDefaultTransitionType(trimmed.mid(firstQuote + 1, lastQuote - firstQuote - 1));
            }
        } else if (trimmed.startsWith(QStringLiteral("default_transition_duration:"))) {
            QString valStr = trimmed.mid(QStringLiteral("default_transition_duration:").length()).trimmed();
            bool ok;
            int val = valStr.toInt(&ok);
            if (ok) {
                show->setDefaultTransitionDurationMs(val);
            }
        }
    }
    if (sections.size() >= 2) {
        QStringList deckLines = sections[1].split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        QDir baseDir(Library::slidedecksDir());

        QString currentDeckFile;
        QString currentArrangement;

        auto processPendingDeck = [&]() {
            if (!currentDeckFile.isEmpty()) {
                QString fullPath = baseDir.absoluteFilePath(currentDeckFile);
                loadFohdFile(fullPath, show, currentArrangement);
                currentDeckFile.clear();
                currentArrangement.clear();
            }
        };

        for (const QString& line : deckLines) {
            QString trimmed = line.trimmed();
            if (trimmed.startsWith(QStringLiteral("- deck:"))) {
                processPendingDeck();

                int firstQuote = static_cast<int>(trimmed.indexOf(QLatin1Char('"')));
                int lastQuote = static_cast<int>(trimmed.lastIndexOf(QLatin1Char('"')));
                if (firstQuote != -1 && lastQuote != -1 && lastQuote > firstQuote) {
                    currentDeckFile = trimmed.mid(firstQuote + 1, lastQuote - firstQuote - 1);
                }
            } else if (trimmed.startsWith(QStringLiteral("arrangement:"))) {
                int firstQuote = static_cast<int>(trimmed.indexOf(QLatin1Char('"')));
                int lastQuote = static_cast<int>(trimmed.lastIndexOf(QLatin1Char('"')));
                if (firstQuote != -1 && lastQuote != -1 && lastQuote > firstQuote) {
                    currentArrangement = trimmed.mid(firstQuote + 1, lastQuote - firstQuote - 1);
                }
            }
        }
        processPendingDeck();
    }

    return show;
}

void ShowSerializer::loadFohdFile(const QString& filePath, Show* parentShow, const QString& preferredArrangement) {
    SlideDeck* deck = loadFohdFile(filePath, static_cast<QObject*>(parentShow));
    if (deck) {
        parentShow->addDeck(deck);
        if (!preferredArrangement.isEmpty()) {
            deck->setActiveArrangement(preferredArrangement);
        }
    }
}

SlideDeck* ShowSerializer::loadFohdFile(const QString& filePath, QObject* parentObj) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return nullptr;

    QByteArray fileData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(fileData, &parseError);

    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        auto* errorDeck = new SlideDeck(QStringLiteral("Load Error"), parentObj);
        errorDeck->addComponent("Error", {QStringList{"Failed to parse JSON:\n" + parseError.errorString()}});
        return errorDeck;
    }

    QJsonObject rootObj = jsonDoc.object();

    QString deckName = QFileInfo(filePath).completeBaseName();
    QMap<QString, QString> parsedMeta;
    if (rootObj.contains(QStringLiteral("metadata")) && rootObj[QStringLiteral("metadata")].isObject()) {
        QJsonObject metaObj = rootObj[QStringLiteral("metadata")].toObject();
        for (const QString& k : metaObj.keys()) {
            const QJsonValue v = metaObj.value(k);
            if (v.isString()) {
                parsedMeta.insert(k, v.toString());
            } else if (v.isDouble()) {
                parsedMeta.insert(k, QString::number(v.toDouble()));
            } else if (v.isArray()) {
                QStringList arrStrs;
                for (const QJsonValue& arrVal : v.toArray()) {
                    if (arrVal.isString()) arrStrs.append(arrVal.toString());
                }
                parsedMeta.insert(k, arrStrs.join(", "));
            } else {
                parsedMeta.insert(k, v.toVariant().toString());
            }
        }
        if (parsedMeta.contains(QStringLiteral("title"))) {
            deckName = parsedMeta.value(QStringLiteral("title"));
        }
    }

    auto* deck = new SlideDeck(deckName, parentObj);
    deck->setMetadata(parsedMeta);

    // Store relative file name for saving later
    deck->setSourceFile(QFileInfo(filePath).fileName());

    // ─── 2. Extract Components ───
    if (!rootObj.contains(QStringLiteral("components")) || !rootObj[QStringLiteral("components")].isObject()) {
        deck->addComponent("Error", {QStringList{"Error: No 'components' object found in file."}});
        return deck;
    }

    QJsonObject componentsObj = rootObj[QStringLiteral("components")].toObject();
    QMap<QString, QList<SlideData>> parsedComponents;

    // Capture the original key order
    QStringList orderedCompKeys;
    if (rootObj.contains("component_order") && rootObj["component_order"].isArray()) {
        QJsonArray orderArr = rootObj["component_order"].toArray();
        for (const QJsonValue& val : orderArr) {
            orderedCompKeys.append(val.toString());
        }
    } else {
        // Fallback to regex hack for older files
        QString raw = QString::fromUtf8(fileData);
        int compStart = static_cast<int>(raw.indexOf(QStringLiteral("\"components\"")));
        if (compStart != -1) {
            int braceOpen = static_cast<int>(raw.indexOf('{', compStart + 12));
            int braceClose = braceOpen;
            int depth = 0;
            for (int ci = braceOpen; ci < raw.size(); ++ci) {
                if (raw[ci] == '{') ++depth;
                else if (raw[ci] == '}') { --depth; if (depth == 0) { braceClose = ci; break; } }
            }
            QString compBlock = raw.mid(braceOpen + 1, braceClose - braceOpen - 1);
            QRegularExpression keyRe(QStringLiteral("^\\s*\"([^\"]+)\"\\s*:"), QRegularExpression::MultilineOption);
            auto it = keyRe.globalMatch(compBlock);
            while (it.hasNext()) {
                auto m = it.next();
                QString key = m.captured(1);
                if (componentsObj.contains(key) && !orderedCompKeys.contains(key)) {
                    orderedCompKeys.append(key);
                }
            }
        }
    }
    
    // Fallback: append any keys that might have been missed
    for (const QString& key : componentsObj.keys()) {
        if (!orderedCompKeys.contains(key)) {
            orderedCompKeys.append(key);
        }
    }
    
    deck->setComponentOrder(orderedCompKeys);

    for (const QString& compKey : componentsObj.keys()) {
        QJsonArray slideArray = componentsObj[compKey].toArray();
        QList<SlideData> componentSlides;

        for (const QJsonValue& slideVal : slideArray) {
            SlideData data;
            
            if (slideVal.isArray()) {
                // Legacy format: just an array of lines
                for (const QJsonValue& lineVal : slideVal.toArray()) {
                    if (lineVal.isString()) {
                        data.lines.append(lineVal.toString());
                    }
                }
            } else if (slideVal.isObject()) {
                // New format: object with layout files
                QJsonObject slideObj = slideVal.toObject();
                if (slideObj.contains("lines") && slideObj["lines"].isArray()) {
                    for (const QJsonValue& lineVal : slideObj["lines"].toArray()) {
                        if (lineVal.isString()) {
                            data.lines.append(lineVal.toString());
                        }
                    }
                }
                if (slideObj.contains("layouts") && slideObj["layouts"].isObject()) {
                    QJsonObject layoutsObj = slideObj["layouts"].toObject();
                    for (auto it = layoutsObj.begin(); it != layoutsObj.end(); ++it) {
                        QString layoutVal = it.value().toString();
                        if (layoutVal.endsWith(".fohl")) layoutVal.chop(5);
                        else if (layoutVal.endsWith(".json")) layoutVal.chop(5);
                        data.layouts.insert(it.key(), layoutVal);
                    }
                } else {
                    // Backwards compatibility
                    QString aud = slideObj["main_layout_file"].toString("Default");
                    if (aud.endsWith(".fohl")) aud.chop(5);
                    data.layouts["Audience"] = aud;
                    
                    QString stage = slideObj["stage_layout_file"].toString("StageDefault");
                    if (stage.endsWith(".fohl")) stage.chop(5);
                    data.layouts["Stage"] = stage;
                }
            }
            componentSlides.append(data);
        }
        parsedComponents.insert(compKey, componentSlides);
    }

    // Hand the raw data to the deck
    deck->setComponents(parsedComponents);

    // ─── 3. Extract Arrangements ───
    if (rootObj.contains(QStringLiteral("arrangements")) && rootObj[QStringLiteral("arrangements")].isObject()) {
        QJsonObject arrangementsObj = rootObj[QStringLiteral("arrangements")].toObject();

        for (const QString& arrName : arrangementsObj.keys()) {
            Arrangement* newArrangement = deck->appendArrangement(arrName);
            QStringList sequence;

            QJsonArray seqArray = arrangementsObj[arrName].toArray();
            for (const QJsonValue& seqVal : seqArray) {
                if (seqVal.isString()) {
                    sequence.append(seqVal.toString());
                }
            }
            newArrangement->setSequence(sequence);
        }
    }

    // ─── 4. Set Active Arrangement & Media ───
    QString defaultArrName = rootObj[QStringLiteral("default_arrangement")].toString(QStringLiteral("Default"));
    if (rootObj.contains(QStringLiteral("global_background_media"))) {
        deck->setGlobalBackgroundMedia(rootObj[QStringLiteral("global_background_media")].toString());
    }

    // Set both default and active to the loaded default
    deck->setDefaultArrangement(defaultArrName);
    deck->setActiveArrangement(defaultArrName);

    return deck;
}

void ShowSerializer::saveShowToFile(Show* show) {
    if (!show) return;

    QString filePath = show->sourceFile();
    if (filePath.isEmpty()) {
        filePath = Library::uniqueDestPath(Library::showsDir(), show->name() + ".fohs");
        show->setSourceFile(filePath);
        show->setName(QFileInfo(filePath).completeBaseName());
    }

    QJsonObject rootObj;
    rootObj["format_version"] = 1;
    rootObj["show_title"] = show->name();
    rootObj["default_transition_type"] = show->defaultTransitionType();
    rootObj["default_transition_duration"] = show->defaultTransitionDurationMs();

    QJsonArray decksArr;
    for (int i = 0; i < show->deckCount(); ++i) {
        SlideDeck* deck = show->deckAt(i);
        QString deckFile = deck->sourceFile();
        if (deckFile.isEmpty()) {
            deckFile = deck->name() + ".fohd";
        }
        saveDeckToFile(deck);

        QJsonObject deckObj;
        deckObj["deck"] = deckFile;
        deckObj["arrangement"] = deck->activeArrangementName();
        decksArr.append(deckObj);
    }
    rootObj["decks"] = decksArr;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QWidget* activeWindow = QApplication::activeWindow();
        QMessageBox::warning(activeWindow, QObject::tr("Error"), QObject::tr("Could not save show file."));
        return;
    }

    QJsonDocument doc(rootObj);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}

void ShowSerializer::saveDeckToFile(SlideDeck* deck) {
    if (!deck) return;

    QString basePath = Library::slidedecksDir();
    QString deckFileName = deck->sourceFile();
    if (deckFileName.isEmpty()) {
        QString uniqueFullPath = Library::uniqueDestPath(basePath, deck->name() + ".fohd");
        deckFileName = QFileInfo(uniqueFullPath).fileName();
        deck->setSourceFile(deckFileName);
    }
    QString fullPath = QDir(basePath).absoluteFilePath(deckFileName);

    // ── Read existing file to preserve metadata fields and style block ──
    QJsonObject existingMeta;
    QJsonObject existingStyle;
    QFile readFile(fullPath);
    if (readFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonDocument existingDoc = QJsonDocument::fromJson(readFile.readAll());
        readFile.close();
        if (existingDoc.isObject()) {
            QJsonObject existingRoot = existingDoc.object();
            if (existingRoot.contains("metadata"))
                existingMeta  = existingRoot["metadata"].toObject();
            if (existingRoot.contains("style"))
                existingStyle = existingRoot["style"].toObject();
        }
    }
    
    const QMap<QString, QString> liveMeta = deck->metadata();
    for (auto it = liveMeta.constBegin(); it != liveMeta.constEnd(); ++it) {
        existingMeta[it.key()] = it.value();
    }
    existingMeta[QStringLiteral("title")] = deck->name();

    // ── Build output using QJsonObject ──
    QJsonObject rootObj;
    rootObj["format_version"] = 1;
    rootObj["metadata"] = existingMeta;
    if (!existingStyle.isEmpty()) {
        rootObj["style"] = existingStyle;
    }

    const auto& comps = deck->components();
    QStringList rawCompOrder = deck->componentOrder();
    QStringList compOrder;
    for (const QString& k : rawCompOrder) {
        if (comps.contains(k) && !compOrder.contains(k)) compOrder.append(k);
    }
    for (const QString& k : comps.keys()) {
        if (!compOrder.contains(k)) compOrder.append(k);
    }

    QJsonArray compOrderArr;
    for (const QString& k : compOrder) {
        compOrderArr.append(k);
    }
    rootObj["component_order"] = compOrderArr;

    QJsonObject compsObj;
    for (const QString& compKey : compOrder) {
        QJsonArray slideArr;
        const auto& slideList = comps.value(compKey);
        for (const auto& slide : slideList) {
            QJsonObject slideObj;
            QJsonArray linesArr;
            for (const QString& line : slide.lines) linesArr.append(line);
            slideObj["lines"] = linesArr;
            QJsonObject layoutsObj;
            for (auto it = slide.layouts.constBegin(); it != slide.layouts.constEnd(); ++it) {
                QString layoutVal = it.value();
                if (layoutVal.endsWith(".fohl")) layoutVal.chop(5);
                else if (layoutVal.endsWith(".json")) layoutVal.chop(5);
                layoutsObj[it.key()] = layoutVal;
            }
            slideObj["layouts"] = layoutsObj;
            slideArr.append(slideObj);
        }
        compsObj[compKey] = slideArr;
    }
    rootObj["components"] = compsObj;

    QJsonObject arrangementsObj;
    for (int i = 0; i < deck->arrangementCount(); ++i) {
        Arrangement* arr = deck->arrangementAt(i);
        QJsonArray seqArr;
        for (const QString& s : arr->sequence()) seqArr.append(s);
        arrangementsObj[arr->name()] = seqArr;
    }
    rootObj["arrangements"] = arrangementsObj;
    rootObj["default_arrangement"] = deck->defaultArrangementName();
    rootObj["global_background_media"] = deck->globalBackgroundMedia();

    // ── Write to disk ──
    QFile file(fullPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QJsonDocument doc(rootObj);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
}

SlideLayout ShowSerializer::loadLayoutFile(const QString& filePath) {
    if (filePath.isEmpty() || !QFile::exists(filePath)) {
        return SlideLayout();
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return SlideLayout();
    }
    
    QByteArray fileData = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(fileData);
    if (!doc.isObject()) {
        return SlideLayout();
    }
    
    return SlideLayout::fromJson(doc.object());
}

void ShowSerializer::saveLayoutToFile(const SlideLayout& layout, const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }
    
    QJsonDocument doc(layout.toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}
