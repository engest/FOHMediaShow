#include "../inc/SlideDeckViewModel.h"
#include "../inc/SlideDeck.h"
#include "../inc/Slide.h"
#include "../inc/Arrangement.h"
#include "../inc/ShowSerializer.h"
#include <QJsonObject>

SlideDeckViewModel::SlideDeckViewModel(SlideDeck* deck, QObject* parent)
    : QAbstractListModel(parent), m_deck(nullptr) {
    setDeck(deck);
}

void SlideDeckViewModel::setDeck(SlideDeck* deck) {
    if (m_deck == deck) return;
    
    if (m_deck) {
        disconnect(m_deck, nullptr, this, nullptr);
    }
    
    m_deck = deck;
    emit deckChanged();
    
    if (m_deck) {
        connect(m_deck, &SlideDeck::activeArrangementChanged, this, &SlideDeckViewModel::onActiveArrangementChanged);
        connect(m_deck, &SlideDeck::arrangementsChanged, this, &SlideDeckViewModel::onArrangementsChanged);
        connect(m_deck, &SlideDeck::componentsChanged, this, &SlideDeckViewModel::buildActiveSlides);
    }
    
    buildActiveSlides();
    
    // Always reset selection to the first slide when switching decks.
    // Must be done AFTER buildActiveSlides (and its endResetModel) so QML properly reacts.
    if (m_selectedSlideIndex == 0) {
        emitSelectedSlideChanged(); // Force emit if already 0 to update layouts
    } else {
        setSelectedSlideIndex(0);
    }
}

SlideDeckViewModel::~SlideDeckViewModel() {
    clearRenderedSlides();
}

QHash<int, QByteArray> SlideDeckViewModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[SlideTextRole] = "slideText";
    roles[ComponentNameRole] = "componentName";
    roles[IsFirstRole] = "isFirst";
    roles[LayoutsRole] = "layouts";
    roles[NextSlideTextRole] = "nextSlideText";
    return roles;
}

int SlideDeckViewModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(m_slides.size());
}

QVariant SlideDeckViewModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_slides.size() || index.row() < 0) return QVariant();
    Slide* slide = m_slides[index.row()];
    
    if (role == SlideTextRole) {
        return slide->plainText();
    } else if (role == ComponentNameRole) {
        return slide->property("componentName");
    } else if (role == IsFirstRole) {
        return slide->isFirst();
    } else if (role == LayoutsRole) {
        QJsonObject layoutsObj;
        for (auto it = slide->layouts().constBegin(); it != slide->layouts().constEnd(); ++it) {
            QString layout = it.value();
            if (layout.endsWith(".fohl")) layout.chop(5);
            layoutsObj[it.key()] = layout;
        }
        return layoutsObj;
    } else if (role == NextSlideTextRole) {
        if (index.row() + 1 < m_slides.size()) {
            return m_slides[index.row() + 1]->plainText();
        }
        return "";
    }
    return QVariant();
}

void SlideDeckViewModel::onActiveArrangementChanged() {
    buildActiveSlides();
}

void SlideDeckViewModel::onArrangementsChanged() {
    buildActiveSlides();
}

int SlideDeckViewModel::calculateSlideOffsetForArrangementIndex(int arrIndex) const {
    if (!m_activeArr) return 0;
    QStringList seq = m_activeArr->sequence();
    int offset = 0;
    const auto& comps = m_deck->components();
    for (int i = 0; i < arrIndex && i < seq.size(); ++i) {
        const QString& compName = seq.at(i);
        if (comps.contains(compName)) {
            int slidesInComp = static_cast<int>(comps.value(compName).size());
            offset += (slidesInComp == 0) ? 1 : slidesInComp;
        } else {
            offset += 1;
        }
    }
    return offset;
}

void SlideDeckViewModel::updateArrangementIndices(int startingArrIndex) {
    if (!m_activeArr) return;
    QStringList seq = m_activeArr->sequence();
    int slideIndex = calculateSlideOffsetForArrangementIndex(startingArrIndex);
    
    const auto& comps = m_deck->components();
    for (int i = startingArrIndex; i < seq.size(); ++i) {
        const QString& compName = seq.at(i);
        int slideCount = 1;
        if (comps.contains(compName)) {
            slideCount = qMax(1, static_cast<int>(comps.value(compName).size()));
        }
        for (int j = 0; j < slideCount && slideIndex < m_slides.size(); ++j) {
            m_slides[slideIndex]->setProperty("arrangementIndex", i);
            slideIndex++;
        }
    }
}

