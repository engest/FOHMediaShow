#include <QtQml/qqmlregistration.h>
#pragma once

#include <QAbstractListModel>
#include <QPointer>

#include "Show.h"
class SlideDeck;

class ShowViewModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString showName READ showName WRITE setShowName NOTIFY showNameChanged)
    Q_PROPERTY(bool hasShow READ hasShow NOTIFY showNameChanged)
    Q_PROPERTY(int activeIndex READ activeIndex WRITE setActiveIndex NOTIFY activeIndexChanged)
    Q_PROPERTY(QString defaultTransitionType READ defaultTransitionType WRITE setDefaultTransitionType NOTIFY defaultTransitionTypeChanged)
    Q_PROPERTY(int defaultTransitionDurationMs READ defaultTransitionDurationMs WRITE setDefaultTransitionDurationMs NOTIFY defaultTransitionDurationMsChanged)

public:
    enum ShowRoles {
        NameRole = Qt::UserRole + 1,
        IsActiveRole,
        HasBackgroundVideoRole
    };

    explicit ShowViewModel(Show* show, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Show* show() const { return m_show; }
    void setShow(Show* show);
    
    QString showName() const;
    void setShowName(const QString& name);
    
    bool hasShow() const { return m_show != nullptr; }

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
    Q_INVOKABLE void addDeck(const QString& filePath);
    Q_INVOKABLE void removeDeck(int index);
    Q_INVOKABLE void moveDeck(int fromIndex, int toIndex);

public slots:
    void reloadDeck(const QString& sourceFile);

signals:
    void activeDeckChanged(SlideDeck* deck);
    void showNameChanged();
    void activeIndexChanged();
    void defaultTransitionTypeChanged();
    void defaultTransitionDurationMsChanged();

private slots:
    void onDeckAdded(int index);
    void onDeckRemoved(int index);
    void onDeckMoved(int fromIndex, int toIndex);
    void onDeckReplaced(int index);
    void autoSaveShow();

private:
    QPointer<Show> m_show;
    int m_activeIndex = -1;
};
