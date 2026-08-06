#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QList>

class SlideDeck;

class Show : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(int deckCount READ deckCount NOTIFY deckCountChanged)
    Q_PROPERTY(QString sourceFile READ sourceFile WRITE setSourceFile)
    Q_PROPERTY(QString defaultTransitionType READ defaultTransitionType WRITE setDefaultTransitionType NOTIFY defaultTransitionTypeChanged)
    Q_PROPERTY(int defaultTransitionDurationMs READ defaultTransitionDurationMs WRITE setDefaultTransitionDurationMs NOTIFY defaultTransitionDurationMsChanged)

public:
    enum DeckRoles {
        DeckNameRole = Qt::UserRole + 1,
        IsActiveRole,
        HasBackgroundVideoRole,
        FlatIndexRole,
        DeckIndexRole,
        ShowNameRole
    };

    explicit Show(QObject* parent = nullptr);
    explicit Show(const QString& name, QObject* parent = nullptr);
    ~Show() override = default;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

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
    Q_INVOKABLE void moveDeck(int fromIndex, int toIndex);

    /// Replace a deck at a specific position
    void replaceDeck(int index, SlideDeck* newDeck);

    void notifyActiveChanged();

    QString sourceFile() const { return m_sourceFile; }
    void setSourceFile(const QString& file) { m_sourceFile = file; }

    QString defaultTransitionType() const { return m_defaultTransitionType; }
    void setDefaultTransitionType(const QString& type);

    int defaultTransitionDurationMs() const { return m_defaultTransitionDurationMs; }
    void setDefaultTransitionDurationMs(int duration);

signals:
    void nameChanged(const QString& name);
    void deckCountChanged();
    void deckAboutToBeAdded(int index);
    void deckAdded(int index);
    void deckAboutToBeRemoved(int index);
    void deckRemoved(int index);
    void deckAboutToBeMoved(int fromIndex, int toIndex);
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