void SlideDeckViewModel::onComponentInserted(int index, const QString& name) {
    if (!m_deck || !m_activeArr) return;
    
    disconnect(m_activeArr, &Arrangement::sequenceChanged, this, &SlideDeckViewModel::buildActiveSlides);
    
    int slideOffset = calculateSlideOffsetForArrangementIndex(index);
    const auto& comps = m_deck->components();
    
    QList<SlideData> slidesData;
    QList<SlideData> newSlides;
    if (comps.contains(name) && !comps.value(name).isEmpty()) {
        newSlides = comps.value(name);
    } else {
        QMap<QString, QString> defaultLayouts;
        defaultLayouts["Audience"] = "Default.fohl";
        defaultLayouts["Stage"] = "StageDefault.fohl";
        
        auto allComps = m_deck->components();
        for (auto it = allComps.constBegin(); it != allComps.constEnd(); ++it) {
            if (!it.value().isEmpty()) {
                defaultLayouts = it.value().first().layouts;
                break;
            }
        }

        SlideData emptySlide;
        emptySlide.lines.append("");
        emptySlide.layouts = defaultLayouts;
        newSlides.append(emptySlide);
    }
    
    int numSlides = static_cast<int>(newSlides.size());
    if (numSlides > 0) {
        beginInsertRows(QModelIndex(), slideOffset, slideOffset + numSlides - 1);
        
        for (int i = 0; i < numSlides; ++i) {
            const SlideData& slideData = newSlides[i];
            bool isfirst = (i == 0);
            Slide* slide = new Slide(this, isfirst);
            slide->setPlainText(slideData.lines.join('\n'));
            slide->setLayouts(slideData.layouts);
            slide->setProperty("componentName", name);
            slide->setProperty("arrangementIndex", index);
            
            m_slides.insert(slideOffset + i, slide);
        }
        
        endInsertRows();
        
        updateArrangementIndices(index + 1);
    }
    
    connect(m_activeArr, &Arrangement::sequenceChanged, this, &SlideDeckViewModel::buildActiveSlides);
}

void SlideDeckViewModel::onComponentRemoved(int index, const QString& name) {
    if (!m_deck || !m_activeArr) return;
    
    disconnect(m_activeArr, &Arrangement::sequenceChanged, this, &SlideDeckViewModel::buildActiveSlides);
    
    int slideOffset = calculateSlideOffsetForArrangementIndex(index);
    const auto& comps = m_deck->components();
    
    int numSlides = 1;
    if (comps.contains(name) && !comps.value(name).isEmpty()) {
        numSlides = static_cast<int>(comps.value(name).size());
    }
    
    if (numSlides > 0 && slideOffset + numSlides <= m_slides.size()) {
        beginRemoveRows(QModelIndex(), slideOffset, slideOffset + numSlides - 1);
        for (int i = 0; i < numSlides; ++i) {
            Slide* slide = m_slides.takeAt(slideOffset);
            slide->deleteLater();
        }
        endRemoveRows();
        
        updateArrangementIndices(index);
    }
    
    connect(m_activeArr, &Arrangement::sequenceChanged, this, &SlideDeckViewModel::buildActiveSlides);
}

void SlideDeckViewModel::onComponentMoved(int /*fromIndex*/, int /*toIndex*/) {
    if (!m_deck || !m_activeArr) return;
    
    disconnect(m_activeArr, &Arrangement::sequenceChanged, this, &SlideDeckViewModel::buildActiveSlides);
    
    buildActiveSlides();
    
    connect(m_activeArr, &Arrangement::sequenceChanged, this, &SlideDeckViewModel::buildActiveSlides);
}

