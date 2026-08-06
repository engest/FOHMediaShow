#include "AppContext.h"
#include <QQuickWindow>
#include <QQmlEngine>
#include <QRegularExpression>

QColor AppContext::getComponentColor(const QString& name, const QColor& fallbackColor) const {
    if (name.isEmpty()) return fallbackColor;
    
    QString trimmedName = name.trimmed();
    static const QRegularExpression re("^([a-zA-Z\\-]+)\\s*(\\d*)$");
    QRegularExpressionMatch match = re.match(trimmedName);
    
    QString baseType = trimmedName.toLower();
    int level = 0;
    
    if (match.hasMatch()) {
        baseType = match.captured(1).toLower();
        if (!match.captured(2).isEmpty()) {
            level = match.captured(2).toInt();
        }
    }
    
    QColor baseColor = fallbackColor;
    if (baseType == "intro") baseColor = AppContext::settingsManager()->introComponentColor();
    else if (baseType == "verse") baseColor = AppContext::settingsManager()->verseComponentColor();
    else if (baseType == "prechorus" || baseType == "pre-chorus") baseColor = AppContext::settingsManager()->prechorusComponentColor();
    else if (baseType == "chorus") baseColor = AppContext::settingsManager()->chorusComponentColor();
    else if (baseType == "bridge") baseColor = AppContext::settingsManager()->bridgeComponentColor();
    else if (baseType == "tag") baseColor = AppContext::settingsManager()->tagComponentColor();
    else if (baseType == "interlude") baseColor = AppContext::settingsManager()->interludeComponentColor();
    else if (baseType == "refrain") baseColor = AppContext::settingsManager()->refrainComponentColor();
    else if (baseType == "instrumental") baseColor = AppContext::settingsManager()->instrumentalComponentColor();
    else if (baseType == "vamp") baseColor = AppContext::settingsManager()->vampComponentColor();
    else if (baseType == "turnaround") baseColor = AppContext::settingsManager()->turnaroundComponentColor();
    else if (baseType == "passthrough") baseColor = AppContext::settingsManager()->passthroughComponentColor();
    else if (baseType == "ending") baseColor = AppContext::settingsManager()->endingComponentColor();
    else if (baseType == "outro") baseColor = AppContext::settingsManager()->outroComponentColor();
    else if (baseType == "blank") baseColor = AppContext::settingsManager()->blankComponentColor();

    if (level > 0) {
        int darkenFactor = 100 + (std::min(level, 4) * 50);
        return baseColor.darker(darkenFactor);
    }
    
    return baseColor;
}

