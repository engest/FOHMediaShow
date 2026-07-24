#include "../inc/LayoutEditorViewModel.h"
#include "../inc/Library.h"
#include "../inc/ShowSerializer.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QUrl>
#include <QFileInfo>

LayoutEditorViewModel::LayoutEditorViewModel(QObject* parent) 
    : QAbstractListModel(parent) {
    Library::ensureDirectories();
    loadLayouts();
}

void LayoutEditorViewModel::loadLayouts() {
    beginResetModel();
    m_layoutFiles.clear();
    QDir dir(Library::layoutsDir());
    m_layoutFiles = dir.entryList({"*.fohl"}, QDir::Files, QDir::Name);
    m_activeIndex = -1;
    endResetModel();
    emit activeLayoutChanged();
}

int LayoutEditorViewModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(m_layoutFiles.size());
}

QVariant LayoutEditorViewModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_layoutFiles.size()) return QVariant();

    QString fileName = m_layoutFiles.at(index.row());
    
    if (role == NameRole) {
        // Strip .fohl
        if (fileName.endsWith(".fohl")) {
            return fileName.left(fileName.length() - 5);
        }
        return fileName;
    } else if (role == IsActiveRole) {
        return index.row() == m_activeIndex;
    }
    
    return QVariant();
}

QHash<int, QByteArray> LayoutEditorViewModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "layoutName";
    roles[IsActiveRole] = "isActive";
    return roles;
}

void LayoutEditorViewModel::setActiveLayoutIndex(int index) {
    if (index >= 0 && index < m_layoutFiles.size()) {
        int oldIndex = m_activeIndex;
        m_activeIndex = index;
        
        if (oldIndex != -1) {
            emit dataChanged(this->index(oldIndex, 0), this->index(oldIndex, 0), {IsActiveRole});
        }
        emit dataChanged(this->index(m_activeIndex, 0), this->index(m_activeIndex, 0), {IsActiveRole});
        
        loadLayoutFromFile(m_layoutFiles.at(m_activeIndex));
    }
}

void LayoutEditorViewModel::loadLayoutFromFile(const QString& fileName) {
    QString path = QDir(Library::layoutsDir()).filePath(fileName);
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            m_activeLayout = SlideLayout::fromJson(doc.object());
            if (fileName.endsWith(".fohl")) {
                m_activeLayout.name = fileName.left(fileName.length() - 5);
            }
            emit activeLayoutChanged();
            emit targetScreenChanged();
            emit layoutNameChanged();
        }
    }
}

void LayoutEditorViewModel::saveActiveLayout() {
    if (m_activeIndex < 0) return;
    
    QString fileName = m_layoutFiles.at(m_activeIndex);
    if (fileName.endsWith(".fohl")) {
        m_activeLayout.name = fileName.left(fileName.length() - 5);
    }
    
    QJsonObject layoutObj = m_activeLayout.toJson();
    QJsonDocument doc(layoutObj);
    
    QString layoutPath = QDir(Library::layoutsDir()).filePath(fileName);
    
    QFile file(layoutPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        emit layoutSaved(m_activeLayout.name);
    }
}

void LayoutEditorViewModel::createNewLayout(const QString& name, const QString& targetScreen) {
    QString fileName = name + ".fohl";
    QString destPath = Library::uniqueDestPath(Library::layoutsDir(), fileName);
    
    SlideLayout newLayout;
    newLayout.name = name;
    newLayout.targetScreen = targetScreen;
    newLayout.width = 1920;
    newLayout.height = 1080;
    newLayout.fontFamily = "Bebas Neue";
    newLayout.fontSize = 90;
    newLayout.fontColor = Qt::white;
    newLayout.backgroundColor = Qt::black;
    newLayout.alignment = Qt::AlignCenter;
    if (newLayout.targetScreen == "Stage") {
        newLayout.textBounds = QRect(0, 0, 1920, 540);
        newLayout.nextTextBounds = QRect(0, 540, 1920, 540);
    } else {
        newLayout.textBounds = QRect(0, 0, 1920, 1080);
    }
    
    ShowSerializer::saveLayoutToFile(newLayout, destPath);
    
    loadLayouts();
    
    // Auto-select the newly created layout
    for (int i = 0; i < m_layoutFiles.size(); ++i) {
        if (QDir(Library::layoutsDir()).filePath(m_layoutFiles.at(i)) == destPath) {
            setActiveLayoutIndex(i);
            break;
        }
    }
}

