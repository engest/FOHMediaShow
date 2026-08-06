#include "SettingsManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QDebug>

SettingsManager::SettingsManager(QObject* parent) : QObject(parent) {
    applyDefaults();
}

SettingsManager::~SettingsManager() {
    saveSettings();
}

QString SettingsManager::getSettingsFilePath() const {
    // $HOME/.config/FOHMedia/settings.json
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(configPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dir.absoluteFilePath("settings.json");
}

void SettingsManager::applyDefaults() {
    m_globalTransition = 0;
    m_practiceDelayMs = 273;
    m_screensJson = QJsonArray();
    m_disableHwVideo = false;
    m_disableSplash = false;
    m_activeControllers.clear();
    m_timersJson = QJsonArray();
}

void SettingsManager::loadSettings() {
    QString filePath = getSettingsFilePath();
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open settings file for reading:" << filePath;
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse settings.json:" << parseError.errorString();
        return;
    }

    if (doc.isObject()) {
        QJsonObject root = doc.object();
        
        // Read timers first so any intermediate saveSettings calls don't overwrite them with empty array
        if (root.contains("timers") && root["timers"].isArray()) {
            m_timersJson = root["timers"].toArray();
        }

        if (root.contains("globalTransition")) {
            setGlobalTransition(root["globalTransition"].toInt());
        }
        if (root.contains("practiceDelayMs")) {
            setPracticeDelayMs(root["practiceDelayMs"].toInt());
        }
        if (root.contains("screens") && root["screens"].isArray()) {
            m_screensJson = root["screens"].toArray();
        }
        if (root.contains("disableHwVideo")) {
            setDisableHwVideo(root["disableHwVideo"].toBool());
        }
        if (root.contains("disableSplash")) {
            setDisableSplash(root["disableSplash"].toBool());
        }
        auto loadColor = [&root](const QString& key, std::function<void(const QColor&)> setter) {
            if (root.contains(key)) {
                QJsonValue val = root[key];
                if (val.isString()) {
                    QColor c(val.toString());
                    if (c.isValid()) setter(c);
                }
            }
        };

        loadColor("lyricsIntroComponentColor", [this](const QColor& c){ setIntroComponentColor(c); });
        loadColor("lyricsVerseComponentColor", [this](const QColor& c){ setVerseComponentColor(c); });
        loadColor("lyricsPrechorusComponentColor", [this](const QColor& c){ setPrechorusComponentColor(c); });
        loadColor("lyricsChorusComponentColor", [this](const QColor& c){ setChorusComponentColor(c); });
        loadColor("lyricsBridgeComponentColor", [this](const QColor& c){ setBridgeComponentColor(c); });
        loadColor("lyricsTagComponentColor", [this](const QColor& c){ setTagComponentColor(c); });
        loadColor("lyricsInterludeComponentColor", [this](const QColor& c){ setInterludeComponentColor(c); });
        loadColor("lyricsRefrainComponentColor", [this](const QColor& c){ setRefrainComponentColor(c); });
        loadColor("lyricsInstrumentalComponentColor", [this](const QColor& c){ setInstrumentalComponentColor(c); });
        loadColor("lyricsVampComponentColor", [this](const QColor& c){ setVampComponentColor(c); });
        loadColor("lyricsTurnaroundComponentColor", [this](const QColor& c){ setTurnaroundComponentColor(c); });
        loadColor("lyricsPassthroughComponentColor", [this](const QColor& c){ setPassthroughComponentColor(c); });
        loadColor("lyricsEndingComponentColor", [this](const QColor& c){ setEndingComponentColor(c); });
        loadColor("lyricsOutroComponentColor", [this](const QColor& c){ setOutroComponentColor(c); });
        loadColor("lyricsBlankComponentColor", [this](const QColor& c){ setBlankComponentColor(c); });
        if (root.contains("activeControllers") && root["activeControllers"].isArray()) {
            QVariantList controllers;
            QJsonArray arr = root["activeControllers"].toArray();
            for (const QJsonValue& val : arr) {
                controllers.append(val.toVariant());
            }
            setActiveControllers(controllers);
        }
    }
    emit settingsLoaded();
}

