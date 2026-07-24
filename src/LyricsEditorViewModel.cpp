#include "../inc/LyricsEditorViewModel.h"
#include "../inc/Library.h"
#include "../inc/ShowSerializer.h"
#include "../inc/ArrangementViewModel.h"
#include "../inc/ProPresenterImporter.h"
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUrl>
#include <QJsonArray>
#include <QJsonDocument>

LyricsEditorViewModel::LyricsEditorViewModel(QObject* parent)
    : QObject(parent), m_arrangementModel(new ArrangementViewModel(nullptr, this)),
      m_previewDeckModel(new SlideDeckViewModel(nullptr, this)) {
    newDeck();
    refreshLibrary();
}

QVariantList LyricsEditorViewModel::libraryFiles() const {
    return m_libraryFiles;
}

QString LyricsEditorViewModel::currentTitle() const {
    return m_currentDeck ? m_currentDeck->name() : "";
}

void LyricsEditorViewModel::setCurrentTitle(const QString& title) {
    if (m_currentDeck && m_currentDeck->name() != title) {
        m_currentDeck->setName(title);
        emit currentTitleChanged();
    }
}

QString LyricsEditorViewModel::currentArtists() const {
    if (!m_currentDeck) return "";
    return m_currentDeck->metadataValue("artists");
}

void LyricsEditorViewModel::setCurrentArtists(const QString& artists) {
    if (m_currentDeck && m_currentDeck->metadataValue("artists") != artists) {
        m_currentDeck->setMetadataValue("artists", artists);
        emit currentArtistsChanged();
    }
}

QString LyricsEditorViewModel::currentCcli() const {
    if (!m_currentDeck) return "";
    return m_currentDeck->metadataValue("ccli");
}

void LyricsEditorViewModel::setCurrentCcli(const QString& ccli) {
    if (m_currentDeck && m_currentDeck->metadataValue("ccli") != ccli) {
        m_currentDeck->setMetadataValue("ccli", ccli);
        emit currentCcliChanged();
    }
}

QString LyricsEditorViewModel::currentCopyright() const {
    if (!m_currentDeck) return "";
    return m_currentDeck->metadataValue("copyright");
}

void LyricsEditorViewModel::setCurrentCopyright(const QString& copyright) {
    if (m_currentDeck && m_currentDeck->metadataValue("copyright") != copyright) {
        m_currentDeck->setMetadataValue("copyright", copyright);
        emit currentCopyrightChanged();
    }
}

QString LyricsEditorViewModel::currentKey() const {
    if (!m_currentDeck) return "";
    return m_currentDeck->metadataValue("key");
}

void LyricsEditorViewModel::setCurrentKey(const QString& key) {
    if (m_currentDeck && m_currentDeck->metadataValue("key") != key) {
        m_currentDeck->setMetadataValue("key", key);
        emit currentKeyChanged();
    }
}

QString LyricsEditorViewModel::currentTempo() const {
    if (!m_currentDeck) return "";
    return m_currentDeck->metadataValue("tempo");
}

void LyricsEditorViewModel::setCurrentTempo(const QString& tempo) {
    if (m_currentDeck && m_currentDeck->metadataValue("tempo") != tempo) {
        m_currentDeck->setMetadataValue("tempo", tempo);
        emit currentTempoChanged();
    }
}

QString LyricsEditorViewModel::currentTheme() const {
    if (!m_currentDeck) return "";
    return m_currentDeck->metadataValue("theme");
}

void LyricsEditorViewModel::setCurrentTheme(const QString& theme) {
    if (m_currentDeck && m_currentDeck->metadataValue("theme") != theme) {
        m_currentDeck->setMetadataValue("theme", theme);
        emit currentThemeChanged();
    }
}

QString LyricsEditorViewModel::rawLyricsText() const {
    return m_rawLyricsText;
}

QVariantList LyricsEditorViewModel::screenLayoutMappings() const {
    QVariantList list;
    for (auto it = m_screenLayouts.constBegin(); it != m_screenLayouts.constEnd(); ++it) {
        QVariantMap map;
        map["screenName"] = it.key();
        map["layoutFile"] = it.value();
        list.append(map);
    }
    return list;
}