void LayoutEditorViewModel::removeActiveLayout() {
    if (m_activeIndex == -1) return;
    QString path = QDir(Library::layoutsDir()).filePath(m_layoutFiles.at(m_activeIndex));
    QFile::remove(path);
    loadLayouts();
}

void LayoutEditorViewModel::cloneActiveLayout(const QString& newName) {
    if (m_activeIndex == -1) return;
    
    QString fileName = newName + ".fohl";
    QString destPath = Library::uniqueDestPath(Library::layoutsDir(), fileName);
    
    SlideLayout clonedLayout = m_activeLayout;
    clonedLayout.name = newName;
    ShowSerializer::saveLayoutToFile(clonedLayout, destPath);
    
    loadLayouts();
    
    // Auto-select the cloned layout
    for (int i = 0; i < m_layoutFiles.size(); ++i) {
        if (QDir(Library::layoutsDir()).filePath(m_layoutFiles.at(i)) == destPath) {
            setActiveLayoutIndex(i);
            break;
        }
    }
}

void LayoutEditorViewModel::renameActiveLayout(const QString& newName) {
    if (m_activeIndex == -1) return;
    
    QString oldPath = QDir(Library::layoutsDir()).filePath(m_layoutFiles.at(m_activeIndex));
    
    QString fileName = newName + ".fohl";
    QString newPath = Library::uniqueDestPath(Library::layoutsDir(), fileName);
    
    m_activeLayout.name = newName;
    ShowSerializer::saveLayoutToFile(m_activeLayout, newPath);
    
    QFile::remove(oldPath);
    
    loadLayouts();
    
    // Auto-select the renamed layout
    for (int i = 0; i < m_layoutFiles.size(); ++i) {
        if (QDir(Library::layoutsDir()).filePath(m_layoutFiles.at(i)) == newPath) {
            setActiveLayoutIndex(i);
            break;
        }
    }
}

bool LayoutEditorViewModel::hasActiveLayout() const {
    return m_activeIndex != -1;
}

void LayoutEditorViewModel::selectBackgroundImage(const QString& fileUrl) {
    if (m_activeIndex == -1) return;
    
    QUrl url(fileUrl);
    QString localPath = url.isLocalFile() ? url.toLocalFile() : fileUrl;
    
    QFileInfo fileInfo(localPath);
    if (!fileInfo.exists()) return;
    
    QString destPath = Library::uniqueDestPath(Library::mediaDir(), fileInfo.fileName());
    if (QFile::copy(localPath, destPath)) {
        setBackgroundImage(QFileInfo(destPath).fileName());
    }
}

// --- Getters ---
QString LayoutEditorViewModel::layoutName() const {
    return m_activeLayout.name;
}

QString LayoutEditorViewModel::targetScreen() const {
    return m_activeLayout.targetScreen;
}
int LayoutEditorViewModel::canvasWidth() const { return m_activeLayout.width; }
int LayoutEditorViewModel::canvasHeight() const { return m_activeLayout.height; }
QColor LayoutEditorViewModel::backgroundColor() const { return m_activeLayout.backgroundColor; }
QString LayoutEditorViewModel::backgroundImage() const { return m_activeLayout.backgroundImage; }
QString LayoutEditorViewModel::mediaDir() const { return Library::mediaDir(); }
QRect LayoutEditorViewModel::textBounds() const { return m_activeLayout.textBounds; }
QString LayoutEditorViewModel::fontFamily() const { return m_activeLayout.fontFamily; }
int LayoutEditorViewModel::fontSize() const { return m_activeLayout.fontSize; }
QColor LayoutEditorViewModel::fontColor() const { return m_activeLayout.fontColor; }
bool LayoutEditorViewModel::isBold() const { return m_activeLayout.isBold; }
bool LayoutEditorViewModel::isItalic() const { return m_activeLayout.isItalic; }
bool LayoutEditorViewModel::allCaps() const { return m_activeLayout.allCaps; }
int LayoutEditorViewModel::alignment() const { return static_cast<int>(m_activeLayout.alignment); }

