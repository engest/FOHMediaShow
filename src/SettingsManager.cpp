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
    m_screensJson = QJsonArray();
    m_disableHwVideo = false;
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
        if (root.contains("screens") && root["screens"].isArray()) {
            m_screensJson = root["screens"].toArray();
        }
        if (root.contains("disableHwVideo")) {
            setDisableHwVideo(root["disableHwVideo"].toBool());
        }
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
    root["screens"] = m_screensJson;
    root["disableHwVideo"] = m_disableHwVideo;
    
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

void SettingsManager::setActiveControllers(const QVariantList& controllers) {
    if (m_activeControllers != controllers) {
        m_activeControllers = controllers;
        emit activeControllersChanged();
        saveSettings();
    }
}

void SettingsManager::setTimersJson(const QJsonArray& timers) {
    m_timersJson = timers;
    saveSettings();
}
