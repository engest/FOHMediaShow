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

static QString extractTextFromRtf(const QByteArray& rtf) {
    QString rtfStr = QString::fromUtf8(rtf);
    QString plainText;
    int i = 0;
    int braces = 0;
    while (i < rtfStr.length()) {
        QChar c = rtfStr[i];
        if (c == '{') { braces++; i++; }
        else if (c == '}') { braces--; i++; }
        else if (c == '\\') {
            i++;
            QString cmd;
            while (i < rtfStr.length() && (rtfStr[i].isLetter() || rtfStr[i].isDigit() || rtfStr[i] == '-')) {
                cmd += rtfStr[i];
                i++;
            }
            if (i < rtfStr.length() && rtfStr[i] == ' ') i++; // space after command
            
            if (cmd == "par" || cmd == "line") plainText += "\n";
            else if (cmd == "'" && i + 1 < rtfStr.length()) {
                QString hex = rtfStr.mid(i, 2);
                plainText += QChar(hex.toInt(nullptr, 16));
                i += 2;
            }
        } else {
            // RTF wraps document in {\rtf1 ...} so content is at braces == 1
            if (braces <= 1 && c != '\r' && c != '\n') {
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
