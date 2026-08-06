import QtQuick
import fohmedia
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    id: root
    
    property string layoutName: "New Layout"
    property string selectedItemId: ""
    Binding {
        target: AppContext.layoutEditorModel
        property: "selectedElementId"
        value: root.selectedItemId
    }
    Binding {
        target: root
        property: "selectedItemId"
        value: AppContext.layoutEditorModel.selectedElementId
    }

    Component.onCompleted: {
        AppContext.layoutEditorModel.loadLayouts()
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
                        FohToolTip {

                            visible: parent.hovered

                            text: "New Layout"

                        }
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
                        leftPadding: 15
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

                // Checkerboard pattern behind canvas when background has transparency
                Rectangle {
                    anchors.fill: parent
                    color: "#181818"
                    clip: true
                    Image {
                        anchors.fill: parent
                        fillMode: Image.Tile
                        source: "data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32'><rect width='16' height='16' fill='%23222'/><rect x='16' width='16' height='16' fill='%23333'/><rect y='16' width='16' height='16' fill='%23333'/><rect x='16' y='16' width='16' height='16' fill='%23222'/></svg>"
                    }
                }

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

                    // Render Custom Elements
                    Repeater {
                        model: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.activeLayoutCustomElements : []
                        
                        delegate: ResizableCanvasItem {
                            id: customElementResizer
                            z: 11
                            visible: true
                            property var elementData: modelData
                            
                            Component.onCompleted: {
                                x = elementData.bounds.x
                                y = elementData.bounds.y
                                width = elementData.bounds.width
                                height = elementData.bounds.height
                            }
                            
                            isSelected: root.selectedItemId === elementData.elementId
                            onClicked: root.selectedItemId = elementData.elementId
                            
                            strokeColor: "#00ff00" // green
                            
                            Label {
                                anchors.fill: parent
                                text: {
                                    let type = elementData.elementId.split("_")[0]
                                    if (type === "SlideCount") return "Slide 1 / 10"
                                    if (type === "GroupSlideCount") return "1 / 4"
                                    if (type === "ComponentName") return "Comp Name"
                                    if (type === "NextComponentName") return "Next Comp Name"
                                    if (type === "SongName") return "Song Name"
                                    if (type === "ShowProgress") return "Song 1 / 3"
                                    if (type === "ShowProgress") return "Song 1\n<b>Song 2</b>\nSong 3"
                                    if (type === "Minimap") return "[ Minimap ]"
                                    return type
                                }
                                color: elementData.fontColor
                                font.family: elementData.fontFamily
                                font.pixelSize: elementData.fontSize
                                font.bold: elementData.isBold
                                font.italic: elementData.isItalic
                                font.capitalization: elementData.allCaps ? Font.AllUppercase : Font.MixedCase
                                horizontalAlignment: (elementData.alignment & (Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) || Text.AlignHCenter
                                verticalAlignment: (elementData.alignment & (Qt.AlignTop | Qt.AlignVCenter | Qt.AlignBottom)) || Text.AlignVCenter
                                wrapMode: Text.WordWrap
                            }
                            
                            onBoundsChanged: function(newX, newY, newW, newH) {
                                AppContext.layoutEditorModel.updateCustomElementBounds(elementData.elementId, newX, newY, newW, newH)
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
                                FohToolTip {

                                    visible: parent.hovered

                                    text: "Clear Background Image"

                                }
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



                        
                        // SELECTED ITEM PROPERTIES
                        Label { 
                            text: {
                                if (root.selectedItemId === "mainText") return "Main Text Box Properties"
                                if (root.selectedItemId === "nextText") return "Next Text Box Properties"
                                if (root.selectedItemId.startsWith("timer_")) return "Timer Properties"
                                return "Element Properties"
                            }
                            font.bold: true; color: palette.highlight; topPadding: 8 
                        }

                        ColumnLayout {
                            id: alignTools
                            Layout.fillWidth: true
                            
                            function setAlignment(hAlign, vAlign) {
                                let cw = AppContext.layoutEditorModel.canvasWidth;
                                let ch = AppContext.layoutEditorModel.canvasHeight;
                                let b = null;
                                if (root.selectedItemId === "mainText") b = AppContext.layoutEditorModel.textBounds;
                                else if (root.selectedItemId === "nextText") b = AppContext.layoutEditorModel.nextTextBounds;
                                else if (root.selectedItemId.startsWith("timer_")) {
                                    let tId = root.selectedItemId.substring(6);
                                    let timers = AppContext.layoutEditorModel.activeLayoutTimers;
                                    for (let i = 0; i < timers.length; i++) {
                                        if (timers[i].timerId === tId) { b = timers[i].bounds; break; }
                                    }
                                } else if (root.selectedItemId !== "") {
                                    let customElements = AppContext.layoutEditorModel.activeLayoutCustomElements;
                                    for (let i = 0; i < customElements.length; i++) {
                                        if (customElements[i].elementId === root.selectedItemId) { b = customElements[i].bounds; break; }
                                    }
                                }
                                if (!b) return;
                                
                                let newX = b.x;
                                let newY = b.y;
                                if (hAlign === -1) newX = 0;
                                else if (hAlign === 0) newX = (cw - b.width) / 2;
                                else if (hAlign === 1) newX = cw - b.width;
                                
                                if (vAlign === -1) newY = 0;
                                else if (vAlign === 0) newY = (ch - b.height) / 2;
                                else if (vAlign === 1) newY = ch - b.height;
                                
                                if (root.selectedItemId === "mainText") AppContext.layoutEditorModel.textBounds = Qt.rect(newX, newY, b.width, b.height);
                                else if (root.selectedItemId === "nextText") AppContext.layoutEditorModel.nextTextBounds = Qt.rect(newX, newY, b.width, b.height);
                                else if (root.selectedItemId.startsWith("timer_")) {
                                    AppContext.layoutEditorModel.updateTimerBounds(root.selectedItemId.substring(6), newX, newY, b.width, b.height);
                                } else if (root.selectedItemId !== "") {
                                    AppContext.layoutEditorModel.updateCustomElementBounds(root.selectedItemId, newX, newY, b.width, b.height);
                                }
                            }

                            GridLayout {
                                Layout.alignment: Qt.AlignHCenter
                                columns: 3
                                rowSpacing: 2
                                columnSpacing: 2
                                
                                ToolButton { text: "\u2196"; onClicked: alignTools.setAlignment(-1, -1); FohToolTip { visible: parent.hovered; text: "Top Left" } }
                                ToolButton { text: "\u2191"; onClicked: alignTools.setAlignment(null, -1); FohToolTip { visible: parent.hovered; text: "Top Center" } }
                                ToolButton { text: "\u2197"; onClicked: alignTools.setAlignment(1, -1); FohToolTip { visible: parent.hovered; text: "Top Right" } }
                                
                                ToolButton { text: "\u2190"; onClicked: alignTools.setAlignment(-1, null); FohToolTip { visible: parent.hovered; text: "Middle Left" } }
                                ColumnLayout {
                                    spacing: 0
                                    ToolButton { text: "|"; onClicked: alignTools.setAlignment(0, null); implicitHeight: 18; font.pixelSize: 10; FohToolTip { visible: parent.hovered; text: "Center Horizontally" } }
                                    ToolButton { text: "\u2014"; onClicked: alignTools.setAlignment(null, 0); implicitHeight: 18; font.pixelSize: 10; font.bold: true; FohToolTip { visible: parent.hovered; text: "Center Vertically" } }
                                }
                                ToolButton { text: "\u2192"; onClicked: alignTools.setAlignment(1, null); FohToolTip { visible: parent.hovered; text: "Middle Right" } }
                                
                                ToolButton { text: "\u2199"; onClicked: alignTools.setAlignment(-1, 1); FohToolTip { visible: parent.hovered; text: "Bottom Left" } }
                                ToolButton { text: "\u2193"; onClicked: alignTools.setAlignment(null, 1); FohToolTip { visible: parent.hovered; text: "Bottom Center" } }
                                ToolButton { text: "\u2198"; onClicked: alignTools.setAlignment(1, 1); FohToolTip { visible: parent.hovered; text: "Bottom Right" } }
                            }
                        }

                        Button {
                            Layout.fillWidth: true
                            text: "Text Color"
                            onClicked: {
                                mainFontColorDialog.selectedColor = AppContext.layoutEditorModel.selectedFontColor
                                mainFontColorDialog.open()
                            }
                        }

                        Label { text: "Font Family"; color: palette.mid }
                        ComboBox {
                            Layout.fillWidth: true
                            model: Qt.fontFamilies()
                            currentIndex: {
                                let c = count; // Force re-evaluation after model populates
                                return AppContext.layoutEditorModel.hasActiveLayout ? find(AppContext.layoutEditorModel.selectedFontFamily) : -1
                            }
                            onActivated: AppContext.layoutEditorModel.selectedFontFamily = currentText
                        }

                        Label { text: "Font Size"; color: palette.mid }
                        SpinBox {
                            Layout.fillWidth: true
                            from: 8
                            to: 500
                            editable: true
                            value: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.selectedFontSize : 40
                            onValueModified: AppContext.layoutEditorModel.selectedFontSize = value
                        }

                        Label { text: "Font Style"; color: palette.mid }
                        RowLayout {
                            Layout.fillWidth: true
                            ToolButton {
                                text: "B"
                                font.bold: true
                                checkable: true
                                checked: AppContext.layoutEditorModel.hasActiveLayout && AppContext.layoutEditorModel.selectedIsBold
                                onClicked: AppContext.layoutEditorModel.selectedIsBold = checked
                            }
                            ToolButton {
                                text: "I"
                                font.italic: true
                                checkable: true
                                checked: AppContext.layoutEditorModel.hasActiveLayout && AppContext.layoutEditorModel.selectedIsItalic
                                onClicked: AppContext.layoutEditorModel.selectedIsItalic = checked
                            }
                            ToolButton {
                                text: "Aa"
                                font.capitalization: Font.AllUppercase
                                checkable: true
                                checked: AppContext.layoutEditorModel.hasActiveLayout && AppContext.layoutEditorModel.selectedAllCaps
                                onClicked: AppContext.layoutEditorModel.selectedAllCaps = checked
                                visible: !root.selectedItemId.startsWith("timer_") && root.selectedItemId !== "mainText" && root.selectedItemId !== "nextText"
                            }
                        }

                        Label { text: "Horizontal Alignment"; color: palette.mid }
                        RowLayout {
                            Layout.fillWidth: true
                            ToolButton {
                                text: "L"
                                checked: AppContext.layoutEditorModel.hasActiveLayout && (AppContext.layoutEditorModel.selectedAlignment & (Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) === Qt.AlignLeft
                                onClicked: AppContext.layoutEditorModel.selectedAlignment = (AppContext.layoutEditorModel.selectedAlignment & ~(Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) | Qt.AlignLeft
                            }
                            ToolButton {
                                text: "C"
                                checked: AppContext.layoutEditorModel.hasActiveLayout && (AppContext.layoutEditorModel.selectedAlignment & (Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) === Qt.AlignHCenter
                                onClicked: AppContext.layoutEditorModel.selectedAlignment = (AppContext.layoutEditorModel.selectedAlignment & ~(Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) | Qt.AlignHCenter
                            }
                            ToolButton {
                                text: "R"
                                checked: AppContext.layoutEditorModel.hasActiveLayout && (AppContext.layoutEditorModel.selectedAlignment & (Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) === Qt.AlignRight
                                onClicked: AppContext.layoutEditorModel.selectedAlignment = (AppContext.layoutEditorModel.selectedAlignment & ~(Qt.AlignLeft | Qt.AlignHCenter | Qt.AlignRight)) | Qt.AlignRight
                            }
                        }

                        Label { text: "Vertical Alignment"; color: palette.mid }
                        RowLayout {
                            Layout.fillWidth: true
                            ToolButton {
                                text: "T"
                                checked: AppContext.layoutEditorModel.hasActiveLayout && (AppContext.layoutEditorModel.selectedAlignment & (Qt.AlignTop | Qt.AlignVCenter | Qt.AlignBottom)) === Qt.AlignTop
                                onClicked: AppContext.layoutEditorModel.selectedAlignment = (AppContext.layoutEditorModel.selectedAlignment & ~(Qt.AlignTop | Qt.AlignVCenter | Qt.AlignBottom)) | Qt.AlignTop
                            }
                            ToolButton {
                                text: "M"
                                checked: AppContext.layoutEditorModel.hasActiveLayout && (AppContext.layoutEditorModel.selectedAlignment & (Qt.AlignTop | Qt.AlignVCenter | Qt.AlignBottom)) === Qt.AlignVCenter
                                onClicked: AppContext.layoutEditorModel.selectedAlignment = (AppContext.layoutEditorModel.selectedAlignment & ~(Qt.AlignTop | Qt.AlignVCenter | Qt.AlignBottom)) | Qt.AlignVCenter
                            }
                            ToolButton {
                                text: "B"
                                checked: AppContext.layoutEditorModel.hasActiveLayout && (AppContext.layoutEditorModel.selectedAlignment & (Qt.AlignTop | Qt.AlignVCenter | Qt.AlignBottom)) === Qt.AlignBottom
                                onClicked: AppContext.layoutEditorModel.selectedAlignment = (AppContext.layoutEditorModel.selectedAlignment & ~(Qt.AlignTop | Qt.AlignVCenter | Qt.AlignBottom)) | Qt.AlignBottom
                            }
                        }

                        Label { text: "Bounds (X, Y, W, H)"; color: palette.mid }
                        RowLayout {
                            Layout.fillWidth: true
                            SpinBox {
                                Layout.fillWidth: true
                                from: -5000; to: 5000; editable: true
                                value: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.selectedBounds.x : 0
                                onValueModified: {
                                    if (AppContext.layoutEditorModel.hasActiveLayout) {
                                        let b = AppContext.layoutEditorModel.selectedBounds;
                                        AppContext.layoutEditorModel.selectedBounds = Qt.rect(value, b.y, b.width, b.height);
                                    }
                                }
                            }
                            SpinBox {
                                Layout.fillWidth: true
                                from: -5000; to: 5000; editable: true
                                value: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.selectedBounds.y : 0
                                onValueModified: {
                                    if (AppContext.layoutEditorModel.hasActiveLayout) {
                                        let b = AppContext.layoutEditorModel.selectedBounds;
                                        AppContext.layoutEditorModel.selectedBounds = Qt.rect(b.x, value, b.width, b.height);
                                    }
                                }
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            SpinBox {
                                Layout.fillWidth: true
                                from: 0; to: 5000; editable: true
                                value: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.selectedBounds.width : 0
                                onValueModified: {
                                    if (AppContext.layoutEditorModel.hasActiveLayout) {
                                        let b = AppContext.layoutEditorModel.selectedBounds;
                                        AppContext.layoutEditorModel.selectedBounds = Qt.rect(b.x, b.y, value, b.height);
                                    }
                                }
                            }
                            SpinBox {
                                Layout.fillWidth: true
                                from: 0; to: 5000; editable: true
                                value: AppContext.layoutEditorModel.hasActiveLayout ? AppContext.layoutEditorModel.selectedBounds.height : 0
                                onValueModified: {
                                    if (AppContext.layoutEditorModel.hasActiveLayout) {
                                        let b = AppContext.layoutEditorModel.selectedBounds;
                                        AppContext.layoutEditorModel.selectedBounds = Qt.rect(b.x, b.y, b.width, value);
                                    }
                                }
                            }
                        }
                        
                        Button {
                            Layout.fillWidth: true
                            text: "Remove Element"
                            // Only show for timers and custom elements, not mainText/nextText
                            visible: root.selectedItemId !== "mainText" && root.selectedItemId !== "nextText" && root.selectedItemId !== ""
                            palette.buttonText: "white"
                            palette.button: "#D32F2F" // Red color for delete action
                            onClicked: {
                                if (root.selectedItemId.startsWith("timer_")) {
                                    AppContext.layoutEditorModel.removeTimerFromLayout(root.selectedItemId.substring(6))
                                } else {
                                    AppContext.layoutEditorModel.removeCustomElementFromLayout(root.selectedItemId)
                                }
                                root.selectedItemId = ""
                            }
                        }

                        // LAYOUT ELEMENTS SECTION
                        Rectangle { Layout.fillWidth: true; height: 1; color: palette.mid; opacity: 0.2; Layout.topMargin: 8; Layout.bottomMargin: 8 }

                        Label { text: "Layout Elements"; font.bold: true; color: palette.highlight }

                        RowLayout {
                            Layout.fillWidth: true
                            Switch {
                                id: nextTextSwitch
                                text: "Enable Next Text"
                                checked: AppContext.layoutEditorModel.hasNextText
                                onCheckedChanged: {
                                    if (AppContext.layoutEditorModel.hasActiveLayout && AppContext.layoutEditorModel.hasNextText !== checked) {
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
                                            AppContext.layoutEditorModel.selectedElementId = "timer_" + timerId
                                        }
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            
                            ComboBox {
                                id: customElementsCombo
                                Layout.fillWidth: true
                                model: ["Slide Count", "Group Slide Count", "Component Name", "Next Component Name", "Show Progress Minimap", "Show Progress (List)"]
                            }
                            
                            Button {
                                text: "Add Element"
                                onClicked: {
                                    let elementTypes = ["SlideCount", "GroupSlideCount", "ComponentName", "NextComponentName", "Minimap", "ShowProgress"]
                                    let type = elementTypes[customElementsCombo.currentIndex]
                                    let idStr = type + "_" + Date.now()
                                    AppContext.layoutEditorModel.addCustomElementToLayout(idStr)
                                    AppContext.layoutEditorModel.selectedElementId = idStr
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
        options: ColorDialog.ShowAlphaChannel
        onAccepted: AppContext.layoutEditorModel.backgroundColor = selectedColor
    }

    ColorDialog {
        id: mainFontColorDialog
        title: "Select Main Text Color"
        onAccepted: AppContext.layoutEditorModel.selectedFontColor = selectedColor
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
