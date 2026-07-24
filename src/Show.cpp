#include "../inc/Show.h"
#include "../inc/SlideDeck.h"

Show::Show(QObject* parent)
    : QObject(parent)
{
}

Show::Show(const QString& name, QObject* parent)
    : QObject(parent)
    , m_name(name)
{
}

// ─── Name ───────────────────────────────────────────────────────────────────

void Show::setName(const QString& name) {
    if (m_name != name) {
        m_name = name;
        emit nameChanged(m_name);
    }
}

// ─── Deck Access ────────────────────────────────────────────────────────────

SlideDeck* Show::deckAt(int index) const {
    if (index < 0 || index >= m_decks.size())
        return nullptr;
    return m_decks[index];
}

// ─── Add / Insert ───────────────────────────────────────────────────────────

SlideDeck* Show::appendDeck() {
    return appendDeck(QStringLiteral("Untitled Deck"));
}

void Show::addDeck(SlideDeck* deck) {
    if (!deck) return;
    emit deckAboutToBeAdded(static_cast<int>(m_decks.size()));
    deck->setParent(this);
    m_decks.append(deck);
    emit deckAdded(static_cast<int>(m_decks.size()) - 1);
}

SlideDeck* Show::appendDeck(const QString& name) {
    emit deckAboutToBeAdded(static_cast<int>(m_decks.size()));
    auto* deck = name.isEmpty()
        ? new SlideDeck(this)
        : new SlideDeck(name, this);

    m_decks.append(deck);
    emit deckAdded(static_cast<int>(m_decks.size()) - 1);
    return deck;
}

SlideDeck* Show::insertDeck(int index, const QString& name) {
    index = qBound(0, index, static_cast<int>(m_decks.size()));

    emit deckAboutToBeAdded(index);
    auto* deck = name.isEmpty()
        ? new SlideDeck(this)
        : new SlideDeck(name, this);

    m_decks.insert(index, deck);
    emit deckAdded(index);
    return deck;
}

// ─── Remove ─────────────────────────────────────────────────────────────────

void Show::removeDeck(int index) {
    if (index < 0 || index >= m_decks.size())
        return;

    emit deckAboutToBeRemoved(index);
    SlideDeck* deck = m_decks.takeAt(index);
    emit deckRemoved(index);
    delete deck;
}

// ─── Reorder ────────────────────────────────────────────────────────────────

void Show::moveDeck(int fromIndex, int toIndex) {
    if (fromIndex < 0 || fromIndex >= m_decks.size())
        return;
    toIndex = qBound(0, toIndex, static_cast<int>(m_decks.size()) - 1);
    if (fromIndex == toIndex)
        return;

    m_decks.move(fromIndex, toIndex);
    emit deckMoved(fromIndex, toIndex);
}

// ─── Replace ────────────────────────────────────────────────────────────────

void Show::replaceDeck(int index, SlideDeck* newDeck) {
    if (index >= 0 && index < m_decks.size() && newDeck) {
        SlideDeck* oldDeck = m_decks[index];
        m_decks[index] = newDeck;
        newDeck->setParent(this);
        oldDeck->deleteLater();
        emit deckReplaced(index);
    }
}

void Show::setDefaultTransitionType(const QString& type) {
    if (m_defaultTransitionType != type) {
        m_defaultTransitionType = type;
        emit defaultTransitionTypeChanged(m_defaultTransitionType);
    }
}

void Show::setDefaultTransitionDurationMs(int duration) {
    if (m_defaultTransitionDurationMs != duration) {
        m_defaultTransitionDurationMs = duration;
        emit defaultTransitionDurationMsChanged(m_defaultTransitionDurationMs);
    }
}
