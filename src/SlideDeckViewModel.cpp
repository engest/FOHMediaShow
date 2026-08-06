#include "../inc/SlideDeckViewModel.h"
#include "../inc/SlideDeck.h"
#include "../inc/Slide.h"
#include "../inc/Arrangement.h"
#include "../inc/ShowSerializer.h"
#include "../inc/ScreenModel.h"
#include <QJsonObject>

SlideDeckViewModel::SlideDeckViewModel(SlideDeck* deck, QObject* parent)
    : QAbstractListModel(parent), m_deck(nullptr) {
    m_saveTimer = new QTimer(this);
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(500); // 500ms debounce
    connect(m_saveTimer, &QTimer::timeout, this, &SlideDeckViewModel::performSaveDeck);
    
    setDeck(deck);
}

void SlideDeckViewModel::setDeck(SlideDeck* deck) {
    if (m_deck == deck) return;
    
    // Force save any pending edits for the old deck before switching
    if (m_saveTimer && m_saveTimer->isActive()) {
        m_saveTimer->stop();
        performSaveDeck();
    }
    
    if (m_deck) {
        disconnect(m_deck, nullptr, this, nullptr);
    }
    if (m_activeArr) {
        disconnect(m_activeArr, nullptr, this, nullptr);
        m_activeArr = nullptr;
    }
    
    m_deck = deck;
    emit deckChanged();
    
    if (m_deck) {
        connect(m_deck, &SlideDeck::activeArrangementChanged, this, &SlideDeckViewModel::onActiveArrangementChanged);
        connect(m_deck, &SlideDeck::arrangementsChanged, this, &SlideDeckViewModel::onArrangementsChanged);
        connect(m_deck, &SlideDeck::componentsChanged, this, &SlideDeckViewModel::buildActiveSlides);
    }
    
    // Always reset selection to the first slide when switching decks.
    // Must be set BEFORE buildActiveSlides so any signals (e.g. slidesRebuilt) read index 0.
    m_selectedSlideIndex = 0;

    buildActiveSlides();
    
    emitSelectedSlideChanged();
}

SlideDeckViewModel::~SlideDeckViewModel() {
    if (m_saveTimer && m_saveTimer->isActive()) {
        m_saveTimer->stop();
        performSaveDeck();
    }
    if (m_activeArr) {
        disconnect(m_activeArr, nullptr, this, nullptr);
        m_activeArr = nullptr;
    }
    clearRenderedSlides();
}

QHash<int, QByteArray> SlideDeckViewModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[SlideTextRole] = "slideText";
    roles[ComponentNameRole] = "componentName";
    roles[IsFirstRole] = "isFirst";
    roles[LayoutsRole] = "layouts";
    roles[NextSlideTextRole] = "nextSlideText";
    roles[CardLayoutRole] = "cardLayout";
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
    } else if (role == CardLayoutRole) {
        return getCardLayout(index.row());
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
    int offset = 0;
    for (int i = 0; i < m_slides.size(); ++i) {
        if (m_slides[i]->property("arrangementIndex").toInt() >= arrIndex) {
            return i;
        }
        offset = i + 1;
    }
    return offset;
}

void SlideDeckViewModel::updateArrangementIndices(int /*startingArrIndex*/) {
    // No-op kept for header compatibility; indices are maintained directly in slots
}

void SlideDeckViewModel::onComponentInserted(int index, const QString& name) {
    if (!m_deck || !m_activeArr) return;
    
    disconnect(m_activeArr, &Arrangement::sequenceChanged, this, &SlideDeckViewModel::buildActiveSlides);
    
    int slideOffset = static_cast<int>(m_slides.size());
    for (int i = 0; i < m_slides.size(); ++i) {
        if (m_slides[i]->property("arrangementIndex").toInt() >= index) {
            slideOffset = i;
            break;
        }
    }
    
    // Shift arrangementIndex for existing slides from slideOffset onwards
    for (int i = slideOffset; i < m_slides.size(); ++i) {
        int currentArrIdx = m_slides[i]->property("arrangementIndex").toInt();
        m_slides[i]->setProperty("arrangementIndex", currentArrIdx + 1);
    }
    
    const auto& comps = m_deck->components();
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
        
        if (slideOffset <= m_selectedSlideIndex) {
            m_selectedSlideIndex += numSlides;
            emitSelectedSlideChanged();
        }
        
        emit dataChanged(this->index(slideOffset, 0), this->index(static_cast<int>(m_slides.size()) - 1, 0),
                         {SlideTextRole, ComponentNameRole, IsFirstRole, LayoutsRole, NextSlideTextRole, CardLayoutRole});
        emit slidesInserted(slideOffset, numSlides);
        emit slidesUpdated();
    }
    
    connect(m_activeArr, &Arrangement::sequenceChanged, this, &SlideDeckViewModel::buildActiveSlides);
}

