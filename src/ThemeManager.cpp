#include "../inc/ThemeManager.h"
#include <QApplication>
#include <QStyleFactory>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>

QIcon ThemeManager::getEmojiIcon(const QString& emoji, int pointSize) {
    QFont font;
    font.setPointSize(pointSize);
    QFontMetrics fm(font);
    
    int width = fm.horizontalAdvance(emoji);
    int height = fm.height();
    
    QPixmap pixmap(width, height);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, emoji);
    painter.end();
    
    return QIcon(pixmap);
}

void ThemeManager::applyTheme() {
    qApp->setStyle(QStyleFactory::create("Fusion"));
    qApp->setStyleSheet(getGlobalStyleSheet());
}

QString ThemeManager::getGlobalStyleSheet() {
    return R"(
/* ==========================================================
   GLOBAL SETTINGS (Soft Dark Mode)
   ========================================================== */
QWidget {
    background-color: #202124; /* Deep charcoal background */
    color: #E8EAED;            /* Soft off-white text */
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    font-size: 13px;
}

/* Base text elements */
QLabel, QCheckBox, QRadioButton {
    background: transparent;
}

/* ==========================================================
   MAIN WINDOW & PANELS
   ========================================================== */
QMainWindow {
    background-color: #202124;
}

/* Toolbars */
QToolBar {
    background-color: #292A2D;
    border-bottom: 1px solid #3C4043;
    padding: 4px;
    spacing: 12px;
}
QToolBar::separator {
    background-color: #3C4043;
    width: 1px;
    margin: 4px 8px;
}

/* Menus */
QMenuBar {
    background-color: #292A2D;
    border-bottom: 1px solid #3C4043;
}
QMenuBar::item {
    padding: 6px 12px;
    background: transparent;
    border-radius: 4px;
}
QMenuBar::item:selected {
    background-color: #3C4043;
}
QMenu {
    background-color: #292A2D;
    border: 1px solid #3C4043;
    border-radius: 6px;
    padding: 4px 0px;
}
QMenu::item {
    padding: 6px 24px;
}
QMenu::item:selected {
    background-color: #3C4043;
    color: #FFFFFF;
}

/* Status Bar */
QStatusBar {
    background-color: #292A2D;
    border-top: 1px solid #3C4043;
}

/* ==========================================================
   BUTTONS
   ========================================================== */
QPushButton {
    background-color: #3C4043;
    border: 1px solid #5F6368;
    border-radius: 6px;
    padding: 6px 16px;
    color: #E8EAED;
    font-weight: 500;
}
QPushButton:hover {
    background-color: #4A4D51;
    border: 1px solid #757A80;
}
QPushButton:pressed {
    background-color: #5F6368;
}
QPushButton:disabled {
    background-color: #292A2D;
    color: #5F6368;
    border: 1px solid #3C4043;
}

/* Special Primary Button Style */
QPushButton#PrimaryButton {
    background-color: #1A73E8; /* Soft Google Blue */
    border: 1px solid #1A73E8;
    color: #FFFFFF;
}
QPushButton#PrimaryButton:hover {
    background-color: #1B63C6;
    border: 1px solid #1B63C6;
}
QPushButton#PrimaryButton:pressed {
    background-color: #174EA6;
}
QPushButton#PrimaryButton:disabled {
    background-color: #3C4043;
    color: #8AB4F8;
    border: 1px solid #3C4043;
}

/* Special Danger Button Style */
QPushButton#DangerButton {
    background-color: #D93025; /* Soft Red */
    border: 1px solid #D93025;
    color: #FFFFFF;
}
QPushButton#DangerButton:hover {
    background-color: #C5221F;
    border: 1px solid #C5221F;
}
QPushButton#DangerButton:disabled {
    background-color: #292A2D;
    color: #5F6368;
    border: 1px solid #3C4043;
}

/* Quick Edit Button Active State */
QPushButton[isEditing="true"] {
    background-color: #1A73E8;
    border: 1px solid #1A73E8;
}

/* ToolButtons (Toolbar) */
QToolButton {
    background-color: transparent;
    border: 1px solid transparent;
    border-radius: 4px;
    padding: 6px;
    color: #E8EAED;
}
QToolButton:hover {
    background-color: #3C4043;
}
QToolButton:pressed {
    background-color: #5F6368;
}

/* ==========================================================
   Tabs
   ========================================================== */
QTabWidget#ServiceLibraryTabs {
    margin-top: 10px;
}

