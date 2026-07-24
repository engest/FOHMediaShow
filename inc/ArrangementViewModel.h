#include <QtQml/qqmlregistration.h>
#pragma once

#include <QAbstractListModel>
#include <QPointer>

class SlideDeck;

class ArrangementViewModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

public:
    enum ArrangementRoles {
        ComponentNameRole = Qt::UserRole + 1,
        IsActiveRole // For highlighting the current part playing
    };

    explicit ArrangementViewModel(SlideDeck* deck = nullptr, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setDeck(SlideDeck* deck);
    SlideDeck* deck() const { return m_deck; }

signals:
    void sequenceChanged();

public slots:
    void setActiveComponentIndex(int index);

    void appendComponent(const QString& name);
    void insertComponent(int index, const QString& name);
    void removeComponent(int index);
    void moveComponent(int from, int to);

private slots:
    void onActiveArrangementChanged();
    void onSequenceChanged();
    void onComponentInserted(int index, const QString& name);
    void onComponentRemoved(int index, const QString& name);
    void onComponentReplaced(int index, const QString& oldName, const QString& newName);
    void onComponentMoved(int fromIndex, int toIndex);

private:
    void reloadSequence();

    QPointer<SlideDeck> m_deck;
    QStringList m_sequence;
    int m_activeComponentIndex = -1;
};