void LyricsEditorViewModel::setScreenLayoutMapping(const QString& screenName, const QString& layoutFile) {
    if (m_screenLayouts.value(screenName) != layoutFile) {
        m_screenLayouts[screenName] = layoutFile;
        if (m_currentDeck) {
            auto comps = m_currentDeck->components();
            for (auto it = comps.begin(); it != comps.end(); ++it) {
                for (int i = 0; i < it.value().size(); ++i) {
                    it.value()[i].layouts[screenName] = layoutFile;
                }
            }
            m_currentDeck->setComponents(comps);
        }
        emit screenLayoutMappingsChanged();
    }
}

void LyricsEditorViewModel::removeScreenLayoutMapping(const QString& screenName) {
    if (m_screenLayouts.contains(screenName)) {
        m_screenLayouts.remove(screenName);
        if (m_currentDeck) {
            auto comps = m_currentDeck->components();
            for (auto it = comps.begin(); it != comps.end(); ++it) {
                for (int i = 0; i < it.value().size(); ++i) {
                    it.value()[i].layouts.remove(screenName);
                }
            }
            m_currentDeck->setComponents(comps);
        }
        emit screenLayoutMappingsChanged();
    }
}

void LyricsEditorViewModel::addScreenLayoutMapping(const QString& screenName, const QString& layoutFile) {
    if (!m_screenLayouts.contains(screenName)) {
        setScreenLayoutMapping(screenName, layoutFile);
    }
}

QStringList LyricsEditorViewModel::availableLayouts() const {
    QStringList files = Library::listLayoutFiles();
    QStringList names;
    for (const QString& file : files) {
        QFileInfo info(file);
        names.append(info.completeBaseName());
    }
    return names;
}

void LyricsEditorViewModel::setRawLyricsText(const QString& text) {
    if (m_rawLyricsText != text) {
        m_rawLyricsText = text;
        emit rawLyricsTextChanged();
    }
}

SlideDeck* LyricsEditorViewModel::currentDeck() const {
    return m_currentDeck;
}

void LyricsEditorViewModel::setDefaultArrangementName(const QString& name) {
    if (m_currentDeck) {
        m_currentDeck->setDefaultArrangement(name);
    }
}

void LyricsEditorViewModel::setActiveArrangementName(const QString& name) {
    if (m_currentDeck) {
        m_currentDeck->setActiveArrangement(name);
    }
}

QObject* LyricsEditorViewModel::arrangementModel() const {
    return m_arrangementModel;
}

QObject* LyricsEditorViewModel::previewDeckModel() const {
    return m_previewDeckModel;
}

QStringList LyricsEditorViewModel::availableComponents() const {
    if (!m_currentDeck) return {};
    return m_currentDeck->componentOrder();
}

void LyricsEditorViewModel::refreshLibrary() {
    m_libraryFiles.clear();
    auto list = Library::listDecksWithTitles();
    for (const auto& pair : list) {
        QVariantMap map;
        map["title"] = pair.first;
        map["filename"] = pair.second;
        m_libraryFiles.append(map);
    }
    emit libraryFilesChanged();
}

void LyricsEditorViewModel::newDeck() {
    if (m_currentDeck) {
        m_currentDeck->deleteLater();
    }
    m_currentDeck = new SlideDeck("Untitled Lyrics", this);
    m_currentDeck->appendArrangement("Default");
    m_currentDeck->setDefaultArrangement("Default");
    m_currentDeck->setActiveArrangement("Default");
    
    static_cast<ArrangementViewModel*>(m_arrangementModel)->setDeck(m_currentDeck);
    m_screenLayouts.clear();
    m_screenLayouts["Audience"] = "Default";
    m_screenLayouts["Stage"] = "StageDefault";
    
    setRawLyricsText("");
    emit currentTitleChanged();
    emit currentArtistsChanged();
    emit currentCcliChanged();
    emit currentCopyrightChanged();
    emit currentTempoChanged();
    emit currentThemeChanged();
    emit currentDeckChanged();
    emit availableComponentsChanged();
    emit screenLayoutMappingsChanged();
}

