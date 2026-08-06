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
void LayoutEditorViewModel::setSelectedElementId(const QString& id) {
    if (m_selectedElementId != id) {
        m_selectedElementId = id;
        emit selectedElementIdChanged();
    }
}

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

QJsonArray LayoutEditorViewModel::activeLayoutCustomElements() const {
    QJsonArray arr;
    if (m_activeIndex >= 0) {
        for (const auto& element : m_activeLayout.customElements) {
            arr.append(element.toJson());
        }
    }
    return arr;
}

void LayoutEditorViewModel::addCustomElementToLayout(const QString& elementId) {
    if (m_activeIndex < 0) return;
    
    for (const auto& el : m_activeLayout.customElements) {
        if (el.elementId == elementId) return;
    }
    
    SlideLayoutElement element;
    element.elementId = elementId;
    element.bounds = QRect(100, 100, 400, 150);
    element.fontFamily = "Bebas Neue";
    element.fontSize = 72;
    element.fontColor = QColor(Qt::white);
    element.isBold = false;
    element.isItalic = false;
    element.allCaps = false;
    element.alignment = Qt::AlignCenter;
    
    m_activeLayout.customElements.append(element);
    saveActiveLayout();
    emit activeLayoutChanged();
}

void LayoutEditorViewModel::removeCustomElementFromLayout(const QString& elementId) {
    if (m_activeIndex < 0) return;
    
    for (int i = 0; i < m_activeLayout.customElements.size(); ++i) {
        if (m_activeLayout.customElements[i].elementId == elementId) {
            m_activeLayout.customElements.removeAt(i);
            saveActiveLayout();
            emit activeLayoutChanged();
            return;
        }
    }
}

void LayoutEditorViewModel::updateCustomElementBounds(const QString& elementId, int x, int y, int w, int h) {
    if (m_activeIndex < 0) return;
    
    for (int i = 0; i < m_activeLayout.customElements.size(); ++i) {
        if (m_activeLayout.customElements[i].elementId == elementId) {
            if (m_activeLayout.customElements[i].bounds == QRect(x, y, w, h)) return;
            m_activeLayout.customElements[i].bounds = QRect(x, y, w, h);
            saveActiveLayout();
            emit activeLayoutChanged();
            return;
        }
    }
}

