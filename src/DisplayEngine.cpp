#include "../inc/DisplayEngine.h"
#include "../inc/ShowSerializer.h"
#include "../inc/ScreenModel.h"
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QGuiApplication>
#include <QScreen>
#include <QSurfaceFormat>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include "../inc/Library.h"
#include <iostream>

QString DisplayEngine::transitionType() const {
    return m_transitionType;
}

QString DisplayEngine::currentLayoutName() const {
    if (m_deck.isEmpty() || m_currentSlideIndex >= m_deck.size()) return "Default";
    QString layout = m_deck[m_currentSlideIndex].layouts.value("Audience", "Default.fohl");
    return layout.isEmpty() ? "Default" : layout.replace(".fohl", "");
}

QString DisplayEngine::currentStageLayoutName() const {
    if (m_deck.isEmpty() || m_currentSlideIndex >= m_deck.size()) return "StageDefault";
    QString layout = m_deck[m_currentSlideIndex].layouts.value("Stage", "StageDefault.fohl");
    return layout.isEmpty() ? "StageDefault" : layout.replace(".fohl", "");
}

QString DisplayEngine::currentSlideText() const {
    if (m_deck.isEmpty() || m_currentSlideIndex >= m_deck.size()) return "";
    return m_deck[m_currentSlideIndex].lines.join("\n");
}

QString DisplayEngine::currentNextSlideText() const {
    if (m_deck.isEmpty() || m_currentSlideIndex + 1 >= m_deck.size()) return "";
    return m_deck[m_currentSlideIndex + 1].lines.join("\n");
}

QString DisplayEngine::currentComponentGroupName() const {
    if (m_deck.isEmpty() || m_currentSlideIndex >= m_deck.size()) return "";
    return m_deck[m_currentSlideIndex].componentName;
}

QString DisplayEngine::nextComponentGroupName() const {
    if (m_deck.isEmpty() || m_currentSlideIndex + 1 >= m_deck.size()) return "";
    return m_deck[m_currentSlideIndex + 1].componentName;
}

int DisplayEngine::currentGroupSlideIndex() const {
    if (m_deck.isEmpty() || m_currentSlideIndex >= m_deck.size()) return 0;
    return m_deck[m_currentSlideIndex].groupSlideIndex + 1;
}

int DisplayEngine::totalGroupSlideCount() const {
    if (m_deck.isEmpty() || m_currentSlideIndex >= m_deck.size()) return 0;
    return m_deck[m_currentSlideIndex].groupSlideCount;
}

QJsonArray DisplayEngine::minimapData() const {
    QJsonArray arr;
    for (int i = 0; i < m_deck.size(); ++i) {
        QJsonObject obj;
        obj["type"] = m_deck[i].componentName;
        obj["isActive"] = (i == m_currentSlideIndex);
        arr.append(obj);
    }
    return arr;
}

QJsonArray DisplayEngine::getLayoutTimers(const QString& layoutName) const {
    if (layoutName.isEmpty()) return QJsonArray();
    QString layoutFile = layoutName + ".fohl";
    QString layoutPath = QDir(Library::layoutsDir()).filePath(layoutFile);
    
    QFile file(layoutPath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isObject()) {
            SlideLayout layout = SlideLayout::fromJson(doc.object());
            QJsonArray arr;
            for (const auto& timer : layout.timers) {
                arr.append(timer.toJson());
            }
            return arr;
        }
    }
    return QJsonArray();
}

bool DisplayEngine::currentNextAllCaps() const {
    // Determine the next allCaps state based on current stage layout
    if (m_currentSlideIndex >= 0 && m_currentSlideIndex < m_deck.size()) {
        const auto& slide = m_deck[m_currentSlideIndex];
        QString layoutFile = slide.layouts.value("Stage", "StageDefault.fohl");
        if (!layoutFile.endsWith(".fohl")) layoutFile += ".fohl";
        QString layoutPath = QDir(Library::layoutsDir()).filePath(layoutFile);
        
        QFile file(layoutPath);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            if (doc.isObject()) {
                SlideLayout layout = SlideLayout::fromJson(doc.object());
                return layout.nextAllCaps;
            }
        }
    }
    return false;
}

int DisplayEngine::currentNextAlignment() const {
    if (m_currentSlideIndex >= 0 && m_currentSlideIndex < m_deck.size()) {
        const auto& slide = m_deck[m_currentSlideIndex];
        QString layoutFile = slide.layouts.value("Stage", "StageDefault.fohl");
        if (!layoutFile.endsWith(".fohl")) layoutFile += ".fohl";
        QString layoutPath = QDir(Library::layoutsDir()).filePath(layoutFile);
        
        QFile file(layoutPath);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            if (doc.isObject()) {
                SlideLayout layout = SlideLayout::fromJson(doc.object());
                return static_cast<int>(layout.nextAlignment);
            }
        }
    }
    return Qt::AlignCenter;
}

