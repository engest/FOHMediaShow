import QtQuick
import fohmedia
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: window
    width: 1920
    height: 1080
    visible: true
    title: qsTr("FOHMedia")

    // Use native window controls on macOS, frameless on others
    flags: Qt.platform.os === "osx" ? Qt.Window : (Qt.Window | Qt.FramelessWindowHint)

    palette.window: "#1e1e1e"
    palette.windowText: "#e0e0e0"
    palette.base: "#181818"
    palette.text: "#e0e0e0"
    palette.button: "#2d2d2d"
    palette.buttonText: "#e0e0e0"
    palette.highlight: "#ff8c00" // Amber/Orange
    palette.highlightedText: "#ffffff"
    palette.dark: "#121212"
    palette.mid: "#3d3d3d"

    property bool forceQuit: false

    onClosing: function(close_event) {
        if (!forceQuit) {
            close_event.accepted = false
            quitDialog.open()
        }
    }

    MessageDialog {
        id: quitDialog
        title: "Confirm Quit"
        text: "Are you sure you want to quit?"
        buttons: MessageDialog.Yes | MessageDialog.No
        onButtonClicked: function(button, role) {
            if (button === MessageDialog.Yes) {
                window.forceQuit = true
                window.close()
                Qt.quit()
            }
        }
    }

    function getComponentColor(name) {
        if (!name) return palette.button;
        
        let trimmedName = name.trim();
        let match = trimmedName.match(/^([a-zA-Z\-]+)\s*(\d*)$/);
        
        let baseType = trimmedName.toLowerCase();
        let level = 0;
        
        if (match) {
            baseType = match[1].toLowerCase();
            if (match[2]) {
                level = parseInt(match[2], 10);
            }
        }
        
        let baseColor = palette.button;
        if (baseType === "verse") baseColor = "#0072C6"; // Blue
        else if (baseType === "chorus") baseColor = "#D8005A"; // Pink/Magenta
        else if (baseType === "bridge") baseColor = "#8A2BE2"; // Purple
        else if (baseType === "prechorus" || baseType === "pre-chorus") baseColor = "#E75480"; // Lighter Pink
        else if (baseType === "tag") baseColor = "#FF0000"; // Red
        else if (baseType === "intro" || baseType === "ending" || baseType === "outro") baseColor = "#BDB76B"; // Yellow/Gold
        else if (baseType === "interlude" || baseType === "vamp" || baseType === "turnaround") baseColor = "#32CD32"; // Green
        else if (baseType === "blank") baseColor = "#000000"; // Black
        
        if (level > 0) {
            let darkenFactor = 1.0 + (Math.min(level, 4) * 0.5);
            return Qt.darker(baseColor, darkenFactor);
        }
        
        return baseColor;
    }

    // Resize Handles and Window Border (hide on macOS)
    Item {
        parent: Overlay.overlay
        anchors.fill: parent
        z: 9999
        visible: Qt.platform.os !== "osx"
        
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.color: palette.mid
            border.width: 1
        }
        
        MouseArea {
            id: topEdge
            height: 5
            anchors { top: parent.top; left: parent.left; right: parent.right; leftMargin: 5; rightMargin: 5 }
            cursorShape: Qt.SizeVerCursor
            onPressed: window.startSystemResize(Qt.TopEdge)
        }
        MouseArea {
            id: bottomEdge
            height: 5
            anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: 5; rightMargin: 5 }
            cursorShape: Qt.SizeVerCursor
            onPressed: window.startSystemResize(Qt.BottomEdge)
        }
        MouseArea {
            id: leftEdge
            width: 5
            anchors { left: parent.left; top: parent.top; bottom: parent.bottom; topMargin: 5; bottomMargin: 5 }
            cursorShape: Qt.SizeHorCursor
            onPressed: window.startSystemResize(Qt.LeftEdge)
        }
        MouseArea {
            id: rightEdge
            width: 5
            anchors { right: parent.right; top: parent.top; bottom: parent.bottom; topMargin: 5; bottomMargin: 5 }
            cursorShape: Qt.SizeHorCursor
            onPressed: window.startSystemResize(Qt.RightEdge)
        }
        MouseArea {
            id: topLeftCorner
            width: 5; height: 5
            anchors { top: parent.top; left: parent.left }
            cursorShape: Qt.SizeFDiagCursor
            onPressed: window.startSystemResize(Qt.TopEdge | Qt.LeftEdge)
        }
        MouseArea {
            id: topRightCorner
            width: 5; height: 5
            anchors { top: parent.top; right: parent.right }
            cursorShape: Qt.SizeBDiagCursor
            onPressed: window.startSystemResize(Qt.TopEdge | Qt.RightEdge)
        }
        MouseArea {
            id: bottomLeftCorner
            width: 5; height: 5
            anchors { bottom: parent.bottom; left: parent.left }
            cursorShape: Qt.SizeBDiagCursor
            onPressed: window.startSystemResize(Qt.BottomEdge | Qt.LeftEdge)
        }
        MouseArea {
            id: bottomRightCorner
            width: 5; height: 5
            anchors { bottom: parent.bottom; right: parent.right }
            cursorShape: Qt.SizeFDiagCursor
            onPressed: window.startSystemResize(Qt.BottomEdge | Qt.RightEdge)
        }
    }

    header: ToolBar {
        height: 50
        // Handle window dragging from the titlebar if frameless
        MouseArea {
            anchors.fill: parent
            enabled: Qt.platform.os !== "osx"
            onPressed: window.startSystemMove()
            onDoubleClicked: {
                if (window.visibility === Window.Maximized) {
                    window.showNormal()
                } else {
                    window.showMaximized()
                }
            }
        }

        RowLayout {
            anchors.fill: parent
            spacing: 2
            
            // Replaces the Drawer with a Row of tasteful navigation buttons
            RowLayout {
                spacing: 1
                Layout.leftMargin: 10
                
                property string currentView: "ServicesView.qml"

                function switchToView(view) {
                    if (currentView !== view) {
                        currentView = view
                        stackView.replace(view)
                    }
                }

                ToolButton {
                    text: "Services"
                    font.pixelSize: 15
                    font.bold: parent.currentView === "ServicesView.qml"
                    palette.buttonText: parent.currentView === "ServicesView.qml" ? window.palette.highlight : window.palette.buttonText
                    onClicked: parent.switchToView("ServicesView.qml")
                }
                ToolButton {
                    text: "Lyrics"
                    font.pixelSize: 15
                    font.bold: parent.currentView === "LyricsEditorView.qml"
                    palette.buttonText: parent.currentView === "LyricsEditorView.qml" ? window.palette.highlight : window.palette.buttonText
                    onClicked: parent.switchToView("LyricsEditorView.qml")
                }
                ToolButton {
                    text: "Layouts"
                    font.pixelSize: 15
                    font.bold: parent.currentView === "LayoutEditorView.qml"
                    palette.buttonText: parent.currentView === "LayoutEditorView.qml" ? window.palette.highlight : window.palette.buttonText
                    onClicked: parent.switchToView("LayoutEditorView.qml")
                }
                ToolButton {
                    text: "Displays"
                    font.pixelSize: 15
                    font.bold: parent.currentView === "DisplaysView.qml"
                    palette.buttonText: parent.currentView === "DisplaysView.qml" ? window.palette.highlight : window.palette.buttonText
                    onClicked: parent.switchToView("DisplaysView.qml")
                }
                ToolButton {
                    text: "Media"
                    font.pixelSize: 15
                    font.bold: parent.currentView === "MediaView.qml"
                    palette.buttonText: parent.currentView === "MediaView.qml" ? window.palette.highlight : window.palette.buttonText
                    onClicked: parent.switchToView("MediaView.qml")
                }
                ToolButton {
                    text: "Settings"
                    font.pixelSize: 15
                    font.bold: parent.currentView === "SettingsView.qml"
                    palette.buttonText: parent.currentView === "SettingsView.qml" ? window.palette.highlight : window.palette.buttonText
                    onClicked: parent.switchToView("SettingsView.qml")
                }
            }
            
            Item { Layout.fillWidth: true } // Spacer
            
            Label {
                text: "FOHMedia"
                font.pixelSize: 18
                font.bold: true
                visible: Qt.platform.os !== "osx"
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
            }
            
            Item { Layout.fillWidth: true } // Spacer
            
            ToolButton {
                text: (AppContext.displayEngine && AppContext.displayEngine.isRunning) ? "\u25A0" : "\u25B6" // Stop or Play icon
                font.pixelSize: 20
                ToolTip.visible: hovered
                ToolTip.text: (AppContext.displayEngine && AppContext.displayEngine.isRunning) ? "Stop Show" : "Present Show"
                palette.buttonText: (AppContext.displayEngine && AppContext.displayEngine.isRunning) ? "#ff4444" : window.palette.buttonText
                onClicked: {
                    if (AppContext.displayEngine && AppContext.displayEngine.isRunning) {
                        AppContext.displayEngine.stop()
                    } else {
                        AppContext.displayEngine.start()
                    }
                }
            }

            // Window Controls (hide on macOS)
            RowLayout {
                visible: Qt.platform.os !== "osx"
                spacing: 0
                
                ToolButton {
                    text: "\u2014" // Minimize
                    onClicked: window.showMinimized()
                }
                ToolButton {
                    text: window.visibility === Window.Maximized ? "\u2750" : "\u25A1" // Restore/Maximize
                    onClicked: {
                        if (window.visibility === Window.Maximized)
                            window.showNormal()
                        else
                            window.showMaximized()
                    }
                }
                ToolButton {
                    text: "\u2715" // Close
                    onClicked: window.close()
                    palette.buttonText: hovered ? "#ff4444" : window.palette.buttonText
                }
            }
        }
    }

    Binding {
        target: AppContext.displayEngine
        property: "transitionType"
        value: AppContext.showModel.defaultTransitionType
    }

    Binding {
        target: AppContext.displayEngine
        property: "transitionDurationMs"
        value: AppContext.showModel.defaultTransitionDurationMs
    }

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: "ServicesView.qml"
    }
}
