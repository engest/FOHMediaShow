#pragma once

#include <QObject>
#include <QString>
#include <QList>

class SlideDeck;

class Show : public QObject {
    Q_OBJECT

public:
    explicit Show(QObject* parent = nullptr);
    explicit Show(const QString& name, QObject* parent = nullptr);
    ~Show() override = default;

    /// Show name
    QString name() const { return m_name; }
    void setName(const QString& name);

    /// Deck access
    int deckCount() const { return static_cast<int>(m_decks.size()); }
    SlideDeck* deckAt(int index) const;
    QList<SlideDeck*> decks() const { return m_decks; }

    /// Add a new empty deck at the end, returns it
    SlideDeck* appendDeck();

    /// Add an existing deck at the end
    void addDeck(SlideDeck* deck);

    /// Add a named deck at the end, returns it
    SlideDeck* appendDeck(const QString& name);

    /// Insert a deck at a specific position, returns it
    SlideDeck* insertDeck(int index, const QString& name = {});

    /// Remove and delete the deck at the given index
    void removeDeck(int index);

    /// Move a deck from one position to another
    void moveDeck(int fromIndex, int toIndex);

    /// Replace a deck at a specific position
    void replaceDeck(int index, SlideDeck* newDeck);

    QString sourceFile() const { return m_sourceFile; }
    void setSourceFile(const QString& file) { m_sourceFile = file; }

    QString defaultTransitionType() const { return m_defaultTransitionType; }
    void setDefaultTransitionType(const QString& type);

    int defaultTransitionDurationMs() const { return m_defaultTransitionDurationMs; }
    void setDefaultTransitionDurationMs(int duration);

signals:
    void nameChanged(const QString& name);
    void deckAboutToBeAdded(int index);
    void deckAdded(int index);
    void deckAboutToBeRemoved(int index);
    void deckRemoved(int index);
    void deckMoved(int fromIndex, int toIndex);
    void deckReplaced(int index);
    void defaultTransitionTypeChanged(const QString& type);
    void defaultTransitionDurationMsChanged(int duration);


private:
    QString m_name = QStringLiteral("Untitled Show");
    QList<SlideDeck*> m_decks;
    QString m_sourceFile;
    // --- Transition Settings ---
    QString m_defaultTransitionType = "Cut";
    int m_defaultTransitionDurationMs = 0;
};