void SlideDeckViewModel::onComponentRemoved(int index, const QString& /*name*/) {
    if (!m_deck || !m_activeArr) return;
    
    disconnect(m_activeArr, &Arrangement::sequenceChanged, this, &SlideDeckViewModel::buildActiveSlides);
    
    int oldSlideStart = -1;
    int slideCount = 0;
    for (int i = 0; i < m_slides.size(); ++i) {
        if (m_slides[i]->property("arrangementIndex").toInt() == index) {
            if (oldSlideStart == -1) oldSlideStart = i;
            slideCount++;
        }
    }
    
    if (slideCount > 0 && oldSlideStart != -1) {
        beginRemoveRows(QModelIndex(), oldSlideStart, oldSlideStart + slideCount - 1);
        for (int i = 0; i < slideCount; ++i) {
            Slide* slide = m_slides.takeAt(oldSlideStart);
            slide->deleteLater();
        }
        endRemoveRows();
        
        if (oldSlideStart <= m_selectedSlideIndex) {
            if (m_selectedSlideIndex < oldSlideStart + slideCount) {
                m_selectedSlideIndex = qBound(0, oldSlideStart, static_cast<int>(m_slides.size()) - 1);
            } else {
                m_selectedSlideIndex -= slideCount;
            }
            emitSelectedSlideChanged();
        }
        
        // Update arrangementIndex for remaining slides
        for (int i = oldSlideStart; i < m_slides.size(); ++i) {
            int currentArrIdx = m_slides[i]->property("arrangementIndex").toInt();
            if (currentArrIdx > index) {
                m_slides[i]->setProperty("arrangementIndex", currentArrIdx - 1);
            }
        }
        
        if (m_slides.isEmpty()) {
            // Failsafe so deck is never empty
            appendSlide();
        } else if (oldSlideStart < m_slides.size()) {
            emit dataChanged(this->index(oldSlideStart, 0), this->index(static_cast<int>(m_slides.size()) - 1, 0),
                             {SlideTextRole, ComponentNameRole, IsFirstRole, LayoutsRole, NextSlideTextRole, CardLayoutRole});
        }
        
        emit slidesRemovedEvent(oldSlideStart, slideCount);
        emit slidesUpdated();
    }
    
    connect(m_activeArr, &Arrangement::sequenceChanged, this, &SlideDeckViewModel::buildActiveSlides);
}

void SlideDeckViewModel::onComponentMoved(int fromIndex, int toIndex) {
    if (!m_deck || !m_activeArr) return;
    if (fromIndex == toIndex) return;
    
    disconnect(m_activeArr, &Arrangement::sequenceChanged, this, &SlideDeckViewModel::buildActiveSlides);
    
    int oldSlideStart = -1;
    int slideCount = 0;
    for (int i = 0; i < m_slides.size(); ++i) {
        if (m_slides[i]->property("arrangementIndex").toInt() == fromIndex) {
            if (oldSlideStart == -1) oldSlideStart = i;
            slideCount++;
        }
    }
    
    if (slideCount > 0 && oldSlideStart != -1) {
        Slide* selectedSlide = (m_selectedSlideIndex >= 0 && m_selectedSlideIndex < m_slides.size()) ? m_slides[m_selectedSlideIndex] : nullptr;

        // Collect moved slides
        QList<Slide*> movedSlides;
        for (int i = 0; i < slideCount; ++i) {
            movedSlides.append(m_slides.takeAt(oldSlideStart));
        }
        
        // Update arrangementIndex on remaining slides
        for (Slide* s : m_slides) {
            int arrIdx = s->property("arrangementIndex").toInt();
            if (fromIndex < toIndex) {
                if (arrIdx > fromIndex && arrIdx <= toIndex) {
                    s->setProperty("arrangementIndex", arrIdx - 1);
                }
            } else {
                if (arrIdx >= toIndex && arrIdx < fromIndex) {
                    s->setProperty("arrangementIndex", arrIdx + 1);
                }
            }
        }
        
        // Determine insertion position in m_slides
        int insertPos = static_cast<int>(m_slides.size());
        for (int i = 0; i < m_slides.size(); ++i) {
            if (m_slides[i]->property("arrangementIndex").toInt() > toIndex) {
                insertPos = i;
                break;
            }
        }
        if (toIndex == 0) insertPos = 0;
        
        // Set new arrangementIndex for moved slides
        for (Slide* s : movedSlides) {
            s->setProperty("arrangementIndex", toIndex);
        }
        
        int destRow = (insertPos > oldSlideStart) ? (insertPos + slideCount) : insertPos;
        beginMoveRows(QModelIndex(), oldSlideStart, oldSlideStart + slideCount - 1, QModelIndex(), destRow);
        for (int i = 0; i < slideCount; ++i) {
            m_slides.insert(insertPos + i, movedSlides[i]);
        }
        endMoveRows();
        
        if (selectedSlide) {
            int newIdx = static_cast<int>(m_slides.indexOf(selectedSlide));
            if (newIdx != -1 && newIdx != m_selectedSlideIndex) {
                m_selectedSlideIndex = newIdx;
                emitSelectedSlideChanged();
            }
        }

        emit dataChanged(this->index(0, 0), this->index(static_cast<int>(m_slides.size()) - 1, 0),
                         {SlideTextRole, ComponentNameRole, IsFirstRole, LayoutsRole, NextSlideTextRole, CardLayoutRole});
        emit slidesUpdated();
    }
    
    connect(m_activeArr, &Arrangement::sequenceChanged, this, &SlideDeckViewModel::buildActiveSlides);
}