AppContext::AppContext(QQmlEngine* engine, QObject* parent) : QObject(parent) {
    Show* defaultShow = nullptr;
    m_showModel = new ShowViewModel(defaultShow, this);
    m_slideDeckModel = new SlideDeckViewModel(m_showModel->activeDeck(), this);
    m_arrangementModel = new ArrangementViewModel(m_showModel->activeDeck(), this);
    m_lyricsModel = new LyricsEditorViewModel(this);
    m_layoutEditorModel = new LayoutEditorViewModel(this);
    m_timerManager = new TimerManager(this);
    m_settingsManager = new SettingsManager(this);
    m_settingsManager->loadSettings();
    
    m_screenModel = new ScreenModel(this);
    QJsonArray screensJson = m_settingsManager->screensJson();
    QList<ScreenConfig> screens;
    for (const QJsonValue& val : screensJson) {
        screens.append(ScreenConfig::fromJson(val.toObject()));
    }
    if (screens.isEmpty()) {
        ScreenConfig aud;
        aud.name = "Audience";
        aud.isLocked = true;
        ScreenConfig stage;
        stage.name = "Stage";
        stage.isLocked = true;
        screens << aud << stage;
    }
    m_screenModel->setScreens(screens);
    QObject::connect(m_screenModel, &ScreenModel::screensChanged, [this]() {
        QJsonArray arr;
        for (const auto& screen : m_screenModel->screens()) {
            arr.append(screen.toJson());
        }
        m_settingsManager->setScreensJson(arr);
    });
    
    
    m_timerManager->loadFromJson(m_settingsManager->getTimersJson());
    QObject::connect(m_timerManager, &TimerManager::timersChanged, [this]() {
        m_settingsManager->setTimersJson(m_timerManager->saveToJson());
    });
    
    m_imageMediaModel = new MediaViewModel(this);
    m_imageMediaModel->setMediaTypeFilter("images");
    
    m_videoMediaModel = new MediaViewModel(this);
    m_videoMediaModel->setMediaTypeFilter("videos");

    DisplayOptions opts;
    opts.screenWidth = 1920;
    opts.screenHeight = 1080;
    opts.audienceDelayMs = m_settingsManager->practiceDelayMs();
    m_displayEngine = new DisplayEngine(opts, this);
    m_displayEngine->setQmlEngine(engine);
    m_displayEngine->setScreenModel(m_screenModel);
    m_slideDeckModel->setScreenModel(m_screenModel);

    QObject::connect(m_settingsManager, &SettingsManager::practiceDelayMsChanged, [this]() {
        if (m_displayEngine) {
            m_displayEngine->setAudienceDelayMs(m_settingsManager->practiceDelayMs());
        }
    });

    qmlRegisterSingletonInstance("com.company.TimerManager", 1, 0, "TimerManager", m_timerManager);
    qmlRegisterSingletonInstance("com.company.SettingsManager", 1, 0, "SettingsManager", m_settingsManager);

    // Lyrics Editor View Model wiring
    QObject::connect(m_lyricsModel, &LyricsEditorViewModel::deckSaved, m_showModel, &ShowViewModel::reloadDeck);

    // Show View Model active deck -> SlideDeck/Arrangement Model wiring
    QObject::connect(m_showModel, &ShowViewModel::activeDeckChanged, m_slideDeckModel, &SlideDeckViewModel::setDeck);
    QObject::connect(m_showModel, &ShowViewModel::activeDeckChanged, m_arrangementModel, &ArrangementViewModel::setDeck);
    QObject::connect(m_showModel, &ShowViewModel::activeDeckChanged, [this]() {
        if (m_showModel->activeDeck()) {
            QObject::connect(m_showModel->activeDeck(), &SlideDeck::globalBackgroundMediaChanged, m_displayEngine, [this]() {
                if (m_showModel->activeDeck()) {
                    m_displayEngine->setGlobalBackgroundMedia(m_showModel->activeDeck()->globalBackgroundMedia());
                }
            });
            // Initial sync on active deck change
            m_displayEngine->setGlobalBackgroundMedia(m_showModel->activeDeck()->globalBackgroundMedia());
        } else {
            m_displayEngine->setGlobalBackgroundMedia("");
        }
    });

    QObject::connect(m_slideDeckModel, &SlideDeckViewModel::slidesRebuilt, [this]() {
        if (m_showModel->activeDeck()) {
            m_displayEngine->setGlobalBackgroundMedia(m_showModel->activeDeck()->globalBackgroundMedia());
        } else {
            m_displayEngine->setGlobalBackgroundMedia("");
        }
        m_displayEngine->updateSlidesContent(m_slideDeckModel->toSlideDataList(), m_slideDeckModel->selectedSlideIndex());
    });

    QObject::connect(m_slideDeckModel, &SlideDeckViewModel::slidesUpdated, [this]() {
        m_displayEngine->updateSlidesContent(m_slideDeckModel->toSlideDataList(), m_slideDeckModel->selectedSlideIndex());
    });

    // Sync transition settings from ShowViewModel to DisplayEngine
    QObject::connect(m_showModel, &ShowViewModel::defaultTransitionTypeChanged, m_displayEngine, [this]() {
        m_displayEngine->setTransitionType(m_showModel->defaultTransitionType());
    });
    QObject::connect(m_showModel, &ShowViewModel::defaultTransitionDurationMsChanged, m_displayEngine, [this]() {
        m_displayEngine->setTransitionDurationMs(m_showModel->defaultTransitionDurationMs());
    });

    // Set initial transition values
    m_displayEngine->setTransitionType(m_showModel->defaultTransitionType());
    m_displayEngine->setTransitionDurationMs(m_showModel->defaultTransitionDurationMs());

    // Push initial slides
    m_displayEngine->setSlidesContent(m_slideDeckModel->toSlideDataList());
    if (m_showModel->activeDeck()) {
        m_displayEngine->setGlobalBackgroundMedia(m_showModel->activeDeck()->globalBackgroundMedia());
    }

    // Connect layout edits to clear the DisplayEngine cache
    QObject::connect(m_layoutEditorModel, &LayoutEditorViewModel::activeLayoutChanged, m_displayEngine, &DisplayEngine::clearLayoutCache);
}