void SlideDeckViewModel::buildActiveSlides() {
    if (m_isFlushing) return;

    beginResetModel();
    if (!m_deck) {
        clearRenderedSlides();
    } else {
        clearRenderedSlides();
    }
    
    if (!m_deck) {
        endResetModel();
        return;
    }
    Arrangement* activeArr = m_deck->arrangement(m_deck->activeArrangementName());
    
    if (m_activeArr != activeArr) {
        if (m_activeArr) {
            disconnect(m_activeArr, &Arrangement::sequenceChanged, this, &SlideDeckViewModel::buildActiveSlides);
        }
        m_activeArr = activeArr;
        if (m_activeArr) {
            connect(m_activeArr, &Arrangement::sequenceChanged, this, &SlideDeckViewModel::buildActiveSlides);
            connect(m_activeArr, &Arrangement::componentInserted, this, &SlideDeckViewModel::onComponentInserted);
            connect(m_activeArr, &Arrangement::componentRemoved, this, &SlideDeckViewModel::onComponentRemoved);
            connect(m_activeArr, &Arrangement::componentMoved, this, &SlideDeckViewModel::onComponentMoved);
        }
    }

    QStringList sequence = activeArr ? activeArr->sequence() : m_deck->components().keys();

    const auto& comps = m_deck->components();

    int arrIndex = 0;
    for (const QString& compName : sequence) {
        bool isfirst = true;
        if (comps.contains(compName)) {
            const auto& slidesList = comps.value(compName);
            for (const SlideData& slideData : slidesList) {
                Slide* slide = nullptr;
                if (slideData.lines.isEmpty()) {
                    slide = appendSlide("", isfirst);
                } else {
                    slide = appendSlide(slideData.lines.join('\n'), isfirst);
                }
                if (slide) {
                    slide->setLayouts(slideData.layouts);
                    slide->setProperty("componentName", compName);
                    slide->setProperty("arrangementIndex", arrIndex);
                }
                isfirst = false;
            }
        } else {
            Slide* slide = appendSlide("", isfirst);
            if (slide) {
                // Try to infer layout from other components in the deck, otherwise default
                QMap<QString, QString> defaultLayouts;
                defaultLayouts["Audience"] = "Default.fohl";
                defaultLayouts["Stage"] = "StageDefault.fohl";
                for (auto it = comps.constBegin(); it != comps.constEnd(); ++it) {
                    if (!it.value().isEmpty()) {
                        defaultLayouts = it.value().first().layouts;
                        break;
                    }
                }
                slide->setLayouts(defaultLayouts);
                slide->setProperty("componentName", compName);
                slide->setProperty("arrangementIndex", arrIndex);
            }
        }
        arrIndex++;
    }

    // Failsafe so the UI never crashes on an empty deck
    if (m_slides.isEmpty()) {
        appendSlide();
    }

    emit slidesRebuilt();
    endResetModel();
    
    emitSelectedSlideChanged();
    emit availableComponentNamesChanged();
}

void SlideDeckViewModel::flushSlidesToComponents(int lastEditedSlideIndex) {
    if (!m_deck) return;

    struct Instance {
        QList<SlideData> slides;
        bool containsLastEdited = false;
    };

    QMap<QString, QList<Instance>> allInstances; // comp name → ordered list of instances
    QMap<QString, int> preferredInstance;         // comp name → index of instance to use

    Instance currentInstance;
    QString currentComp;
    int slidePos = 0;

    auto commitInstance = [&]() {
        if (currentComp.isEmpty()) return;
        int instanceIdx = static_cast<int>(allInstances[currentComp].size());
        if (currentInstance.containsLastEdited)
            preferredInstance[currentComp] = instanceIdx;
        allInstances[currentComp].append(currentInstance);
        currentInstance = Instance();
    };

    for (Slide* slide : m_slides) {
        QString compName = slide->property("componentName").toString();
        if (compName.isEmpty()) { ++slidePos; continue; }

        if (slide->isFirst()) {
            commitInstance();
            currentComp = compName;
        }

        if (slidePos == lastEditedSlideIndex)
            currentInstance.containsLastEdited = true;

        SlideData data;
        data.lines = slide->plainText().split(QLatin1Char('\n'));
        data.layouts = slide->layouts();
        
        currentInstance.slides.append(data);
        ++slidePos;
    }
    commitInstance();

    // Write the preferred instance for each component
    auto comps = m_deck->components();
    for (auto it = allInstances.constBegin(); it != allInstances.constEnd(); ++it) {
        const QString& compName = it.key();
        const QList<Instance>& instances = it.value();
        if (instances.isEmpty()) continue;
        int useIdx = preferredInstance.value(compName, 0); // default to first
        comps[compName] = instances[useIdx].slides;
    }
    m_isFlushing = true;
    m_deck->setComponents(comps);
    m_isFlushing = false;
}

