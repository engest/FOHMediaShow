import QtQuick
import QtQuick.Controls

ToolTip {
    id: control
    delay: 400

    contentItem: Text {
        text: control.text
        font: control.font
        color: "white"
    }

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#444444"
        radius: 4
        opacity: 0.95
    }
}
