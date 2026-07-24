#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>
#include <QUrl>

#include "ShowViewModel.h"
#include "SlideDeckViewModel.h"
#include "ArrangementViewModel.h"
#include "LyricsEditorViewModel.h"
#include "LayoutEditorViewModel.h"
#include "DisplayEngine.h"
#include "MediaViewModel.h"
#include "TimerManager.h"
#include "SettingsManager.h"
#include "ScreenModel.h"

class QQmlEngine;

class AppContext : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(ShowViewModel* showModel READ showModel CONSTANT)
    Q_PROPERTY(SlideDeckViewModel* slideDeckModel READ slideDeckModel CONSTANT)
    Q_PROPERTY(ArrangementViewModel* arrangementModel READ arrangementModel CONSTANT)
    Q_PROPERTY(LyricsEditorViewModel* lyricsModel READ lyricsModel CONSTANT)
    Q_PROPERTY(LayoutEditorViewModel* layoutEditorModel READ layoutEditorModel CONSTANT)
    Q_PROPERTY(ScreenModel* screenModel READ screenModel CONSTANT)
    Q_PROPERTY(DisplayEngine* displayEngine READ displayEngine CONSTANT)
    Q_PROPERTY(TimerManager* timerManager READ timerManager CONSTANT)
    Q_PROPERTY(SettingsManager* settingsManager READ settingsManager CONSTANT)
    Q_PROPERTY(MediaViewModel* imageMediaModel READ imageMediaModel CONSTANT)
    Q_PROPERTY(MediaViewModel* videoMediaModel READ videoMediaModel CONSTANT)

public:
    // Requires QQmlEngine pointer so DisplayEngine can reuse the main QML engine
    // and avoid the "Singleton registered by registerSingletonInstance must only be accessed from one engine" error.
    explicit AppContext(QQmlEngine* engine = nullptr, QObject* parent = nullptr);
    ~AppContext() override = default;

    ShowViewModel* showModel() const { return m_showModel; }
    SlideDeckViewModel* slideDeckModel() const { return m_slideDeckModel; }
    ArrangementViewModel* arrangementModel() const { return m_arrangementModel; }
    LyricsEditorViewModel* lyricsModel() const { return m_lyricsModel; }
    LayoutEditorViewModel* layoutEditorModel() const { return m_layoutEditorModel; }
    ScreenModel* screenModel() const { return m_screenModel; }
    DisplayEngine* displayEngine() const { return m_displayEngine; }
    TimerManager* timerManager() const { return m_timerManager; }
    SettingsManager* settingsManager() const { return m_settingsManager; }
    MediaViewModel* imageMediaModel() const { return m_imageMediaModel; }
    MediaViewModel* videoMediaModel() const { return m_videoMediaModel; }

    Q_INVOKABLE QUrl getLocalFileUrl(const QString& path) const {
        return path.isEmpty() ? QUrl() : QUrl::fromLocalFile(path);
    }

private:
    ShowViewModel* m_showModel = nullptr;
    SlideDeckViewModel* m_slideDeckModel = nullptr;
    ArrangementViewModel* m_arrangementModel = nullptr;
    LyricsEditorViewModel* m_lyricsModel = nullptr;
    LayoutEditorViewModel* m_layoutEditorModel = nullptr;
    ScreenModel* m_screenModel = nullptr;
    DisplayEngine* m_displayEngine = nullptr;
    TimerManager* m_timerManager = nullptr;
    SettingsManager* m_settingsManager = nullptr;
    MediaViewModel* m_imageMediaModel = nullptr;
    MediaViewModel* m_videoMediaModel = nullptr;
};
