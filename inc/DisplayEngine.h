#include <QtQml/qqmlregistration.h>
#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QPointer>
#include <QQuickView>
#include <memory>

#include "../inc/SlideDeck.h"
#include "../inc/SlideLayout.h"
#include <QJsonArray>
class ScreenModel;
struct ScreenConfig;

struct DisplayOptions {
    int screenWidth = 1920;
    int screenHeight = 1080;
    uint32_t slideLatency = 0;
    int primaryDisplayIndex = -1; // -1 = use auto-detect logic
    int stageDisplayIndex   = -1; // -1 = use auto-detect logic, -2 = disabled
    int audienceDelayMs     = 273; // 273ms latency for main audience display (simulates SDI/switcher/LED wall)
};

class QQmlEngine;

class DisplayEngine : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool currentNextAllCaps READ currentNextAllCaps NOTIFY currentLayoutChanged)
    Q_PROPERTY(int currentNextAlignment READ currentNextAlignment NOTIFY currentLayoutChanged)

    Q_PROPERTY(QString currentLayoutName READ currentLayoutName NOTIFY currentSlideChanged)
    Q_PROPERTY(QString currentStageLayoutName READ currentStageLayoutName NOTIFY currentSlideChanged)
    Q_PROPERTY(QString currentSlideText READ currentSlideText NOTIFY currentSlideChanged)
    Q_PROPERTY(QString currentNextSlideText READ currentNextSlideText NOTIFY currentSlideChanged)

    Q_PROPERTY(int currentSlideNumber READ currentSlideNumber NOTIFY currentSlideChanged)
    Q_PROPERTY(int totalSlideCount READ totalSlideCount NOTIFY currentSlideChanged)
    Q_PROPERTY(QString currentComponentGroupName READ currentComponentGroupName NOTIFY currentSlideChanged)
    Q_PROPERTY(QString nextComponentGroupName READ nextComponentGroupName NOTIFY currentSlideChanged)
    Q_PROPERTY(int currentGroupSlideIndex READ currentGroupSlideIndex NOTIFY currentSlideChanged)
    Q_PROPERTY(int totalGroupSlideCount READ totalGroupSlideCount NOTIFY currentSlideChanged)
    Q_PROPERTY(QJsonArray minimapData READ minimapData NOTIFY currentSlideChanged)

    Q_PROPERTY(int audienceDelayMs READ audienceDelayMs WRITE setAudienceDelayMs NOTIFY audienceDelayMsChanged)
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)
    Q_PROPERTY(QString transitionType READ transitionType WRITE setTransitionType NOTIFY transitionTypeChanged)
    Q_PROPERTY(int transitionDurationMs READ transitionDurationMs WRITE setTransitionDurationMs NOTIFY transitionDurationMsChanged)
    Q_PROPERTY(QString globalBackgroundMedia READ globalBackgroundMedia WRITE setGlobalBackgroundMedia NOTIFY globalBackgroundMediaChanged)

public:
    enum class TransitionType { Cut, Fade, SlideLeft, SlideRight, SlideUp, SlideDown };
    Q_ENUM(TransitionType)

    explicit DisplayEngine(const DisplayOptions& opts, QObject* parent = nullptr);
    ~DisplayEngine() override;

    int audienceDelayMs() const { return m_audienceDelayMs; }
    void setAudienceDelayMs(int delay);

    bool currentNextAllCaps() const;
    int currentNextAlignment() const;

    QString currentLayoutName() const;
    QString currentStageLayoutName() const;
    QString currentSlideText() const;
    QString currentNextSlideText() const;

    int currentSlideNumber() const { return m_currentSlideIndex + 1; }
    int totalSlideCount() const { return static_cast<int>(m_deck.size()); }
    QString currentComponentGroupName() const;
    QString nextComponentGroupName() const;
    int currentGroupSlideIndex() const;
    int totalGroupSlideCount() const;
    QJsonArray minimapData() const;

    Q_INVOKABLE QJsonArray getLayoutTimers(const QString& layoutName) const;

    bool isRunning() const { return m_running; }
    uint32_t slideLatency() const { return m_opts.slideLatency; }
    
    QString transitionType() const;
    void setTransitionType(const QString& type);
    
    int transitionDurationMs() const { return m_transitionDurationMs; }
    void setTransitionDurationMs(int duration);

    QString globalBackgroundMedia() const { return m_globalBackgroundMedia; }
    void setGlobalBackgroundMedia(const QString& media);

    void setQmlEngine(QQmlEngine* engine) { m_qmlEngine = engine; }
    void setScreenModel(ScreenModel* model) { m_screenModel = model; }

public slots:
    void setPrimaryDisplayIndex(int index) { m_opts.primaryDisplayIndex = index; }
    void setStageDisplayIndex(int index)   { m_opts.stageDisplayIndex = index; }

    /// Create the Qt Quick windows and start rendering
    void start();

    /// Tear down the Qt Quick windows and stop rendering
    void stop();

    /// Replace the current slide deck with new content
    void setSlidesContent(const QList<SlideData>& slides);

    /// Update the current slide deck without resetting the index (or optionally setting a new slide index)
    void updateSlidesContent(const QList<SlideData>& slides, int newSlideIndex = -1);

    /// Clear the layout cache to force reloading from disk
    Q_INVOKABLE void clearLayoutCache();

    /// Navigation
    void advanceSlides(int count);
    void retreatSlides(int count);

    /// Jump directly to a specific slide
    void jumpToSlide(size_t index);

signals:
    void started();
    void stopped();
    void isRunningChanged();
    void slideIndexChanged(size_t index);
    void reachedEndOfDeck();
    void reachedBeginningOfDeck();
    void errorOccurred(const QString& errorMessage);
    void currentLayoutChanged();
    void currentSlideChanged();
    void transitionTypeChanged();
    void transitionDurationMsChanged();
    void globalBackgroundMediaChanged();
    void audienceDelayMsChanged();

private:
    struct ScreenViewInfo {
        QString screenName;
        QPointer<QQuickView> view;
    };

    void updateQmlContext(bool immediate = false);
    void applyAudienceUpdate();
    void setupView(QQuickView* view, const ScreenConfig& config, bool isStage, int screenIndex = 0);

    DisplayOptions m_opts;
    bool m_running = false;
    
    QList<SlideData> m_deck;
    int m_currentSlideIndex = 0;
    
    QString m_transitionType = "Cut";
    int m_transitionDurationMs = 0;
    QString m_globalBackgroundMedia;
    int m_audienceDelayMs = 273;
    uint64_t m_runGeneration = 0;
    QTimer* m_audienceTimer = nullptr;

    QList<ScreenViewInfo> m_screenViews;
    QQmlEngine* m_qmlEngine = nullptr;
    ScreenModel* m_screenModel = nullptr;
};
