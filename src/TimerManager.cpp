#include "../inc/TimerManager.h"
#include <QDateTime>
#include <QTimeZone>
#include <QDebug>

QJsonObject TimerManager::TimerData::toJson() const {
    QJsonObject obj;
    obj["id"] = id;
    obj["name"] = name;
    obj["type"] = static_cast<int>(type);
    obj["durationMs"] = durationMs;
    obj["targetTime"] = targetTime.toString("HH:mm:ss");
    obj["timezone"] = timezone;
    obj["is24Hour"] = is24Hour;
    obj["applyDST"] = applyDST;
    obj["formatString"] = formatString;
    obj["state"] = static_cast<int>(state);
    obj["accumulatedMs"] = accumulatedMs;
    obj["lastStartTimeEpoch"] = lastStartTimeEpoch;
    return obj;
}

TimerManager::TimerData TimerManager::TimerData::fromJson(const QJsonObject& json) {
    TimerData t;
    t.id = json["id"].toString();
    t.name = json["name"].toString();
    t.type = static_cast<TimerType>(json["type"].toInt(0));
    t.durationMs = json["durationMs"].toVariant().toLongLong();
    t.targetTime = QTime::fromString(json["targetTime"].toString(), "HH:mm:ss");
    t.timezone = json["timezone"].toString("Local");
    t.is24Hour = json["is24Hour"].toBool(false);
    t.applyDST = json["applyDST"].toBool(true);
    t.formatString = json["formatString"].toString("hh:mm:ss AP");
    t.state = static_cast<TimerState>(json["state"].toInt(0));
    t.accumulatedMs = json["accumulatedMs"].toVariant().toLongLong();
    t.lastStartTimeEpoch = json["lastStartTimeEpoch"].toVariant().toLongLong();
    return t;
}

TimerManager::TimerManager(QObject* parent) : QAbstractListModel(parent) {
    connect(&m_tickTimer, &QTimer::timeout, this, &TimerManager::tick);
    m_tickTimer.start(100); 
}

TimerManager::~TimerManager() = default;

int TimerManager::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(m_timers.count());
}

QVariant TimerManager::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_timers.count()) return QVariant();

    const auto& t = m_timers[index.row()];
    switch (role) {
        case IdRole: return t.id;
        case NameRole: return t.name;
        case TypeRole: return static_cast<int>(t.type);
        case DurationMsRole: return t.durationMs;
        case TargetTimeRole: return QDateTime(QDate::currentDate(), t.targetTime);
        case TimezoneRole: return t.timezone;
        case Is24HourRole: return t.is24Hour;
        case ApplyDSTRole: return t.applyDST;
        case FormatStringRole: return t.formatString;
        case StateRole: return static_cast<int>(t.state);
        case CurrentStringRole: return t.currentString;
        case IsNegativeRole: return t.isNegative;
    }
    return QVariant();
}

QHash<int, QByteArray> TimerManager::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[NameRole] = "name";
    roles[TypeRole] = "type";
    roles[DurationMsRole] = "durationMs";
    roles[TargetTimeRole] = "targetTime";
    roles[TimezoneRole] = "timezone";
    roles[Is24HourRole] = "is24Hour";
    roles[ApplyDSTRole] = "applyDST";
    roles[FormatStringRole] = "formatString";
    roles[StateRole] = "state";
    roles[CurrentStringRole] = "currentString";
    roles[IsNegativeRole] = "isNegative";
    return roles;
}

void TimerManager::addTimer(const QString& name, int type, qint64 durationMs, const QDateTime& targetTime) {
    beginInsertRows(QModelIndex(), static_cast<int>(m_timers.count()), static_cast<int>(m_timers.count()));
    TimerData t;
    t.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    t.name = name;
    t.type = static_cast<TimerType>(type);
    t.durationMs = durationMs;
    t.targetTime = targetTime.time();
    calculateCurrentString(t);
    m_timers.append(t);
    endInsertRows();
    emit timersChanged();
}