void LayoutEditorViewModel::updateCustomElementFont(const QString& elementId, const QString& family, int size, const QColor& color, bool isBold, bool isItalic, bool allCaps, int alignment) {
    if (m_activeIndex < 0) return;
    
    for (int i = 0; i < m_activeLayout.customElements.size(); ++i) {
        if (m_activeLayout.customElements[i].elementId == elementId) {
            m_activeLayout.customElements[i].fontFamily = family;
            m_activeLayout.customElements[i].fontSize = size;
            m_activeLayout.customElements[i].fontColor = color;
            m_activeLayout.customElements[i].isBold = isBold;
            m_activeLayout.customElements[i].isItalic = isItalic;
            m_activeLayout.customElements[i].allCaps = allCaps;
            m_activeLayout.customElements[i].alignment = static_cast<Qt::Alignment>(alignment);
            saveActiveLayout();
            emit activeLayoutChanged();
            return;
        }
    }
}
// --- Selected Element Proxies ---
QRect LayoutEditorViewModel::selectedBounds() const {
    if (m_selectedElementId == "mainText") return textBounds();
    if (m_selectedElementId == "nextText") return nextTextBounds();
    if (m_selectedElementId.startsWith("timer_")) {
        QString id = m_selectedElementId.mid(6);
        for (const auto& t : m_activeLayout.timers) if (t.timerId == id) return t.bounds;
    } else {
        for (const auto& e : m_activeLayout.customElements) if (e.elementId == m_selectedElementId) return e.bounds;
    }
    return QRect();
}
void LayoutEditorViewModel::setSelectedBounds(const QRect& val) {
    if (m_selectedElementId == "mainText") { setTextBounds(val); }
    else if (m_selectedElementId == "nextText") { setNextTextBounds(val); }
    else if (m_selectedElementId.startsWith("timer_")) {
        QString id = m_selectedElementId.mid(6);
        for (auto& t : m_activeLayout.timers) if (t.timerId == id) { t.bounds = val; saveActiveLayout(); emit activeLayoutChanged(); emit selectedElementIdChanged(); break; }
    } else {
        for (auto& e : m_activeLayout.customElements) if (e.elementId == m_selectedElementId) { e.bounds = val; saveActiveLayout(); emit activeLayoutChanged(); emit selectedElementIdChanged(); break; }
    }
}
QString LayoutEditorViewModel::selectedFontFamily() const {
    if (m_selectedElementId == "mainText") return fontFamily();
    if (m_selectedElementId == "nextText") return nextFontFamily();
    if (m_selectedElementId.startsWith("timer_")) {
        QString id = m_selectedElementId.mid(6);
        for (const auto& t : m_activeLayout.timers) if (t.timerId == id) return t.fontFamily;
    } else {
        for (const auto& e : m_activeLayout.customElements) if (e.elementId == m_selectedElementId) return e.fontFamily;
    }
    return "";
}
void LayoutEditorViewModel::setSelectedFontFamily(const QString& val) {
    if (m_selectedElementId == "mainText") { setFontFamily(val); }
    else if (m_selectedElementId == "nextText") { setNextFontFamily(val); }
    else if (m_selectedElementId.startsWith("timer_")) {
        QString id = m_selectedElementId.mid(6);
        for (auto& t : m_activeLayout.timers) if (t.timerId == id) { t.fontFamily = val; saveActiveLayout(); emit activeLayoutChanged(); emit selectedElementIdChanged(); break; }
    } else {
        for (auto& e : m_activeLayout.customElements) if (e.elementId == m_selectedElementId) { e.fontFamily = val; saveActiveLayout(); emit activeLayoutChanged(); emit selectedElementIdChanged(); break; }
    }
}
int LayoutEditorViewModel::selectedFontSize() const {
    if (m_selectedElementId == "mainText") return fontSize();
    if (m_selectedElementId == "nextText") return nextFontSize();
    if (m_selectedElementId.startsWith("timer_")) {
        QString id = m_selectedElementId.mid(6);
        for (const auto& t : m_activeLayout.timers) if (t.timerId == id) return t.fontSize;
    } else {
        for (const auto& e : m_activeLayout.customElements) if (e.elementId == m_selectedElementId) return e.fontSize;
    }
    return 0;
}
void LayoutEditorViewModel::setSelectedFontSize(int val) {
    if (m_selectedElementId == "mainText") { setFontSize(val); }
    else if (m_selectedElementId == "nextText") { setNextFontSize(val); }
    else if (m_selectedElementId.startsWith("timer_")) {
        QString id = m_selectedElementId.mid(6);
        for (auto& t : m_activeLayout.timers) if (t.timerId == id) { t.fontSize = val; saveActiveLayout(); emit activeLayoutChanged(); emit selectedElementIdChanged(); break; }
    } else {
        for (auto& e : m_activeLayout.customElements) if (e.elementId == m_selectedElementId) { e.fontSize = val; saveActiveLayout(); emit activeLayoutChanged(); emit selectedElementIdChanged(); break; }
    }
}
QColor LayoutEditorViewModel::selectedFontColor() const {
    if (m_selectedElementId == "mainText") return fontColor();
    if (m_selectedElementId == "nextText") return nextFontColor();
    if (m_selectedElementId.startsWith("timer_")) {
        QString id = m_selectedElementId.mid(6);
        for (const auto& t : m_activeLayout.timers) if (t.timerId == id) return t.fontColor;
    } else {
        for (const auto& e : m_activeLayout.customElements) if (e.elementId == m_selectedElementId) return e.fontColor;
    }
    return Qt::white;
}
void LayoutEditorViewModel::setSelectedFontColor(const QColor& val) {
    if (m_selectedElementId == "mainText") { setFontColor(val); }
    else if (m_selectedElementId == "nextText") { setNextFontColor(val); }
    else if (m_selectedElementId.startsWith("timer_")) {
        QString id = m_selectedElementId.mid(6);
        for (auto& t : m_activeLayout.timers) if (t.timerId == id) { t.fontColor = val; saveActiveLayout(); emit activeLayoutChanged(); emit selectedElementIdChanged(); break; }
    } else {
        for (auto& e : m_activeLayout.customElements) if (e.elementId == m_selectedElementId) { e.fontColor = val; saveActiveLayout(); emit activeLayoutChanged(); emit selectedElementIdChanged(); break; }
    }
}
bool LayoutEditorViewModel::selectedIsBold() const {
    if (m_selectedElementId == "mainText") return isBold();
    if (m_selectedElementId == "nextText") return nextIsBold();
    if (m_selectedElementId.startsWith("timer_")) {
        QString id = m_selectedElementId.mid(6);
        for (const auto& t : m_activeLayout.timers) if (t.timerId == id) return t.isBold;
    } else {
        for (const auto& e : m_activeLayout.customElements) if (e.elementId == m_selectedElementId) return e.isBold;
    }
    return false;
}
void LayoutEditorViewModel::setSelectedIsBold(bool val) {
    if (m_selectedElementId == "mainText") { setIsBold(val); }
    else if (m_selectedElementId == "nextText") { setNextIsBold(val); }
    else if (m_selectedElementId.startsWith("timer_")) {
        QString id = m_selectedElementId.mid(6);
        for (auto& t : m_activeLayout.timers) if (t.timerId == id) { t.isBold = val; saveActiveLayout(); emit activeLayoutChanged(); emit selectedElementIdChanged(); break; }
    } else {
        for (auto& e : m_activeLayout.customElements) if (e.elementId == m_selectedElementId) { e.isBold = val; saveActiveLayout(); emit activeLayoutChanged(); emit selectedElementIdChanged(); break; }
    }
}
bool LayoutEditorViewModel::selectedIsItalic() const {
    if (m_selectedElementId == "mainText") return isItalic();
    if (m_selectedElementId == "nextText") return nextIsItalic();
    if (m_selectedElementId.startsWith("timer_")) {
        QString id = m_selectedElementId.mid(6);
        for (const auto& t : m_activeLayout.timers) if (t.timerId == id) return t.isItalic;
    } else {
        for (const auto& e : m_activeLayout.customElements) if (e.elementId == m_selectedElementId) return e.isItalic;
    }
    return false;
}
void LayoutEditorViewModel::setSelectedIsItalic(bool val) {
    if (m_selectedElementId == "mainText") { setIsItalic(val); }
    else if (m_selectedElementId == "nextText") { setNextIsItalic(val); }
    else if (m_selectedElementId.startsWith("timer_")) {
        QString id = m_selectedElementId.mid(6);
        for (auto& t : m_activeLayout.timers) if (t.timerId == id) { t.isItalic = val; saveActiveLayout(); emit activeLayoutChanged(); emit selectedElementIdChanged(); break; }
    } else {
        for (auto& e : m_activeLayout.customElements) if (e.elementId == m_selectedElementId) { e.isItalic = val; saveActiveLayout(); emit activeLayoutChanged(); emit selectedElementIdChanged(); break; }
    }
}
bool LayoutEditorViewModel::selectedAllCaps() const {
    if (m_selectedElementId == "mainText") return allCaps();
    if (m_selectedElementId == "nextText") return nextAllCaps();
    if (m_selectedElementId.startsWith("timer_")) {
        QString id = m_selectedElementId.mid(6);
        for (const auto& t : m_activeLayout.timers) if (t.timerId == id) return false;
    } else {
        for (const auto& e : m_activeLayout.customElements) if (e.elementId == m_selectedElementId) return e.allCaps;
    }
    return false;
}
void LayoutEditorViewModel::setSelectedAllCaps(bool val) {
    if (m_selectedElementId == "mainText") { setAllCaps(val); }
    else if (m_selectedElementId == "nextText") { setNextAllCaps(val); }
    else if (m_selectedElementId.startsWith("timer_")) {
        QString id = m_selectedElementId.mid(6);
        for (auto& t : m_activeLayout.timers) if (t.timerId == id) {  saveActiveLayout(); emit activeLayoutChanged(); emit selectedElementIdChanged(); break; }
    } else {
        for (auto& e : m_activeLayout.customElements) if (e.elementId == m_selectedElementId) { e.allCaps = val; saveActiveLayout(); emit activeLayoutChanged(); emit selectedElementIdChanged(); break; }
    }
}
int LayoutEditorViewModel::selectedAlignment() const {
    if (m_selectedElementId == "mainText") return alignment();
    if (m_selectedElementId == "nextText") return nextAlignment();
    if (m_selectedElementId.startsWith("timer_")) {
        QString id = m_selectedElementId.mid(6);
        for (const auto& t : m_activeLayout.timers) if (t.timerId == id) return static_cast<int>(t.alignment);
    } else {
        for (const auto& e : m_activeLayout.customElements) if (e.elementId == m_selectedElementId) return static_cast<int>(e.alignment);
    }
    return 0;
}
void LayoutEditorViewModel::setSelectedAlignment(int val) {
    if (m_selectedElementId == "mainText") { setAlignment(val); }
    else if (m_selectedElementId == "nextText") { setNextAlignment(val); }
    else if (m_selectedElementId.startsWith("timer_")) {
        QString id = m_selectedElementId.mid(6);
        for (auto& t : m_activeLayout.timers) if (t.timerId == id) { t.alignment = static_cast<Qt::Alignment>(val); saveActiveLayout(); emit activeLayoutChanged(); emit selectedElementIdChanged(); break; }
    } else {
        for (auto& e : m_activeLayout.customElements) if (e.elementId == m_selectedElementId) { e.alignment = static_cast<Qt::Alignment>(val); saveActiveLayout(); emit activeLayoutChanged(); emit selectedElementIdChanged(); break; }
    }
}