void LyricsEditorViewModel::loadFromLibrary(const QString& filename) {
    QString fullPath = QDir(Library::slidedecksDir()).filePath(filename);
    
    if (m_currentDeck) {
        m_currentDeck->deleteLater();
    }
    
    m_currentDeck = ShowSerializer::loadFohdFile(fullPath, this);
    if (!m_currentDeck) {
        newDeck(); // Fallback on failure
        return;
    }
    
    static_cast<ArrangementViewModel*>(m_arrangementModel)->setDeck(m_currentDeck);
    m_previewDeckModel->setDeck(m_currentDeck);
    
    // Extract layouts from the loaded deck
    m_screenLayouts.clear();
    auto comps = m_currentDeck->components();
    if (!comps.isEmpty() && !comps.first().isEmpty()) {
        m_screenLayouts = comps.first().first().layouts;
    }
    if (!m_screenLayouts.contains("Audience")) m_screenLayouts["Audience"] = "Default";
    if (!m_screenLayouts.contains("Stage")) m_screenLayouts["Stage"] = "StageDefault";
    
    updateRawTextFromDeck();
    
    emit currentTitleChanged();
    emit currentArtistsChanged();
    emit currentCcliChanged();
    emit currentCopyrightChanged();
    emit currentKeyChanged();
    emit currentTempoChanged();
    emit currentThemeChanged();
    emit currentDeckChanged();
    emit availableComponentsChanged();
    emit screenLayoutMappingsChanged();
}

void LyricsEditorViewModel::reloadCurrentDeck() {
    if (m_currentDeck) {
        QString filename = m_currentDeck->sourceFile();
        if (!filename.isEmpty()) {
            loadFromLibrary(filename);
        }
    }
}

void LyricsEditorViewModel::importFile(const QString& filePath) {
    QString imported = Library::importSlidedeckFile(filePath);
    if (!imported.isEmpty()) {
        refreshLibrary();
        loadFromLibrary(QFileInfo(imported).fileName());
    }
}

void LyricsEditorViewModel::importProFiles(const QVariantList& fileUrls) {
    bool importedAny = false;
    for (const QVariant& v : fileUrls) {
        QUrl url = v.toUrl();
        QString path = url.isLocalFile() ? url.toLocalFile() : url.toString();
        
        SlideDeck* parsedDeck = ProPresenterImporter::importProFile(path);
        if (parsedDeck) {
            ShowSerializer::saveDeckToFile(parsedDeck);
            parsedDeck->deleteLater();
            importedAny = true;
        }
    }
    if (importedAny) {
        refreshLibrary();
    }
}

void LyricsEditorViewModel::removeLibraryItem(const QString& filename) {
    QString fullPath = QDir(Library::slidedecksDir()).filePath(filename);
    if (QFile::remove(fullPath)) {
        refreshLibrary();
    }
}

void LyricsEditorViewModel::saveDeck(bool updateFromText) {
    if (!m_currentDeck) return;
    
    // Parse the current raw text into components only if requested
    if (updateFromText) {
        updateDeckFromRawText();
    }
    
    // Save to disk
    ShowSerializer::saveDeckToFile(m_currentDeck);
    
    // Refresh library in case it's a new file
    refreshLibrary();
    
    emit deckSaved(m_currentDeck->sourceFile());
}

void LyricsEditorViewModel::updateRawTextFromDeck() {
    if (!m_currentDeck) return;
    
    QString text;
    for (const QString& compName : m_currentDeck->componentOrder()) {
        text += "[" + compName + "]\n";
        
        const auto& slides = m_currentDeck->components().value(compName);
        for (int i = 0; i < slides.size(); ++i) {
            const auto& slide = slides[i];
            for (const QString& line : slide.lines) {
                text += line + "\n";
            }
            if (i < slides.size() - 1) {
                text += "\n"; // double newline for slide break
            }
        }
        text += "\n"; // space between components
    }
    
    setRawLyricsText(text.trimmed());
}

