import QtQuick
import fohmedia
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    id: root
    
    property string layoutName: "New Layout"
    property string selectedItemId: "mainText"

    Component.onCompleted: {
        AppContext.layoutEditorModel.reloadLayouts()
        AppContext.screenModel.reloadScreens()
    }

    SplitView {
        anchors.fill: parent

        // Left Pane: Layout Library
        Rectangle {
            SplitView.minimumWidth: 200
            SplitView.preferredWidth: 300
            SplitView.maximumWidth: 400
            color: palette.base

            ColumnLayout {
                anchors.fill: parent

                RowLayout {
                    Layout.fillWidth: true
                    Layout.margins: 8
                    Label {
                        text: "Layout Library"
                        font.bold: true
                        font.pixelSize: 16
                        Layout.fillWidth: true
                    }
                    ToolButton {
                        text: "⊕"
                        font.pixelSize: 20
                        ToolTip.text: "New Layout"
                        ToolTip.visible: hovered
                        onClicked: newLayoutDialog.open()
                    }
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: AppContext.layoutEditorModel
                    clip: true

                    delegate: ItemDelegate {
                        width: ListView.view.width
                        text: model.layoutName
                        highlighted: model.isActive
                        onClicked: AppContext.layoutEditorModel.setActiveLayoutIndex(index)
                    }
                }
            }
        }

        // Center Pane: Canvas Workspace
        Rectangle {
            SplitView.fillWidth: true
            SplitView.minimumWidth: 400
            color: palette.window
            clip: true

            // The scaling container
            Item {
                id: canvasContainer
                anchors.centerIn: parent
                // Maintain dynamic aspect ratio based on layout size
                property real targetWidth: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.canvasWidth : 1920
                property real targetHeight: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.canvasHeight : 1080
                
                width: targetWidth
                height: targetHeight

                scale: Math.min((parent.width - 80) / targetWidth, (parent.height - 80) / targetHeight)

                Rectangle {
                    anchors.fill: parent
                    color: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.backgroundColor : "black"
                    
                    Image {
                        anchors.fill: parent
                        source: {
                            if (!AppContext.layoutEditorModel.hasActiveLayout || AppContext.layoutEditorModel.backgroundImage === "") {
                                return ""
                            }
                            if (AppContext.layoutEditorModel.backgroundImage.indexOf("/") !== -1) {
                                return "file://" + AppContext.layoutEditorModel.backgroundImage
                            } else {
                                return "file://" + AppContext.layoutEditorModel.mediaDir + "/" + AppContext.layoutEditorModel.backgroundImage
                            }
                        }
                        fillMode: Image.PreserveAspectCrop
                        visible: AppContext.layoutEditorModel.hasActiveLayout && AppContext.layoutEditorModel.backgroundImage !== ""
                    }

                    // Main Text Bounds
                    ResizableCanvasItem {
                        id: mainTextResizer
                        visible: AppContext.layoutEditorModel.hasActiveLayout
                        
                        property bool updatingFromModel: false
                        
                        Connections {
                            target: AppContext.layoutEditorModel
                            function onActiveLayoutChanged() {
                                if (AppContext.layoutEditorModel.hasActiveLayout) {
                                    mainTextResizer.updatingFromModel = true;
                                    let b = AppContext.layoutEditorModel.textBounds;
                                    mainTextResizer.x = b.x;
                                    mainTextResizer.y = b.y;
                                    mainTextResizer.width = b.width;
                                    mainTextResizer.height = b.height;
                                    mainTextResizer.updatingFromModel = false;
                                }
                            }
                        }
                        Component.onCompleted: {
                            if (AppContext.layoutEditorModel.hasActiveLayout) {
                                updatingFromModel = true;
                                let b = AppContext.layoutEditorModel.textBounds;
                                x = b.x; y = b.y; width = b.width; height = b.height;
                                updatingFromModel = false;
                            }
                        }
                        
                        isSelected: root.selectedItemId === "mainText"
                        onClicked: root.selectedItemId = "mainText"
                        
                        strokeColor: "#ff8c00"
                        
                        // Fake text preview
                        Label {
                            anchors.fill: parent
                            text: "Main Text Box"
                            color: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.fontColor : "white"
                            font.family: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.fontFamily : "sans-serif"
                            font.pixelSize: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.fontSize : 40
                            font.bold: AppContext.layoutEditorModel.hasActiveLayout && AppContext.layoutEditorModel.isBold
                            font.italic: AppContext.layoutEditorModel.hasActiveLayout && AppContext.layoutEditorModel.isItalic
                            font.capitalization: AppContext.layoutEditorModel.hasActiveLayout && AppContext.layoutEditorModel.allCaps ? Font.AllUppercase : Font.MixedCase
                            horizontalAlignment: AppContext.layoutEditorModel.hasActiveLayout ? (AppContext.layoutEditorModel.alignment & (Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight) || Text.AlignHCenter) : Text.AlignHCenter
                            verticalAlignment: AppContext.layoutEditorModel.hasActiveLayout ? (AppContext.layoutEditorModel.alignment & (Qt.AlignTop | Qt.AlignVCenter | Qt.AlignBottom) || Text.AlignVCenter) : Text.AlignVCenter
                            wrapMode: Text.WordWrap
                        }

                        onBoundsChanged: function(newX, newY, newW, newH) {
                            if (!updatingFromModel && AppContext.layoutEditorModel.hasActiveLayout) {
                                let rect = Qt.rect(newX, newY, newW, newH)
                                if (AppContext.layoutEditorModel.textBounds !== rect) {
                                    AppContext.layoutEditorModel.textBounds = rect
                                }
                            }
                        }

                    }
                    
                    // Next Text Bounds
                    ResizableCanvasItem {
                        id: nextTextResizer
                        visible: AppContext.layoutEditorModel.hasActiveLayout && AppContext.layoutEditorModel.hasNextText
                        property bool updatingFromModel: false
                        
                        Connections {
                            target: AppContext.layoutEditorModel
                            function onActiveLayoutChanged() {
                                if (AppContext.layoutEditorModel.hasActiveLayout) {
                                    nextTextResizer.updatingFromModel = true;
                                    let b = AppContext.layoutEditorModel.nextTextBounds;
                                    nextTextResizer.x = b.x;
                                    nextTextResizer.y = b.y;
                                    nextTextResizer.width = b.width;
                                    nextTextResizer.height = b.height;
                                    nextTextResizer.updatingFromModel = false;
                                }
                            }
                        }
                        Component.onCompleted: {
                            if (AppContext.layoutEditorModel.hasActiveLayout) {
                                updatingFromModel = true;
                                let b = AppContext.layoutEditorModel.nextTextBounds;
                                x = b.x; y = b.y; width = b.width; height = b.height;
                                updatingFromModel = false;
                            }
                        }
                        
                        isSelected: root.selectedItemId === "nextText"
                        onClicked: root.selectedItemId = "nextText"
                        
                        strokeColor: "#00bfff" // light blue to distinguish
                        
                        Label {
                            anchors.fill: parent
                            text: "Next Text Box"
                            color: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.nextFontColor : "white"
                            font.family: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.nextFontFamily : "sans-serif"
                            font.pixelSize: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.nextFontSize : 30
                            font.bold: AppContext.layoutEditorModel.hasActiveLayout && AppContext.layoutEditorModel.nextIsBold
                            font.italic: AppContext.layoutEditorModel.hasActiveLayout && AppContext.layoutEditorModel.nextIsItalic
                            font.capitalization: AppContext.layoutEditorModel.hasActiveLayout && AppContext.layoutEditorModel.nextAllCaps ? Font.AllUppercase : Font.MixedCase
                            horizontalAlignment: AppContext.layoutEditorModel.hasActiveLayout ? (AppContext.layoutEditorModel.nextAlignment & (Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight) || Text.AlignHCenter) : Text.AlignHCenter
                            verticalAlignment: AppContext.layoutEditorModel.hasActiveLayout ? (AppContext.layoutEditorModel.nextAlignment & (Qt.AlignTop | Qt.AlignVCenter | Qt.AlignBottom) || Text.AlignVCenter) : Text.AlignVCenter
                            wrapMode: Text.WordWrap
                        }

                        onBoundsChanged: function(newX, newY, newW, newH) {
                            if (!updatingFromModel && AppContext.layoutEditorModel.hasActiveLayout) {
                                let rect = Qt.rect(newX, newY, newW, newH)
                                if (AppContext.layoutEditorModel.nextTextBounds !== rect) {
                                    AppContext.layoutEditorModel.nextTextBounds = rect
                                }
                            }
                        }
                    }

                    // Render Timers
                    Repeater {
                        model: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.activeLayoutTimers : []
                        
                        delegate: ResizableCanvasItem {
                            id: timerResizer
                            z: 10
                            visible: true
                            property var timerData: modelData
                            
                            Component.onCompleted: {
                                x = timerData.bounds.x
                                y = timerData.bounds.y
                                width = timerData.bounds.width
                                height = timerData.bounds.height
                            }
                            
                            isSelected: root.selectedItemId === "timer_" + timerData.timerId
                            onClicked: root.selectedItemId = "timer_" + timerData.timerId
                            
                            strokeColor: "#ff00ff" // magenta
                            
                            Label {
                                anchors.fill: parent
                                text: AppContext.timerManager.getTimer(timerData.timerId).name || "Timer"
                                color: timerData.fontColor
                                font.family: timerData.fontFamily
                                font.pixelSize: timerData.fontSize
                                font.bold: timerData.isBold
                                font.italic: timerData.isItalic
                                horizontalAlignment: (timerData.alignment & (Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) || Text.AlignHCenter
                                verticalAlignment: (timerData.alignment & (Qt.AlignTop | Qt.AlignVCenter | Qt.AlignBottom)) || Text.AlignVCenter
                                wrapMode: Text.WordWrap
                            }
                            
                            onBoundsChanged: function(newX, newY, newW, newH) {
                                AppContext.layoutEditorModel.updateTimerBounds(timerData.timerId, newX, newY, newW, newH)
                            }
                        }
                    }

                }
            }
        }

        // Right Pane: Inspector
        Rectangle {
            SplitView.minimumWidth: 250
            SplitView.preferredWidth: 300
            color: palette.base

            ScrollView {
                anchors.fill: parent
                contentWidth: availableWidth
                contentHeight: mainColumn.implicitHeight + 32
                clip: true
                
                ColumnLayout {
                    id: mainColumn
                    width: parent.width - 32 // 16px margins on both sides
                    x: 16
                    y: 16
                    spacing: 12

                    Label {
                        text: "Layout Design"
                        font.bold: true
                        font.pixelSize: 16
                    }
                    
                    Label {
                        text: "Select a layout to edit properties."
                        visible: !AppContext.layoutEditorModel.hasActiveLayout
                        color: palette.mid
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }

                    // Properties when layout is selected
                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: AppContext.layoutEditorModel.hasActiveLayout
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            Label { text: "Name"; color: palette.mid }
                            TextField {
                                Layout.fillWidth: true
                                text: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.layoutName : ""
                                onEditingFinished: {
                                    if (text !== "" && text !== AppContext.layoutEditorModel.layoutName) {
                                        AppContext.layoutEditorModel.renameActiveLayout(text)
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Button {
                                text: "Clone"
                                Layout.fillWidth: true
                                onClicked: cloneLayoutDialog.open()
                            }
                            Button {
                                text: "Remove"
                                Layout.fillWidth: true
                                onClicked: deleteLayoutDialog.open()
                            }
                        }
                        
                        Rectangle { Layout.fillWidth: true; height: 1; color: palette.mid; opacity: 0.2; Layout.topMargin: 4; Layout.bottomMargin: 4 }

                        Label { text: "Canvas Dimensions"; font.bold: true; color: palette.highlight; topPadding: 8 }
                        RowLayout {
                            Layout.fillWidth: true
                            SpinBox {
                                Layout.fillWidth: true
                                from: 100; to: 10000; editable: true
                                value: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.canvasWidth : 1920
                                onValueModified: AppContext.layoutEditorModel.canvasWidth = value
                            }
                            SpinBox {
                                Layout.fillWidth: true
                                from: 100; to: 10000; editable: true
                                value: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.canvasHeight : 1080
                                onValueModified: AppContext.layoutEditorModel.canvasHeight = value
                            }
                        }

                        Rectangle { Layout.fillWidth: true; height: 1; color: palette.mid; opacity: 0.2; Layout.topMargin: 8; Layout.bottomMargin: 8 }

                        Label { text: "Background"; font.bold: true; color: palette.highlight }
                        Button {
                            Layout.fillWidth: true
                            text: "Background Color"
                            onClicked: {
                                bgColorDialog.selectedColor = AppContext.layoutEditorModel.backgroundColor
                                bgColorDialog.open()
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            visible: AppContext.layoutEditorModel.hasActiveLayout
                            
                            Button {
                                Layout.fillWidth: true
                                text: "Background Image"
                                onClicked: bgImageDialog.open()
                            }
                            Button {
                                text: "✖"
                                visible: AppContext.layoutEditorModel.backgroundImage !== ""
                                ToolTip.text: "Clear Background Image"
                                ToolTip.visible: hovered
                                onClicked: AppContext.layoutEditorModel.backgroundImage = ""
                            }
                        }

                        Rectangle { Layout.fillWidth: true; height: 1; color: palette.mid; opacity: 0.2; Layout.topMargin: 8; Layout.bottomMargin: 8 }

                        Label { text: "Screen for Layout"; color: palette.mid }
                        ComboBox {
                            Layout.fillWidth: true
                            model: AppContext.screenModel.getScreenNames()
                            currentIndex: {
                                let names = AppContext.screenModel.getScreenNames()
                                return names.indexOf(AppContext.layoutEditorModel.targetScreen)
                            }
                            onActivated: AppContext.layoutEditorModel.targetScreen = currentText
                        }
                        
                        Rectangle { Layout.fillWidth: true; height: 1; color: palette.mid; opacity: 0.2 }

                        // GLOBAL SELECTED ITEM TOOLS
                        Label { text: "Selected Object Tools"; font.bold: true; color: palette.highlight; topPadding: 8 }
                        RowLayout {
                            Layout.fillWidth: true
                            ToolButton {
                                text: "\u2194" // Left-Right Arrow
                                ToolTip.text: "Center Horizontally"
                                ToolTip.visible: hovered
                                Layout.fillWidth: true
                                onClicked: {
                                    if (root.selectedItemId === "mainText") {
                                        let b = AppContext.layoutEditorModel.textBounds;
                                        AppContext.layoutEditorModel.textBounds = Qt.rect((AppContext.layoutEditorModel.canvasWidth - b.width) / 2, b.y, b.width, b.height);
                                    } else if (root.selectedItemId === "nextText") {
                                        let b = AppContext.layoutEditorModel.nextTextBounds;
                                        AppContext.layoutEditorModel.nextTextBounds = Qt.rect((AppContext.layoutEditorModel.canvasWidth - b.width) / 2, b.y, b.width, b.height);
                                    } else if (root.selectedItemId.startsWith("timer_")) {
                                        let tId = root.selectedItemId.substring(6);
                                        let timers = AppContext.layoutEditorModel.activeLayoutTimers;
                                        for (let i = 0; i < timers.length; i++) {
                                            if (timers[i].timerId === tId) {
                                                let b = timers[i].bounds;
                                                AppContext.layoutEditorModel.updateTimerBounds(tId, (AppContext.layoutEditorModel.canvasWidth - b.width) / 2, b.y, b.width, b.height);
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                            ToolButton {
                                text: "\u2195" // Up-Down Arrow
                                ToolTip.text: "Center Vertically"
                                ToolTip.visible: hovered
                                Layout.fillWidth: true
                                onClicked: {
                                    if (root.selectedItemId === "mainText") {
                                        let b = AppContext.layoutEditorModel.textBounds;
                                        AppContext.layoutEditorModel.textBounds = Qt.rect(b.x, (AppContext.layoutEditorModel.canvasHeight - b.height) / 2, b.width, b.height);
                                    } else if (root.selectedItemId === "nextText") {
                                        let b = AppContext.layoutEditorModel.nextTextBounds;
                                        AppContext.layoutEditorModel.nextTextBounds = Qt.rect(b.x, (AppContext.layoutEditorModel.canvasHeight - b.height) / 2, b.width, b.height);
                                    } else if (root.selectedItemId.startsWith("timer_")) {
                                        let tId = root.selectedItemId.substring(6);
                                        let timers = AppContext.layoutEditorModel.activeLayoutTimers;
                                        for (let i = 0; i < timers.length; i++) {
                                            if (timers[i].timerId === tId) {
                                                let b = timers[i].bounds;
                                                AppContext.layoutEditorModel.updateTimerBounds(tId, b.x, (AppContext.layoutEditorModel.canvasHeight - b.height) / 2, b.width, b.height);
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        
                        Rectangle { Layout.fillWidth: true; height: 1; color: palette.mid; opacity: 0.2; Layout.topMargin: 8; Layout.bottomMargin: 8 }

                        // MAIN TEXT BOX PROPERTIES
                        Label { text: "Main Text Box"; font.bold: true; color: palette.highlight; topPadding: 8 }

                        Button {
                            Layout.fillWidth: true
                            text: "Text Color"
                            onClicked: {
                                mainFontColorDialog.selectedColor = AppContext.layoutEditorModel.fontColor
                                mainFontColorDialog.open()
                            }
                        }

                        Label { text: "Font Family"; color: palette.mid }
                        ComboBox {
                            Layout.fillWidth: true
                            model: Qt.fontFamilies()
                            currentIndex: {
                                let c = count; // Force re-evaluation after model populates
                                return AppContext.layoutEditorModel.hasActiveLayout ? find(AppContext.layoutEditorModel.fontFamily) : -1
                            }
                            onActivated: AppContext.layoutEditorModel.fontFamily = currentText
                        }

                        Label { text: "Font Size"; color: palette.mid }
                        SpinBox {
                            Layout.fillWidth: true
                            from: 8
                            to: 500
                            editable: true
                            value: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.fontSize : 40
                            onValueModified: AppContext.layoutEditorModel.fontSize = value
                        }

                        Label { text: "Font Style"; color: palette.mid }
                        RowLayout {
                            Layout.fillWidth: true
                            ToolButton {
                                text: "B"
                                font.bold: true
                                checkable: true
                                checked: AppContext.layoutEditorModel.hasActiveLayout && AppContext.layoutEditorModel.isBold
                                onClicked: AppContext.layoutEditorModel.isBold = checked
                            }
                            ToolButton {
                                text: "I"
                                font.italic: true
                                checkable: true
                                checked: AppContext.layoutEditorModel.hasActiveLayout && AppContext.layoutEditorModel.isItalic
                                onClicked: AppContext.layoutEditorModel.isItalic = checked
                            }
                            ToolButton {
                                text: "Aa"
                                font.capitalization: Font.AllUppercase
                                checkable: true
                                checked: AppContext.layoutEditorModel.hasActiveLayout && AppContext.layoutEditorModel.allCaps
                                onClicked: AppContext.layoutEditorModel.allCaps = checked
                            }
                        }

                        Label { text: "Horizontal Alignment"; color: palette.mid }
                        RowLayout {
                            Layout.fillWidth: true
                            ToolButton {
                                text: "L"
                                checked: AppContext.layoutEditorModel.hasActiveLayout && (AppContext.layoutEditorModel.alignment & (Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) === Qt.AlignLeft
                                onClicked: AppContext.layoutEditorModel.alignment = (AppContext.layoutEditorModel.alignment & ~(Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) | Qt.AlignLeft
                            }
                            ToolButton {
                                text: "C"
                                checked: AppContext.layoutEditorModel.hasActiveLayout && (AppContext.layoutEditorModel.alignment & (Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) === Qt.AlignHCenter
                                onClicked: AppContext.layoutEditorModel.alignment = (AppContext.layoutEditorModel.alignment & ~(Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) | Qt.AlignHCenter
                            }
                            ToolButton {
                                text: "R"
                                checked: AppContext.layoutEditorModel.hasActiveLayout && (AppContext.layoutEditorModel.alignment & (Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) === Qt.AlignRight
                                onClicked: AppContext.layoutEditorModel.alignment = (AppContext.layoutEditorModel.alignment & ~(Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) | Qt.AlignRight
                            }
                        }

                        Label { text: "Vertical Alignment"; color: palette.mid }
                        RowLayout {
                            Layout.fillWidth: true
                            ToolButton {
                                text: "T"
                                checked: AppContext.layoutEditorModel.hasActiveLayout && (AppContext.layoutEditorModel.alignment & (Qt.AlignTop | Qt.AlignVCenter | Qt.AlignBottom)) === Qt.AlignTop
                                onClicked: AppContext.layoutEditorModel.alignment = (AppContext.layoutEditorModel.alignment & ~(Qt.AlignTop | Qt.AlignVCenter | Qt.AlignBottom)) | Qt.AlignTop
                            }
                            ToolButton {
                                text: "M"
                                checked: AppContext.layoutEditorModel.hasActiveLayout && (AppContext.layoutEditorModel.alignment & (Qt.AlignTop | Qt.AlignVCenter | Qt.AlignBottom)) === Qt.AlignVCenter
                                onClicked: AppContext.layoutEditorModel.alignment = (AppContext.layoutEditorModel.alignment & ~(Qt.AlignTop | Qt.AlignVCenter | Qt.AlignBottom)) | Qt.AlignVCenter
                            }
                            ToolButton {
                                text: "B"
                                checked: AppContext.layoutEditorModel.hasActiveLayout && (AppContext.layoutEditorModel.alignment & (Qt.AlignTop | Qt.AlignVCenter | Qt.AlignBottom)) === Qt.AlignBottom
                                onClicked: AppContext.layoutEditorModel.alignment = (AppContext.layoutEditorModel.alignment & ~(Qt.AlignTop | Qt.AlignVCenter | Qt.AlignBottom)) | Qt.AlignBottom
                            }
                        }

                        Label { text: "Bounds (X, Y, W, H)"; color: palette.mid }
                        RowLayout {
                            Layout.fillWidth: true
                            SpinBox {
                                Layout.fillWidth: true
                                from: -5000; to: 5000; editable: true
                                value: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.textBounds.x : 0
                                onValueModified: {
                                    if (AppContext.layoutEditorModel.hasActiveLayout) {
                                        let b = AppContext.layoutEditorModel.textBounds;
                                        AppContext.layoutEditorModel.textBounds = Qt.rect(value, b.y, b.width, b.height);
                                    }
                                }
                            }
                            SpinBox {
                                Layout.fillWidth: true
                                from: -5000; to: 5000; editable: true
                                value: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.textBounds.y : 0
                                onValueModified: {
                                    if (AppContext.layoutEditorModel.hasActiveLayout) {
                                        let b = AppContext.layoutEditorModel.textBounds;
                                        AppContext.layoutEditorModel.textBounds = Qt.rect(b.x, value, b.width, b.height);
                                    }
                                }
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            SpinBox {
                                Layout.fillWidth: true
                                from: 0; to: 5000; editable: true
                                value: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.textBounds.width : 0
                                onValueModified: {
                                    if (AppContext.layoutEditorModel.hasActiveLayout) {
                                        let b = AppContext.layoutEditorModel.textBounds;
                                        AppContext.layoutEditorModel.textBounds = Qt.rect(b.x, b.y, value, b.height);
                                    }
                                }
                            }
                            SpinBox {
                                Layout.fillWidth: true
                                from: 0; to: 5000; editable: true
                                value: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.textBounds.height : 0
                                onValueModified: {
                                    if (AppContext.layoutEditorModel.hasActiveLayout) {
                                        let b = AppContext.layoutEditorModel.textBounds;
                                        AppContext.layoutEditorModel.textBounds = Qt.rect(b.x, b.y, b.width, value);
                                    }
                                }
                            }
                        }

                        // NEXT TEXT BOX PROPERTIES
                        ColumnLayout {
                            Layout.fillWidth: true
                            visible: AppContext.layoutEditorModel.hasActiveLayout
                            spacing: 8
                            
                            Rectangle { Layout.fillWidth: true; height: 1; color: palette.mid; opacity: 0.2; Layout.topMargin: 8; Layout.bottomMargin: 8 }

                            RowLayout {
                                Layout.fillWidth: true
                                Switch {
                                    id: nextTextSwitch
                                    text: "Enable Next Text"
                                    checked: AppContext.layoutEditorModel.hasNextText
                                    onCheckedChanged: {
                                        if (AppContext.layoutEditorModel.hasActiveLayout) {
                                            AppContext.layoutEditorModel.hasNextText = checked;
                                        }
                                    }

                                    indicator: Rectangle {
                                        implicitWidth: 40
                                        implicitHeight: 22
                                        x: nextTextSwitch.leftPadding
                                        y: parent.height / 2 - height / 2
                                        radius: 11
                                        color: nextTextSwitch.checked ? palette.highlight : palette.mid
                                        border.color: Qt.darker(color, 1.2)
                                        border.width: 1

                                        Rectangle {
                                            x: nextTextSwitch.checked ? parent.width - width - 2 : 2
                                            y: 2
                                            width: 16
                                            height: 16
                                            radius: 8
                                            color: "white"
                                            
                                            Behavior on x {
                                                NumberAnimation { duration: 150; easing.type: Easing.OutQuad }
                                            }
                                        }
                                    }

                                    contentItem: Label {
                                        text: nextTextSwitch.text
                                        font: nextTextSwitch.font
                                        verticalAlignment: Text.AlignVCenter
                                        leftPadding: nextTextSwitch.indicator.width + nextTextSwitch.spacing
                                    }
                                }
                            }
                            
                            ColumnLayout {
                                Layout.fillWidth: true
                                visible: AppContext.layoutEditorModel.hasNextText
                                spacing: 8

                            Button {
                                Layout.fillWidth: true
                                text: "Text Color"
                                onClicked: {
                                    nextFontColorDialog.selectedColor = AppContext.layoutEditorModel.nextFontColor
                                    nextFontColorDialog.open()
                                }
                            }

                            Label { text: "Font Family"; color: palette.mid }
                            ComboBox {
                                Layout.fillWidth: true
                                model: Qt.fontFamilies()
                                currentIndex: {
                                    let c = count; // Force re-evaluation after model populates
                                    return AppContext.layoutEditorModel.hasActiveLayout ? find(AppContext.layoutEditorModel.nextFontFamily) : -1
                                }
                                onActivated: AppContext.layoutEditorModel.nextFontFamily = currentText
                            }

                            Label { text: "Font Size"; color: palette.mid }
                            SpinBox {
                                Layout.fillWidth: true
                                from: 8
                                to: 500
                                editable: true
                                value: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.nextFontSize : 40
                                onValueModified: AppContext.layoutEditorModel.nextFontSize = value
                            }
                            
                            Label { text: "Font Style"; color: palette.mid }
                            RowLayout {
                                Layout.fillWidth: true
                                ToolButton {
                                    text: "B"
                                    font.bold: true
                                    checkable: true
                                    checked: AppContext.layoutEditorModel.hasActiveLayout && AppContext.layoutEditorModel.nextIsBold
                                    onClicked: AppContext.layoutEditorModel.nextIsBold = checked
                                }
                                ToolButton {
                                    text: "I"
                                    font.italic: true
                                    checkable: true
                                    checked: AppContext.layoutEditorModel.hasActiveLayout && AppContext.layoutEditorModel.nextIsItalic
                                    onClicked: AppContext.layoutEditorModel.nextIsItalic = checked
                                }
                                ToolButton {
                                    text: "Aa"
                                    font.capitalization: Font.AllUppercase
                                    checkable: true
                                    checked: AppContext.layoutEditorModel.hasActiveLayout && AppContext.layoutEditorModel.nextAllCaps
                                    onClicked: AppContext.layoutEditorModel.nextAllCaps = checked
                                }
                            }

                            Label { text: "Horizontal Alignment"; color: palette.mid }
                            RowLayout {
                                Layout.fillWidth: true
                                ToolButton {
                                    text: "L"
                                    checked: AppContext.layoutEditorModel.hasActiveLayout && (AppContext.layoutEditorModel.nextAlignment & (Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) === Qt.AlignLeft
                                    onClicked: AppContext.layoutEditorModel.nextAlignment = (AppContext.layoutEditorModel.nextAlignment & ~(Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) | Qt.AlignLeft
                                }
                                ToolButton {
                                    text: "C"
                                    checked: AppContext.layoutEditorModel.hasActiveLayout && (AppContext.layoutEditorModel.nextAlignment & (Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) === Qt.AlignHCenter
                                    onClicked: AppContext.layoutEditorModel.nextAlignment = (AppContext.layoutEditorModel.nextAlignment & ~(Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) | Qt.AlignHCenter
                                }
                                ToolButton {
                                    text: "R"
                                    checked: AppContext.layoutEditorModel.hasActiveLayout && (AppContext.layoutEditorModel.nextAlignment & (Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) === Qt.AlignRight
                                    onClicked: AppContext.layoutEditorModel.nextAlignment = (AppContext.layoutEditorModel.nextAlignment & ~(Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) | Qt.AlignRight
                                }
                            }

                            Label { text: "Vertical Alignment"; color: palette.mid }
                            RowLayout {
                                Layout.fillWidth: true
                                ToolButton {
                                    text: "T"
                                    checked: AppContext.layoutEditorModel.hasActiveLayout && (AppContext.layoutEditorModel.nextAlignment & (Qt.AlignTop | Qt.AlignVCenter | Qt.AlignBottom)) === Qt.AlignTop
                                    onClicked: AppContext.layoutEditorModel.nextAlignment = (AppContext.layoutEditorModel.nextAlignment & ~(Qt.AlignTop | Qt.AlignVCenter | Qt.AlignBottom)) | Qt.AlignTop
                                }
                                ToolButton {
                                    text: "M"
                                    checked: AppContext.layoutEditorModel.hasActiveLayout && (AppContext.layoutEditorModel.nextAlignment & (Qt.AlignTop | Qt.AlignVCenter | Qt.AlignBottom)) === Qt.AlignVCenter
                                    onClicked: AppContext.layoutEditorModel.nextAlignment = (AppContext.layoutEditorModel.nextAlignment & ~(Qt.AlignTop | Qt.AlignVCenter | Qt.AlignBottom)) | Qt.AlignVCenter
                                }
                                ToolButton {
                                    text: "B"
                                    checked: AppContext.layoutEditorModel.hasActiveLayout && (AppContext.layoutEditorModel.nextAlignment & (Qt.AlignTop | Qt.AlignVCenter | Qt.AlignBottom)) === Qt.AlignBottom
                                    onClicked: AppContext.layoutEditorModel.nextAlignment = (AppContext.layoutEditorModel.nextAlignment & ~(Qt.AlignTop | Qt.AlignVCenter | Qt.AlignBottom)) | Qt.AlignBottom
                                }
                            }
                            
                            Label { text: "Bounds (X, Y, W, H)"; color: palette.mid }
                            RowLayout {
                                Layout.fillWidth: true
                                SpinBox {
                                    Layout.fillWidth: true
                                    from: -5000; to: 5000; editable: true
                                    value: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.nextTextBounds.x : 0
                                    onValueModified: {
                                        if (AppContext.layoutEditorModel.hasActiveLayout) {
                                            let b = AppContext.layoutEditorModel.nextTextBounds;
                                            AppContext.layoutEditorModel.nextTextBounds = Qt.rect(value, b.y, b.width, b.height);
                                        }
                                    }
                                }
                                SpinBox {
                                    Layout.fillWidth: true
                                    from: -5000; to: 5000; editable: true
                                    value: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.nextTextBounds.y : 0
                                    onValueModified: {
                                        if (AppContext.layoutEditorModel.hasActiveLayout) {
                                            let b = AppContext.layoutEditorModel.nextTextBounds;
                                            AppContext.layoutEditorModel.nextTextBounds = Qt.rect(b.x, value, b.width, b.height);
                                        }
                                    }
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                SpinBox {
                                    Layout.fillWidth: true
                                    from: 0; to: 5000; editable: true
                                    value: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.nextTextBounds.width : 0
                                    onValueModified: {
                                        if (AppContext.layoutEditorModel.hasActiveLayout) {
                                            let b = AppContext.layoutEditorModel.nextTextBounds;
                                            AppContext.layoutEditorModel.nextTextBounds = Qt.rect(b.x, b.y, value, b.height);
                                        }
                                    }
                                }
                                SpinBox {
                                    Layout.fillWidth: true
                                    from: 0; to: 5000; editable: true
                                    value: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.nextTextBounds.height : 0
                                    onValueModified: {
                                        if (AppContext.layoutEditorModel.hasActiveLayout) {
                                            let b = AppContext.layoutEditorModel.nextTextBounds;
                                            AppContext.layoutEditorModel.nextTextBounds = Qt.rect(b.x, b.y, b.width, value);
                                        }
                                    }
                                }
                            }
                            }
                        }
                        
                        // TIMERS SECTION
                        Rectangle { Layout.fillWidth: true; height: 1; color: palette.mid; opacity: 0.2; Layout.topMargin: 8; Layout.bottomMargin: 8 }

                        Label { text: "Timers"; font.bold: true; color: palette.highlight }
                        
                        RowLayout {
                            Layout.fillWidth: true
                            
                            ComboBox {
                                id: availableTimersCombo
                                Layout.fillWidth: true
                                textRole: "name"
                                valueRole: "id"
                                model: AppContext.timerManager
                            }
                            
                            Button {
                                text: "Add Timer"
                                onClicked: {
                                    if (availableTimersCombo.currentIndex >= 0) {
                                        var timerId = availableTimersCombo.currentValue
                                        if (timerId !== "") {
                                            AppContext.layoutEditorModel.addTimerToLayout(timerId)
                                        }
                                    }
                                }
                            }
                        }
                        
                        // List of added timers
                        Repeater {
                            model: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.activeLayoutTimers : []
                            
                            delegate: Rectangle {
                                Layout.fillWidth: true
                                height: timerSettingsColumn.implicitHeight + 16
                                color: palette.alternateBase
                                border.color: palette.mid
                                border.width: 1
                                radius: 4
                                
                                property var timerData: modelData
                                
                                ColumnLayout {
                                    id: timerSettingsColumn
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    spacing: 4
                                    
                                    RowLayout {
                                        Layout.fillWidth: true
                                        Label { 
                                            text: "Timer: " + (AppContext.timerManager.getTimer(timerData.timerId) ? AppContext.timerManager.getTimer(timerData.timerId).name : timerData.timerId)
                                            font.bold: true
                                            Layout.fillWidth: true
                                        }
                                        Button {
                                            text: "Remove"
                                            onClicked: AppContext.layoutEditorModel.removeTimerFromLayout(timerData.timerId)
                                        }
                                    }
                                    
                                    // Timer Bounds (X, Y, W, H)
                                    Label { text: "Bounds (X, Y, W, H)"; color: palette.mid }
                                    RowLayout {
                                        Layout.fillWidth: true
                                        SpinBox {
                                            Layout.fillWidth: true
                                            from: -5000; to: 5000; editable: true
                                            value: timerData.bounds.x
                                            onValueModified: AppContext.layoutEditorModel.updateTimerBounds(timerData.timerId, value, timerData.bounds.y, timerData.bounds.width, timerData.bounds.height)
                                        }
                                        SpinBox {
                                            Layout.fillWidth: true
                                            from: -5000; to: 5000; editable: true
                                            value: timerData.bounds.y
                                            onValueModified: AppContext.layoutEditorModel.updateTimerBounds(timerData.timerId, timerData.bounds.x, value, timerData.bounds.width, timerData.bounds.height)
                                        }
                                    }
                                    RowLayout {
                                        Layout.fillWidth: true
                                        SpinBox {
                                            Layout.fillWidth: true
                                            from: 0; to: 5000; editable: true
                                            value: timerData.bounds.width
                                            onValueModified: AppContext.layoutEditorModel.updateTimerBounds(timerData.timerId, timerData.bounds.x, timerData.bounds.y, value, timerData.bounds.height)
                                        }
                                        SpinBox {
                                            Layout.fillWidth: true
                                            from: 0; to: 5000; editable: true
                                            value: timerData.bounds.height
                                            onValueModified: AppContext.layoutEditorModel.updateTimerBounds(timerData.timerId, timerData.bounds.x, timerData.bounds.y, timerData.bounds.width, value)
                                        }
                                    }
                                    
                                    // Timer Styling
                                    Button {
                                        Layout.fillWidth: true
                                        text: "Text Color"
                                        onClicked: {
                                            var d = timerColorDialog.createObject(parent, {
                                                selectedColor: timerData.fontColor
                                            })
                                            d.accepted.connect(function() {
                                                AppContext.layoutEditorModel.updateTimerFont(timerData.timerId, timerData.fontFamily, timerData.fontSize, d.selectedColor, timerData.isBold, timerData.isItalic, timerData.alignment)
                                                d.destroy()
                                            })
                                            d.open()
                                        }
                                    }
                                    
                                    RowLayout {
                                        Layout.fillWidth: true
                                        ComboBox {
                                            Layout.fillWidth: true
                                            model: Qt.fontFamilies()
                                            currentIndex: { let c = count; return find(timerData.fontFamily) }
                                            onActivated: AppContext.layoutEditorModel.updateTimerFont(timerData.timerId, currentText, timerData.fontSize, timerData.fontColor, timerData.isBold, timerData.isItalic, timerData.alignment)
                                        }
                                        SpinBox {
                                            Layout.preferredWidth: 80
                                            from: 8; to: 500; editable: true
                                            value: timerData.fontSize
                                            onValueModified: AppContext.layoutEditorModel.updateTimerFont(timerData.timerId, timerData.fontFamily, value, timerData.fontColor, timerData.isBold, timerData.isItalic, timerData.alignment)
                                        }
                                    }
                                    
                                    RowLayout {
                                        Layout.fillWidth: true
                                        ToolButton {
                                            text: "B"
                                            font.bold: true
                                            checkable: true
                                            checked: timerData.isBold
                                            onClicked: AppContext.layoutEditorModel.updateTimerFont(timerData.timerId, timerData.fontFamily, timerData.fontSize, timerData.fontColor, checked, timerData.isItalic, timerData.alignment)
                                        }
                                        ToolButton {
                                            text: "I"
                                            font.italic: true
                                            checkable: true
                                            checked: timerData.isItalic
                                            onClicked: AppContext.layoutEditorModel.updateTimerFont(timerData.timerId, timerData.fontFamily, timerData.fontSize, timerData.fontColor, timerData.isBold, checked, timerData.alignment)
                                        }
                                        
                                        Item { Layout.fillWidth: true } // spacer
                                        
                                        ToolButton {
                                            text: "L"
                                            checked: (timerData.alignment & (Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) === Qt.AlignLeft
                                            onClicked: AppContext.layoutEditorModel.updateTimerFont(timerData.timerId, timerData.fontFamily, timerData.fontSize, timerData.fontColor, timerData.isBold, timerData.isItalic, (timerData.alignment & ~(Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) | Qt.AlignLeft)
                                        }
                                        ToolButton {
                                            text: "C"
                                            checked: (timerData.alignment & (Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) === Qt.AlignHCenter
                                            onClicked: AppContext.layoutEditorModel.updateTimerFont(timerData.timerId, timerData.fontFamily, timerData.fontSize, timerData.fontColor, timerData.isBold, timerData.isItalic, (timerData.alignment & ~(Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) | Qt.AlignHCenter)
                                        }
                                        ToolButton {
                                            text: "R"
                                            checked: (timerData.alignment & (Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) === Qt.AlignRight
                                            onClicked: AppContext.layoutEditorModel.updateTimerFont(timerData.timerId, timerData.fontFamily, timerData.fontSize, timerData.fontColor, timerData.isBold, timerData.isItalic, (timerData.alignment & ~(Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) | Qt.AlignRight)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: newLayoutDialog
        title: "New Layout"
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        
        ColumnLayout {
            spacing: 10
            TextField {
                id: layoutNameField
                placeholderText: "Layout Name"
                Layout.fillWidth: true
            }
            ComboBox {
                id: layoutTypeCombo
                model: ["Audience", "Stage"]
                Layout.fillWidth: true
            }
        }

        onAccepted: {
            if (layoutNameField.text !== "") {
                AppContext.layoutEditorModel.createNewLayout(layoutNameField.text, layoutTypeCombo.currentIndex)
                layoutNameField.text = ""
            }
        }
    }

    ColorDialog {
        id: bgColorDialog
        title: "Select Background Color"
        onAccepted: AppContext.layoutEditorModel.backgroundColor = selectedColor
    }

    ColorDialog {
        id: mainFontColorDialog
        title: "Select Main Text Color"
        onAccepted: AppContext.layoutEditorModel.fontColor = selectedColor
    }

    ColorDialog {
        id: nextFontColorDialog
        title: "Select Next Text Color"
        onAccepted: AppContext.layoutEditorModel.nextFontColor = selectedColor
    }

    Component {
        id: timerColorDialog
        ColorDialog {
            title: "Select Timer Text Color"
        }
    }

    MediaPickerDialog {
        id: bgImageDialog
        mediaType: "image"
        onFileSelected: function(filePath, fileUrl) {
            AppContext.layoutEditorModel.selectBackgroundImage(fileUrl)
        }
    }

    Dialog {
        id: cloneLayoutDialog
        title: "Clone Layout"
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        
        ColumnLayout {
            spacing: 10
            TextField {
                id: cloneLayoutNameField
                placeholderText: "New Layout Name"
                Layout.fillWidth: true
                text: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.activeLayoutName + " Copy" : ""
            }
        }

        onAccepted: {
            if (cloneLayoutNameField.text !== "") {
                AppContext.layoutEditorModel.cloneActiveLayout(cloneLayoutNameField.text)
            }
        }
        
        onOpened: {
            cloneLayoutNameField.text = AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.activeLayoutName + " Copy" : ""
            cloneLayoutNameField.forceActiveFocus()
            cloneLayoutNameField.selectAll()
        }
    }

    MessageDialog {
        id: deleteLayoutDialog
        title: "Delete Layout"
        text: "Are you sure you want to permanently delete '" + (AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.activeLayoutName : "") + "'?"
        buttons: MessageDialog.Yes | MessageDialog.No
        onButtonClicked: function(button, role) {
            if (button === MessageDialog.Yes) {
                AppContext.layoutEditorModel.removeActiveLayout()
            }
        }
    }
}