void SlideDeckViewModel::buildActiveSlides() {
    if (m_isFlushing) return;

    QString prevCompName;
    QString prevSlideText;
    int prevCompSlideIdx = 0;
    int prevArrIdx = -1;
    bool hadSelection = (m_selectedSlideIndex >= 0 && m_selectedSlideIndex < m_slides.size());
    if (hadSelection) {
        Slide* cur = m_slides[m_selectedSlideIndex];
        prevCompName = cur->property("componentName").toString();
        prevArrIdx = cur->property("arrangementIndex").toInt();
        prevSlideText = cur->plainText();
        for (int i = 0; i < m_selectedSlideIndex; ++i) {
            if (m_slides[i]->property("arrangementIndex").toInt() == prevArrIdx) {
                prevCompSlideIdx++;
            }
        }
    }

    beginResetModel();
    clearRenderedSlides();
    
    if (!m_deck) {
        endResetModel();
        emit slidesRebuilt();
        return;
    }
    Arrangement* activeArr = m_deck->arrangement(m_deck->activeArrangementName());
    
    if (m_activeArr != activeArr) {
        if (m_activeArr) {
            disconnect(m_activeArr, nullptr, this, nullptr);
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
                    slide = new Slide(this, isfirst);
                } else {
                    slide = new Slide(slideData.lines.join('\n'), this, isfirst);
                }
                if (slide) {
                    slide->setLayouts(slideData.layouts);
                    slide->setProperty("componentName", compName);
                    slide->setProperty("arrangementIndex", arrIndex);
                    m_slides.append(slide);
                }
                isfirst = false;
            }
        } else {
            Slide* slide = new Slide(this, isfirst);
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
                m_slides.append(slide);
            }
        }
        arrIndex++;
    }

    // Failsafe so the UI never crashes on an empty deck
    if (m_slides.isEmpty()) {
        Slide* slide = new Slide(this, true);
        slide->setProperty("componentName", "Slide");
        slide->setProperty("arrangementIndex", 0);
        m_slides.append(slide);
    }

    if (hadSelection) {
        int matchedIndex = -1;
        int curCompSlide = 0;
        for (int i = 0; i < m_slides.size(); ++i) {
            if (m_slides[i]->property("arrangementIndex").toInt() == prevArrIdx) {
                if (curCompSlide == prevCompSlideIdx) {
                    matchedIndex = i;
                    break;
                }
                curCompSlide++;
            }
        }
        if (matchedIndex == -1) {
            for (int i = 0; i < m_slides.size(); ++i) {
                if (m_slides[i]->property("componentName").toString() == prevCompName &&
                    m_slides[i]->plainText() == prevSlideText) {
                    matchedIndex = i;
                    break;
                }
            }
        }
        if (matchedIndex != -1) {
            m_selectedSlideIndex = matchedIndex;
        } else {
            m_selectedSlideIndex = qBound(0, m_selectedSlideIndex, static_cast<int>(m_slides.size()) - 1);
        }
    } else {
        m_selectedSlideIndex = 0;
    }

    endResetModel();
    
    emit slidesRebuilt();
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
        emit slidesUpdated();
    }
}

