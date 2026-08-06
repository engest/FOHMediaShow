#include "../inc/ProPresenterImporter.h"
#include <QFile>
#include <QDebug>
#include <QTextDocument>
#include "presentation.pb.h"
#include "slide.pb.h"
#include "groups.pb.h"
#include "cue.pb.h"
#include "action.pb.h"
#include "graphicsData.pb.h"
#include "basicTypes.pb.h"

// Helper to convert rv::data::UUID to QString
static QString uuidToString(const rv::data::UUID& uuid) {
    return QString::fromStdString(uuid.string());
}

QString ProPresenterImporter::extractTextFromRtf(const QByteArray& rtf) {
    // Decode as Latin1 so raw bytes match their 0-255 values
    QString rtfStr = QString::fromLatin1(rtf);
    QString plainText;
    int i = 0;
    int braces = 0;
    int uc = 1; // Default fallback character count per RTF spec
    int skipDepth = 0;

    while (i < rtfStr.length()) {
        QChar c = rtfStr[i];
        if (c == '{') {
            braces++;
            i++;
            if (i < rtfStr.length() - 1 && rtfStr.mid(i, 2) == "\\*") {
                if (skipDepth == 0) {
                    skipDepth = braces;
                }
            }
        } else if (c == '}') {
            if (skipDepth == braces) {
                skipDepth = 0;
            }
            braces--;
            i++;
        } else if (c == '\\') {
            i++;
            if (i >= rtfStr.length()) break;

            QChar nextChar = rtfStr[i];
            if (nextChar == '\\' || nextChar == '{' || nextChar == '}') {
                if (braces <= 1 && skipDepth == 0) {
                    plainText += nextChar;
                }
                i++;
                continue;
            }
            if (nextChar == '~') { // Non-breaking space
                if (braces <= 1 && skipDepth == 0) {
                    plainText += ' ';
                }
                i++;
                continue;
            }
            if (nextChar == '_') { // Optional hyphen
                i++;
                continue;
            }
            if (nextChar == '\'') { // Hex-encoded byte \'xx
                i++;
                if (i + 2 <= rtfStr.length()) {
                    bool ok = false;
                    int byteVal = rtfStr.mid(i, 2).toInt(&ok, 16);
                    if (ok && braces <= 1 && skipDepth == 0) {
                        if (byteVal == 0x91 || byteVal == 0x92) {
                            plainText += '\''; // Smart single quotes / apostrophes
                        } else if (byteVal == 0x93 || byteVal == 0x94) {
                            plainText += '"'; // Smart double quotes
                        } else if (byteVal >= 32 && byteVal <= 126) {
                            plainText += QChar(byteVal);
                        } else {
                            // Convert CP1252 byte to Unicode
                            QByteArray b(1, static_cast<char>(byteVal));
                            QString s = QString::fromLocal8Bit(b);
                            plainText += s.isEmpty() ? QChar(byteVal) : s;
                        }
                    }
                    i += 2;
                }
                continue;
            }

            // Command name (alphabetic word)
            int cmdStart = i;
            while (i < rtfStr.length() && rtfStr[i].isLetter()) {
                i++;
            }
            QString cmdName = rtfStr.mid(cmdStart, i - cmdStart);

            // Optional signed integer parameter
            int paramStart = i;
            if (i < rtfStr.length() && (rtfStr[i] == '-' || rtfStr[i].isDigit())) {
                i++;
                while (i < rtfStr.length() && rtfStr[i].isDigit()) {
                    i++;
                }
            }
            QString paramStr = rtfStr.mid(paramStart, i - paramStart);
            bool hasParam = !paramStr.isEmpty() && paramStr != "-";
            int paramVal = hasParam ? paramStr.toInt() : 0;

            // Space after command is a delimiter and should be consumed
            if (i < rtfStr.length() && rtfStr[i] == ' ') {
                i++;
            }

            if (cmdName == "fonttbl" || cmdName == "colortbl" || cmdName == "stylesheet" || cmdName == "info") {
                if (skipDepth == 0) {
                    skipDepth = braces;
                }
            } else if (cmdName == "uc") {
                if (hasParam) {
                    uc = qMax(0, paramVal);
                }
            } else if (cmdName == "u") {
                if (hasParam) {
                    int codePoint = paramVal;
                    if (codePoint < 0) {
                        codePoint += 65536;
                    }

                    if (braces <= 1 && skipDepth == 0) {
                        if (codePoint == 8216 || codePoint == 8217 || codePoint == 8242) {
                            plainText += '\''; // Map smart apostrophes / quotes to standard single quote
                        } else if (codePoint == 8220 || codePoint == 8221) {
                            plainText += '"'; // Map smart double quotes
                        } else {
                            plainText += QChar(codePoint);
                        }
                    }

                    // Skip the uc fallback character(s)
                    for (int k = 0; k < uc && i < rtfStr.length(); ++k) {
                        if (i + 1 < rtfStr.length() && rtfStr.mid(i, 2) == "\\'") {
                            i += 4;
                        } else if (rtfStr[i] == '\\') {
                            while (i < rtfStr.length() && rtfStr[i] != ' ' && rtfStr[i] != '\\' && rtfStr[i] != '{' && rtfStr[i] != '}') {
                                i++;
                            }
                            if (i < rtfStr.length() && rtfStr[i] == ' ') i++;
                        } else {
                            i++;
                        }
                    }
                }
            } else if (cmdName == "par" || cmdName == "line") {
                if (braces <= 1 && skipDepth == 0) {
                    plainText += '\n';
                }
            } else if (cmdName == "tab") {
                if (braces <= 1 && skipDepth == 0) {
                    plainText += ' ';
                }
            }
        } else {
            if (braces <= 1 && skipDepth == 0 && c != '\r' && c != '\n') {
                plainText += c;
            }
            i++;
        }
    }
    return plainText.trimmed();
}

