#pragma once

#include <QColor>
#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantList>

class SettingsManager : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(int globalTransition READ globalTransition WRITE setGlobalTransition NOTIFY globalTransitionChanged)
    Q_PROPERTY(int practiceDelayMs READ practiceDelayMs WRITE setPracticeDelayMs NOTIFY practiceDelayMsChanged)
    Q_PROPERTY(QJsonArray screensJson READ screensJson WRITE setScreensJson NOTIFY screensJsonChanged)
    Q_PROPERTY(QVariantList activeControllers READ activeControllers WRITE setActiveControllers NOTIFY activeControllersChanged)
    Q_PROPERTY(bool disableHwVideo READ disableHwVideo WRITE setDisableHwVideo NOTIFY disableHwVideoChanged)
    Q_PROPERTY(bool disableSplash READ disableSplash WRITE setDisableSplash NOTIFY disableSplashChanged)
    // Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY activeLayoutChanged)
    Q_PROPERTY(QColor introComponentColor READ introComponentColor WRITE setIntroComponentColor NOTIFY lyricsColorChanged)
    Q_PROPERTY(QColor verseComponentColor READ verseComponentColor WRITE setVerseComponentColor NOTIFY lyricsColorChanged)
    Q_PROPERTY(QColor prechorusComponentColor READ prechorusComponentColor WRITE setPrechorusComponentColor NOTIFY lyricsColorChanged)
    Q_PROPERTY(QColor chorusComponentColor READ chorusComponentColor WRITE setChorusComponentColor NOTIFY lyricsColorChanged)
    Q_PROPERTY(QColor bridgeComponentColor READ bridgeComponentColor WRITE setBridgeComponentColor NOTIFY lyricsColorChanged)
    Q_PROPERTY(QColor tagComponentColor READ tagComponentColor WRITE setTagComponentColor NOTIFY lyricsColorChanged)
    Q_PROPERTY(QColor interludeComponentColor READ interludeComponentColor WRITE setInterludeComponentColor NOTIFY lyricsColorChanged)
    Q_PROPERTY(QColor refrainComponentColor READ refrainComponentColor WRITE setRefrainComponentColor NOTIFY lyricsColorChanged)
    Q_PROPERTY(QColor instrumentalComponentColor READ instrumentalComponentColor WRITE setInstrumentalComponentColor NOTIFY lyricsColorChanged)
    Q_PROPERTY(QColor vampComponentColor READ vampComponentColor WRITE setVampComponentColor NOTIFY lyricsColorChanged)
    Q_PROPERTY(QColor turnaroundComponentColor READ turnaroundComponentColor WRITE setTurnaroundComponentColor NOTIFY lyricsColorChanged)
    Q_PROPERTY(QColor passthroughComponentColor READ passthroughComponentColor WRITE setPassthroughComponentColor NOTIFY lyricsColorChanged)
    Q_PROPERTY(QColor endingComponentColor READ endingComponentColor WRITE setEndingComponentColor NOTIFY lyricsColorChanged)
    Q_PROPERTY(QColor outroComponentColor READ outroComponentColor WRITE setOutroComponentColor NOTIFY lyricsColorChanged)
    Q_PROPERTY(QColor blankComponentColor READ blankComponentColor WRITE setBlankComponentColor NOTIFY lyricsColorChanged)


public:
    explicit SettingsManager(QObject* parent = nullptr);
    ~SettingsManager() override;

    void loadSettings();
    void saveSettings();

    int globalTransition() const { return m_globalTransition; }
    void setGlobalTransition(int transition);

    int practiceDelayMs() const { return m_practiceDelayMs; }
    void setPracticeDelayMs(int ms);

    QJsonArray screensJson() const { return m_screensJson; }
    void setScreensJson(const QJsonArray& screens);

    bool disableHwVideo() const { return m_disableHwVideo; }
    void setDisableHwVideo(bool disable);

    bool disableSplash() const { return m_disableSplash; }
    void setDisableSplash(bool disable);

    QVariantList activeControllers() const { return m_activeControllers; }
    void setActiveControllers(const QVariantList& controllers);

    QJsonArray getTimersJson() const { return m_timersJson; }
    void setTimersJson(const QJsonArray& timers);

    QColor introComponentColor() const;
    void setIntroComponentColor(const QColor& color);

    QColor verseComponentColor() const;
    void setVerseComponentColor(const QColor& color);

    QColor prechorusComponentColor() const;
    void setPrechorusComponentColor(const QColor& color);

    QColor chorusComponentColor() const;
    void setChorusComponentColor(const QColor& color);

    QColor bridgeComponentColor() const;
    void setBridgeComponentColor(const QColor& color);

    QColor tagComponentColor() const;
    void setTagComponentColor(const QColor& color);

    QColor interludeComponentColor() const;
    void setInterludeComponentColor(const QColor& color);

    QColor refrainComponentColor() const;
    void setRefrainComponentColor(const QColor& color);

    QColor instrumentalComponentColor() const;
    void setInstrumentalComponentColor(const QColor& color);

    QColor vampComponentColor() const;
    void setVampComponentColor(const QColor& color);

    QColor turnaroundComponentColor() const;
    void setTurnaroundComponentColor(const QColor& color);

    QColor passthroughComponentColor() const;
    void setPassthroughComponentColor(const QColor& color);

    QColor endingComponentColor() const;
    void setEndingComponentColor(const QColor& color);

    QColor outroComponentColor() const;
    void setOutroComponentColor(const QColor& color);

    QColor blankComponentColor() const;
    void setBlankComponentColor(const QColor& color);



signals:
    void globalTransitionChanged();
    void practiceDelayMsChanged();
    void screensJsonChanged();
    void activeControllersChanged();
    void disableHwVideoChanged();
    void disableSplashChanged();
    void settingsLoaded();
    void lyricsColorChanged();

private:
    QString getSettingsFilePath() const;
    void applyDefaults();

    int m_globalTransition = 0; // Default transition (e.g. Cut)
    int m_practiceDelayMs = 273; // Audience practice latency delay (ms)
    QJsonArray m_screensJson;
    bool m_disableHwVideo = false;
    bool m_disableSplash = false;
    QVariantList m_activeControllers;
    QJsonArray m_timersJson;

    QColor m_introComponentColor = QColor("#BDB76B");
    QColor m_verseComponentColor = QColor("#0072C6");
    QColor m_prechorusComponentColor = QColor("#E75480");
    QColor m_chorusComponentColor = QColor("#D8005A");
    QColor m_bridgeComponentColor = QColor("#8A2BE2");
    QColor m_tagComponentColor = QColor("#FF0000");
    QColor m_interludeComponentColor = QColor("#32CD32");
    QColor m_refrainComponentColor = QColor("#1db825");
    QColor m_instrumentalComponentColor = QColor("#f1df2c");
    QColor m_vampComponentColor = QColor("#32CD32");
    QColor m_turnaroudComponentColor = QColor("#32CD32");
    QColor m_passthroughComponentColor = QColor("#000000");
    QColor m_endingComponentColor = QColor("#BDB76B");
    QColor m_outroComponentColor = QColor("#777132");
    QColor m_blankComponentColor = QColor("#000000");
};