void SlideDeckViewModel::saveAllEdits() {
    flushSlidesToComponents(-1);
    if (m_saveTimer) {
        m_saveTimer->stop();
    }
    performSaveDeck();
    emit slidesUpdated();
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
    emit slidesUpdated();
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
    emit slidesUpdated();
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
    emit slidesUpdated();
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

int SlideDeckViewModel::arrangementIndexForSlideIndex(int slideIndex) const {
    if (slideIndex >= 0 && slideIndex < m_slides.size()) {
        return m_slides[slideIndex]->property("arrangementIndex").toInt();
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
    for (int i = 0; i < m_slides.size(); ++i) {
        SlideData data;
        data.lines = m_slides[i]->plainText().split('\n');
        data.layouts = m_slides[i]->layouts();
        data.componentName = m_slides[i]->property("componentName").toString();
        
        // Calculate contiguous block bounds
        int start = i;
        while (start > 0 && m_slides[start - 1]->property("componentName").toString() == data.componentName) {
            start--;
        }
        int end = i;
        while (end + 1 < m_slides.size() && m_slides[end + 1]->property("componentName").toString() == data.componentName) {
            end++;
        }
        
        data.groupSlideCount = (end - start) + 1;
        data.groupSlideIndex = i - start;
        
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

void SlideDeckViewModel::setScreenModel(ScreenModel* model) {
    if (m_screenModel == model) return;
    if (m_screenModel) {
        disconnect(m_screenModel, nullptr, this, nullptr);
    }
    m_screenModel = model;
    if (m_screenModel) {
        auto refreshSlides = [this]() {
            if (!m_slides.isEmpty()) {
                emit dataChanged(index(0, 0), index(static_cast<int>(m_slides.size()) - 1, 0));
                emit slidesUpdated();
            }
        };
        connect(m_screenModel, &ScreenModel::screensChanged, this, refreshSlides);
        connect(m_screenModel, &ScreenModel::dataChanged, this, refreshSlides);
    }
}

QString SlideDeckViewModel::getCardLayout(int index) const {
    if (index < 0 || index >= m_slides.size()) return "Default";

    const Slide* slide = m_slides[index];
    const auto& layouts = slide->layouts();

    // Check if Audience screen is disabled in ScreenModel
    bool audienceDisabled = false;
    if (m_screenModel) {
        for (const auto& s : m_screenModel->screens()) {
            if (s.name.compare("Audience", Qt::CaseInsensitive) == 0) {
                if (s.disabled || s.hardwareDisplayIndex == -2) {
                    audienceDisabled = true;
                }
                break;
            }
        }
    }

    // 1. Check "Audience" screen layout first (if Audience is not disabled)
    if (!audienceDisabled) {
        QString audLayout = layouts.value("Audience");
        if (audLayout.endsWith(".fohl")) audLayout.chop(5);
        if (!audLayout.isEmpty() && audLayout != "Disabled") {
            return audLayout;
        }
    }

    // 2. Check other non-Stage screens that are enabled
    for (auto it = layouts.constBegin(); it != layouts.constEnd(); ++it) {
        if (it.key().compare("Stage", Qt::CaseInsensitive) == 0 ||
            it.key().compare("Audience", Qt::CaseInsensitive) == 0) {
            continue;
        }
        if (m_screenModel) {
            bool isScDisabled = false;
            for (const auto& s : m_screenModel->screens()) {
                if (s.name.compare(it.key(), Qt::CaseInsensitive) == 0) {
                    if (s.disabled || s.hardwareDisplayIndex == -2) {
                        isScDisabled = true;
                    }
                    break;
                }
            }
            if (isScDisabled) continue;
        }

        QString l = it.value();
        if (l.endsWith(".fohl")) l.chop(5);
        if (!l.isEmpty() && l != "Disabled") {
            return l;
        }
    }

    // 3. Fallback to "Default"
    return "Default";
}

void SlideDeckViewModel::setLayoutForScreen(int index, const QString& screenName, const QString& layoutFile) {
    if (index >= 0 && index < m_slides.size()) {
        QString file = layoutFile;
        if (!file.endsWith(".fohl")) file += ".fohl";
        m_slides[index]->setLayoutForScreen(screenName, file);
        emit dataChanged(this->index(index, 0), this->index(index, 0), {LayoutsRole});
        if (m_selectedSlideIndex == index) emitSelectedSlideChanged();
        saveDeck();
        emit slidesUpdated();
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
    if (m_saveTimer) {
        m_saveTimer->start();
    }
}

void SlideDeckViewModel::performSaveDeck() {
    if (m_deck) {
        ShowSerializer::saveDeckToFile(m_deck);
    }
}
