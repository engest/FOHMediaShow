#include "../inc/ScreenModel.h"
#include <QGuiApplication>
#include <QScreen>
#include <QJsonObject>
#include <QUuid>

QJsonObject ScreenConfig::toJson() const {
    QJsonObject json;
    json["id"] = id;
    json["name"] = name;
    json["hardwareDisplayIndex"] = hardwareDisplayIndex;
    json["width"] = width;
    json["height"] = height;
    json["isFullscreen"] = isFullscreen;
    json["isLocked"] = isLocked;
    json["monitorEnabled"] = monitorEnabled;
    return json;
}

ScreenConfig ScreenConfig::fromJson(const QJsonObject& json) {
    ScreenConfig config;
    config.id = json["id"].toString(QUuid::createUuid().toString(QUuid::WithoutBraces));
    config.name = json["name"].toString("Unnamed Screen");
    config.hardwareDisplayIndex = json["hardwareDisplayIndex"].toInt(-1);
    config.width = json["width"].toInt(1920);
    config.height = json["height"].toInt(1080);
    config.isFullscreen = json["isFullscreen"].toBool(false);
    config.isLocked = json["isLocked"].toBool(false);
    config.monitorEnabled = json["monitorEnabled"].toBool(true);
    return config;
}

ScreenModel::ScreenModel(QObject* parent) : QAbstractListModel(parent) {
}

int ScreenModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_screens.size();
}

QVariant ScreenModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_screens.size()) return QVariant();

    const ScreenConfig& screen = m_screens.at(index.row());

    switch (role) {
        case IdRole: return screen.id;
        case NameRole: return screen.name;
        case HardwareDisplayIndexRole: return screen.hardwareDisplayIndex;
        case WidthRole: return screen.width;
        case HeightRole: return screen.height;
        case IsFullscreenRole: return screen.isFullscreen;
        case IsLockedRole: return screen.isLocked;
        case MonitorEnabledRole: return screen.monitorEnabled;
    }
    return QVariant();
}

bool ScreenModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid() || index.row() >= m_screens.size()) return false;

    ScreenConfig& screen = m_screens[index.row()];
    bool changed = false;

    switch (role) {
        case NameRole:
            if (screen.name != value.toString()) {
                screen.name = value.toString();
                changed = true;
            }
            break;
        case HardwareDisplayIndexRole:
            if (screen.hardwareDisplayIndex != value.toInt()) {
                screen.hardwareDisplayIndex = value.toInt();
                changed = true;
            }
            break;
        case WidthRole:
            if (screen.width != value.toInt()) {
                screen.width = value.toInt();
                changed = true;
            }
            break;
        case HeightRole:
            if (screen.height != value.toInt()) {
                screen.height = value.toInt();
                changed = true;
            }
            break;
        case IsFullscreenRole:
            if (screen.isFullscreen != value.toBool()) {
                screen.isFullscreen = value.toBool();
                changed = true;
            }
            break;
        case MonitorEnabledRole:
            if (screen.monitorEnabled != value.toBool()) {
                screen.monitorEnabled = value.toBool();
                changed = true;
            }
            break;
    }

    if (changed) {
        emit dataChanged(index, index, {role});
        emit screensChanged();
        return true;
    }
    return false;
}

QHash<int, QByteArray> ScreenModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[NameRole] = "name";
    roles[HardwareDisplayIndexRole] = "hardwareDisplayIndex";
    roles[WidthRole] = "width";
    roles[HeightRole] = "height";
    roles[IsFullscreenRole] = "isFullscreen";
    roles[IsLockedRole] = "isLocked";
    roles[MonitorEnabledRole] = "monitorEnabled";
    return roles;
}

void ScreenModel::setScreens(const QList<ScreenConfig>& screens) {
    beginResetModel();
    m_screens = screens;
    endResetModel();
    emit screensChanged();
}

QList<ScreenConfig> ScreenModel::screens() const {
    return m_screens;
}

void ScreenModel::addScreen(const QString& name) {
    beginInsertRows(QModelIndex(), m_screens.size(), m_screens.size());
    ScreenConfig config;
    config.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    config.name = name;
    m_screens.append(config);
    endInsertRows();
    emit screensChanged();
}

void ScreenModel::removeScreen(int index) {
    if (index < 0 || index >= m_screens.size()) return;
    if (m_screens[index].isLocked) return;

    beginRemoveRows(QModelIndex(), index, index);
    m_screens.removeAt(index);
    endRemoveRows();
    emit screensChanged();
}

void ScreenModel::updateScreen(int index, const QString& name, int displayIndex, int width, int height, bool isFullscreen, bool monitorEnabled) {
    if (index < 0 || index >= m_screens.size()) return;
    
    ScreenConfig& screen = m_screens[index];
    screen.name = name;
    
    screen.hardwareDisplayIndex = displayIndex;
    screen.width = width;
    screen.height = height;
    screen.isFullscreen = isFullscreen;
    screen.monitorEnabled = monitorEnabled;
    
    emit dataChanged(this->index(index, 0), this->index(index, 0));
    emit screensChanged();
}

QStringList ScreenModel::getHardwareDisplays() const {
    QStringList displayNames;
    displayNames << "Unassigned"; // Index -1 maps to index 0 in UI? Usually combo box is 0-indexed.
    // Actually, if we just return a list, index 0 is "Unassigned", index 1 is "Display 1"...
    
    const QList<QScreen*> screens = QGuiApplication::screens();
    for (int i = 0; i < screens.size(); ++i) {
        QScreen* screen = screens.at(i);
        displayNames << QString("Display %1 (%2x%3)").arg(i + 1).arg(screen->geometry().width()).arg(screen->geometry().height());
    }
    return displayNames;
}

void ScreenModel::applyHardwareDisplayGeometry(int screenIndex, int hardwareDisplayIndex) {
    if (screenIndex < 0 || screenIndex >= m_screens.size()) return;
    if (hardwareDisplayIndex < 0) return;
    
    const QList<QScreen*> screens = QGuiApplication::screens();
    if (hardwareDisplayIndex < screens.size()) {
        QScreen* hwScreen = screens.at(hardwareDisplayIndex);
        setData(this->index(screenIndex, 0), hwScreen->geometry().width(), WidthRole);
        setData(this->index(screenIndex, 0), hwScreen->geometry().height(), HeightRole);
    }
}

QStringList ScreenModel::getScreenNames() const {
    QStringList names;
    for (const auto& screen : m_screens) {
        names << screen.name;
    }
    return names;
}
