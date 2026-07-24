#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>
#include <QUuid>
#include <QtQml/qqmlregistration.h>

struct ScreenConfig {
    QString id;
    QString name;
    int hardwareDisplayIndex = -1;
    int width = 1920;
    int height = 1080;
    bool isFullscreen = false;
    bool isLocked = false;
    bool monitorEnabled = true;

    QJsonObject toJson() const;
    static ScreenConfig fromJson(const QJsonObject& json);
};

class ScreenModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

public:
    enum ScreenRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        HardwareDisplayIndexRole,
        WidthRole,
        HeightRole,
        IsFullscreenRole,
        IsLockedRole,
        MonitorEnabledRole
    };

    explicit ScreenModel(QObject* parent = nullptr);
    ~ScreenModel() override = default;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;

    void setScreens(const QList<ScreenConfig>& screens);
    QList<ScreenConfig> screens() const;

    Q_INVOKABLE void addScreen(const QString& name);
    Q_INVOKABLE void removeScreen(int index);
    Q_INVOKABLE void updateScreen(int index, const QString& name, int displayIndex, int width, int height, bool isFullscreen, bool monitorEnabled);

    // Returns a list of strings for UI combobox: ["Display 1 (1920x1080)", "Display 2 (1280x720)"]
    Q_INVOKABLE QStringList getHardwareDisplays() const;
    
    // Updates the width and height of the screen to match the hardware display's native resolution
    Q_INVOKABLE void applyHardwareDisplayGeometry(int screenIndex, int hardwareDisplayIndex);

    Q_INVOKABLE QStringList getScreenNames() const;

signals:
    void screensChanged(); // Emitted when a screen is added/removed/updated

private:
    QList<ScreenConfig> m_screens;
};