QTabWidget#ServiceLibraryTabs > QTabBar {
    margin-top: 10px;
}

/* ==========================================================
   INPUTS (ComboBox, LineEdit, SpinBox)
   ========================================================== */
QLineEdit {
    background-color: #292A2D;
    border: 1px solid #5F6368;
    border-radius: 4px;
    padding: 6px 10px;
    color: #E8EAED;
}
QLineEdit:focus {
    border: 1px solid #8AB4F8;
}

QComboBox {
    background-color: #292A2D;
    border: 1px solid #5F6368;
    border-radius: 4px;
    padding: 6px 10px;
    color: #E8EAED;
}
QComboBox:focus {
    border: 1px solid #8AB4F8;
}
QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: top right;
    width: 24px;
    border-left: 1px solid #3C4043;
}
QComboBox::down-arrow {
    image: url(":/down_arrow.png");
    width: 10px; height: 10px;
}

QComboBox QAbstractItemView {
    background-color: #292A2D;
    border: 1px solid #3C4043;
    selection-background-color: #3C4043;
}

QSpinBox, QDoubleSpinBox {
    background-color: #292A2D;
    border: 1px solid #5F6368;
    border-radius: 4px;
    padding: 6px 24px 6px 10px; /* Leave space on the right for buttons */
    color: #E8EAED;
}
QSpinBox:focus, QDoubleSpinBox:focus {
    border: 1px solid #8AB4F8;
}
QSpinBox::up-button, QDoubleSpinBox::up-button, QSpinBox::down-button, QDoubleSpinBox::down-button {
    background-color: #3C4043;
    border-left: 1px solid #5F6368;
    width: 16px;
}
QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover, QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover {
    background-color: #4A4D51;
}
QSpinBox::up-arrow, QDoubleSpinBox::up-arrow {
    image: url(":/up_arrow.png");
    width: 10px; height: 10px;
}
QSpinBox::down-arrow, QDoubleSpinBox::down-arrow {
    image: url(":/down_arrow.png");
    width: 10px; height: 10px;
}

/* Slider (Zoom) */
QSlider#ZoomSlider::groove:horizontal { 
    height: 4px; 
    background: #555555; 
    border-radius: 2px; 
}
QSlider#ZoomSlider::handle:horizontal { 
    background: #1A73E8; 
    width: 14px; 
    height: 14px;
    margin: -5px 0; 
    border-radius: 7px; 
}
QSlider#ZoomSlider::sub-page:horizontal { 
    background: #1A73E8; 
    border-radius: 2px; 
}

/* ==========================================================
   SCROLLBARS
   ========================================================== */
QScrollBar:vertical {
    background: transparent;
    width: 12px;
    margin: 0px 0px 0px 0px;
}
QScrollBar::handle:vertical {
    background: #5F6368;
    min-height: 20px;
    border-radius: 6px;
    margin: 2px;
}
QScrollBar::handle:vertical:hover {
    background: #80868B;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
    background: none;
    border: none;
}
QScrollBar:horizontal {
    background: transparent;
    height: 12px;
    margin: 0px 0px 0px 0px;
}
QScrollBar::handle:horizontal {
    background: #5F6368;
    min-width: 20px;
    border-radius: 6px;
    margin: 2px;
}
QScrollBar::handle:horizontal:hover {
    background: #80868B;
}
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal,
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
    background: none;
    border: none;
}
QAbstractScrollArea::corner {
    background: transparent;
}

/* ==========================================================
   TREE WIDGET (Library)
   ========================================================== */
QTreeView, QListView {
    background-color: #292A2D;
    border: 1px solid #3C4043;
    border-radius: 6px;
    alternate-background-color: #303134;
}
QTreeView::item, QListView::item {
    padding: 6px 8px;
}
QTreeView::item:hover, QListView::item:hover {
    background-color: #3C4043;
}
QTreeView::item:selected, QListView::item:selected {
    background-color: #1A73E8;
    color: #FFFFFF;
}
QHeaderView::section {
    background-color: #303134;
    color: #9AA0A6;
    padding: 6px;
    border: none;
    border-bottom: 1px solid #3C4043;
    border-right: 1px solid #3C4043;
    font-weight: bold;
}

/* ==========================================================
   SLIDE CARDS & SCROLL AREA
   ========================================================== */