void DisplayEngine::setTransitionType(const QString& type) {
    if (m_transitionType != type) {
        m_transitionType = type;
        emit transitionTypeChanged();
        updateQmlContext();
    }
}

void DisplayEngine::setTransitionDurationMs(int duration) {
    if (m_transitionDurationMs != duration) {
        m_transitionDurationMs = duration;
        emit transitionDurationMsChanged();
        updateQmlContext();
    }
}

void DisplayEngine::setAudienceDelayMs(int delay) {
    if (m_audienceDelayMs != delay) {
        m_audienceDelayMs = delay;
        emit audienceDelayMsChanged();
    }
}

void DisplayEngine::setGlobalBackgroundMedia(const QString& media) {
    if (m_globalBackgroundMedia != media) {
        m_globalBackgroundMedia = media;
        emit globalBackgroundMediaChanged();
        updateQmlContext();
    }
}

// ─── Construction / Destruction ─────────────────────────────────────────────

DisplayEngine::DisplayEngine(const DisplayOptions& opts, QObject* parent)
    : QObject(parent), m_opts(opts), m_audienceDelayMs(opts.audienceDelayMs)
{
    m_audienceTimer = new QTimer(this);
    m_audienceTimer->setSingleShot(true);
    connect(m_audienceTimer, &QTimer::timeout, this, &DisplayEngine::applyAudienceUpdate);
}

DisplayEngine::~DisplayEngine() {
    stop();
}

// ─── Slide Data Access ──────────────────────────────────────────────────────

void DisplayEngine::setSlidesContent(const QList<SlideData>& slides)
{
    m_deck = slides;
    m_currentSlideIndex = 0;
    updateQmlContext();
    emit slideIndexChanged(m_currentSlideIndex);
    emit currentSlideChanged();
}

void DisplayEngine::updateSlidesContent(const QList<SlideData>& slides, int newSlideIndex)
{
    m_deck = slides;
    if (newSlideIndex >= 0) {
        m_currentSlideIndex = qBound(0, newSlideIndex, qMax(0, static_cast<int>(m_deck.size()) - 1));
    } else {
        if (m_deck.isEmpty()) {
            m_currentSlideIndex = 0;
        } else if (m_currentSlideIndex >= m_deck.size()) {
            m_currentSlideIndex = static_cast<int>(m_deck.size()) - 1;
        }
    }
    updateQmlContext();
    emit slideIndexChanged(m_currentSlideIndex);
    emit currentSlideChanged();
}

void DisplayEngine::advanceSlides(int count) {
    if (m_deck.isEmpty()) {
        m_currentSlideIndex = 0;
        return;
    }

    int newIndex = m_currentSlideIndex + count;

    if (newIndex < 0) {
        emit reachedBeginningOfDeck();
        return;
    }

    if (newIndex >= m_deck.size()) {
        emit reachedEndOfDeck();
        return;
    }

    m_currentSlideIndex = newIndex;
    updateQmlContext();
    emit slideIndexChanged(m_currentSlideIndex);
    emit currentSlideChanged();
}

void DisplayEngine::retreatSlides(int count) {
    if (m_deck.isEmpty()) {
        m_currentSlideIndex = 0;
        return;
    }
    if (m_currentSlideIndex < count) {
        emit reachedBeginningOfDeck();
        return;
    } else {
        m_currentSlideIndex -= count;
    }
    updateQmlContext();
    emit slideIndexChanged(m_currentSlideIndex);
    emit currentSlideChanged();
}

void DisplayEngine::jumpToSlide(size_t index) {
    if (m_deck.isEmpty() || static_cast<qsizetype>(index) >= m_deck.size()) return;

    m_currentSlideIndex = static_cast<int>(index);
    updateQmlContext();
    emit slideIndexChanged(m_currentSlideIndex);
    emit currentSlideChanged();
}

void DisplayEngine::clearLayoutCache() {
    // Layout cache is no longer managed by DisplayEngine (handled by QML and view models)
}

// ─── Start / Stop ───────────────────────────────────────────────────────────