void SettingsManager::saveSettings() {
    QString filePath = getSettingsFilePath();
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Could not open settings file for writing:" << filePath;
        return;
    }

    QJsonObject root;
    root["globalTransition"] = m_globalTransition;
    root["practiceDelayMs"] = m_practiceDelayMs;
    root["screens"] = m_screensJson;
    root["disableHwVideo"] = m_disableHwVideo;
    root["disableSplash"] = m_disableSplash;
    root["lyricsIntroComponentColor"] = m_introComponentColor.name(QColor::HexArgb);
    root["lyricsVerseComponentColor"] = m_verseComponentColor.name(QColor::HexArgb);
    root["lyricsPrechorusComponentColor"] = m_prechorusComponentColor.name(QColor::HexArgb);
    root["lyricsChorusComponentColor"] = m_chorusComponentColor.name(QColor::HexArgb);
    root["lyricsBridgeComponentColor"] = m_bridgeComponentColor.name(QColor::HexArgb);
    root["lyricsTagComponentColor"] = m_tagComponentColor.name(QColor::HexArgb);
    root["lyricsInterludeComponentColor"] = m_interludeComponentColor.name(QColor::HexArgb);
    root["lyricsRefrainComponentColor"] = m_refrainComponentColor.name(QColor::HexArgb);
    root["lyricsInstrumentalComponentColor"] = m_instrumentalComponentColor.name(QColor::HexArgb);
    root["lyricsVampComponentColor"] = m_vampComponentColor.name(QColor::HexArgb);
    root["lyricsTurnaroundComponentColor"] = m_turnaroudComponentColor.name(QColor::HexArgb);
    root["lyricsPassthroughComponentColor"] = m_passthroughComponentColor.name(QColor::HexArgb);
    root["lyricsEndingComponentColor"] = m_endingComponentColor.name(QColor::HexArgb);
    root["lyricsOutroComponentColor"] = m_outroComponentColor.name(QColor::HexArgb);
    root["lyricsBlankComponentColor"] = m_blankComponentColor.name(QColor::HexArgb);

    QJsonArray controllersArray;
    for (const QVariant& var : m_activeControllers) {
        controllersArray.append(QJsonValue::fromVariant(var));
    }
    root["activeControllers"] = controllersArray;
    
    root["timers"] = m_timersJson;

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}

void SettingsManager::setGlobalTransition(int transition) {
    if (m_globalTransition != transition) {
        m_globalTransition = transition;
        emit globalTransitionChanged();
        saveSettings();
    }
}

void SettingsManager::setPracticeDelayMs(int ms) {
    if (m_practiceDelayMs != ms) {
        m_practiceDelayMs = ms;
        emit practiceDelayMsChanged();
        saveSettings();
    }
}

void SettingsManager::setScreensJson(const QJsonArray& screens) {
    if (m_screensJson != screens) {
        m_screensJson = screens;
        emit screensJsonChanged();
        saveSettings();
    }
}

void SettingsManager::setDisableHwVideo(bool disable) {
    if (m_disableHwVideo != disable) {
        m_disableHwVideo = disable;
        emit disableHwVideoChanged();
        saveSettings();
    }
}

void SettingsManager::setDisableSplash(bool disable) {
    if (m_disableSplash != disable) {
        m_disableSplash = disable;
        emit disableSplashChanged();
        saveSettings();
    }
}

void SettingsManager::setActiveControllers(const QVariantList& controllers) {
    if (m_activeControllers != controllers) {
        m_activeControllers = controllers;
        emit activeControllersChanged();
    }
}

void SettingsManager::setTimersJson(const QJsonArray& timers) {
    m_timersJson = timers;
    saveSettings();
}

// Lyrics Colors Getters and Setters
QColor SettingsManager::introComponentColor() const { return m_introComponentColor; }
void SettingsManager::setIntroComponentColor(const QColor& color) {
    if (m_introComponentColor != color) { 
        m_introComponentColor = color; 
        emit lyricsColorChanged(); 
        saveSettings();
    }
}

QColor SettingsManager::verseComponentColor() const { return m_verseComponentColor; }
void SettingsManager::setVerseComponentColor(const QColor& color) {
    if (m_verseComponentColor != color) {
        m_verseComponentColor = color;
        emit lyricsColorChanged();
        saveSettings();
    }
}

