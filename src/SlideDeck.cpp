#include "../inc/SlideDeck.h"
#include "../inc/Slide.h"
#include "../inc/Arrangement.h"

SlideDeck::SlideDeck(QObject* parent) : QObject(parent) {}

SlideDeck::SlideDeck(const QString& name, QObject* parent)
    : QObject(parent), m_name(name) {}

void SlideDeck::setName(const QString& name) {
    if (m_name != name) {
        m_name = name;
        emit nameChanged(m_name);
    }
}

void SlideDeck::copyFrom(SlideDeck* other) {
    if (!other || other == this) return;
    
    setName(other->name());
    m_components = other->components();
    m_componentOrder = other->componentOrder();
    m_metadata = other->metadata();
    m_defaultArrangementName = other->defaultArrangementName();
    m_sourceFile = other->sourceFile();
    
    qDeleteAll(m_arrangements);
    m_arrangements.clear();
    
    for (int i = 0; i < other->arrangementCount(); ++i) {
        Arrangement* oldArr = other->arrangementAt(i);
        Arrangement* newArr = new Arrangement(oldArr->name(), this);
        newArr->setSequence(oldArr->sequence());
        m_arrangements.append(newArr);
    }
    
    emit arrangementsChanged();
    emit activeArrangementChanged(m_activeArrangementName);
}

// ─── Components & Arrangements ─────────────────────────────────────────────

void SlideDeck::setComponents(const QMap<QString, QList<SlideData>>& components) {
    m_components = components;
    emit componentsChanged();
}

void SlideDeck::addComponent(const QString& name, const QList<SlideData>& slides) {
    m_components.insert(name, slides);
    emit componentsChanged();
}

void SlideDeck::addComponent(const QString& name, const QList<QStringList>& slideLines) {
    QList<SlideData> slides;
    for (const auto& lines : slideLines) {
        SlideData data;
        data.lines = lines;
        slides.append(data);
    }
    m_components.insert(name, slides);
}

QStringList SlideDeck::componentNames() const {
    QStringList result;
    
    if (!m_componentOrder.isEmpty()) {
        result = m_componentOrder;
    } else {
        result = m_components.keys();
    }
    
    for (Arrangement* arr : m_arrangements) {
        for (const QString& name : arr->sequence()) {
            if (!result.contains(name)) {
                result.append(name);
            }
        }
    }
    
    return result;
}
Arrangement* SlideDeck::arrangementAt(int index) const {
    if (index < 0 || index >= m_arrangements.size()) return nullptr;
    return m_arrangements[index];
}

Arrangement* SlideDeck::arrangement(const QString& name) const {
    for (auto* arr : m_arrangements) {
        if (arr->name() == name)
            return arr;
    }
    return nullptr;
}

QStringList SlideDeck::arrangementNames() const {
    QStringList names;
    for (auto* arr : m_arrangements) {
        names << arr->name();
    }
    return names;
}

Arrangement* SlideDeck::appendArrangement(const QString& name) {
    auto* arr = new Arrangement(name.isEmpty() ? QStringLiteral("New Arrangement") : name, this);
    m_arrangements.append(arr);
    emit arrangementsChanged();
    return arr;
}

void SlideDeck::cloneArrangement(const QString& sourceName, const QString& newName) {
    if (sourceName.isEmpty() || newName.isEmpty()) return;
    Arrangement* source = arrangement(sourceName);
    if (!source) return;
    Arrangement* newArr = appendArrangement(newName);
    newArr->setSequence(source->sequence());
}

void SlideDeck::removeArrangement(int index) {
    if (index < 0 || index >= m_arrangements.size()) return;
    Arrangement* arr = m_arrangements.takeAt(index);
    emit arrangementsChanged();
    delete arr;
}

void SlideDeck::setActiveArrangement(const QString& name) {
    if (m_activeArrangementName != name) {
        m_activeArrangementName = name;
        emit activeArrangementChanged(m_activeArrangementName);
    }
}

void SlideDeck::setDefaultArrangement(const QString& name) {
    if (m_defaultArrangementName != name) {
        m_defaultArrangementName = name;
        emit defaultArrangementChanged(m_defaultArrangementName);
    }
}

void SlideDeck::setGlobalBackgroundMedia(const QString& mediaPath) {
    if (m_globalBackgroundMedia != mediaPath) {
        m_globalBackgroundMedia = mediaPath;
        emit globalBackgroundMediaChanged(m_globalBackgroundMedia);
    }
}