void DisplayEngine::setupView(QQuickView* view, const ScreenConfig& config, bool isStage, int screenIndex) {
    QList<QScreen*> screens = QGuiApplication::screens();
    if (screens.isEmpty()) return;

    // Request alpha buffer and transparent background for the QQuickView
    QSurfaceFormat fmt = view->format();
    fmt.setAlphaBufferSize(8);
    view->setFormat(fmt);
    view->setColor(QColor(0, 0, 0, 0));

    QScreen* targetScreen = nullptr;
    if (config.hardwareDisplayIndex >= 0 && config.hardwareDisplayIndex < screens.size()) {
        targetScreen = screens[config.hardwareDisplayIndex];
    }

    view->setFlags(Qt::Window);

    if (targetScreen) {
        view->setScreen(targetScreen);
        QRect screenGeom = targetScreen->geometry();
        
        if (config.isFullscreen) {
            view->setGeometry(screenGeom);
            view->showFullScreen();
        } else {
            // Stagger or center the window on the target screen
            int x = screenGeom.x() + (screenGeom.width() - config.width) / 2 + (screenIndex * 30);
            int y = screenGeom.y() + (screenGeom.height() - config.height) / 2 + (screenIndex * 30);
            view->setGeometry(x, y, config.width, config.height);
            view->show();
        }
    } else {
        // No hardware display assigned, open as window on primary screen
        QScreen* primary = QGuiApplication::primaryScreen();
        int px = primary ? primary->geometry().x() : 0;
        int py = primary ? primary->geometry().y() : 0;
        view->setGeometry(px + 50 + (screenIndex * 30), py + 50 + (screenIndex * 30), config.width, config.height);
        view->show();
    }

    view->setTitle("FOHMedia - " + config.name);
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    view->setSource(QUrl("qrc:/fohmedia/qml/LiveOutputWindow.qml"));
    
    if (view->status() == QQuickView::Error) {
        for (const auto& err : view->errors()) {
            std::cerr << "LiveOutputWindow QML Error (" << config.name.toStdString() << "): " << err.toString().toStdString() << std::endl;
        }
    }
    
    if (QQuickItem* root = view->rootObject()) {
        root->setProperty("isStage", isStage);
    }
}

void DisplayEngine::start() {
    if (m_running) return;

    try {
        QList<QScreen*> screens = QGuiApplication::screens();
        if (screens.isEmpty()) {
            throw std::runtime_error("No displays detected!");
        }

        for (auto& item : m_screenViews) {
            if (item.view) {
                item.view->close();
                item.view->deleteLater();
            }
        }
        m_screenViews.clear();

        QList<ScreenConfig> screenConfigs;
        if (m_screenModel && !m_screenModel->screens().isEmpty()) {
            screenConfigs = m_screenModel->screens();
        } else {
            ScreenConfig aud;
            aud.name = "Audience";
            aud.hardwareDisplayIndex = m_opts.primaryDisplayIndex;
            ScreenConfig stg;
            stg.name = "Stage";
            stg.hardwareDisplayIndex = m_opts.stageDisplayIndex;
            screenConfigs.append(aud);
            if (stg.hardwareDisplayIndex != -2) {
                screenConfigs.append(stg);
            }
        }

        for (int i = 0; i < screenConfigs.size(); ++i) {
            const ScreenConfig& cfg = screenConfigs[i];
            if (cfg.hardwareDisplayIndex == -2 || cfg.disabled) continue; // Disabled screen

            bool isStage = (cfg.name.compare("Stage", Qt::CaseInsensitive) == 0);

            QQuickView* view = new QQuickView(m_qmlEngine, nullptr);
            setupView(view, cfg, isStage, i);

            m_screenViews.append({cfg.name, view});
        }

        m_running = true;
        m_runGeneration++;
        updateQmlContext(true); // Initial display is immediate
        
        emit started();
        emit isRunningChanged();
    } catch (const std::exception& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        stop();
    }
}

void DisplayEngine::stop() {
    m_runGeneration++;
    if (m_audienceTimer) {
        m_audienceTimer->stop();
    }
    const bool wasRunning = m_running;
    m_running = false;
    
    for (auto& item : m_screenViews) {
        if (item.view) {
            item.view->close();
            item.view->deleteLater();
        }
    }
    m_screenViews.clear();

    if (wasRunning) {
        emit stopped();
        emit isRunningChanged();
    }
}

void DisplayEngine::applyAudienceUpdate() {
    if (!m_running) return;

    SlideData currentSlide;
    QString nextSlideLines;

    if (!m_deck.isEmpty() && m_currentSlideIndex < static_cast<int>(m_deck.size())) {
        currentSlide = m_deck[m_currentSlideIndex];
    }

    if (!m_deck.isEmpty() && m_currentSlideIndex + 1 < static_cast<int>(m_deck.size())) {
        nextSlideLines = m_deck[m_currentSlideIndex + 1].lines.join("\n");
    }

    const int slideIdx = m_currentSlideIndex;
    const QString transType = m_transitionType;
    const int transDur = m_transitionDurationMs;
    const QString bgMedia = m_globalBackgroundMedia;

    for (const auto& item : m_screenViews) {
        if (!item.view || !item.view->rootObject()) continue;

        const QString& sName = item.screenName;
        bool isStage = (sName.compare("Stage", Qt::CaseInsensitive) == 0);
        if (isStage) continue; // Stage is updated immediately in updateQmlContext

        QQuickItem* root = item.view->rootObject();
        QString defaultLayout = "Default.fohl";
        QString layoutFile = currentSlide.layouts.value(sName);
        if (layoutFile.isEmpty()) {
            if (QFile::exists(Library::layoutsDir() + "/" + sName + ".fohl")) {
                layoutFile = sName + ".fohl";
            } else if (sName.compare("Overlay", Qt::CaseInsensitive) == 0 && QFile::exists(Library::layoutsDir() + "/LyricsOverlay.fohl")) {
                layoutFile = "LyricsOverlay.fohl";
            } else {
                layoutFile = defaultLayout;
            }
        }
        QString layout = layoutFile;
        if (layout.endsWith(".fohl")) layout.chop(5);

        QString slideText = currentSlide.lines.join("\n");

        root->setProperty("layoutName", layout);
        root->setProperty("slideText", slideText);
        root->setProperty("nextSlideText", nextSlideLines);
        root->setProperty("transitionType", transType);
        root->setProperty("transitionDurationMs", transDur);
        root->setProperty("globalBackgroundMedia", bgMedia);
        root->setProperty("currentSlideIndex", slideIdx);
    }
}