void TimerManager::updateTimer(const QString& id, const QString& name, int type, qint64 durationMs, const QDateTime& targetTime) {
    for (int i = 0; i < m_timers.count(); ++i) {
        if (m_timers[i].id == id) {
            m_timers[i].name = name;
            m_timers[i].type = static_cast<TimerType>(type);
            m_timers[i].durationMs = durationMs;
            m_timers[i].targetTime = targetTime.time();
            
            if (m_timers[i].type != Clock) {
                m_timers[i].state = Stopped;
                m_timers[i].accumulatedMs = 0;
                m_timers[i].currentString = "";
            } else {
                m_timers[i].currentString = "dirty";
            }
            
            calculateCurrentString(m_timers[i]);
            emit dataChanged(index(i), index(i), {NameRole, TypeRole, DurationMsRole, TargetTimeRole, CurrentStringRole, StateRole});
            emit timersChanged();
            break;
        }
    }
}

void TimerManager::updateClock(const QString& id, const QString& name, const QString& timezone, bool is24Hour, bool applyDST, const QString& formatString) {
    for (int i = 0; i < m_timers.count(); ++i) {
        if (m_timers[i].id == id) {
            m_timers[i].name = name;
            m_timers[i].type = Clock;
            m_timers[i].timezone = timezone;
            m_timers[i].is24Hour = is24Hour;
            m_timers[i].applyDST = applyDST;
            m_timers[i].formatString = formatString;
            calculateCurrentString(m_timers[i]);
            emit dataChanged(index(i), index(i));
            emit timersChanged();
            break;
        }
    }
}

void TimerManager::removeTimer(const QString& id) {
    for (int i = 0; i < m_timers.count(); ++i) {
        if (m_timers[i].id == id) {
            beginRemoveRows(QModelIndex(), i, i);
            m_timers.removeAt(i);
            endRemoveRows();
            emit timersChanged();
            break;
        }
    }
}

void TimerManager::moveTimer(int from, int to) {
    if (from < 0 || from >= m_timers.count() || to < 0 || to >= m_timers.count() || from == to) {
        return;
    }
    
    int modelTo = to > from ? to + 1 : to;
    beginMoveRows(QModelIndex(), from, from, QModelIndex(), modelTo);
    m_timers.move(from, to);
    endMoveRows();
    emit timersChanged();
}

void TimerManager::startTimer(const QString& id) {
    for (int i = 0; i < m_timers.count(); ++i) {
        if (m_timers[i].id == id) {
            if (m_timers[i].state != Running) {
                m_timers[i].state = Running;
                m_timers[i].lastStartTimeEpoch = QDateTime::currentMSecsSinceEpoch();
                
                if (m_timers[i].type == CountdownToTime) {
                    QDateTime now = QDateTime::currentDateTime();
                    QDateTime target(now.date(), m_timers[i].targetTime);
                    if (target <= now) {
                        target = target.addDays(1);
                    }
                    m_timers[i].targetDateTime = target;
                }
                
                calculateCurrentString(m_timers[i]);
                emit dataChanged(index(i), index(i), {StateRole, CurrentStringRole});
            }
            break;
        }
    }
}

void TimerManager::stopTimer(const QString& id) {
    for (int i = 0; i < m_timers.count(); ++i) {
        if (m_timers[i].id == id) {
            if (m_timers[i].state == Running) {
                m_timers[i].state = Stopped;
                qint64 now = QDateTime::currentMSecsSinceEpoch();
                m_timers[i].accumulatedMs += (now - m_timers[i].lastStartTimeEpoch);
                calculateCurrentString(m_timers[i]);
                emit dataChanged(index(i), index(i), {StateRole, CurrentStringRole});
            }
            break;
        }
    }
}

void TimerManager::resetTimer(const QString& id) {
    for (int i = 0; i < m_timers.count(); ++i) {
        if (m_timers[i].id == id) {
            m_timers[i].state = Stopped;
            m_timers[i].accumulatedMs = 0;
            m_timers[i].lastStartTimeEpoch = 0;
            if (m_timers[i].type != Clock) {
                m_timers[i].currentString = ""; // Clear string to hide on stage display
            } else {
                calculateCurrentString(m_timers[i]);
            }
            emit dataChanged(index(i), index(i), {StateRole, CurrentStringRole});
            break;
        }
    }
}