bool LayoutEditorViewModel::hasNextText() const { return m_activeLayout.hasNextText; }
QRect LayoutEditorViewModel::nextTextBounds() const { return m_activeLayout.nextTextBounds; }
QString LayoutEditorViewModel::nextFontFamily() const { return m_activeLayout.nextFontFamily; }
int LayoutEditorViewModel::nextFontSize() const { return m_activeLayout.nextFontSize; }
QColor LayoutEditorViewModel::nextFontColor() const { return m_activeLayout.nextFontColor; }
bool LayoutEditorViewModel::nextIsBold() const { return m_activeLayout.nextIsBold; }
bool LayoutEditorViewModel::nextIsItalic() const { return m_activeLayout.nextIsItalic; }
bool LayoutEditorViewModel::nextAllCaps() const { return m_activeLayout.nextAllCaps; }
int LayoutEditorViewModel::nextAlignment() const { return static_cast<int>(m_activeLayout.nextAlignment); }

// --- Setters ---
// Setters removed or moved down
void LayoutEditorViewModel::setCanvasWidth(int width) {
    if (m_activeLayout.width != width) { m_activeLayout.width = width; saveActiveLayout(); emit activeLayoutChanged(); }
}
void LayoutEditorViewModel::setCanvasHeight(int height) {
    if (m_activeLayout.height != height) { m_activeLayout.height = height; saveActiveLayout(); emit activeLayoutChanged(); }
}
void LayoutEditorViewModel::setBackgroundColor(const QColor& color) {
    if (m_activeLayout.backgroundColor != color) { m_activeLayout.backgroundColor = color; saveActiveLayout(); emit activeLayoutChanged(); }
}
void LayoutEditorViewModel::setBackgroundImage(const QString& image) {
    if (m_activeLayout.backgroundImage != image) { m_activeLayout.backgroundImage = image; saveActiveLayout(); emit activeLayoutChanged(); }
}
void LayoutEditorViewModel::setTextBounds(const QRect& bounds) {
    if (m_activeLayout.textBounds != bounds) { m_activeLayout.textBounds = bounds; saveActiveLayout(); emit activeLayoutChanged(); }
}
void LayoutEditorViewModel::setFontFamily(const QString& font) {
    if (m_activeLayout.fontFamily != font) { m_activeLayout.fontFamily = font; saveActiveLayout(); emit activeLayoutChanged(); }
}
void LayoutEditorViewModel::setFontSize(int size) {
    if (m_activeLayout.fontSize != size) { m_activeLayout.fontSize = size; saveActiveLayout(); emit activeLayoutChanged(); }
}
void LayoutEditorViewModel::setFontColor(const QColor& color) {
    if (m_activeLayout.fontColor != color) { m_activeLayout.fontColor = color; saveActiveLayout(); emit activeLayoutChanged(); }
}
void LayoutEditorViewModel::setIsBold(bool bold) {
    if (m_activeLayout.isBold != bold) { m_activeLayout.isBold = bold; saveActiveLayout(); emit activeLayoutChanged(); }
}
void LayoutEditorViewModel::setIsItalic(bool italic) {
    if (m_activeLayout.isItalic != italic) { m_activeLayout.isItalic = italic; saveActiveLayout(); emit activeLayoutChanged(); }
}
void LayoutEditorViewModel::setAllCaps(bool caps) {
    if (m_activeLayout.allCaps != caps) { m_activeLayout.allCaps = caps; saveActiveLayout(); emit activeLayoutChanged(); }
}
void LayoutEditorViewModel::setLayoutName(const QString& name) {
    if (m_activeIndex >= 0 && m_activeLayout.name != name) {
        m_activeLayout.name = name;
        emit layoutNameChanged();
        saveActiveLayout();
    }
}

