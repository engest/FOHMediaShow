#include "../inc/Arrangement.h"

Arrangement::Arrangement(QObject* parent) : QObject(parent) {}

Arrangement::Arrangement(const QString& name, QObject* parent)
    : QObject(parent), m_name(name) {}

void Arrangement::setName(const QString& name) {
    if (m_name != name) {
        m_name = name;
        emit nameChanged(m_name);
    }
}

void Arrangement::setSequence(const QStringList& sequence) {
    if (m_sequence != sequence) {
        m_sequence = sequence;
        emit sequenceChanged();
    }
}

void Arrangement::appendComponent(const QString& componentName) {
    if (!componentName.isEmpty()) {
        m_sequence.append(componentName);
        emit componentInserted(static_cast<int>(m_sequence.size()) - 1, componentName);
    }
}

void Arrangement::insertComponentAt(int index, const QString& componentName) {
    if (!componentName.isEmpty()) {
        int insertPos = qBound(0, index, static_cast<int>(m_sequence.size()));
        m_sequence.insert(insertPos, componentName);
        emit componentInserted(insertPos, componentName);
    }
}

void Arrangement::removeComponentAt(int index) {
    if (index >= 0 && index < m_sequence.size()) {
        QString removedComponent = m_sequence.at(index);
        m_sequence.removeAt(index);
        emit componentRemoved(index, removedComponent);
    }
}

void Arrangement::replaceComponentAt(int index, const QString& newComponentName) {
    if (index >= 0 && index < m_sequence.size() && !newComponentName.isEmpty()) {
        QString oldName = m_sequence.at(index);
        if (oldName != newComponentName) {
            m_sequence[index] = newComponentName;
            emit componentReplaced(index, oldName, newComponentName);
        }
    }
}

void Arrangement::moveComponentAt(int from, int to) {
    if (from < 0 || from >= m_sequence.size() || to < 0 || to > m_sequence.size()) return;
    if (from == to) return;
    
    m_sequence.move(from, to);
    emit componentMoved(from, to);
}

void Arrangement::clear() {
    if (!m_sequence.isEmpty()) {
        m_sequence.clear();
        emit sequenceChanged();
    }
}
