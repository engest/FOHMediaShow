#include <QtQml/qqmlregistration.h>
#pragma once

#include <QAbstractListModel>
#include <QStringList>
#include <QColor>
#include <QRect>
#include <QJsonArray>
#include "../inc/SlideLayout.h"

class LayoutEditorViewModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

    // Active layout properties for QML binding
    Q_PROPERTY(QString layoutName READ layoutName WRITE setLayoutName NOTIFY layoutNameChanged)
    Q_PROPERTY(QString targetScreen READ targetScreen WRITE setTargetScreen NOTIFY targetScreenChanged)
    
    Q_PROPERTY(int canvasWidth READ canvasWidth WRITE setCanvasWidth NOTIFY activeLayoutChanged)
    Q_PROPERTY(int canvasHeight READ canvasHeight WRITE setCanvasHeight NOTIFY activeLayoutChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY activeLayoutChanged)
    Q_PROPERTY(QString backgroundImage READ backgroundImage WRITE setBackgroundImage NOTIFY activeLayoutChanged)
    Q_PROPERTY(QString mediaDir READ mediaDir CONSTANT)

    // Main text properties
    Q_PROPERTY(QRect textBounds READ textBounds WRITE setTextBounds NOTIFY activeLayoutChanged)
    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY activeLayoutChanged)
    Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize NOTIFY activeLayoutChanged)
    Q_PROPERTY(QColor fontColor READ fontColor WRITE setFontColor NOTIFY activeLayoutChanged)
    Q_PROPERTY(bool isBold READ isBold WRITE setIsBold NOTIFY activeLayoutChanged)
    Q_PROPERTY(bool isItalic READ isItalic WRITE setIsItalic NOTIFY activeLayoutChanged)
    Q_PROPERTY(bool allCaps READ allCaps WRITE setAllCaps NOTIFY activeLayoutChanged)
    Q_PROPERTY(int alignment READ alignment WRITE setAlignment NOTIFY activeLayoutChanged)

    // Next text properties
    Q_PROPERTY(bool hasNextText READ hasNextText WRITE setHasNextText NOTIFY activeLayoutChanged)
    Q_PROPERTY(QRect nextTextBounds READ nextTextBounds WRITE setNextTextBounds NOTIFY activeLayoutChanged)
    Q_PROPERTY(QString nextFontFamily READ nextFontFamily WRITE setNextFontFamily NOTIFY activeLayoutChanged)
    Q_PROPERTY(int nextFontSize READ nextFontSize WRITE setNextFontSize NOTIFY activeLayoutChanged)
    Q_PROPERTY(QColor nextFontColor READ nextFontColor WRITE setNextFontColor NOTIFY activeLayoutChanged)
    Q_PROPERTY(bool nextIsBold READ nextIsBold WRITE setNextIsBold NOTIFY activeLayoutChanged)
    Q_PROPERTY(bool nextIsItalic READ nextIsItalic WRITE setNextIsItalic NOTIFY activeLayoutChanged)
    Q_PROPERTY(bool nextAllCaps READ nextAllCaps WRITE setNextAllCaps NOTIFY activeLayoutChanged)
    Q_PROPERTY(int nextAlignment READ nextAlignment WRITE setNextAlignment NOTIFY activeLayoutChanged)

    Q_PROPERTY(QJsonArray activeLayoutTimers READ activeLayoutTimers NOTIFY activeLayoutChanged)

    Q_PROPERTY(bool hasActiveLayout READ hasActiveLayout NOTIFY activeLayoutChanged)

public:
    enum LayoutRoles {
        NameRole = Qt::UserRole + 1,
        IsActiveRole
    };

    explicit LayoutEditorViewModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void loadLayouts();
    Q_INVOKABLE void setActiveLayoutIndex(int index);
    Q_INVOKABLE void saveActiveLayout();
    Q_INVOKABLE void createNewLayout(const QString& name, const QString& targetScreen);
    Q_INVOKABLE void removeActiveLayout();
    Q_INVOKABLE void cloneActiveLayout(const QString& newName);
    Q_INVOKABLE void renameActiveLayout(const QString& newName);
    Q_INVOKABLE void selectBackgroundImage(const QString& fileUrl);

    // Timer Actions
    Q_INVOKABLE void addTimerToLayout(const QString& timerId);
    Q_INVOKABLE void removeTimerFromLayout(const QString& timerId);
    Q_INVOKABLE void updateTimerBounds(const QString& timerId, int x, int y, int w, int h);
    Q_INVOKABLE void updateTimerFont(const QString& timerId, const QString& family, int size, const QColor& color, bool isBold, bool isItalic, int alignment);

    // Getters
    QString layoutName() const;
    QString targetScreen() const;
    int canvasWidth() const;
    int canvasHeight() const;
    QColor backgroundColor() const;
    QString backgroundImage() const;
    QString mediaDir() const;

    QRect textBounds() const;
    QString fontFamily() const;
    int fontSize() const;
    QColor fontColor() const;
    bool isBold() const;
    bool isItalic() const;
    bool allCaps() const;
    int alignment() const;

    bool hasNextText() const;
    QRect nextTextBounds() const;
    QString nextFontFamily() const;
    int nextFontSize() const;
    QColor nextFontColor() const;
    bool nextIsBold() const;
    bool nextIsItalic() const;
    bool nextAllCaps() const;
    int nextAlignment() const;

    QJsonArray activeLayoutTimers() const;

    bool hasActiveLayout() const;

    // Setters
    void setLayoutName(const QString& name);
    void setTargetScreen(const QString& targetScreen);
    void setCanvasWidth(int width);
    void setCanvasHeight(int height);
    void setBackgroundColor(const QColor& color);
    void setBackgroundImage(const QString& image);

    void setTextBounds(const QRect& bounds);
    void setFontFamily(const QString& font);
    void setFontSize(int size);
    void setFontColor(const QColor& color);
    void setIsBold(bool bold);
    void setIsItalic(bool italic);
    void setAllCaps(bool caps);
    void setAlignment(int align);

    void setHasNextText(bool hasNext);
    void setNextTextBounds(const QRect& bounds);
    void setNextFontFamily(const QString& font);
    void setNextFontSize(int size);
    void setNextFontColor(const QColor& color);
    void setNextIsBold(bool bold);
    void setNextIsItalic(bool italic);
    void setNextAllCaps(bool caps);
    void setNextAlignment(int align);

signals:
    void activeLayoutChanged();
    void layoutNameChanged();
    void targetScreenChanged();
    void layoutSaved(const QString& layoutName);

private:
    void loadLayoutFromFile(const QString& filePath);
    
    QList<QString> m_layoutFiles; // just the filenames
    int m_activeIndex = -1;
    SlideLayout m_activeLayout;
};