QColor SettingsManager::prechorusComponentColor() const { return m_prechorusComponentColor; }
void SettingsManager::setPrechorusComponentColor(const QColor& color) {
    if (m_prechorusComponentColor != color) {
        m_prechorusComponentColor = color;
        emit lyricsColorChanged();
        saveSettings();
    }
}
QColor SettingsManager::chorusComponentColor() const { return m_chorusComponentColor; }
void SettingsManager::setChorusComponentColor(const QColor& color) {
    if (m_chorusComponentColor != color) {
        m_chorusComponentColor = color;
        emit lyricsColorChanged();
        saveSettings();
    }
}
QColor SettingsManager::bridgeComponentColor() const { return m_bridgeComponentColor; }
void SettingsManager::setBridgeComponentColor(const QColor& color) {
    if (m_bridgeComponentColor != color) {
        m_bridgeComponentColor = color;
        emit lyricsColorChanged();
        saveSettings();
    }
}
QColor SettingsManager::tagComponentColor() const { return m_tagComponentColor; }
void SettingsManager::setTagComponentColor(const QColor& color) {
    if (m_tagComponentColor != color) {
        m_tagComponentColor = color;
        emit lyricsColorChanged();
        saveSettings();
    }
}
QColor SettingsManager::interludeComponentColor() const { return m_interludeComponentColor; }
void SettingsManager::setInterludeComponentColor(const QColor& color) {
    if (m_interludeComponentColor != color) {
        m_interludeComponentColor = color;
        emit lyricsColorChanged();
        saveSettings();
    }
}
QColor SettingsManager::refrainComponentColor() const { return m_refrainComponentColor; }
void SettingsManager::setRefrainComponentColor(const QColor& color) {
    if (m_refrainComponentColor != color) {
        m_refrainComponentColor = color;
        emit lyricsColorChanged();
        saveSettings();
    }
}
QColor SettingsManager::instrumentalComponentColor() const { return m_instrumentalComponentColor; }
void SettingsManager::setInstrumentalComponentColor(const QColor& color) {
    if (m_instrumentalComponentColor != color) {
        m_instrumentalComponentColor = color;
        emit lyricsColorChanged();
        saveSettings();
    }
}
QColor SettingsManager::vampComponentColor() const { return m_vampComponentColor; }
void SettingsManager::setVampComponentColor(const QColor& color) {
    if (m_vampComponentColor != color) {
        m_vampComponentColor = color;
        emit lyricsColorChanged();
        saveSettings();
    }
}
QColor SettingsManager::turnaroundComponentColor() const { return m_turnaroudComponentColor; }
void SettingsManager::setTurnaroundComponentColor(const QColor& color) {
    if (m_turnaroudComponentColor != color) {
        m_turnaroudComponentColor = color;
        emit lyricsColorChanged();
        saveSettings();
    }
}
QColor SettingsManager::passthroughComponentColor() const { return m_passthroughComponentColor; }
void SettingsManager::setPassthroughComponentColor(const QColor& color) {
    if (m_passthroughComponentColor != color) {
        m_passthroughComponentColor = color;
        emit lyricsColorChanged();
        saveSettings();
    }
}
QColor SettingsManager::endingComponentColor() const { return m_endingComponentColor; }
void SettingsManager::setEndingComponentColor(const QColor& color) {
    if (m_endingComponentColor != color) {
        m_endingComponentColor = color;
        emit lyricsColorChanged();
        saveSettings();
    }
}
QColor SettingsManager::outroComponentColor() const { return m_outroComponentColor; }
void SettingsManager::setOutroComponentColor(const QColor& color) {
    if (m_outroComponentColor != color) {
        m_outroComponentColor = color;
        emit lyricsColorChanged();
        saveSettings();
    }
}
QColor SettingsManager::blankComponentColor() const { return m_blankComponentColor; }
void SettingsManager::setBlankComponentColor(const QColor& color) {
    if (m_blankComponentColor != color) {
        m_blankComponentColor = color;
        emit lyricsColorChanged();
        saveSettings();
    }
}