void SlideDeckViewModel::clearRenderedSlides() {
    for (int i = static_cast<int>(m_slides.size()) - 1; i >= 0; --i) {
        Slide* slide = m_slides.takeAt(i);
        emit slideRemoved(i);
        delete slide;
    }
}

Slide* SlideDeckViewModel::slideAt(int index) const {
    if (index < 0 || index >= m_slides.size()) {
        return nullptr;
    }
    return m_slides[index];
}

Slide* SlideDeckViewModel::appendSlide(const QString& text, bool isfirst) {
    beginInsertRows(QModelIndex(), static_cast<int>(m_slides.size()), static_cast<int>(m_slides.size()));
    auto* slide = text.isEmpty() ? new Slide(this, isfirst) : new Slide(text, this, isfirst);
    if (!m_isFlushing) {
        SlideData slideData;
        slideData.lines = text.split('\n');
        slideData.layouts["Audience"] = "Default.fohl";
        slideData.layouts["Stage"] = "StageDefault.fohl";
        
        if (m_deck) {
            // Find a default from the deck if possible
            auto activeComps = m_deck->components();
            if (!activeComps.isEmpty()) {
                slideData.layouts = activeComps.first().first().layouts;
            }
        }
        
        slide->setLayouts(slideData.layouts);
    }
    m_slides.append(slide);
    endInsertRows();
    emit slideAdded(static_cast<int>(m_slides.size()) - 1);
    return slide;
}

Slide* SlideDeckViewModel::insertSlide(int index, const QString& text) {
    index = qBound(0, index, static_cast<int>(m_slides.size()));
    beginInsertRows(QModelIndex(), index, index);
    auto* slide = text.isEmpty() ? new Slide(this) : new Slide(text, this);
    m_slides.insert(index, slide);
    endInsertRows();
    emit slideAdded(index);
    return slide;
}

void SlideDeckViewModel::removeSlide(int index) {
    if (index < 0 || index >= m_slides.size()) return;
    beginRemoveRows(QModelIndex(), index, index);
    Slide* slide = m_slides.takeAt(index);
    endRemoveRows();
    emit slideRemoved(index);
    delete slide;
}

void SlideDeckViewModel::moveSlide(int fromIndex, int toIndex) {
    if (fromIndex < 0 || fromIndex >= m_slides.size() || toIndex < 0 || toIndex >= m_slides.size() || fromIndex == toIndex) return;

    beginMoveRows(QModelIndex(), fromIndex, fromIndex, QModelIndex(), toIndex > fromIndex ? toIndex + 1 : toIndex);
    m_slides.move(fromIndex, toIndex);
    endMoveRows();
    emit slideMoved(fromIndex, toIndex);
}

void SlideDeckViewModel::setSlideText(int index, const QString& text) {
    if (index >= 0 && index < m_slides.size()) {
        QString sanitized = text;
        sanitized.replace(QChar(0x2028), '\n'); // QML Line Separator
        sanitized.replace(QChar(0x2029), '\n'); // QML Paragraph Separator
        
        m_slides[index]->setPlainText(sanitized);
        if (m_selectedSlideIndex == index || m_selectedSlideIndex == index - 1) {
            emitSelectedSlideChanged();
        }
        flushSlidesToComponents(index);
        saveDeck();
    }
}

void SlideDeckViewModel::saveAllEdits() {
    flushSlidesToComponents(-1);
    saveDeck();
}

