#include <QtQml/qqmlregistration.h>
#pragma once

#include <QAbstractListModel>
#include <QPointer>
#include <QList>

#include "Show.h"
class SlideDeck;

class ShowViewModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString showName READ showName WRITE setShowName NOTIFY showNameChanged)
    Q_PROPERTY(bool hasShow READ hasShow NOTIFY showNameChanged)
    Q_PROPERTY(QList<QObject*> shows READ showsList NOTIFY showsChanged)
    Q_PROPERTY(QVariantList loadedShows READ loadedShows NOTIFY loadedShowsChanged)
    Q_PROPERTY(int activeIndex READ activeIndex WRITE setActiveIndex NOTIFY activeIndexChanged)
    Q_PROPERTY(QString showProgressText READ showProgressText NOTIFY showProgressTextChanged)
    Q_PROPERTY(QString defaultTransitionType READ defaultTransitionType WRITE setDefaultTransitionType NOTIFY defaultTransitionTypeChanged)
    Q_PROPERTY(int defaultTransitionDurationMs READ defaultTransitionDurationMs WRITE setDefaultTransitionDurationMs NOTIFY defaultTransitionDurationMsChanged)

public:
    enum ShowRoles {
        NameRole = Qt::UserRole + 1,
        IsActiveRole,
        HasBackgroundVideoRole,
        ShowNameRole,
        ShowIndexRole,
        FlatIndexRole
    };

    explicit ShowViewModel(Show* show, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Show* activeShow() const;
    Show* showAtFlatIndex(int flatIndex, int* outDeckIndex = nullptr) const;

    QList<QObject*> showsList() const;
    QVariantList loadedShows() const;
    int flatIndexForShowDeck(Show* show, int deckIndex) const;

    QString showName() const;
    void setShowName(const QString& name);
    QString showProgressText() const;
    
    bool hasShow() const { return !m_shows.isEmpty(); }

    int activeIndex() const { return m_activeIndex; }
    SlideDeck* activeDeck() const;

    QString defaultTransitionType() const;
    void setDefaultTransitionType(const QString& type);

    int defaultTransitionDurationMs() const;
    void setDefaultTransitionDurationMs(int duration);

public slots:
    void setActiveIndex(int index);
    
    Q_INVOKABLE void loadShow(const QString& filePath);
    Q_INVOKABLE void importShow(const QString& filePath);
    Q_INVOKABLE QVariantMap getLayoutProperties(const QString& layoutName) const;
    Q_INVOKABLE QVariantList getAvailableShows() const;
    Q_INVOKABLE QVariantList getAvailableDecks() const;
    Q_INVOKABLE void newShow(const QString& title);
    Q_INVOKABLE QVariantList getShowsWithDecks() const;
    Q_INVOKABLE void removeShow(const QString& filePath);
    Q_INVOKABLE void unloadShow(const QString& showName);
    Q_INVOKABLE bool isSameShow(int index1, int index2) const;
    Q_INVOKABLE bool isFirstDeckOfShow(int flatIndex) const;
    Q_INVOKABLE void renameShow(const QString& oldName, const QString& newName);
    Q_INVOKABLE void addDeck(const QString& filePath);
    Q_INVOKABLE void removeDeck(int index);
    Q_INVOKABLE void moveDeck(int fromIndex, int toIndex);
    Q_INVOKABLE void reloadDeck(const QString& sourceFile);

signals:
    void activeDeckChanged(SlideDeck* deck);
    void showNameChanged();
    void showsChanged();
    void loadedShowsChanged();
    void activeIndexChanged();
    void defaultTransitionTypeChanged();
    void defaultTransitionDurationMsChanged();
    void showProgressTextChanged();

private slots:
    void onDeckAdded(int index);
    void onDeckRemoved(int index);
    void onDeckAboutToBeMoved(int fromIndex, int toIndex);
    void onDeckMoved(int fromIndex, int toIndex);
    void onDeckReplaced(int index);
    void onShowNameChanged();
    void autoSaveShow();
    void autoSaveAllShows();

private:
    void connectShow(Show* show);
    void disconnectShow(Show* show);
    
    QList<QPointer<Show>> m_shows;
    int m_activeIndex = -1;
};
