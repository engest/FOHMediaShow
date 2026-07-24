#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QTime>
#include <QList>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>

class TimerManager : public QAbstractListModel {
    Q_OBJECT
public:
    enum TimerType {
        Stopwatch = 0,
        CountdownDuration = 1,
        CountdownToTime = 2,
        Clock = 3
    };
    Q_ENUM(TimerType)

    enum TimerState {
        Stopped = 0,
        Running = 1,
        Expired = 2
    };
    Q_ENUM(TimerState)

    struct TimerData {
        QString id;
        QString name;
        TimerType type = Stopwatch;
        
        qint64 durationMs = 0; 
        QTime targetTime;
        QDateTime targetDateTime; // Runtime state for CountdownToTime
        
        // Clock specific settings
        QString timezone = "Local";
        bool is24Hour = false;
        bool applyDST = true;
        QString formatString = "hh:mm:ss AP";

        TimerState state = Stopped;
        qint64 accumulatedMs = 0; 
        qint64 lastStartTimeEpoch = 0; 
        
        QString currentString = "";
        bool isNegative = false;

        QJsonObject toJson() const;
        static TimerData fromJson(const QJsonObject& json);
    };

    enum TimerRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        TypeRole,
        DurationMsRole,
        TargetTimeRole,
        TimezoneRole,
        Is24HourRole,
        ApplyDSTRole,
        FormatStringRole,
        StateRole,
        CurrentStringRole,
        IsNegativeRole
    };

    explicit TimerManager(QObject* parent = nullptr);
    ~TimerManager() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // QML Callable Methods
    Q_INVOKABLE void addTimer(const QString& name, int type, qint64 durationMs, const QDateTime& targetTime);
    Q_INVOKABLE void updateTimer(const QString& id, const QString& name, int type, qint64 durationMs, const QDateTime& targetTime);
    Q_INVOKABLE void updateClock(const QString& id, const QString& name, const QString& timezone, bool is24Hour, bool applyDST, const QString& formatString);
    Q_INVOKABLE void removeTimer(const QString& id);
    Q_INVOKABLE void moveTimer(int from, int to);
    
    Q_INVOKABLE void startTimer(const QString& id);
    Q_INVOKABLE void stopTimer(const QString& id);
    Q_INVOKABLE void resetTimer(const QString& id);

    Q_INVOKABLE QString getTimerString(const QString& id) const;
    Q_INVOKABLE QJsonObject getTimer(const QString& id) const;

    void loadFromJson(const QJsonArray& json);
    QJsonArray saveToJson() const;

signals:
    void timersChanged();
    void timerTicked();

private slots:
    void tick();

private:
    void calculateCurrentString(TimerData& timer);
    QList<TimerData> m_timers;
    QTimer m_tickTimer;
};