void SlideDeckViewModel::insertBlankSlideAfter(int index) {
    if (!m_deck || index < 0 || index >= m_slides.size()) return;

    QString compName = m_slides[index]->property("componentName").toString();

    int arrIndex = m_slides[index]->property("arrangementIndex").toInt();

    // Insert a blank slide in the view model
    int newIndex = index + 1;
    beginInsertRows(QModelIndex(), newIndex, newIndex);
    Slide* slide = new Slide(this, false);
    slide->setLayoutForScreen("Audience", "Default.fohl");
    slide->setLayoutForScreen("Stage", "StageDefault.fohl");
    
    // Copy layouts from previous slide if it exists
    if (index > 0 && index - 1 < m_slides.size()) {
        slide->setLayouts(m_slides[index - 1]->layouts());
    }
    slide->setProperty("componentName", compName);
    slide->setProperty("arrangementIndex", arrIndex);
    m_slides.insert(newIndex, slide);
    endInsertRows();

    // Persist immediately
    flushSlidesToComponents(newIndex);
    saveDeck();
    emit slideAdded(newIndex);
}

void SlideDeckViewModel::removeSlideAndSave(int index) {
    if (!m_deck || index < 0 || index >= m_slides.size()) return;

    beginRemoveRows(QModelIndex(), index, index);
    Slide* slide = m_slides.takeAt(index);
    endRemoveRows();
    delete slide;

    // Persist immediately
    flushSlidesToComponents(-1);
    saveDeck();
    emit slideRemoved(index);
}

QStringList SlideDeckViewModel::availableComponentNames() const {
    if (!m_deck) return {};
    
    QStringList all = {
        "Verse", "Verse 1", "Verse 2", "Verse 3", "Verse 4", "Verse 5", "Verse 6",
        "Chorus", "Chorus 1", "Chorus 2", "Chorus 3", "Chorus 4",
        "Bridge", "Bridge 1", "Bridge 2", "Bridge 3",
        "PreChorus", "Refrain",
        "Tag", "Tag 1", "Tag 2",
        "Intro", "Ending", "Outro",
        "Interlude", "Instrumental", "Vamp", "Turnaround", "PassThrough",
        "Blank"
    };
    
    QStringList used = m_deck->components().keys();
    QStringList available;
    for (const QString& comp : all) {
        if (!used.contains(comp)) {
            available.append(comp);
        }
    }
    return available;
}

void SlideDeckViewModel::splitComponentGroup(int slideIndex, const QString& newComponent) {
    if (!m_deck || slideIndex < 0 || slideIndex >= m_slides.size() || newComponent.isEmpty()) return;

    QString oldComp = m_slides[slideIndex]->property("componentName").toString();
    int arrIndex = m_slides[slideIndex]->property("arrangementIndex").toInt();

    bool replacingEntireGroup = m_slides[slideIndex]->isFirst();

    // Iterate forward and change component name for all contiguous slides belonging to oldComp
    m_slides[slideIndex]->setProperty("componentName", newComponent);
    m_slides[slideIndex]->setIsFirst(true);
    emit dataChanged(index(slideIndex, 0), index(slideIndex, 0), {ComponentNameRole, IsFirstRole});
    
    for (int i = slideIndex + 1; i < m_slides.size(); ++i) {
        if (m_slides[i]->isFirst()) {
            break; // Start of a new component group, even if it has the same name
        }
        if (m_slides[i]->property("componentName").toString() == oldComp) {
            m_slides[i]->setProperty("componentName", newComponent);
            m_slides[i]->setIsFirst(false);
            emit dataChanged(index(i, 0), index(i, 0), {ComponentNameRole, IsFirstRole});
        } else {
            break; // End of contiguous group
        }
    }

    // Save modified slides to components FIRST, before changing the arrangement
    flushSlidesToComponents(-1);

    // Update the active arrangement
    if (m_activeArr) {
        // Disconnect temporarily so SlideDeckViewModel doesn't duplicate the mutation
        disconnect(m_activeArr, &Arrangement::sequenceChanged, this, &SlideDeckViewModel::buildActiveSlides);
        disconnect(m_activeArr, &Arrangement::componentInserted, this, &SlideDeckViewModel::onComponentInserted);
        
        if (replacingEntireGroup) {
            m_activeArr->replaceComponentAt(arrIndex, newComponent);
        } else {
            m_activeArr->insertComponentAt(arrIndex + 1, newComponent);
        }
        
        connect(m_activeArr, &Arrangement::sequenceChanged, this, &SlideDeckViewModel::buildActiveSlides);
        connect(m_activeArr, &Arrangement::componentInserted, this, &SlideDeckViewModel::onComponentInserted);
    }

    // Save changes to disk
    saveDeck();
}

