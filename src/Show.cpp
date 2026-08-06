#include "../inc/Show.h"
#include "../inc/SlideDeck.h"
#include "../inc/ShowViewModel.h"

Show::Show(QObject* parent)
    : QAbstractListModel(parent)
{
}

Show::Show(const QString& name, QObject* parent)
    : QAbstractListModel(parent)
    , m_name(name)
{
}

int Show::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(m_decks.size());
}

QVariant Show::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_decks.size())
        return {};

    SlideDeck* deck = m_decks[index.row()];
    if (!deck) return {};

    switch (role) {
        case DeckNameRole:
            return deck->name();
        case IsActiveRole: {
            if (auto* svm = qobject_cast<ShowViewModel*>(parent())) {
                int flatIdx = svm->flatIndexForShowDeck(const_cast<Show*>(this), index.row());
                return (flatIdx == svm->activeIndex());
            }
            return false;
        }
        case HasBackgroundVideoRole:
            return !deck->globalBackgroundMedia().isEmpty();
        case FlatIndexRole: {
            if (auto* svm = qobject_cast<ShowViewModel*>(parent())) {
                return svm->flatIndexForShowDeck(const_cast<Show*>(this), index.row());
            }
            return index.row();
        }
        case DeckIndexRole:
            return index.row();
        case ShowNameRole:
            return m_name;
        default:
            return {};
    }
}

QHash<int, QByteArray> Show::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[DeckNameRole] = "deckName";
    roles[IsActiveRole] = "isActive";
    roles[HasBackgroundVideoRole] = "hasBackgroundVideo";
    roles[FlatIndexRole] = "flatIndex";
    roles[DeckIndexRole] = "deckIndex";
    roles[ShowNameRole] = "showName";
    return roles;
}

void Show::notifyActiveChanged() {
    if (!m_decks.isEmpty()) {
        emit dataChanged(index(0, 0), index(static_cast<int>(m_decks.size()) - 1, 0), {IsActiveRole, FlatIndexRole});
    }
}

// ─── Name ───────────────────────────────────────────────────────────────────

void Show::setName(const QString& name) {
    if (m_name != name) {
        m_name = name;
        emit nameChanged(m_name);
        if (!m_decks.isEmpty()) {
            emit dataChanged(index(0, 0), index(static_cast<int>(m_decks.size()) - 1, 0), {ShowNameRole});
        }
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
    int idx = static_cast<int>(m_decks.size());
    beginInsertRows(QModelIndex(), idx, idx);
    emit deckAboutToBeAdded(idx);
    deck->setParent(this);
    m_decks.append(deck);
    endInsertRows();
    emit deckAdded(idx);
    emit deckCountChanged();
}

SlideDeck* Show::appendDeck(const QString& name) {
    int idx = static_cast<int>(m_decks.size());
    beginInsertRows(QModelIndex(), idx, idx);
    emit deckAboutToBeAdded(idx);
    auto* deck = name.isEmpty()
        ? new SlideDeck(this)
        : new SlideDeck(name, this);

    m_decks.append(deck);
    endInsertRows();
    emit deckAdded(idx);
    emit deckCountChanged();
    return deck;
}

SlideDeck* Show::insertDeck(int index, const QString& name) {
    index = qBound(0, index, static_cast<int>(m_decks.size()));

    beginInsertRows(QModelIndex(), index, index);
    emit deckAboutToBeAdded(index);
    auto* deck = name.isEmpty()
        ? new SlideDeck(this)
        : new SlideDeck(name, this);

    m_decks.insert(index, deck);
    endInsertRows();
    emit deckAdded(index);
    emit deckCountChanged();
    return deck;
}

// ─── Remove ─────────────────────────────────────────────────────────────────

void Show::removeDeck(int index) {
    if (index < 0 || index >= m_decks.size())
        return;

    beginRemoveRows(QModelIndex(), index, index);
    emit deckAboutToBeRemoved(index);
    SlideDeck* deck = m_decks.takeAt(index);
    endRemoveRows();
    emit deckRemoved(index);
    emit deckCountChanged();
    delete deck;
}

// ─── Reorder ────────────────────────────────────────────────────────────────

void Show::moveDeck(int fromIndex, int toIndex) {
    if (fromIndex < 0 || fromIndex >= m_decks.size())
        return;
    toIndex = qBound(0, toIndex, static_cast<int>(m_decks.size()) - 1);
    if (fromIndex == toIndex)
        return;

    int dest = (toIndex > fromIndex) ? toIndex + 1 : toIndex;
    beginMoveRows(QModelIndex(), fromIndex, fromIndex, QModelIndex(), dest);
    emit deckAboutToBeMoved(fromIndex, toIndex);
    m_decks.move(fromIndex, toIndex);
    endMoveRows();
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
        emit dataChanged(this->index(index, 0), this->index(index, 0));
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
