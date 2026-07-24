#include "../inc/ArrangementViewModel.h"
#include "../inc/SlideDeck.h"
#include "../inc/Arrangement.h"

ArrangementViewModel::ArrangementViewModel(SlideDeck* deck, QObject* parent)
    : QAbstractListModel(parent) {
    setDeck(deck);
}

void ArrangementViewModel::setDeck(SlideDeck* deck) {
    if (m_deck) {
        disconnect(m_deck, nullptr, this, nullptr);
        // Also disconnect from any active arrangement
        Arrangement* arr = m_deck->arrangement(m_deck->activeArrangementName());
        if (arr) {
            disconnect(arr, nullptr, this, nullptr);
        }
    }
    
    m_deck = deck;
    if (m_deck) {
        connect(m_deck, &SlideDeck::activeArrangementChanged, this, &ArrangementViewModel::onActiveArrangementChanged);
        connect(m_deck, &SlideDeck::componentsChanged, this, &ArrangementViewModel::onSequenceChanged);
        Arrangement* arr = m_deck->arrangement(m_deck->activeArrangementName());
        if (arr) {
            connect(arr, &Arrangement::sequenceChanged, this, &ArrangementViewModel::onSequenceChanged);
            connect(arr, &Arrangement::componentInserted, this, &ArrangementViewModel::onComponentInserted);
            connect(arr, &Arrangement::componentRemoved, this, &ArrangementViewModel::onComponentRemoved);
            connect(arr, &Arrangement::componentReplaced, this, &ArrangementViewModel::onComponentReplaced);
            connect(arr, &Arrangement::componentMoved, this, &ArrangementViewModel::onComponentMoved);
        }
    }
    reloadSequence();
}

void ArrangementViewModel::onActiveArrangementChanged() {
    // Reconnect to the new active arrangement
    if (m_deck) {
        // Disconnect all previous arrangements (simple way is to disconnect all from this slot, wait, better to track it)
        // To be safe, disconnect from all arrangements since only one is active
        for (int i = 0; i < m_deck->arrangementCount(); ++i) {
            Arrangement* a = m_deck->arrangementAt(i);
            disconnect(a, &Arrangement::sequenceChanged, this, &ArrangementViewModel::onSequenceChanged);
            disconnect(a, &Arrangement::componentInserted, this, &ArrangementViewModel::onComponentInserted);
            disconnect(a, &Arrangement::componentRemoved, this, &ArrangementViewModel::onComponentRemoved);
            disconnect(a, &Arrangement::componentReplaced, this, &ArrangementViewModel::onComponentReplaced);
            disconnect(a, &Arrangement::componentMoved, this, &ArrangementViewModel::onComponentMoved);
        }
        
        Arrangement* arr = m_deck->arrangement(m_deck->activeArrangementName());
        if (arr) {
            connect(arr, &Arrangement::sequenceChanged, this, &ArrangementViewModel::onSequenceChanged);
            connect(arr, &Arrangement::componentInserted, this, &ArrangementViewModel::onComponentInserted);
            connect(arr, &Arrangement::componentRemoved, this, &ArrangementViewModel::onComponentRemoved);
            connect(arr, &Arrangement::componentReplaced, this, &ArrangementViewModel::onComponentReplaced);
            connect(arr, &Arrangement::componentMoved, this, &ArrangementViewModel::onComponentMoved);
        }
    }
    reloadSequence();
}

void ArrangementViewModel::onSequenceChanged() {
    reloadSequence();
}

void ArrangementViewModel::reloadSequence() {
    QStringList newSequence;
    if (m_deck) {
        Arrangement* arr = m_deck->arrangement(m_deck->activeArrangementName());
        if (arr) {
            newSequence = arr->sequence();
        } else {
            // Fallback to all components if no arrangement
            newSequence = m_deck->componentNames();
        }
    }
    
    if (m_sequence == newSequence) {
        return; // Nothing changed, prevent UI thrashing
    }

    beginResetModel();
    m_sequence = newSequence;
    m_activeComponentIndex = m_sequence.isEmpty() ? -1 : 0;
    endResetModel();
}

int ArrangementViewModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(m_sequence.size());
}

QVariant ArrangementViewModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_sequence.size()) return {};

    switch (role) {
        case ComponentNameRole: return m_sequence.at(index.row());
        case IsActiveRole: return index.row() == m_activeComponentIndex;
        default: return {};
    }
}

QHash<int, QByteArray> ArrangementViewModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[ComponentNameRole] = "componentName";
    roles[IsActiveRole] = "isActive";
    return roles;
}

void ArrangementViewModel::setActiveComponentIndex(int index) {
    if (index == m_activeComponentIndex || index < 0 || index >= m_sequence.size()) return;
    
    int oldIndex = m_activeComponentIndex;
    m_activeComponentIndex = index;
    
    emit dataChanged(this->index(oldIndex), this->index(oldIndex), {IsActiveRole});
    emit dataChanged(this->index(m_activeComponentIndex), this->index(m_activeComponentIndex), {IsActiveRole});
}

void ArrangementViewModel::appendComponent(const QString& name) {
    if (!m_deck) return;
    Arrangement* arr = m_deck->arrangement(m_deck->activeArrangementName());
    if (arr) {
        arr->appendComponent(name);
    }
}

void ArrangementViewModel::insertComponent(int index, const QString& name) {
    if (!m_deck || index < 0 || index > m_sequence.size()) return;
    Arrangement* arr = m_deck->arrangement(m_deck->activeArrangementName());
    if (arr) {
        arr->insertComponentAt(index, name);
    }
}

void ArrangementViewModel::removeComponent(int index) {
    if (!m_deck || index < 0 || index >= m_sequence.size()) return;
    Arrangement* arr = m_deck->arrangement(m_deck->activeArrangementName());
    if (arr) {
        arr->removeComponentAt(index);
    }
}

void ArrangementViewModel::moveComponent(int from, int to) {
    if (!m_deck || from < 0 || from >= m_sequence.size() || to < 0 || to > m_sequence.size()) return;
    if (from == to) return;
    Arrangement* arr = m_deck->arrangement(m_deck->activeArrangementName());
    if (arr) {
        arr->moveComponentAt(from, to);
    }
}

void ArrangementViewModel::onComponentInserted(int index, const QString& name) {
    beginInsertRows(QModelIndex(), index, index);
    m_sequence.insert(index, name);
    endInsertRows();
}

void ArrangementViewModel::onComponentRemoved(int index, const QString& /*name*/) {
    beginRemoveRows(QModelIndex(), index, index);
    m_sequence.removeAt(index);
    endRemoveRows();
}

void ArrangementViewModel::onComponentReplaced(int index, const QString& /*oldName*/, const QString& newName) {
    if (index >= 0 && index < m_sequence.size()) {
        m_sequence[index] = newName;
        QModelIndex idx = createIndex(index, 0);
        emit dataChanged(idx, idx, {ComponentNameRole});
    }
}

void ArrangementViewModel::onComponentMoved(int fromIndex, int toIndex) {
    int dest = (toIndex > fromIndex) ? toIndex + 1 : toIndex;
    beginMoveRows(QModelIndex(), fromIndex, fromIndex, QModelIndex(), dest);
    m_sequence.move(fromIndex, toIndex);
    endMoveRows();
}
