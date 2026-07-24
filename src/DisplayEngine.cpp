#include "../inc/DisplayEngine.h"
#include "../inc/ShowSerializer.h"
#include "../inc/ScreenModel.h"
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QGuiApplication>
#include <QScreen>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
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

void DisplayEngine::setGlobalBackgroundMedia(const QString& media) {
    if (m_globalBackgroundMedia != media) {
        m_globalBackgroundMedia = media;
        emit globalBackgroundMediaChanged();
        updateQmlContext();
    }
}

// ─── Construction / Destruction ─────────────────────────────────────────────

DisplayEngine::DisplayEngine(const DisplayOptions& opts, QObject* parent)
    : QObject(parent), m_opts(opts)
{
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

void DisplayEngine::setupView(QQuickView* view, const ScreenConfig& config, bool isStage) {
    QList<QScreen*> screens = QGuiApplication::screens();
    if (screens.isEmpty()) return;

    QScreen* targetScreen = nullptr;
    if (config.hardwareDisplayIndex >= 0 && config.hardwareDisplayIndex < screens.size()) {
        targetScreen = screens[config.hardwareDisplayIndex];
    }

    if (targetScreen) {
        view->setScreen(targetScreen);
        QRect screenGeom = targetScreen->geometry();
        
        if (config.isFullscreen) {
            view->setGeometry(screenGeom);
            view->setFlags(Qt::Window | Qt::FramelessWindowHint);
            view->showFullScreen();
        } else {
            view->setFlags(Qt::Window);
            // Center the window on the target screen
            int x = screenGeom.x() + (screenGeom.width() - config.width) / 2;
            int y = screenGeom.y() + (screenGeom.height() - config.height) / 2;
            view->setGeometry(x, y, config.width, config.height);
        }
    } else {
        // No hardware display assigned, open as window on primary screen
        view->setFlags(Qt::Window);
        view->resize(config.width, config.height);
    }

    if (isStage) {
        if (!config.isFullscreen && targetScreen) {
            view->setPosition(targetScreen->geometry().x() + 100, targetScreen->geometry().y() + 100);
        }
        view->setTitle("FOHMediaShow - Stage");
    } else {
        if (!config.isFullscreen && targetScreen) {
            view->setPosition(targetScreen->geometry().x() + 50, targetScreen->geometry().y() + 50);
        }
        view->setTitle("FOHMediaShow - Primary");
    }
    
    // Need access to layout properties from ShowViewModel, but since ShowViewModel is registered as a context property globally in main.cpp,
    // we can access it if we create the view properly, or we can just pass the engine.
    // Wait, QQuickView creates its own engine. We need to pass the same engine or just register context properties.
    // Inject context properties from the main engine so SlidePreview can access showModel
    // Inherit the global context but no need to manually copy properties
    if (m_qmlEngine) {
        // Properties are handled by AppContext singleton now.
    }
    
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    view->setSource(QUrl("qrc:/fohmedia/qml/LiveOutputWindow.qml"));
    
    if (view->status() == QQuickView::Error) {
        for (const auto& err : view->errors()) {
            std::cerr << "LiveOutputWindow QML Error: " << err.toString().toStdString() << std::endl;
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

        ScreenConfig audienceConfig;
        ScreenConfig stageConfig;
        
        if (m_screenModel) {
            for (const auto& cfg : m_screenModel->screens()) {
                if (cfg.name == "Audience") audienceConfig = cfg;
                else if (cfg.name == "Stage") stageConfig = cfg;
            }
        } else {
            audienceConfig.hardwareDisplayIndex = m_opts.primaryDisplayIndex;
            stageConfig.hardwareDisplayIndex = m_opts.stageDisplayIndex;
        }

        m_primaryView = new QQuickView(m_qmlEngine, nullptr);
        
        // Inherit root context properties so we can use `showModel` etc.
        setupView(m_primaryView, audienceConfig, false);
        if (!audienceConfig.isFullscreen) m_primaryView->show();

        if (stageConfig.hardwareDisplayIndex != -2) {
            m_stageView = new QQuickView(m_qmlEngine, nullptr);
            setupView(m_stageView, stageConfig, true);
            if (!stageConfig.isFullscreen) m_stageView->show();
        }

        m_running = true;
        updateQmlContext();
        
        emit started();
        emit isRunningChanged();
    } catch (const std::exception& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        stop();
    }
}

void DisplayEngine::stop() {
    const bool wasRunning = m_running;
    m_running = false;
    
    if (m_primaryView) {
        m_primaryView->close();
        m_primaryView->deleteLater();
    }
    if (m_stageView) {
        m_stageView->close();
        m_stageView->deleteLater();
    }

    if (wasRunning) {
        emit stopped();
        emit isRunningChanged();
    }
}

void DisplayEngine::updateQmlContext() {
    if (!m_running) return;
    
    SlideData currentSlide;
    QString nextSlideLines;
    
    if (!m_deck.isEmpty() && m_currentSlideIndex < static_cast<int>(m_deck.size())) {
        currentSlide = m_deck[m_currentSlideIndex];
    }
    
    if (!m_deck.isEmpty() && m_currentSlideIndex + 1 < static_cast<int>(m_deck.size())) {
        nextSlideLines = m_deck[m_currentSlideIndex + 1].lines.join("\n");
    }

    if (m_primaryView && m_primaryView->rootObject()) {
        QQuickItem* root = m_primaryView->rootObject();
        QString layoutFile = currentSlide.layouts.value("Audience", "Default.fohl");
        QString layout = layoutFile.replace(".fohl", "");
        root->setProperty("layoutName", layout);
        root->setProperty("slideText", currentSlide.lines.join("\n"));
        root->setProperty("nextSlideText", nextSlideLines);
        root->setProperty("transitionType", m_transitionType);
        root->setProperty("transitionDurationMs", m_transitionDurationMs);
        root->setProperty("globalBackgroundMedia", m_globalBackgroundMedia);
        root->setProperty("currentSlideIndex", static_cast<int>(m_currentSlideIndex));
    }
    
    if (m_stageView && m_stageView->rootObject()) {
        QQuickItem* root = m_stageView->rootObject();
        QString layoutFile = currentSlide.layouts.value("Stage", "StageDefault.fohl");
        QString layout = layoutFile.replace(".fohl", "");
        root->setProperty("layoutName", layout);
        // Stage displays next slide text implicitly or explicitly based on layout, but let's just pass main text for now
        // since the old engine drew both. If we want next text in stage, we should pass it.
        // Let's pass main text, and QML can handle it.
        root->setProperty("slideText", currentSlide.lines.join("\n"));
        root->setProperty("nextSlideText", nextSlideLines);
        root->setProperty("transitionType", m_transitionType);
        root->setProperty("transitionDurationMs", m_transitionDurationMs);
        root->setProperty("currentSlideIndex", static_cast<int>(m_currentSlideIndex));
    }
}