SlideDeck* ProPresenterImporter::importProFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open" << filePath;
        return nullptr;
    }

    QByteArray data = file.readAll();
    rv::data::Presentation presentation;
    if (!presentation.ParseFromArray(data.constData(), static_cast<int>(data.size()))) {
        qWarning() << "Failed to parse protobuf presentation from" << filePath;
        return nullptr;
    }

    SlideDeck* deck = new SlideDeck();
    deck->setName(QString::fromStdString(presentation.name()));

    // 1. Map all cues by UUID for quick lookup
    QMap<QString, const rv::data::Cue*> cueMap;
    for (int i = 0; i < presentation.cues_size(); ++i) {
        const auto& cue = presentation.cues(i);
        cueMap.insert(uuidToString(cue.uuid()), &cue);
    }

    // 2. Iterate through CueGroups (Components)
    QStringList masterComponentOrder;
    for (int i = 0; i < presentation.cue_groups_size(); ++i) {
        const auto& cueGroup = presentation.cue_groups(i);
        QString groupName = QString::fromStdString(cueGroup.group().name());
        if (groupName.isEmpty()) groupName = "Default";

        masterComponentOrder.append(groupName);

        QList<QStringList> componentSlides;

        for (int j = 0; j < cueGroup.cue_identifiers_size(); ++j) {
            QString cueId = uuidToString(cueGroup.cue_identifiers(j));
            if (!cueMap.contains(cueId)) continue;
            const rv::data::Cue* cue = cueMap.value(cueId);

            QStringList slideLines;

            // Search for Slide action in the cue
            for (int k = 0; k < cue->actions_size(); ++k) {
                const auto& action = cue->actions(k);
                if (action.has_slide()) {
                    const auto& slide = action.slide();
                    if (slide.has_presentation()) {
                        const auto& presSlide = slide.presentation();
                        for (int e = 0; e < presSlide.base_slide().elements_size(); ++e) {
                            const auto& element = presSlide.base_slide().elements(e);
                            if (element.element().has_text()) {
                                const auto& textElement = element.element().text();
                                QByteArray rtfData(textElement.rtf_data().data(), textElement.rtf_data().size());
                                QString plainText = extractTextFromRtf(rtfData);
                                if (!plainText.isEmpty()) {
                                    slideLines.append(plainText.split('\n', Qt::SkipEmptyParts));
                                }
                            }
                        }
                    }
                }
            }
            if (slideLines.isEmpty()) {
                slideLines.append(" "); // Add an empty line so the slide still exists
            }
            componentSlides.append(slideLines);
        }

        deck->addComponent(groupName, componentSlides);
    }

    // Create the implicit Master arrangement (called "Default" in FOHMediaShow)
    Arrangement* defaultArr = deck->appendArrangement("Default");
    for (const QString& comp : masterComponentOrder) {
        defaultArr->appendComponent(comp);
    }
    deck->setDefaultArrangement("Default");
    deck->setActiveArrangement("Default");

    // 3. Import explicit arrangements
    for (int i = 0; i < presentation.arrangements_size(); ++i) {
        const auto& arrData = presentation.arrangements(i);
        QString arrName = QString::fromStdString(arrData.name());
        if (arrName == "Master" || arrName.isEmpty()) arrName = "Default";

        Arrangement* arr = deck->arrangement(arrName);
        if (!arr) {
            arr = deck->appendArrangement(arrName);
        } else {
            arr->clear(); // We overwrite the implicit Master with their explicitly defined one
        }
        
        for (int j = 0; j < arrData.group_identifiers_size(); ++j) {
            QString groupId = uuidToString(arrData.group_identifiers(j));
            for (int k = 0; k < presentation.cue_groups_size(); ++k) {
                if (uuidToString(presentation.cue_groups(k).group().uuid()) == groupId) {
                    QString gName = QString::fromStdString(presentation.cue_groups(k).group().name());
                    if (gName.isEmpty()) gName = "Default";
                    arr->appendComponent(gName);
                    break;
                }
            }
        }
    }

    return deck;
}
