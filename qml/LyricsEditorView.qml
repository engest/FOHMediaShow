import QtQuick
import fohmedia
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCore
import QtQuick.Effects
import QtQuick.Shapes
import Qt.labs.platform as Platform

Item {
    id: root
    width: StackView.view ? StackView.view.width : (parent ? parent.width : 0)
    height: StackView.view ? StackView.view.height : (parent ? parent.height : 0)

    Component.onCompleted: {
        AppContext.lyricsModel.reloadCurrentDeck()
    }

    Dialog {
        id: newArrangementDialog
        title: "New Arrangement"
        standardButtons: Dialog.Ok | Dialog.Cancel
        anchors.centerIn: parent
        modal: true
        padding: 24
        
        ColumnLayout {
            spacing: 12
            Label { text: "Arrangement Name:" }
            TextField {
                id: newArrangementField
                Layout.fillWidth: true
                onAccepted: newArrangementDialog.accept()
            }
        }
        
        onAccepted: {
            if (newArrangementField.text.trim() !== "") {
                AppContext.lyricsModel.addArrangement(newArrangementField.text.trim())
                newArrangementField.text = ""
                arrangementAutoSaveTimer.restart()
            }
        }
        onOpened: {
            newArrangementField.forceActiveFocus()
        }
    }

    Dialog {
        id: cloneArrangementDialog
        title: "Clone Arrangement"
        standardButtons: Dialog.Ok | Dialog.Cancel
        anchors.centerIn: parent
        modal: true
        padding: 24
        
        ColumnLayout {
            spacing: 12
            Label { text: "New Arrangement Name:" }
            TextField {
                id: cloneArrangementField
                Layout.fillWidth: true
                onAccepted: cloneArrangementDialog.accept()
            }
        }
        
        onAccepted: {
            if (cloneArrangementField.text.trim() !== "") {
                if (AppContext.lyricsModel.currentDeck) {
                    AppContext.lyricsModel.cloneArrangement(AppContext.lyricsModel.currentDeck.activeArrangementName, cloneArrangementField.text.trim())
                }
                cloneArrangementField.text = ""
                arrangementAutoSaveTimer.restart()
            }
        }
        onOpened: {
            cloneArrangementField.forceActiveFocus()
        }
    }
    
    Dialog {
        id: deleteMappingDialog
        title: "Remove Screen Layout"
        standardButtons: Dialog.Yes | Dialog.No
        anchors.centerIn: parent
        modal: true
        padding: 24
        property string screenName: ""
        
        Label {
            text: "Are you sure you want to remove the layout mapping for screen '" + deleteMappingDialog.screenName + "'?"
        }
        
        onAccepted: {
            AppContext.lyricsModel.removeScreenLayoutMapping(deleteMappingDialog.screenName)
        }
    }

    Connections {
        target: AppContext.lyricsModel
        function onRawLyricsTextChanged() {
            if (lyricsEditor.text !== AppContext.lyricsModel.rawLyricsText) {
                lyricsEditor.text = AppContext.lyricsModel.rawLyricsText
            }
        }
    }

    Timer {
        id: arrangementAutoSaveTimer
        interval: 200
        repeat: false
        onTriggered: {
            if (AppContext.lyricsModel.currentDeck) AppContext.lyricsModel.saveDeck(false);
        }
    }

    Connections {
        target: AppContext.lyricsModel.arrangementModel
        function onSequenceChanged() {
            arrangementAutoSaveTimer.restart()
        }
    }

    SplitView {
        anchors.fill: parent

        // Left Pane: Library
        Rectangle {
            SplitView.preferredWidth: 300
            SplitView.minimumWidth: 200
            SplitView.maximumWidth: 400
            color: palette.base

            ColumnLayout {
                anchors.fill: parent
                
                RowLayout {
                    Layout.fillWidth: true
                    Layout.margins: 8
                    Label {
                        text: "Library"
                        font.bold: true
                        font.pixelSize: 16
                        Layout.fillWidth: true
                    }
                    ToolButton {
                        text: "⎘"
                        font.pixelSize: 20
                        ToolTip.text: "Import..."
                        ToolTip.visible: hovered
                        onClicked: importDialog.open()
                    }
                    ToolButton {
                        text: "⎘"
                        font.pixelSize: 20
                        ToolTip.text: "Import Pro files..."
                        ToolTip.visible: hovered
                        onClicked: importProDialog.open()
                    }
                    ToolButton {
                        text: "⊕"
                        font.pixelSize: 20
                        ToolTip.text: "New"
                        ToolTip.visible: hovered
                        onClicked: AppContext.lyricsModel.newDeck()
                    }
                }

                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    Layout.margins: 8
                    Layout.topMargin: 0
                    placeholderText: "Search library..."
                    selectByMouse: true
                }

                ListView {
                    id: libraryList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: {
                        let allFiles = AppContext.lyricsModel.libraryFiles
                        let query = searchField.text.toLowerCase().trim()
                        if (query === "") return allFiles
                        return allFiles.filter(function(item) {
                            return item.title.toLowerCase().includes(query)
                        })
                    }
                    
                    delegate: ItemDelegate {
                        width: ListView.view.width
                        text: modelData.title
                        highlighted: AppContext.lyricsModel.currentTitle === modelData.title // basic highlighting
                        onClicked: {
                            AppContext.lyricsModel.loadFromLibrary(modelData.filename)
                        }
                        
                        ToolButton {
                            text: "\u2296" // Minus in circle
                            font.pixelSize: 16
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            ToolTip.text: "Remove " + modelData.title
                            ToolTip.visible: hovered
                            onClicked: {
                                deleteDialog.filename = modelData.filename
                                deleteDialog.deckTitle = modelData.title
                                deleteDialog.open()
                            }
                        }
                    }
                }
            }
        }

        // Right Pane: Lyrics Editor
        Item {
            SplitView.preferredWidth: 400
            SplitView.minimumWidth: 200

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 16

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: "Lyrics Content"
                        font.pixelSize: 16
                        font.bold: true
                        Layout.fillWidth: true
                        topPadding: 10
                    }
                    ToolButton {
                        text: "➕"
                        ToolTip.visible: hovered
                        ToolTip.text: "Insert default components..."
                        onClicked: {
                            let currentText = lyricsEditor.text.trim()

                            let hasIntro = currentText.indexOf("[Intro]") !== -1
                            let hasInstrumental = currentText.indexOf("[Instrumental]") !== -1
                            let hasPassThrough = currentText.indexOf("[PassThrough]") !== -1
                            let hasOutro = currentText.indexOf("[Outro]") !== -1

                            if (!hasIntro) {
                                if (currentText.length > 0) currentText = "[Intro]\n\n" + currentText
                                else currentText = "[Intro]"
                            }
                            if (!hasInstrumental) {
                                if (currentText.length > 0) currentText += "\n\n[Instrumental]"
                                else currentText = "[Instrumental]"
                            }
                            if (!hasPassThrough) {
                                if (currentText.length > 0) currentText += "\n\n[PassThrough]"
                                else currentText = "[PassThrough]"
                            }
                            if (!hasOutro) {
                                if (currentText.length > 0) currentText += "\n\n[Outro]"
                                else currentText = "[Outro]"
                            }

                            lyricsEditor.text = currentText
                        }
                    }
                    ToolButton {
                        text: "🏷️"
                        ToolTip.visible: hovered
                        ToolTip.text: "Insert Component Tag"
                        onClicked: {
                            let masterComponentList = [
                                "Verse", "Verse 1", "Verse 2", "Verse 3", "Verse 4", "Verse 5", "Verse 6",
                                "Chorus", "Chorus 1", "Chorus 2", "Chorus 3", "Chorus 4",
                                "Bridge", "Bridge 1", "Bridge 2", "Bridge 3",
                                "PreChorus", "Refrain",
                                "Tag", "Tag 1", "Tag 2",
                                "Intro", "Ending", "Outro",
                                "Interlude", "Instrumental", "Vamp", "Turnaround", "PassThrough",
                                "Blank"
                            ]
                            let currentText = lyricsEditor.text
                            menuInstantiator.model = masterComponentList.filter(comp => currentText.indexOf("[" + comp + "]") === -1)
                            componentMenu.open()
                        }

                        Menu {
                            id: componentMenu
                            y: parent.height

                            Instantiator {
                                id: menuInstantiator
                                model: []
                                onObjectAdded: function(index, object) {
                                    componentMenu.insertItem(index, object)
                                }
                                onObjectRemoved: function(index, object) {
                                    componentMenu.removeItem(object)
                                }
                                delegate: MenuItem {
                                    id: menuItem
                                    text: modelData
                                    contentItem: Row {
                                        spacing: 8
                                        Rectangle {
                                            width: 12
                                            height: 12
                                            color: window.getComponentColor(modelData)
                                            border.color: "#222"
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                        Text {
                                            text: menuItem.text
                                            color: menuItem.enabled ? palette.text : palette.mid
                                            font: menuItem.font
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }
                                    onTriggered: {
                                        let compName = modelData
                                        let start = lyricsEditor.selectionStart
                                        let currentText = lyricsEditor.text

                                        let lineStart = currentText.lastIndexOf('\n', start - 1) + 1

                                        let insertText = "[" + compName + "]\n"

                                        if (lineStart > 0) {
                                            if (lineStart > 1 && currentText.charAt(lineStart - 2) === '\n') {
                                                // Already has a blank line above
                                            } else {
                                                insertText = "\n" + insertText
                                            }
                                        }

                                        lyricsEditor.insert(lineStart, insertText)
                                    }
                                }
                            }
                        }
                    }
                    ToolButton {
                        text: "🧹"
                        ToolTip.visible: hovered
                        ToolTip.text: "Clean Lyrics"
                        onClicked: {
                            let lines = lyricsEditor.text.split('\n');
                            let punctRegex = /[!"#$%&()*+,\-./:;<=>?@\[\\\]^_`{|}~“”…«»]/g;
                            for (let i = 0; i < lines.length; i++) {
                                let trimmed = lines[i].trim();
                                if (trimmed.startsWith("[") && trimmed.endsWith("]")) {
                                    continue;
                                }
                                lines[i] = lines[i].replace(punctRegex, "");
                            }
                            lyricsEditor.text = lines.join('\n');
                        }
                    }

                    Button {
                        text: "Save"
                        highlighted: true
                        onClicked: {
                            AppContext.lyricsModel.rawLyricsText = lyricsEditor.text
                            AppContext.lyricsModel.saveDeck()
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: palette.base
                    radius: 4

                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: 16
                        clip: true

                        TextArea {
                            id: lyricsEditor
                            text: AppContext.lyricsModel.rawLyricsText
                            font.family: "Monospace"
                            font.pixelSize: 14
                            wrapMode: TextArea.Wrap
                            selectByMouse: true
                            background: null // Remove default background

                            onTextChanged: {
                                if (AppContext.lyricsModel.rawLyricsText !== text) {
                                    AppContext.lyricsModel.rawLyricsText = text
                                }
                            }
                        }
                    }
                }
            }
        }

        // Middle Pane: Metadata and Arrangement
        Item {
            SplitView.preferredWidth: 600
            SplitView.minimumWidth: 200

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 16

            GroupBox {
                label: Label {
                    text: "Properties"
                    font.bold: true
                    font.pixelSize: 16
                    topPadding: 10
                }
                Layout.fillWidth: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    TextField {
                        id: titleField
                        placeholderText: "Title"
                        Layout.fillWidth: true
                        text: AppContext.lyricsModel.currentTitle
                        onTextEdited: AppContext.lyricsModel.currentTitle = text
                    }

                    TextField {
                        id: artistField
                        placeholderText: "Artist(s)"
                        Layout.fillWidth: true
                        text: AppContext.lyricsModel.currentArtists
                        onTextEdited: AppContext.lyricsModel.currentArtists = text
                    }

                    TextField {
                        id: copyrightField
                        placeholderText: "Copyright"
                        Layout.fillWidth: true
                        text: AppContext.lyricsModel.currentCopyright
                        onTextEdited: AppContext.lyricsModel.currentCopyright = text
                    }

                    TextField {
                        id: ccliField
                        placeholderText: "CCLI Number"
                        Layout.fillWidth: true
                        text: AppContext.lyricsModel.currentCcli
                        onTextEdited: AppContext.lyricsModel.currentCcli = text
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        TextField {
                            id: keyField
                            placeholderText: "Key"
                            Layout.fillWidth: true
                            text: AppContext.lyricsModel.currentKey
                            onTextEdited: AppContext.lyricsModel.currentKey = text
                        }

                        TextField {
                            id: tempoField
                            placeholderText: "Tempo"
                            Layout.fillWidth: true
                            text: AppContext.lyricsModel.currentTempo
                            onTextEdited: AppContext.lyricsModel.currentTempo = text
                        }

                        TextField {
                            id: themeField
                            placeholderText: "Theme"
                            Layout.fillWidth: true
                            text: AppContext.lyricsModel.currentTheme
                            onTextEdited: AppContext.lyricsModel.currentTheme = text
                        }
                    }
                }
            }

            GroupBox {
                label: Label {
                    text: "Screen Layouts"
                    font.bold: true
                    font.pixelSize: 16
                    topPadding: 10
                }
                Layout.fillWidth: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    Repeater {
                        model: AppContext.lyricsModel.screenLayoutMappings

                        RowLayout {
                            Layout.fillWidth: true

                            Label {
                                text: modelData.screenName
                                Layout.preferredWidth: 100
                            }
                            ComboBox {
                                id: layoutCombo
                                Layout.fillWidth: true
                                model: AppContext.lyricsModel.availableLayouts()

                                Component.onCompleted: {
                                    currentIndex = indexOfValue(modelData.layoutFile)
                                }

                                onActivated: {
                                    if (currentText !== "") {
                                        AppContext.lyricsModel.setScreenLayoutMapping(modelData.screenName, currentText)
                                    }
                                }
                            }
                            ToolButton {
                                text: "❌"
                                visible: modelData.screenName !== "Audience" && modelData.screenName !== "Stage"
                                ToolTip.text: "Remove layout mapping"
                                ToolTip.visible: hovered
                                onClicked: {
                                    deleteMappingDialog.screenName = modelData.screenName
                                    deleteMappingDialog.open()
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Button {
                            text: "Add Screen Layout..."
                            onClicked: addScreenLayoutMenu.open()

                            Menu {
                                id: addScreenLayoutMenu
                                y: parent.height

                                Instantiator {
                                    model: {
                                        let currentMappings = AppContext.lyricsModel.screenLayoutMappings.map(m => m.screenName)
                                        let allScreens = AppContext.screenModel.getScreenNames()
                                        return allScreens.filter(s => currentMappings.indexOf(s) === -1)
                                    }
                                    onObjectAdded: function(index, object) { addScreenLayoutMenu.insertItem(index, object) }
                                    onObjectRemoved: function(index, object) { addScreenLayoutMenu.removeItem(object) }
                                    delegate: MenuItem {
                                        text: modelData
                                        onTriggered: {
                                            AppContext.lyricsModel.addScreenLayoutMapping(modelData, "Default.fohl")
                                        }
                                    }
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        
                        Label {
                            text: "Background Video:"
                        }
                        
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            
                            ToolButton {
                                id: bgVideoBtn
                                text: "🎥"
                                font.pixelSize: 16
                                ToolTip.text: AppContext.lyricsModel.currentDeck && AppContext.lyricsModel.currentDeck.globalBackgroundMedia !== "" ? "Change Background Video\nCurrent: " + AppContext.lyricsModel.currentDeck.globalBackgroundMedia : "Set Background Video"
                                ToolTip.visible: hovered
                                onClicked: bgVideoDialog.open()
                            }
                            
                            ToolButton {
                                id: clearBgVideoBtn
                                text: "❌"
                                font.pixelSize: 12
                                visible: AppContext.lyricsModel.currentDeck && AppContext.lyricsModel.currentDeck.globalBackgroundMedia !== ""
                                ToolTip.text: "Remove Background Video"
                                ToolTip.visible: hovered
                                onClicked: {
                                    if (AppContext.lyricsModel.currentDeck) {
                                        AppContext.lyricsModel.currentDeck.globalBackgroundMedia = ""
                                        AppContext.lyricsModel.saveDeck(false)
                                    }
                                }
                            }
                            
                            Label {
                                text: AppContext.lyricsModel.currentDeck && AppContext.lyricsModel.currentDeck.globalBackgroundMedia !== "" ? AppContext.lyricsModel.currentDeck.globalBackgroundMedia.split('/').pop() : "None"
                                elide: Text.ElideLeft
                                Layout.fillWidth: true
                                color: "#888"
                                font.italic: true
                            }
                        }
                    }

                    MediaPickerDialog {
                        id: bgVideoDialog
                        mediaType: "video"
                        onFileSelected: function(filePath, fileUrl) {
                            if (AppContext.lyricsModel.currentDeck) {
                                AppContext.lyricsModel.currentDeck.globalBackgroundMedia = filePath
                                AppContext.lyricsModel.saveDeck(false)
                            }
                        }
                    }
                }
            }

            GroupBox {
                id: arrangementGroup
                label: Label {
                    text: "Arrangements"
                    font.bold: true
                    font.pixelSize: 16
                }
                Layout.fillWidth: true
                Layout.fillHeight: true
                property int activeInsertIndex: -1

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 16

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: "Default Arrangement:"
                        }
                        ComboBox {
                            Layout.fillWidth: true
                            model: AppContext.lyricsModel.currentDeck ? AppContext.lyricsModel.currentDeck.arrangementNames : []
                            currentIndex: {
                                if (!AppContext.lyricsModel.currentDeck) return -1;
                                return find(AppContext.lyricsModel.currentDeck.defaultArrangementName);
                            }
                            onActivated: {
                                if (AppContext.lyricsModel.currentDeck && index >= 0) {
                                    console.log("Setting default arrangement to: " + AppContext.lyricsModel.currentDeck.arrangementNames[index]);
                                    AppContext.lyricsModel.setDefaultArrangementName(AppContext.lyricsModel.currentDeck.arrangementNames[index]);
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        
                        Label {
                            text: "Active:"
                        }
                        
                        ComboBox {
                            Layout.fillWidth: true
                            model: AppContext.lyricsModel.currentDeck ? AppContext.lyricsModel.currentDeck.arrangementNames : []
                            currentIndex: {
                                if (!AppContext.lyricsModel.currentDeck) return -1;
                                return find(AppContext.lyricsModel.currentDeck.activeArrangementName);
                            }
                            onActivated: {
                                if (AppContext.lyricsModel.currentDeck && index >= 0) {
                                    AppContext.lyricsModel.setActiveArrangementName(AppContext.lyricsModel.currentDeck.arrangementNames[index]);
                                    arrangementAutoSaveTimer.restart();
                                }
                            }
                        }
                        
                        Button {
                            text: "New..."
                            onClicked: newArrangementDialog.open()
                        }
                        
                        Button {
                            text: "Clone..."
                            onClicked: cloneArrangementDialog.open()
                        }
                        
                        Button {
                            text: "Delete"
                            enabled: (AppContext.lyricsModel.currentDeck ? AppContext.lyricsModel.currentDeck.arrangementNames.length : 0) > 1
                            onClicked: {
                                if (AppContext.lyricsModel.currentDeck) {
                                    AppContext.lyricsModel.removeArrangement(AppContext.lyricsModel.currentDeck.activeArrangementName)
                                    arrangementAutoSaveTimer.restart()
                                }
                            }
                        }
                    }
                    
                    Label {
                        text: "Available Components"
                        font.bold: true
                    }
                    
                    Flow {
                        id: availableFlow
                        Layout.fillWidth: true
                        spacing: 4

                        move: Transition {
                            NumberAnimation { properties: "x,y"; duration: 250; easing.type: Easing.OutQuad }
                        }
                        add: Transition {
                            NumberAnimation { properties: "x,y"; duration: 250; easing.type: Easing.OutQuad }
                        }

                        Repeater {
                            model: AppContext.lyricsModel.availableComponents
                            delegate: Item {
                                id: availableWrapper
                                property string dragText: modelData
                                property bool isDragging: false
                                property int calculatedWidth: availableLabel.implicitWidth + 32
                                width: calculatedWidth
                                height: 32
                                
                                Rectangle {
                                    id: availablePill
                                    height: 32
                                    width: availableWrapper.calculatedWidth
                                    radius: 2
                                    color: window.getComponentColor(modelData)
                                    border.color: Qt.lighter(color, 1.2)
                                    border.width: 1
                                    z: availableWrapper.isDragging ? 100 : 1
                                    
                                    Label {
                                        id: availableLabel
                                        text: modelData
                                        anchors.centerIn: parent
                                        font.bold: true
                                    }
                                    
                                    Drag.active: availableMouseArea.drag.active
                                    Drag.supportedActions: Qt.CopyAction
                                    Drag.dragType: Drag.Internal
                                    Drag.source: availableWrapper
                                    Drag.hotSpot.x: width / 2
                                    Drag.hotSpot.y: height / 2
                                    
                                    states: [
                                        State {
                                            when: availableWrapper.isDragging
                                            ParentChange { target: availablePill; parent: arrangementGroup }
                                        }
                                    ]
                                    
                                    MouseArea {
                                        id: availableMouseArea
                                        anchors.fill: parent
                                        drag.target: availablePill
                                        cursorShape: Qt.OpenHandCursor
                                        
                                        onPressed: {
                                            cursorShape = Qt.ClosedHandCursor
                                            availableWrapper.isDragging = true
                                        }
                                        onReleased: {
                                            cursorShape = Qt.OpenHandCursor
                                            availablePill.Drag.drop()
                                            availableWrapper.isDragging = false
                                            releaseTimer.start()
                                        }
                                        
                                        Timer {
                                            id: releaseTimer
                                            interval: 10
                                            onTriggered: {
                                                if (arrangementGroup.activeInsertIndex !== -1) {
                                                    AppContext.lyricsModel.arrangementModel.removeComponent(arrangementGroup.activeInsertIndex);
                                                    arrangementGroup.activeInsertIndex = -1;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    
                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: palette.mid
                    }
                    
                    Label {
                        text: "Current Sequence"
                        font.bold: true
                    }
                    
                    DropArea {
                        id: sequenceOuterDropArea
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        
                        onEntered: (drag) => {
                            if (drag.source && drag.source.parent === availableFlow) {
                                let text = drag.source.dragText;
                                if (arrangementGroup.activeInsertIndex === -1 && text) {
                                    AppContext.lyricsModel.arrangementModel.appendComponent(text);
                                    arrangementGroup.activeInsertIndex = AppContext.lyricsModel.arrangementModel.rowCount() - 1;
                                }
                            }
                        }
                        
                        onExited: {
                            // Only remove if we actually left the DropArea entirely.
                            // The release timer handles drops outside the window.
                        }
                        
                        onDropped: (drop) => {
                            if (arrangementGroup.activeInsertIndex !== -1) {
                                arrangementGroup.activeInsertIndex = -1; // Commit!
                                drop.accept();
                            } else if (drop.source && drop.source.dragText) {
                                let text = drop.source.dragText;
                                if (text) AppContext.lyricsModel.arrangementModel.appendComponent(text);
                                drop.accept();
                            }
                        }
                        
                        Flow {
                            id: sequenceFlow
                            anchors.fill: parent
                            spacing: 4
                            
                            move: Transition {
                                NumberAnimation { properties: "x,y"; duration: 250; easing.type: Easing.OutQuad }
                            }
                            add: Transition {
                                NumberAnimation { properties: "x,y"; duration: 250; easing.type: Easing.OutQuad }
                            }
                            
                            Repeater {
                                model: AppContext.lyricsModel.arrangementModel
                                delegate: Item {
                                    id: sequenceWrapper
                                    property int itemIndex: index
                                    property int calculatedWidth: sequenceLabel.implicitWidth + 48
                                    width: calculatedWidth
                                    height: 32
                                    
                                    Rectangle {
                                        id: sequencePill
                                        height: 32
                                        width: sequenceWrapper.calculatedWidth
                                        radius: 2
                                        opacity: index === arrangementGroup.activeInsertIndex ? 0 : 1
                                        color: window.getComponentColor(model.componentName)
                                        border.color: Qt.lighter(color, 1.2)
                                        border.width: 1
                                        z: sequenceMouseArea.drag.active ? 100 : 1
                                        
                                        Label {
                                            id: sequenceLabel
                                            text: model.componentName
                                            anchors.verticalCenter: parent.verticalCenter
                                            anchors.left: parent.left
                                            anchors.leftMargin: 16
                                            font.bold: true
                                        }
                                        
                                        ToolButton {
                                            anchors.right: parent.right
                                            anchors.rightMargin: 4
                                            anchors.verticalCenter: parent.verticalCenter
                                            width: 24
                                            height: 24
                                            padding: 0
                                            text: "✕"
                                            font.pixelSize: 12
                                            onClicked: AppContext.lyricsModel.arrangementModel.removeComponent(index)
                                        }
                                        
                                        Drag.active: sequenceMouseArea.drag.active
                                        Drag.supportedActions: Qt.MoveAction
                                        Drag.dragType: Drag.Internal
                                        Drag.source: sequenceWrapper
                                        Drag.hotSpot.x: width / 2
                                        Drag.hotSpot.y: height / 2
                                        
                                        states: [
                                            State {
                                                when: sequenceMouseArea.drag.active
                                                ParentChange { target: sequencePill; parent: arrangementGroup }
                                            }
                                        ]
                                        
                                        MouseArea {
                                            id: sequenceMouseArea
                                            anchors.fill: parent
                                            anchors.rightMargin: 32
                                            drag.target: sequencePill
                                            cursorShape: Qt.OpenHandCursor
                                            
                                            onPressed: cursorShape = Qt.ClosedHandCursor
                                            onReleased: cursorShape = Qt.OpenHandCursor
                                        }
                                    }
                                    
                                    DropArea {
                                        id: sequenceDropArea
                                        anchors.fill: parent
                                        
                                        onPositionChanged: (drag) => {
                                            let sourceIsAvailable = (drag.source && drag.source.parent === availableFlow);
                                            let sourceIsSequence = (drag.source && drag.source.parent === sequenceFlow);
                                            
                                            if (sourceIsAvailable && arrangementGroup.activeInsertIndex !== -1) {
                                                let insertIndex = drag.x < width / 2 ? index : index + 1;
                                                let fromIndex = arrangementGroup.activeInsertIndex;
                                                
                                                if (fromIndex !== insertIndex) {
                                                    if (fromIndex < insertIndex) insertIndex--;
                                                    if (fromIndex !== insertIndex) {
                                                        AppContext.lyricsModel.arrangementModel.moveComponent(fromIndex, insertIndex);
                                                        arrangementGroup.activeInsertIndex = insertIndex;
                                                    }
                                                }
                                            } else if (sourceIsSequence) {
                                                let fromIndex = drag.source.itemIndex;
                                                let insertIndex = drag.x < width / 2 ? index : index + 1;
                                                
                                                if (fromIndex !== insertIndex) {
                                                    if (fromIndex < insertIndex) insertIndex--;
                                                    if (fromIndex !== insertIndex) {
                                                        AppContext.lyricsModel.arrangementModel.moveComponent(fromIndex, insertIndex);
                                                    }
                                                }
                                            }
                                        }
                                        
                                        onDropped: (drop) => {
                                            if (arrangementGroup.activeInsertIndex !== -1) {
                                                arrangementGroup.activeInsertIndex = -1;
                                                drop.accept();
                                            } else if (drop.source && drop.source.dragText) {
                                                let text = drop.source.dragText;
                                                let insertIndex = drop.x < width / 2 ? index : index + 1;
                                                if (text) AppContext.lyricsModel.arrangementModel.insertComponent(insertIndex, text);
                                                drop.accept();
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
        }

        // Fourth Pane: Arrangement Preview
        Rectangle {
            SplitView.fillWidth: true
            color: palette.window

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 16

                Label {
                    text: "Arrangement Preview"
                    font.pixelSize: 16
                    font.bold: true
                    Layout.fillWidth: true
                    topPadding: 10
                }

                GridView {
                    id: previewGrid
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    cellWidth: 320 * zoomSlider.value
                    cellHeight: 180 * zoomSlider.value + 20
                    clip: true
                    model: AppContext.lyricsModel.previewDeckModel

                    delegate: Item {
                        width: previewGrid.cellWidth - 20
                        height: previewGrid.cellHeight - 20

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 1
                            spacing: 0
                            
                            // Decorator Bar
                            Rectangle {
                                Layout.fillHeight: true
                                Layout.preferredWidth: 20
                                color: window.getComponentColor(model.componentName)
                                topLeftRadius: 11
                                bottomLeftRadius: 11
                                
                                Text {
                                    visible: model.isFirst
                                    text: model.componentName ? model.componentName.toUpperCase() : ""
                                    color: "white"
                                    font.bold: true
                                    font.pixelSize: 10
                                    
                                    transformOrigin: Item.TopLeft
                                    rotation: -90
                                    x: (parent.width - implicitHeight) / 2
                                    y: implicitWidth + 12
                                }
                                
                                Label {
                                    anchors.bottom: parent.bottom
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.bottomMargin: 4
                                    text: (index + 1).toString()
                                    font.pixelSize: 10
                                    color: "white"
                                    font.bold: true
                                }
                            }
                            
                            // Slide Preview
                            SlidePreview {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                layoutName: model.mainLayoutFile ? model.mainLayoutFile : "Default.fohl"
                                slideText: model.slideText
                                showBackground: true
                                topRightRadius: 11
                                bottomRightRadius: 11
                            }
                        }

                        // Top-right corner hider (12x12)
                        Image {
                            anchors.top: parent.top
                            anchors.right: parent.right
                            width: 12
                            height: 12
                            z: 9
                            smooth: true
                            antialiasing: true
                            source: {
                                let colorStr = palette.window.toString();
                                let svg = `<svg xmlns="http://www.w3.org/2000/svg" width="12" height="12"><path d="M0,0 L12,0 L12,12 A12,12 0 0,0 0,0 Z" fill="${colorStr}"/></svg>`;
                                return "data:image/svg+xml;utf8," + encodeURIComponent(svg);
                            }
                        }

                        // Bottom-right corner hider (12x12)
                        Image {
                            anchors.bottom: parent.bottom
                            anchors.right: parent.right
                            width: 12
                            height: 12
                            z: 9
                            smooth: true
                            antialiasing: true
                            source: {
                                let colorStr = palette.window.toString();
                                let svg = `<svg xmlns="http://www.w3.org/2000/svg" width="12" height="12"><path d="M12,0 L12,12 L0,12 A12,12 0 0,0 12,0 Z" fill="${colorStr}"/></svg>`;
                                return "data:image/svg+xml;utf8," + encodeURIComponent(svg);
                            }
                        }

                        // Static border for read-only preview
                        Rectangle {
                            anchors.fill: parent
                            color: "transparent"
                            border.color: Qt.rgba(1,1,1,0.1)
                            border.width: 1
                            radius: 12
                            visible: true
                            antialiasing: true
                            z: 10
                        }
                    }
                }
                
                // Bottom control bar for Zoom
                RowLayout {
                    Layout.fillWidth: true
                    Label { 
                        text: "Zoom:" 
                        font.bold: true
                    }
                    Slider {
                        id: zoomSlider
                        Layout.preferredWidth: 200
                        from: 0.5
                        to: 2.5
                        value: 0.75 // Base size multiplier as requested
                    }
                    Label {
                        text: Math.round(zoomSlider.value * 100) + "%"
                    }
                    Item {
                        Layout.fillWidth: true // Spacer
                    }
                }
            }
        }
    }
    
    Platform.FileDialog {
        id: importDialog
        title: "Import FOHD File"
        nameFilters: ["FOH Media SlideDeck (*.fohd)"]
        onAccepted: AppContext.lyricsModel.importFile(file)
    }

    Platform.FileDialog {
        id: importProDialog
        title: "Import ProPresenter Files"
        nameFilters: ["ProPresenter Files (*.pro)"]
        fileMode: Platform.FileDialog.OpenFiles
        onAccepted: AppContext.lyricsModel.importProFiles(files)
    }

    Dialog {
        id: deleteDialog
        title: "Confirm Deletion"
        standardButtons: Dialog.Yes | Dialog.No
        anchors.centerIn: parent
        modal: true
        
        property string filename: ""
        property string deckTitle: ""

        background: Rectangle {
            color: palette.window
            border.color: palette.dark
            border.width: 1
            radius: 8
        }
        
        Label {
            text: "Are you sure you want to delete '" + deleteDialog.deckTitle + "'?\nThis action cannot be undone."
        }
        
        onAccepted: {
            AppContext.lyricsModel.removeLibraryItem(filename)
        }
    }
}