QString TimerManager::getTimerString(const QString& id) const {
    for (const auto& t : m_timers) {
        if (t.id == id) {
            return t.currentString;
        }
    }
    return "";
}

void TimerManager::loadFromJson(const QJsonArray& json) {
    beginResetModel();
    m_timers.clear();
    for (const QJsonValue& val : json) {
        TimerData t = TimerData::fromJson(val.toObject());
        calculateCurrentString(t);
        m_timers.append(t);
    }
    endResetModel();
}

QJsonArray TimerManager::saveToJson() const {
    QJsonArray arr;
    for (const auto& t : m_timers) {
        arr.append(t.toJson());
    }
    return arr;
}

void TimerManager::calculateCurrentString(TimerData& t) {
    if (t.state == Stopped && t.currentString == "" && t.type != Clock && t.type != CountdownToTime) {
        // If it's stopped and cleared (reset or expired), stay cleared
        return;
    }

    if (t.type == Clock) {
        QDateTime now = QDateTime::currentDateTime();
        if (t.timezone == "UTC") {
            now = QDateTime::currentDateTimeUtc();
        } else if (t.timezone != "Local" && !t.timezone.isEmpty()) {
            now = now.toTimeZone(QTimeZone(t.timezone.toUtf8()));
        }
        QString newString = now.toString(t.formatString);
        if (t.currentString != newString) {
            t.currentString = newString;
        }
        return;
    }

    qint64 totalElapsedMs = t.accumulatedMs;
    if (t.state == Running && t.type != CountdownToTime) {
        totalElapsedMs += (QDateTime::currentMSecsSinceEpoch() - t.lastStartTimeEpoch);
    }

    qint64 timeMs = 0;
    t.isNegative = false;

    if (t.type == Stopwatch) {
        timeMs = totalElapsedMs;
    } else if (t.type == CountdownDuration) {
        timeMs = t.durationMs - totalElapsedMs;
        if (timeMs <= 0) {
            t.state = Expired;
            t.currentString = ""; // Hide when expired
            return;
        }
    } else if (t.type == CountdownToTime) {
        if (t.state == Stopped || t.state == Expired) {
            t.currentString = "";
            return;
        }

        QDateTime now = QDateTime::currentDateTime();
        qint64 secsTo = now.secsTo(t.targetDateTime);
        if (secsTo <= 0) {
            t.state = Expired;
            t.currentString = ""; // Hide when expired
            return;
        } else {
            timeMs = secsTo * 1000;
        }
    }

    qint64 totalSecs = timeMs / 1000;
    qint64 hours = totalSecs / 3600;
    qint64 mins = (totalSecs % 3600) / 60;
    qint64 secs = totalSecs % 60;

    QString newString;
    if (t.isNegative) newString += "-";
    
    if (hours > 0) {
        newString += QString::asprintf("%02lld:%02lld:%02lld", hours, mins, secs);
    } else {
        newString += QString::asprintf("%02lld:%02lld", mins, secs);
    }

    if (timeMs == 0 && t.type != Clock) {
        newString = "";
    }

    if (t.currentString != newString) {
        t.currentString = newString;
    }
}

void TimerManager::tick() {
    bool anyTicked = false;
    for (int i = 0; i < m_timers.count(); ++i) {
        if (m_timers[i].state == Running || m_timers[i].type == Clock) {
            QString oldString = m_timers[i].currentString;
            TimerState oldState = m_timers[i].state;
            calculateCurrentString(m_timers[i]);
            if (oldString != m_timers[i].currentString || oldState != m_timers[i].state) {
                emit dataChanged(index(i), index(i), {CurrentStringRole, IsNegativeRole, StateRole});
                anyTicked = true;
            }
        }
    }
    if (anyTicked) {
        emit timerTicked();
    }
}



QJsonObject TimerManager::getTimer(const QString& id) const {
    for (const auto& t : m_timers) {
        if (t.id == id) {
            return t.toJson();
        }
    }
    return QJsonObject();
}
