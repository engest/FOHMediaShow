#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantList>

class SettingsManager : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(int globalTransition READ globalTransition WRITE setGlobalTransition NOTIFY globalTransitionChanged)
    Q_PROPERTY(QJsonArray screensJson READ screensJson WRITE setScreensJson NOTIFY screensJsonChanged)
    Q_PROPERTY(QVariantList activeControllers READ activeControllers WRITE setActiveControllers NOTIFY activeControllersChanged)
    Q_PROPERTY(bool disableHwVideo READ disableHwVideo WRITE setDisableHwVideo NOTIFY disableHwVideoChanged)

public:
    explicit SettingsManager(QObject* parent = nullptr);
    ~SettingsManager() override;

    void loadSettings();
    void saveSettings();

    int globalTransition() const { return m_globalTransition; }
    void setGlobalTransition(int transition);

    QJsonArray screensJson() const { return m_screensJson; }
    void setScreensJson(const QJsonArray& screens);

    bool disableHwVideo() const { return m_disableHwVideo; }
    void setDisableHwVideo(bool disable);

    QVariantList activeControllers() const { return m_activeControllers; }
    void setActiveControllers(const QVariantList& controllers);

    QJsonArray getTimersJson() const { return m_timersJson; }
    void setTimersJson(const QJsonArray& timers);

signals:
    void globalTransitionChanged();
    void screensJsonChanged();
    void activeControllersChanged();
    void disableHwVideoChanged();
    void settingsLoaded();

private:
    QString getSettingsFilePath() const;
    void applyDefaults();

    int m_globalTransition = 0; // Default transition (e.g. Cut)
    QJsonArray m_screensJson;
    bool m_disableHwVideo = false;
    QVariantList m_activeControllers;
    QJsonArray m_timersJson;
};