void LyricsEditorViewModel::updateDeckFromRawText() {
    if (!m_currentDeck) return;
    
    QStringList lines = m_rawLyricsText.split('\n');
    QMap<QString, QList<SlideData>> components;
    QStringList componentOrder;
    
    QString currentComp;
    QStringList currentLines;
    bool parsingDuplicate = false;
    
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();
        
        if (line.isEmpty()) {
            if (!parsingDuplicate && !currentComp.isEmpty() && !currentLines.isEmpty()) {
                SlideData data;
                data.lines = currentLines;
                data.layouts = m_screenLayouts;
                components[currentComp].append(data);
                currentLines.clear();
            } else if (parsingDuplicate) {
                currentLines.clear();
            }
            continue;
        }

        if (line.startsWith("[") && line.endsWith("]")) {
            if (!parsingDuplicate && !currentComp.isEmpty() && !currentLines.isEmpty()) {
                SlideData data;
                data.lines = currentLines;
                data.layouts = m_screenLayouts;
                components[currentComp].append(data);
                currentLines.clear();
            } else if (parsingDuplicate) {
                currentLines.clear();
            }
            
            currentComp = line.mid(1, line.length() - 2);
            if (!components.contains(currentComp)) {
                componentOrder.append(currentComp);
                components.insert(currentComp, QList<SlideData>());
                parsingDuplicate = false;
            } else {
                parsingDuplicate = true;
            }
        } else {
            if (!parsingDuplicate) {
                currentLines.append(line);
            }
        }
    }
    
    if (!parsingDuplicate && !currentComp.isEmpty() && !currentLines.isEmpty()) {
        SlideData data;
        data.lines = currentLines;
        data.layouts = m_screenLayouts;
        components[currentComp].append(data);
    }
    
    for (const QString& compName : componentOrder) {
        if (components[compName].isEmpty()) {
            SlideData data;
            data.layouts = m_screenLayouts;
            components[compName].append(data);
        }
    }
    
    m_currentDeck->setComponents(components);
    m_currentDeck->setComponentOrder(componentOrder);
    
    Arrangement* defaultArr = m_currentDeck->arrangement("Default");
    if (!defaultArr) {
        defaultArr = m_currentDeck->appendArrangement("Default");
        defaultArr->setSequence(componentOrder);
    } else if (defaultArr->sequence().isEmpty()) {
        defaultArr->setSequence(componentOrder);
    }
    
    if (m_currentDeck->defaultArrangementName().isEmpty()) {
        m_currentDeck->setDefaultArrangement("Default");
    }
    if (m_currentDeck->activeArrangementName().isEmpty()) {
        m_currentDeck->setActiveArrangement("Default");
    }
    
    emit availableComponentsChanged();
}

void LyricsEditorViewModel::addArrangement(const QString& name) {
    if (m_currentDeck) {
        m_currentDeck->appendArrangement(name);
        m_currentDeck->setActiveArrangement(name);
    }
}

void LyricsEditorViewModel::removeArrangement(const QString& name) {
    if (m_currentDeck) {
        for (int i = 0; i < m_currentDeck->arrangementCount(); ++i) {
            if (m_currentDeck->arrangementAt(i)->name() == name) {
                m_currentDeck->removeArrangement(i);
                if (m_currentDeck->activeArrangementName() == name) {
                    m_currentDeck->setActiveArrangement(m_currentDeck->arrangementNames().isEmpty() ? "" : m_currentDeck->arrangementNames().first());
                }
                break;
            }
        }
    }
}

void LyricsEditorViewModel::cloneArrangement(const QString& sourceName, const QString& newName) {
    if (m_currentDeck) {
        Arrangement* sourceArr = m_currentDeck->arrangement(sourceName);
        if (sourceArr) {
            Arrangement* newArr = m_currentDeck->appendArrangement(newName);
            newArr->setSequence(sourceArr->sequence());
            m_currentDeck->setActiveArrangement(newName);
        }
    }
}
