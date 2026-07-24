#include <QtQml/qqmlregistration.h>
#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QPointer>
#include "Slide.h"

#include "SlideDeck.h"

class SlideDeckViewModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(SlideDeck* deck READ deck NOTIFY deckChanged)
    Q_PROPERTY(QStringList availableComponentNames READ availableComponentNames NOTIFY availableComponentNamesChanged)

    Q_PROPERTY(int selectedSlideIndex READ selectedSlideIndex WRITE setSelectedSlideIndex NOTIFY selectedSlideChanged)
    Q_PROPERTY(QString selectedSlideText READ selectedSlideText NOTIFY selectedSlideChanged)
    Q_PROPERTY(QString selectedNextSlideText READ selectedNextSlideText NOTIFY selectedSlideChanged)

public:
    enum SlideRoles {
        SlideTextRole = Qt::UserRole + 1,
        ComponentNameRole,
        IsFirstRole,
        LayoutsRole,
        NextSlideTextRole
    };
    explicit SlideDeckViewModel(SlideDeck* deck, QObject* parent = nullptr);
    ~SlideDeckViewModel() override;

    SlideDeck* deck() const { return m_deck; }

public slots:
    void setDeck(SlideDeck* deck);

    // ─── QAbstractListModel overrides ───
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // ─── Render Layer: Active Slides ───
    int slideCount() const { return static_cast<int>(m_slides.size()); }
    Slide* slideAt(int index) const;
    QList<Slide*> slides() const { return m_slides; }

    Slide* appendSlide(const QString& text = {}, bool isfirst = true);
    Slide* insertSlide(int index, const QString& text = {});
    void removeSlide(int index);
    void moveSlide(int fromIndex, int toIndex);

    /// Returns the first slide index for a given component name
    Q_INVOKABLE int firstSlideIndexOfComponent(const QString& componentName) const;
    Q_INVOKABLE int firstSlideIndexOfArrangementIndex(int arrIndex) const;

    Q_INVOKABLE void saveDeck();
    
    Q_INVOKABLE void setSlideText(int index, const QString& text);
    Q_INVOKABLE void saveAllEdits();
    Q_INVOKABLE QString getLayoutForScreen(int index, const QString& screenName) const;
    Q_INVOKABLE void setLayoutForScreen(int index, const QString& screenName, const QString& layoutFile);
    Q_INVOKABLE QJsonObject getLayoutsForSlide(int index) const;
    Q_INVOKABLE void insertBlankSlideAfter(int index);
    Q_INVOKABLE void removeSlideAndSave(int index);
    Q_INVOKABLE void splitComponentGroup(int slideIndex, const QString& newComponent);
    
    QStringList availableComponentNames() const;

    /// Converts current slides to raw SlideData for the DisplayEngine
    QList<SlideData> toSlideDataList() const;

    Q_INVOKABLE QVariantMap getSlideDataForPreview(int index) const;

    int selectedSlideIndex() const { return m_selectedSlideIndex; }
    void setSelectedSlideIndex(int index);
    
    QString selectedSlideText() const;
    QString selectedNextSlideText() const;

    /// Rebuilds the m_slides list dynamically based on the active arrangement
    void buildActiveSlides();
    void flushSlidesToComponents(int lastEditedSlideIndex = -1);

signals:
    void deckChanged();
    void slideAdded(int index);
    void slideRemoved(int index);
    void slideMoved(int fromIndex, int toIndex);
    void availableComponentNamesChanged();
    void slidesRebuilt(); // Emitted when buildActiveSlides finishes
    void selectedSlideChanged();

private slots:
    void onActiveArrangementChanged();
    void onArrangementsChanged();
    void onComponentInserted(int index, const QString& name);
    void onComponentRemoved(int index, const QString& name);
    void onComponentMoved(int fromIndex, int toIndex);

private:
    void clearRenderedSlides();
    int calculateSlideOffsetForArrangementIndex(int arrIndex) const;
    void updateArrangementIndices(int startingArrIndex);
    void emitSelectedSlideChanged();
    
    SlideDeck* m_deck;
    QPointer<class Arrangement> m_activeArr;
    QList<Slide*> m_slides;
    bool m_isFlushing = false;
    int m_selectedSlideIndex = 0;
};
