import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import fohmedia

Rectangle {
    id: root
    width: ListView.view ? ListView.view.width : 300
    
    // States: 0 = Minimized, 1 = Control, 2 = Edit
    property int uiState: 0
    
    // Model properties mapped
    property string timerId: model.id
    property string timerName: model.name
    property int timerType: model.type // 0: Stopwatch, 1: CountdownDuration, 2: CountdownToTime, 3: Clock
    property string currentString: model.currentString
    property bool isNegative: model.isNegative
    property int timerState: model.state // 0: Stopped, 1: Running, 2: Expired

    height: mainLayout.height + 10
    color: palette.button
    border.color: palette.mid
    radius: 5

    property int timerIndex: -1
    // Either dragArea or dragArea2 dragging
    property bool isDragging: (typeof dragArea !== "undefined" && dragArea.dragging) || (typeof dragArea2 !== "undefined" && dragArea2.dragging)
    property real dragOffsetY: visualTranslate.y

    transform: Translate { id: visualTranslate; y: 0 }
    z: isDragging ? 10 : 0
    
    function formatDuration(ms) {
        let totalSecs = Math.floor(ms / 1000)
        let h = Math.floor(totalSecs / 3600)
        let m = Math.floor((totalSecs % 3600) / 60)
        let s = totalSecs % 60
        
        let pad = (num) => num.toString().padStart(2, '0')
        if (h > 0) {
            return pad(h) + ":" + pad(m) + ":" + pad(s)
        } else {
            return pad(m) + ":" + pad(s)
        }
    }

    function openEditMode() {
        nameField.text = root.timerName
        typeCombo.currentIndex = root.timerType
        
        if (root.timerType === 1) { // CountdownDuration
            let d = model.durationMs / 1000
            hrTumbler.currentIndex = d / 3600
            minTumbler.currentIndex = (d % 3600) / 60
            secTumbler.currentIndex = d % 60
        } else if (root.timerType === 2) { // CountdownToTime
            targetHrTumbler.currentIndex = model.targetTime.getHours() % 12 || 12
            targetMinTumbler.currentIndex = model.targetTime.getMinutes()
            targetSecTumbler.currentIndex = model.targetTime.getSeconds()
            ampmTumbler.currentIndex = model.targetTime.getHours() >= 12 ? 1 : 0
        } else if (root.timerType === 3) { // Clock
            tzField.currentIndex = tzField.find(model.timezone)
            is24Toggle.checked = model.is24Hour
            dstToggle.checked = model.applyDST
            formatCombo.currentIndex = formatCombo.find(model.formatString)
        }
        root.uiState = 2
    }
    
    Component.onCompleted: {
        if (root.timerName === "New Timer") {
            openEditMode()
        }
    }

    ColumnLayout {
        id: mainLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 5
        spacing: 5

        // ==========================================
        // MINIMIZED STATE (uiState === 0)
        // ==========================================
        RowLayout {
            visible: root.uiState === 0
            Layout.fillWidth: true
            
            Label {
                text: "≡"
                color: root.isDragging ? palette.highlight : palette.mid
                font.pixelSize: 18
                Layout.alignment: Qt.AlignVCenter
                
                MouseArea {
                    id: dragArea
                    anchors.fill: parent
                    anchors.margins: -10
                    cursorShape: dragging ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                    preventStealing: true
                    
                    property int globalStartY: 0
                    property int originalIndex: -1
                    property bool dragging: false
                    
                    onPressed: (mouse) => {
                        globalStartY = mapToItem(null, mouse.x, mouse.y).y
                        originalIndex = root.timerIndex
                        dragging = false
                    }
                    onPositionChanged: (mouse) => {
                        let currentGlobalY = mapToItem(null, mouse.x, mouse.y).y
                        let offset = currentGlobalY - globalStartY
                        if (!dragging && Math.abs(offset) > 5) {
                            dragging = true
                        }
                        if (dragging) {
                            let delta = Math.round(offset / root.height)
                            let targetIndex = originalIndex + delta
                            targetIndex = Math.max(0, Math.min(targetIndex, AppContext.timerManager.rowCount() - 1))
                            
                            if (targetIndex !== originalIndex) {
                                let diffY = (targetIndex - originalIndex) * root.height
                                AppContext.timerManager.moveTimer(originalIndex, targetIndex)
                                globalStartY += diffY
                                originalIndex = targetIndex
                                offset = currentGlobalY - globalStartY
                            }
                            visualTranslate.y = offset
                        }
                    }
                    onReleased: (mouse) => {
                        if (dragging) {
                            visualTranslate.y = 0
                            dragging = false
                        }
                    }
                    onCanceled: {
                        visualTranslate.y = 0
                        dragging = false
                    }
                }
            }
            
            Label {
                text: root.timerName
                font.bold: true
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            ToolButton {
                text: "\u2304" // Chevron down
                font.pixelSize: 18
                onClicked: root.uiState = 1
            }
        }
        
        Label {
            visible: root.uiState === 0
            text: root.currentString !== "" ? root.currentString : (root.timerType === 2 ? Qt.formatTime(model.targetTime, "hh:mm:ss AP") : (root.timerType === 1 ? formatDuration(model.durationMs) : "00:00:00"))
            color: root.isNegative ? "red" : "#00ffff" // Electric blue
            font.pixelSize: 24
            font.bold: true
            font.family: "Monospace"
            horizontalAlignment: Text.AlignRight
            Layout.fillWidth: true
            Layout.rightMargin: 10
            
            // Black background for 7-seg look
            Rectangle {
                anchors.fill: parent
                anchors.margins: -5
                color: "black"
                z: -1
                radius: 3
            }
        }

        // ==========================================
        // CONTROL STATE (uiState === 1)
        // ==========================================
        RowLayout {
            visible: root.uiState === 1
            Layout.fillWidth: true
            
            Label {
                text: "≡"
                color: root.isDragging ? palette.highlight : palette.mid
                font.pixelSize: 18
                Layout.alignment: Qt.AlignVCenter
                
                MouseArea {
                    id: dragArea2 // Use a separate ID
                    anchors.fill: parent
                    anchors.margins: -10
                    cursorShape: dragging ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                    preventStealing: true
                    
                    property int globalStartY: 0
                    property int originalIndex: -1
                    property bool dragging: false
                    
                    onPressed: (mouse) => {
                        globalStartY = mapToItem(null, mouse.x, mouse.y).y
                        originalIndex = root.timerIndex
                        dragging = false
                        // Set the main dragArea dragging property manually
                        dragArea.dragging = false 
                    }
                    onPositionChanged: (mouse) => {
                        let currentGlobalY = mapToItem(null, mouse.x, mouse.y).y
                        let offset = currentGlobalY - globalStartY
                        if (!dragging && Math.abs(offset) > 5) {
                            dragging = true
                        }
                        if (dragging) {
                            let delta = Math.round(offset / root.height)
                            let targetIndex = originalIndex + delta
                            targetIndex = Math.max(0, Math.min(targetIndex, AppContext.timerManager.rowCount() - 1))
                            
                            if (targetIndex !== originalIndex) {
                                let diffY = (targetIndex - originalIndex) * root.height
                                AppContext.timerManager.moveTimer(originalIndex, targetIndex)
                                globalStartY += diffY
                                originalIndex = targetIndex
                                offset = currentGlobalY - globalStartY
                            }
                            visualTranslate.y = offset
                        }
                    }
                    onReleased: (mouse) => {
                        if (dragging) {
                            visualTranslate.y = 0
                            dragging = false
                        }
                    }
                    onCanceled: {
                        visualTranslate.y = 0
                        dragging = false
                    }
                }
            }
            
            Label {
                text: root.timerName
                font.bold: true
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            
            // Icon Buttons
            ToolButton {
                text: "\u25B6" // Play
                visible: root.timerType !== 3 // Not Clock
                onClicked: AppContext.timerManager.startTimer(root.timerId)
            }
            ToolButton {
                text: "\u25A0" // Stop
                visible: root.timerType !== 3
                onClicked: AppContext.timerManager.stopTimer(root.timerId)
            }
            ToolButton {
                text: "\u21BB" // Reset
                visible: root.timerType !== 3
                onClicked: AppContext.timerManager.resetTimer(root.timerId)
            }
            ToolButton {
                text: "\u270E" // Edit
                onClicked: {
                    root.openEditMode()
                }
            }
            ToolButton {
                text: "\u2715" // Delete
                onClicked: deleteDialog.open()
            }
            ToolButton {
                text: "\u2303" // Chevron up
                font.pixelSize: 18
                onClicked: root.uiState = 0
            }
        }
        
        Label {
            visible: root.uiState === 1
            text: root.currentString !== "" ? root.currentString : (root.timerType === 2 ? Qt.formatTime(model.targetTime, "hh:mm:ss AP") : (root.timerType === 1 ? formatDuration(model.durationMs) : "00:00:00"))
            color: root.isNegative ? "red" : "#00ffff" // Electric blue
            font.pixelSize: 36 // Larger in control state
            font.bold: true
            font.family: "Monospace"
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
            Layout.margins: 5
            Layout.minimumHeight: 50
            verticalAlignment: Text.AlignVCenter
            
            Rectangle {
                anchors.fill: parent
                color: "black"
                z: -1
                radius: 3
                border.color: "#333333"
                border.width: 2
            }
        }

        // ==========================================
        // EDIT STATE (uiState === 2)
        // ==========================================
        ColumnLayout {
            visible: root.uiState === 2
            Layout.fillWidth: true
            spacing: 10
            
            TextField {
                id: nameField
                Layout.fillWidth: true
                placeholderText: "Timer Name"
            }
            
            ComboBox {
                id: typeCombo
                Layout.fillWidth: true
                model: ["Stopwatch", "Countdown Duration", "Countdown to Time", "Clock"]
            }
            
            // Duration Tumblers (CountdownDuration)
            RowLayout {
                visible: typeCombo.currentIndex === 1
                Layout.alignment: Qt.AlignHCenter
                Tumbler {
                    id: hrTumbler
                    model: 24
                    visibleItemCount: 3
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.NoButton
                        onWheel: (wheel) => {
                            if (wheel.angleDelta.y > 0) hrTumbler.currentIndex = (hrTumbler.currentIndex - 1 + hrTumbler.count) % hrTumbler.count
                            else if (wheel.angleDelta.y < 0) hrTumbler.currentIndex = (hrTumbler.currentIndex + 1) % hrTumbler.count
                        }
                    }
                }
                Label { text: ":" }
                Tumbler {
                    id: minTumbler
                    model: 60
                    visibleItemCount: 3
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.NoButton
                        onWheel: (wheel) => {
                            if (wheel.angleDelta.y > 0) minTumbler.currentIndex = (minTumbler.currentIndex - 1 + minTumbler.count) % minTumbler.count
                            else if (wheel.angleDelta.y < 0) minTumbler.currentIndex = (minTumbler.currentIndex + 1) % minTumbler.count
                        }
                    }
                }
                Label { text: ":" }
                Tumbler {
                    id: secTumbler
                    model: 60
                    visibleItemCount: 3
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.NoButton
                        onWheel: (wheel) => {
                            if (wheel.angleDelta.y > 0) secTumbler.currentIndex = (secTumbler.currentIndex - 1 + secTumbler.count) % secTumbler.count
                            else if (wheel.angleDelta.y < 0) secTumbler.currentIndex = (secTumbler.currentIndex + 1) % secTumbler.count
                        }
                    }
                }
            }
            
            // Time Tumblers (CountdownToTime)
            RowLayout {
                visible: typeCombo.currentIndex === 2
                Layout.alignment: Qt.AlignHCenter
                Tumbler {
                    id: targetHrTumbler
                    model: 12
                    visibleItemCount: 3
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.NoButton
                        onWheel: (wheel) => {
                            if (wheel.angleDelta.y > 0) targetHrTumbler.currentIndex = (targetHrTumbler.currentIndex - 1 + targetHrTumbler.count) % targetHrTumbler.count
                            else if (wheel.angleDelta.y < 0) targetHrTumbler.currentIndex = (targetHrTumbler.currentIndex + 1) % targetHrTumbler.count
                        }
                    }
                }
                Label { text: ":" }
                Tumbler {
                    id: targetMinTumbler
                    model: 60
                    visibleItemCount: 3
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.NoButton
                        onWheel: (wheel) => {
                            if (wheel.angleDelta.y > 0) targetMinTumbler.currentIndex = (targetMinTumbler.currentIndex - 1 + targetMinTumbler.count) % targetMinTumbler.count
                            else if (wheel.angleDelta.y < 0) targetMinTumbler.currentIndex = (targetMinTumbler.currentIndex + 1) % targetMinTumbler.count
                        }
                    }
                }
                Label { text: ":" }
                Tumbler {
                    id: targetSecTumbler
                    model: 60
                    visibleItemCount: 3
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.NoButton
                        onWheel: (wheel) => {
                            if (wheel.angleDelta.y > 0) targetSecTumbler.currentIndex = (targetSecTumbler.currentIndex - 1 + targetSecTumbler.count) % targetSecTumbler.count
                            else if (wheel.angleDelta.y < 0) targetSecTumbler.currentIndex = (targetSecTumbler.currentIndex + 1) % targetSecTumbler.count
                        }
                    }
                }
                Tumbler {
                    id: ampmTumbler
                    model: ["AM", "PM"]
                    visibleItemCount: 3
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.NoButton
                        onWheel: (wheel) => {
                            if (wheel.angleDelta.y > 0) ampmTumbler.currentIndex = (ampmTumbler.currentIndex - 1 + ampmTumbler.count) % ampmTumbler.count
                            else if (wheel.angleDelta.y < 0) ampmTumbler.currentIndex = (ampmTumbler.currentIndex + 1) % ampmTumbler.count
                        }
                    }
                }
            }
            
            // Clock Settings
            GridLayout {
                visible: typeCombo.currentIndex === 3
                columns: 2
                Layout.fillWidth: true
                
                Label { text: "Timezone:" }
                ComboBox {
                    id: tzField
                    Layout.fillWidth: true
                    model: ["Local", "UTC", "America/New_York", "America/Chicago", "America/Denver", "America/Los_Angeles", "Europe/London"]
                    editable: true
                }
                
                Label { text: "24-Hour Time:" }
                Switch { id: is24Toggle }
                
                Label { text: "Apply DST:" }
                Switch { id: dstToggle; checked: true }
                
                Label { text: "Format:" }
                ComboBox {
                    id: formatCombo
                    Layout.fillWidth: true
                    model: ["hh:mm:ss AP", "hh:mm AP", "HH:mm:ss", "HH:mm"]
                    editable: true
                }
            }
            
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true } // Spacer
                Button {
                    text: "\u2713 Set" // Checkmark
                    onClicked: {
                        if (typeCombo.currentIndex === 3) {
                            AppContext.timerManager.updateClock(
                                root.timerId,
                                nameField.text,
                                tzField.currentText,
                                is24Toggle.checked,
                                dstToggle.checked,
                                formatCombo.currentText
                            )
                        } else {
                            let duration = 0
                            let targetTime = new Date()
                            
                            if (typeCombo.currentIndex === 1) {
                                duration = (hrTumbler.currentIndex * 3600 + minTumbler.currentIndex * 60 + secTumbler.currentIndex) * 1000
                            } else if (typeCombo.currentIndex === 2) {
                                let h = targetHrTumbler.currentIndex
                                if (h === 0) h = 12
                                if (ampmTumbler.currentIndex === 1 && h < 12) h += 12
                                if (ampmTumbler.currentIndex === 0 && h === 12) h = 0
                                targetTime.setHours(h)
                                targetTime.setMinutes(targetMinTumbler.currentIndex)
                                targetTime.setSeconds(targetSecTumbler.currentIndex)
                            }
                            
                            AppContext.timerManager.updateTimer(
                                root.timerId,
                                nameField.text,
                                typeCombo.currentIndex,
                                duration,
                                targetTime
                            )
                        }
                        root.uiState = 1 // Return to Control state
                    }
                }
                Button {
                    text: "Cancel"
                    onClicked: root.uiState = 1
                }
            }
        }
    }
    
    MessageDialog {
        id: deleteDialog
        title: "Delete Timer"
        text: "Are you sure you want to delete this timer?"
        buttons: MessageDialog.Yes | MessageDialog.No
        onButtonClicked: function(button, role) {
            if (button === MessageDialog.Yes) {
                AppContext.timerManager.removeTimer(root.timerId)
            }
        }
    }
}