void DisplayEngine::updateQmlContext(bool immediate) {
    if (!m_running) return;
    
    SlideData currentSlide;
    QString nextSlideLines;
    
    if (!m_deck.isEmpty() && m_currentSlideIndex < static_cast<int>(m_deck.size())) {
        currentSlide = m_deck[m_currentSlideIndex];
    }
    
    if (!m_deck.isEmpty() && m_currentSlideIndex + 1 < static_cast<int>(m_deck.size())) {
        nextSlideLines = m_deck[m_currentSlideIndex + 1].lines.join("\n");
    }

    const int slideIdx = m_currentSlideIndex;
    const QString transType = m_transitionType;
    const int transDur = m_transitionDurationMs;
    const QString bgMedia = m_globalBackgroundMedia;

    for (const auto& item : m_screenViews) {
        if (!item.view || !item.view->rootObject()) continue;

        QQuickItem* root = item.view->rootObject();
        const QString& sName = item.screenName;
        bool isStage = (sName.compare("Stage", Qt::CaseInsensitive) == 0);

        QString defaultLayout = isStage ? "StageDefault.fohl" : "Default.fohl";
        QString layoutFile = currentSlide.layouts.value(sName);
        if (layoutFile.isEmpty()) {
            if (QFile::exists(Library::layoutsDir() + "/" + sName + ".fohl")) {
                layoutFile = sName + ".fohl";
            } else if (sName.compare("Overlay", Qt::CaseInsensitive) == 0 && QFile::exists(Library::layoutsDir() + "/LyricsOverlay.fohl")) {
                layoutFile = "LyricsOverlay.fohl";
            } else {
                layoutFile = defaultLayout;
            }
        }
        QString layout = layoutFile;
        if (layout.endsWith(".fohl")) layout.chop(5);

        QString slideText = currentSlide.lines.join("\n");

        if (isStage || immediate || m_audienceDelayMs <= 0) {
            // Stage and non-delayed updates apply immediately (0 ms latency)
            root->setProperty("layoutName", layout);
            root->setProperty("slideText", slideText);
            root->setProperty("nextSlideText", nextSlideLines);
            root->setProperty("transitionType", transType);
            root->setProperty("transitionDurationMs", transDur);
            root->setProperty("globalBackgroundMedia", bgMedia);
            root->setProperty("currentSlideIndex", slideIdx);
        }
    }

    if (immediate || m_audienceDelayMs <= 0) {
        if (m_audienceTimer) m_audienceTimer->stop();
        applyAudienceUpdate();
    } else {
        bool audienceAlreadyMatches = true;
        for (const auto& item : m_screenViews) {
            if (!item.view || !item.view->rootObject()) continue;
            if (item.screenName.compare("Stage", Qt::CaseInsensitive) == 0) continue;
            QQuickItem* root = item.view->rootObject();
            QString curText = root->property("displayedSlideText").toString();
            QString curLayout = root->property("displayedLayoutName").toString();
            QString newText = currentSlide.lines.join("\n");
            QString layoutFile = currentSlide.layouts.value(item.screenName);
            if (layoutFile.isEmpty()) layoutFile = "Default.fohl";
            if (layoutFile.endsWith(".fohl")) layoutFile.chop(5);
            if (curText != newText || curLayout != layoutFile) {
                audienceAlreadyMatches = false;
                break;
            }
        }

        if (audienceAlreadyMatches) {
            if (m_audienceTimer) m_audienceTimer->stop();
            applyAudienceUpdate();
        } else {
            // Cancel any pending delayed audience update and restart with the latest target state
            if (m_audienceTimer) {
                m_audienceTimer->stop();
                m_audienceTimer->start(m_audienceDelayMs);
            }
        }
    }
}