QScrollArea#SlideScrollArea {
    background-color: #202124;
    border: none;
}
QWidget#SlideCardContainer {
    background-color: #202124;
}
QWidget#ZoomPanel {
    background-color: #292A2D;
    border-top: 1px solid #3C4043;
    padding: 4px;
}
QWidget#ArrangementPanel {
    background-color: #292A2D;
    border-bottom: 1px solid #3C4043;
    padding: 4px;
}

/* Component Buttons (Available Components) */
QPushButton[isComponentBtn="true"] {
    color: #F2EAD8;
    border-radius: 4px;
    padding: 6px 12px;
    font-weight: bold;
    border: none;
}
QPushButton[isComponentBtn="true"]:hover {
    /* We can't dynamically lighten the background-color via QSS easily without a custom paintEvent, 
       but we can overlay a semi-transparent white on top of the button, or rely on a border change. */
    border: 1px solid rgba(255, 255, 255, 0.4);
}

/* Component Chips */
QFrame[isChip="true"] {
    border-radius: 4px; 
    padding: 2px;
    margin: 2px;
}
QLabel[isChipLabel="true"] {
    color: white; 
    font-weight: bold; 
    border: none;
}


/* The cards themselves */
QWidget#SlideCardWrapper {
    background-color: #000000;
    border: 2px solid #5F6368;
    border-radius: 8px;
    padding: 4px; /* Default padding for base state */
}
QWidget#SlideCardWrapper[state="selected"] {
    border: 4px solid #8AB4F8; /* Soft blue */
    padding: 2px; /* 4px + 2px = 6px total */
}
QWidget#SlideCardWrapper[state="live"] {
    border: 4px solid #81C995; /* Soft green */
    padding: 2px; /* 4px + 2px = 6px total */
}

QWidget#TextContainer {
    background-color: transparent;
}

/* Inner Text Edit */
QTextEdit#SlideTextEdit {
    font-family: 'BebasNeue-Regular', 'Bebas Neue', 'Arial', sans-serif;
    background-color: transparent;
    color: #E8EAED;
    border: none;
    padding: 0px 12px;
}
QTextEdit#SlideTextEdit[isFirst="true"] {
    font-weight: bold;
}

/* Dialogs */
QDialog {
    background-color: #292A2D;
}
QLabel[isHintLabel="true"] {
    color: #9AA0A6;
    font-style: italic;
    font-size: 15px;
}

/* Creator View */
QGroupBox#CreatorMetaPanel, QGroupBox {
    background-color: #292A2D;
    border: 1px solid #3C4043;
    border-radius: 6px;
    margin-top: 10px;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 10px;
    padding: 0 3px 0 3px;
    color: #8AB4F8;
}

QTextEdit#AuthoringTextEdit {
    background-color: #202124;
    border: 1px solid #3C4043;
    border-radius: 6px;
    padding: 10px;
    font-size: 14px;
}

    )";
}

// ArrangementEditorWidget

QString ThemeManager::getCloseButtonStyleSheet() {
    return R"(
QPushButton {
    background-color: rgba(255, 255, 255, %1);
    color: #FFFFFF;
    border: none;
    border-radius: 9px;
    font-size: 14px;
    font-weight: bold;
    padding: 0px;
}
    )";
}

 QMap<QString, QColor> ThemeManager::getComponentBaseColorMap() {
    return {
        {"verse", QColor("#8C7156")},        {"chorus", QColor("#6B7C45")},
        {"bridge", QColor("#8B5E3C")},       {"tag", QColor("#A08C70")},
        {"intro", QColor("#A0784A")},        {"instrumental", QColor("#7A6A52")},
        {"outro", QColor("#5C6E5B")},        {"passthrough", QColor("#1C1C1C")}
    };
}

QString ThemeManager::getSequenceChipStyleSheet(const QString& bgColor) {
    return QString("QFrame#SequenceChip { background-color: %1; border-radius: 4px; padding: 2px; margin: 2px; }").arg(bgColor);
}

// DraggableComponentBtn
QString ThemeManager::getBaseStyle(QColor baseColor) {
    return QString("QPushButton { background-color: %1; color: #F2EAD8; border-radius: 4px;"
            " padding: 6px 12px; font-weight: bold; border: none; }"
        ).arg(baseColor.isValid() ? baseColor.name() : QString("#555555"));
}

QString ThemeManager::getHoverStyle(QColor hoverColor) {
    return QString("QPushButton { background-color: %1; color: #F2EAD8; border-radius: 4px;"
            " padding: 6px 12px; font-weight: bold; border: none; }"
        ).arg(hoverColor.isValid() ? hoverColor.name() : QString("#777777"));
}