int SlideDeckViewModel::firstSlideIndexOfComponent(const QString& componentName) const {
    for (int i = 0; i < m_slides.size(); ++i) {
        if (m_slides[i]->property("componentName").toString() == componentName) {
            return i;
        }
    }
    return -1;
}

int SlideDeckViewModel::firstSlideIndexOfArrangementIndex(int arrIndex) const {
    for (int i = 0; i < m_slides.size(); ++i) {
        if (m_slides[i]->property("arrangementIndex").toInt() == arrIndex) {
            return i;
        }
    }
    return -1;
}

QVariantMap SlideDeckViewModel::getSlideDataForPreview(int index) const {
    QVariantMap map;
    if (index >= 0 && index < m_slides.size()) {
        Slide* slide = m_slides[index];
        map["slideText"] = slide->plainText();
        QVariantMap layouts;
        for (auto it = slide->layouts().constBegin(); it != slide->layouts().constEnd(); ++it) {
            QString layout = it.value();
            if (layout.endsWith(".fohl")) layout.chop(5);
            layouts[it.key()] = layout;
        }
        map["layouts"] = layouts;
        
        if (index + 1 < m_slides.size()) {
            map["nextSlideText"] = m_slides[index + 1]->plainText();
        } else {
            map["nextSlideText"] = "";
        }
    } else {
        map["slideText"] = "";
        map["layouts"] = QVariantMap();
        map["nextSlideText"] = "";
    }
    return map;
}

QList<SlideData> SlideDeckViewModel::toSlideDataList() const {
    QList<SlideData> dataList;
    for (const Slide* slide : m_slides) {
        SlideData data;
        data.lines = slide->plainText().split('\n');
        data.layouts = slide->layouts();
        dataList.append(data);
    }
    return dataList;
}

void SlideDeckViewModel::setSelectedSlideIndex(int index) {
    if (m_selectedSlideIndex != index) {
        m_selectedSlideIndex = index;
        emitSelectedSlideChanged();
    }
}

QString SlideDeckViewModel::selectedSlideText() const {
    if (m_selectedSlideIndex >= 0 && m_selectedSlideIndex < m_slides.size()) {
        return m_slides[m_selectedSlideIndex]->plainText();
    }
    return "";
}

QString SlideDeckViewModel::getLayoutForScreen(int index, const QString& screenName) const {
    if (index >= 0 && index < m_slides.size()) {
        QString layout = m_slides[index]->layoutForScreen(screenName);
        if (layout.endsWith(".fohl")) layout.chop(5);
        if (layout.isEmpty()) {
            return (screenName == "Stage") ? "StageDefault" : "Default";
        }
        return layout;
    }
    return (screenName == "Stage") ? "StageDefault" : "Default";
}

void SlideDeckViewModel::setLayoutForScreen(int index, const QString& screenName, const QString& layoutFile) {
    if (index >= 0 && index < m_slides.size()) {
        QString file = layoutFile;
        if (!file.endsWith(".fohl")) file += ".fohl";
        m_slides[index]->setLayoutForScreen(screenName, file);
        emit dataChanged(this->index(index, 0), this->index(index, 0), {LayoutsRole});
        if (m_selectedSlideIndex == index) emitSelectedSlideChanged();
        saveDeck();
    }
}

QJsonObject SlideDeckViewModel::getLayoutsForSlide(int index) const {
    QJsonObject obj;
    if (index >= 0 && index < m_slides.size()) {
        auto map = m_slides[index]->layouts();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            QString layout = it.value();
            if (layout.endsWith(".fohl")) layout.chop(5);
            obj[it.key()] = layout;
        }
    }
    return obj;
}

QString SlideDeckViewModel::selectedNextSlideText() const {
    if (m_selectedSlideIndex >= 0 && m_selectedSlideIndex + 1 < m_slides.size()) {
        return m_slides[m_selectedSlideIndex + 1]->plainText();
    }
    return "";
}

void SlideDeckViewModel::emitSelectedSlideChanged() {
    emit selectedSlideChanged();
}

void SlideDeckViewModel::saveDeck() {
    if (m_deck) {
        ShowSerializer::saveDeckToFile(m_deck);
    }
}