void LayoutEditorViewModel::setTargetScreen(const QString& targetScreen) {
    if (m_activeIndex >= 0 && m_activeLayout.targetScreen != targetScreen) {
        m_activeLayout.targetScreen = targetScreen;
        emit targetScreenChanged();
        saveActiveLayout();
    }
}
void LayoutEditorViewModel::setAlignment(int align) {
    if (m_activeIndex >= 0) m_activeLayout.alignment = static_cast<Qt::Alignment>(align);
    emit activeLayoutChanged();
    saveActiveLayout();
}
void LayoutEditorViewModel::setHasNextText(bool hasNext) {
    if (m_activeLayout.hasNextText != hasNext) { m_activeLayout.hasNextText = hasNext; saveActiveLayout(); emit activeLayoutChanged(); }
}
void LayoutEditorViewModel::setNextTextBounds(const QRect& bounds) {
    if (m_activeLayout.nextTextBounds != bounds) { m_activeLayout.nextTextBounds = bounds; saveActiveLayout(); emit activeLayoutChanged(); }
}
void LayoutEditorViewModel::setNextFontFamily(const QString& font) {
    if (m_activeLayout.nextFontFamily != font) { m_activeLayout.nextFontFamily = font; saveActiveLayout(); emit activeLayoutChanged(); }
}
void LayoutEditorViewModel::setNextFontSize(int size) {
    if (m_activeLayout.nextFontSize != size) { m_activeLayout.nextFontSize = size; saveActiveLayout(); emit activeLayoutChanged(); }
}
void LayoutEditorViewModel::setNextFontColor(const QColor& color) {
    if (m_activeLayout.nextFontColor != color) { m_activeLayout.nextFontColor = color; saveActiveLayout(); emit activeLayoutChanged(); }
}
void LayoutEditorViewModel::setNextIsBold(bool bold) {
    if (m_activeLayout.nextIsBold != bold) { m_activeLayout.nextIsBold = bold; saveActiveLayout(); emit activeLayoutChanged(); }
}
void LayoutEditorViewModel::setNextIsItalic(bool italic) {
    if (m_activeLayout.nextIsItalic != italic) { m_activeLayout.nextIsItalic = italic; saveActiveLayout(); emit activeLayoutChanged(); }
}
void LayoutEditorViewModel::setNextAllCaps(bool caps) {
    if (m_activeLayout.nextAllCaps != caps) { m_activeLayout.nextAllCaps = caps; saveActiveLayout(); emit activeLayoutChanged(); }
}
void LayoutEditorViewModel::setNextAlignment(int align) {
    if (m_activeLayout.nextAlignment != static_cast<Qt::Alignment>(align)) { m_activeLayout.nextAlignment = static_cast<Qt::Alignment>(align); saveActiveLayout(); emit activeLayoutChanged(); }
}

QJsonArray LayoutEditorViewModel::activeLayoutTimers() const {
    QJsonArray arr;
    if (m_activeIndex >= 0) {
        for (const auto& timer : m_activeLayout.timers) {
            arr.append(timer.toJson());
        }
    }
    return arr;
}

void LayoutEditorViewModel::addTimerToLayout(const QString& timerId) {
    if (m_activeIndex < 0) return;
    
    // Check if it already exists
    for (const auto& t : m_activeLayout.timers) {
        if (t.timerId == timerId) return;
    }
    
    SlideLayoutTimer timer;
    timer.timerId = timerId;
    timer.bounds = QRect(100, 100, 400, 150);
    timer.fontFamily = "Arial";
    timer.fontSize = 72;
    timer.fontColor = QColor(Qt::white);
    timer.isBold = false;
    timer.isItalic = false;
    timer.alignment = Qt::AlignCenter;
    
    m_activeLayout.timers.append(timer);
    saveActiveLayout();
    emit activeLayoutChanged();
}

void LayoutEditorViewModel::removeTimerFromLayout(const QString& timerId) {
    if (m_activeIndex < 0) return;
    
    for (int i = 0; i < m_activeLayout.timers.size(); ++i) {
        if (m_activeLayout.timers[i].timerId == timerId) {
            m_activeLayout.timers.removeAt(i);
            saveActiveLayout();
            emit activeLayoutChanged();
            return;
        }
    }
}

void LayoutEditorViewModel::updateTimerBounds(const QString& timerId, int x, int y, int w, int h) {
    if (m_activeIndex < 0) return;
    
    for (int i = 0; i < m_activeLayout.timers.size(); ++i) {
        if (m_activeLayout.timers[i].timerId == timerId) {
            if (m_activeLayout.timers[i].bounds == QRect(x, y, w, h)) return;
            m_activeLayout.timers[i].bounds = QRect(x, y, w, h);
            saveActiveLayout();
            emit activeLayoutChanged();
            return;
        }
    }
}

void LayoutEditorViewModel::updateTimerFont(const QString& timerId, const QString& family, int size, const QColor& color, bool isBold, bool isItalic, int alignment) {
    if (m_activeIndex < 0) return;
    
    for (int i = 0; i < m_activeLayout.timers.size(); ++i) {
        if (m_activeLayout.timers[i].timerId == timerId) {
            m_activeLayout.timers[i].fontFamily = family;
            m_activeLayout.timers[i].fontSize = size;
            m_activeLayout.timers[i].fontColor = color;
            m_activeLayout.timers[i].isBold = isBold;
            m_activeLayout.timers[i].isItalic = isItalic;
            m_activeLayout.timers[i].alignment = static_cast<Qt::Alignment>(alignment);
            saveActiveLayout();
            emit activeLayoutChanged();
            return;
        }
    }
}
