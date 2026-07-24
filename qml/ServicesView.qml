import QtQuick
import fohmedia
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Effects
import QtQuick.Shapes
import com.company.TimerManager 1.0
import Qt.labs.platform as Platform

SplitView {
    id: servicesView
    width: StackView.view ? StackView.view.width : (parent ? parent.width : 0)
    height: StackView.view ? StackView.view.height : (parent ? parent.height : 0)
    orientation: Qt.Horizontal
    
    property bool isEditingArrangement: false
    property int activeInsertIndex: -1

    TapHandler {
        onTapped: (eventPoint) => {
            if (showTitleField.activeFocus) {
                var pos = showTitleField.mapFromItem(servicesView, eventPoint.position.x, eventPoint.position.y);
                if (!showTitleField.contains(Qt.point(pos.x, pos.y))) {
                    showTitleField.focus = false;
                }
            }
        }
    }

    Timer {
        id: autoSaveTimer
        interval: 200
        repeat: false
        onTriggered: {
            if (AppContext.slideDeckModel.deck) AppContext.slideDeckModel.saveDeck();
        }
    }
    
    function triggerAutoSave() {
        autoSaveTimer.restart();
    }

    Connections {
        target: AppContext.arrangementModel
        function onSequenceChanged() {
            servicesView.triggerAutoSave()
        }
    }

    // Left Panel - Arrangement / Playlist View
    Rectangle {
        id: leftPanelRect
        SplitView.preferredWidth: 250
        SplitView.minimumWidth: 150
        color: palette.base

        ColumnLayout {
            id: leftPanel
            anchors.fill: parent
            anchors.margins: 10

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                RowLayout {
                    Layout.fillWidth: true
                    ToolButton {
                        text: "📄" // New icon
                        font.pixelSize: 20
                        ToolTip.text: "New Service"
                        ToolTip.visible: hovered
                        onClicked: {
                            newServiceInput.text = ""
                            newServiceDialog.open()
                        }
                    }
                    ToolButton {
                        text: "🎵" // Lyrics icon
                        font.pixelSize: 20
                        ToolTip.text: "Load Lyrics into Service"
                        ToolTip.visible: hovered
                        onClicked: loadLyricsDialog.open()
                    }
                    ToolButton {
                        text: "⊕" // Plus/Load icon
                        font.pixelSize: 20
                        ToolTip.text: "Load Service from Library"
                        ToolTip.visible: hovered
                        onClicked: loadServiceDialog.open()
                    }
                    ToolButton {
                        text: "⎘" // Import icon
                        font.pixelSize: 20
                        ToolTip.text: "Import Service from File"
                        ToolTip.visible: hovered
                        onClicked: serviceFileDialog.open()
                    }
                    ToolButton {
                        text: "🗑️" // Trash icon
                        font.pixelSize: 20
                        ToolTip.text: "Remove Services"
                        ToolTip.visible: hovered
                        onClicked: removeServicesDialog.open()
                    }
                }

                TextField {
                    id: showTitleField
                    text: AppContext.showModel.showName
                    font.bold: true
                    font.pixelSize: 16
                    Layout.fillWidth: true
                    padding: 4
                    leftPadding: 2
                    
                    background: Rectangle {
                        color: showTitleField.activeFocus ? "#333333" : "transparent"
                        radius: 4
                    }
                    
                    readOnly: !activeFocus
                    selectByMouse: activeFocus
                    
                    onEditingFinished: {
                        if (text.trim() !== "") {
                            AppContext.showModel.showName = text.trim();
                        } else {
                            text = Qt.binding(() => AppContext.showModel.showName);
                        }
                        focus = false;
                    }
                    
                    Keys.onEscapePressed: {
                        text = Qt.binding(() => AppContext.showModel.showName);
                        focus = false;
                    }

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        onDoubleClicked: {
                            if (AppContext.showModel.hasShow) {
                                showTitleField.forceActiveFocus();
                                showTitleField.selectAll();
                            }
                        }
                        onPressed: (mouse) => {
                            if (!showTitleField.activeFocus) {
                                mouse.accepted = true;
                            } else {
                                mouse.accepted = false;
                            }
                        }
                    }
                    

                }


                Label {
                    text: "Slides"
                    font.bold: true
                    font.pixelSize: 14
                    color: palette.highlight
                }
            }

            // File Dialog for loading a service
            Platform.FileDialog {
                id: serviceFileDialog
                title: "Open Service File"
                nameFilters: ["FOHMedia Service Files (*.fohs *.yaml)"]
                onAccepted: {
                    AppContext.showModel.importShow(serviceFileDialog.file)
                }
            }

            Dialog {
                id: loadServiceDialog
                title: "Load Service"
                standardButtons: Dialog.Cancel
                width: 400
                height: 500
                parent: Overlay.overlay
                anchors.centerIn: parent

                property var allShows: []
                property var filteredShows: []

                onOpened: {
                    allShows = AppContext.showModel.getAvailableShows()
                    filterShows()
                    searchInput.forceActiveFocus()
                }

                function filterShows() {
                    let term = searchInput.text.toLowerCase()
                    if (term === "") {
                        filteredShows = allShows
                    } else {
                        filteredShows = allShows.filter(s => s.title.toLowerCase().includes(term))
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    
                    TextField {
                        id: searchInput
                        Layout.fillWidth: true
                        placeholderText: "Search services..."
                        onTextChanged: loadServiceDialog.filterShows()
                    }
                    
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: loadServiceDialog.filteredShows
                        clip: true
                        delegate: ItemDelegate {
                            width: ListView.view.width
                            text: modelData.title
                            onClicked: {
                                AppContext.showModel.loadShow(modelData.path)
                                loadServiceDialog.close()
                                slideGrid.currentIndex = 0
                                AppContext.displayEngine.jumpToSlide(0)
                            }
                        }
                    }
                }
            }
            
            Dialog {
                id: newServiceDialog
                title: "New Service"
                standardButtons: Dialog.Ok | Dialog.Cancel
                width: 300
                parent: Overlay.overlay
                anchors.centerIn: parent
                
                onOpened: newServiceInput.forceActiveFocus()
                onAccepted: {
                    if (newServiceInput.text.trim() !== "") {
                        AppContext.showModel.newShow(newServiceInput.text.trim())
                        slideGrid.currentIndex = 0
                        AppContext.displayEngine.jumpToSlide(0)
                    }
                }
                
                ColumnLayout {
                    anchors.fill: parent
                    Label {
                        text: "Service Name:"
                    }
                    TextField {
                        id: newServiceInput
                        Layout.fillWidth: true
                        placeholderText: "e.g., Sunday Morning"
                        onAccepted: newServiceDialog.accept()
                    }
                }
            }
            
            Dialog {
                id: removeServicesDialog
                title: "Remove Services"
                standardButtons: Dialog.Close
                width: 500
                height: 600
                parent: Overlay.overlay
                anchors.centerIn: parent
                
                property var allShows: []
                
                onOpened: {
                    allShows = AppContext.showModel.getShowsWithDecks()
                }
                
                ColumnLayout {
                    anchors.fill: parent
                    
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: removeServicesDialog.allShows
                        clip: true
                        spacing: 2
                        
                        delegate: Rectangle {
                            width: ListView.view.width
                            height: contentLayout.implicitHeight + 10
                            color: isHovered.hovered ? palette.alternateBase : "transparent"
                            border.color: palette.mid
                            border.width: 1
                            radius: 4
                            
                            property bool isExpanded: false
                            
                            HoverHandler {
                                id: isHovered
                            }
                            
                            ColumnLayout {
                                id: contentLayout
                                anchors.fill: parent
                                anchors.margins: 5
                                
                                RowLayout {
                                    Layout.fillWidth: true
                                    
                                    ToolButton {
                                        text: "⊖" // Minus icon
                                        font.pixelSize: 20
                                        palette.buttonText: "red"
                                        onClicked: {
                                            confirmRemoveServiceDialog.showPath = modelData.path
                                            confirmRemoveServiceDialog.showTitle = modelData.title
                                            confirmRemoveServiceDialog.open()
                                        }
                                    }
                                    
                                    Label {
                                        text: modelData.title
                                        font.bold: true
                                        font.pixelSize: 16
                                        Layout.fillWidth: true
                                    }
                                    
                                    ToolButton {
                                        text: isExpanded ? "▲" : "▼"
                                        onClicked: isExpanded = !isExpanded
                                    }
                                }
                                
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    visible: isExpanded
                                    Layout.leftMargin: 40
                                    
                                    Repeater {
                                        model: modelData.decks
                                        delegate: Label {
                                            text: "• " + modelData
                                            color: palette.text
                                            font.pixelSize: 13
                                        }
                                    }
                                    
                                    Label {
                                        text: "No slide decks found."
                                        visible: modelData.decks.length === 0
                                        color: palette.mid
                                        font.pixelSize: 13
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            Dialog {
                id: confirmRemoveServiceDialog
                title: "Confirm Removal"
                standardButtons: Dialog.Yes | Dialog.No
                parent: Overlay.overlay
                anchors.centerIn: parent
                
                property string showPath: ""
                property string showTitle: ""
                
                onAccepted: {
                    AppContext.showModel.removeShow(showPath)
                    removeServicesDialog.allShows = AppContext.showModel.getShowsWithDecks()
                }
                
                Label {
                    text: "Are you sure you want to completely delete the service '" + confirmRemoveServiceDialog.showTitle + "'?\n\nThis cannot be undone."
                    wrapMode: Text.WordWrap
                    width: 300
                }
            }

            Dialog {
                id: loadLyricsDialog
                title: "Load Lyrics"
                standardButtons: Dialog.Cancel
                width: 400
                height: 500
                parent: Overlay.overlay
                anchors.centerIn: parent

                property var allDecks: []
                property var filteredDecks: []

                onOpened: {
                    allDecks = AppContext.showModel.getAvailableDecks()
                    filterDecks()
                    lyricsSearchInput.forceActiveFocus()
                }

                function filterDecks() {
                    let term = lyricsSearchInput.text.toLowerCase()
                    if (term === "") {
                        filteredDecks = allDecks
                    } else {
                        filteredDecks = allDecks.filter(d => d.title.toLowerCase().includes(term))
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    
                    TextField {
                        id: lyricsSearchInput
                        Layout.fillWidth: true
                        placeholderText: "Search lyrics..."
                        onTextChanged: loadLyricsDialog.filterDecks()
                    }
                    
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: loadLyricsDialog.filteredDecks
                        clip: true
                        delegate: ItemDelegate {
                            width: ListView.view.width
                            text: modelData.title
                            onClicked: {
                                AppContext.showModel.addDeck(modelData.path)
                                loadLyricsDialog.close()
                            }
                        }
                    }
                }
            }

            Dialog {
                id: confirmRemoveDeckDialog
                title: "Confirm Removal"
                standardButtons: Dialog.Yes | Dialog.No
                parent: Overlay.overlay
                anchors.centerIn: parent

                property int indexToRemove: -1
                property string nameToRemove: ""

                Label {
                    text: "Are you sure you want to remove '" + confirmRemoveDeckDialog.nameToRemove + "' from the service?"
                }

                onAccepted: {
                    if (indexToRemove >= 0) {
                        AppContext.showModel.removeDeck(indexToRemove)
                        indexToRemove = -1
                    }
                }
            }

            ListView {
                id: showListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: AppContext.showModel
                
                Connections {
                    target: AppContext.showModel
                    function onRowsInserted(parent, first, last) {
                        showListView.positionViewAtIndex(last, ListView.Contain)
                    }
                }
                
                delegate: ItemDelegate {
                    id: delegateRoot
                    width: ListView.view.width
                    highlighted: isActive
                    onClicked: AppContext.showModel.setActiveIndex(index)
                    
                    property bool isDragging: dragArea.dragging
                    
                    Behavior on y {
                        enabled: !delegateRoot.isDragging
                        NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
                    }
                    
                    transform: Translate { id: visualTranslate; y: 0 }
                    z: delegateRoot.isDragging ? 10 : 0
                    
                    contentItem: RowLayout {
                        spacing: 10
                        
                        Label {
                            text: "≡"
                            color: delegateRoot.isDragging ? palette.highlight : palette.mid
                            font.pixelSize: 18
                            
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
                                    originalIndex = index
                                    dragging = false
                                }
                                onPositionChanged: (mouse) => {
                                    let currentGlobalY = mapToItem(null, mouse.x, mouse.y).y
                                    let offset = currentGlobalY - globalStartY
                                    if (!dragging && Math.abs(offset) > 5) {
                                        dragging = true
                                    }
                                    if (dragging) {
                                        let delta = Math.round(offset / delegateRoot.height)
                                        let targetIndex = originalIndex + delta
                                        targetIndex = Math.max(0, Math.min(targetIndex, AppContext.showModel.rowCount() - 1))
                                        
                                        if (targetIndex !== originalIndex) {
                                            let diffY = (targetIndex - originalIndex) * delegateRoot.height
                                            
                                            AppContext.showModel.moveDeck(originalIndex, targetIndex)
                                            
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
                                    } else {
                                        AppContext.showModel.setActiveIndex(index)
                                    }
                                }
                                onCanceled: {
                                    visualTranslate.y = 0
                                    dragging = false
                                }
                            }
                        }
                        
                        Label {
                            text: deckName
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            color: isActive ? palette.highlightedText : palette.text
                        }
                        Label {
                            text: "🎥"
                            font.pixelSize: 14
                            visible: hasBackgroundVideo
                            color: isActive ? palette.highlightedText : palette.text
                            ToolTip.text: "Has Background Video"
                            ToolTip.visible: inputMouseArea.containsMouse
                            
                            MouseArea {
                                id: inputMouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                            }
                        }
                        ToolButton {
                            text: "🗑️"
                            font.pixelSize: 14
                            ToolTip.text: "Remove Lyrics"
                            ToolTip.visible: hovered
                            onClicked: {
                                confirmRemoveDeckDialog.indexToRemove = index
                                confirmRemoveDeckDialog.nameToRemove = deckName
                                confirmRemoveDeckDialog.open()
                            }
                        }
                    }
                }
            }
        }
    }

    // Center Panel - Slide Deck Grid
    Rectangle {
        SplitView.fillWidth: true
        color: palette.window
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10

            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: AppContext.slideDeckModel.deck ? AppContext.slideDeckModel.deck.name : "Slide Deck"
                    font.pixelSize: 16
                    font.bold: true
                }
                ComboBox {
                    id: arrangementCombo
                    Layout.preferredWidth: 200
                    model: AppContext.slideDeckModel.deck ? AppContext.slideDeckModel.deck.arrangementNames : []
                    currentIndex: {
                        if (AppContext.slideDeckModel.deck && AppContext.slideDeckModel.deck.activeArrangementName) {
                            return model.indexOf(AppContext.slideDeckModel.deck.activeArrangementName)
                        }
                        return 0
                    }
                    onActivated: {
                        if (AppContext.slideDeckModel.deck) {
                            AppContext.slideDeckModel.deck.activeArrangementName = model[currentIndex]
                            servicesView.triggerAutoSave()
                        }
                    }
                }
                ToolButton {
                    id: editArrangementBtn
                    text: "✎"
                    font.pixelSize: 20
                    ToolTip.text: "Edit Arrangement"
                    ToolTip.visible: hovered
                    checkable: true
                    checked: servicesView.isEditingArrangement
                    onClicked: {
                        servicesView.isEditingArrangement = !servicesView.isEditingArrangement
                        if (!servicesView.isEditingArrangement) {
                            AppContext.slideDeckModel.saveAllEdits()
                        }
                    }
                }
                ToolButton {
                    id: cloneArrangementBtn
                    text: "⧉" // Two joined squares (Clone)
                    font.pixelSize: 20
                    ToolTip.text: "Clone Arrangement"
                    ToolTip.visible: hovered
                    onClicked: {
                        if (AppContext.slideDeckModel.deck && AppContext.slideDeckModel.deck.activeArrangementName) {
                            cloneDialog.sourceName = AppContext.slideDeckModel.deck.activeArrangementName
                            cloneDialog.nameInput = cloneDialog.sourceName + " (Copy)"
                            cloneDialog.open()
                        }
                    }
                }
            }

            // Editable Arrangement Editor
            GroupBox {
                id: arrangementGroup
                Layout.fillWidth: true
                visible: servicesView.isEditingArrangement
                
                label: RowLayout {
                    spacing: 8
                    
                    Label {
                        text: "Arrangement Editor"
                        font.bold: true
                    }
                    Rectangle {
                        width: 1
                        height: 24
                        color: Qt.rgba(1, 1, 1, 0.2)
                        Layout.alignment: Qt.AlignVCenter
                        Layout.leftMargin: 8
                        Layout.rightMargin: 8
                    }

                    ToolButton {
                        id: insertSlideBtn
                        text: "+"
                        font.pixelSize: 20
                        ToolTip.text: "Insert Blank Slide"
                        ToolTip.visible: hovered
                        onClicked: {
                            if (slideGrid.currentIndex >= 0) {
                                AppContext.slideDeckModel.insertBlankSlideAfter(slideGrid.currentIndex)
                            }
                        }
                    }
                    
                    ToolButton {
                        id: removeSlideBtn
                        text: "-"
                        font.pixelSize: 20
                        ToolTip.text: "Remove Slide"
                        ToolTip.visible: hovered
                        onClicked: {
                            if (slideGrid.currentIndex >= 0) {
                                AppContext.slideDeckModel.removeSlideAndSave(slideGrid.currentIndex)
                            }
                        }
                    }
                    
                    Button {
                        id: changeComponentBtn
                        text: "Change Component ▾"
                        ToolTip.text: "Split into new component group"
                        ToolTip.visible: hovered
                        enabled: slideGrid.currentIndex >= 0
                        onClicked: {
                            changeComponentMenu.open()
                        }

                        Menu {
                            id: changeComponentMenu
                            y: changeComponentBtn.height

                            Instantiator {
                                model: AppContext.slideDeckModel.availableComponentNames
                                onObjectAdded: function(index, object) {
                                    changeComponentMenu.insertItem(index, object)
                                }
                                onObjectRemoved: function(index, object) {
                                    changeComponentMenu.removeItem(object)
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
                                    onClicked: {
                                        changeComponentMenu.close()
                                        if (slideGrid.currentIndex >= 0) {
                                            AppContext.slideDeckModel.splitComponentGroup(slideGrid.currentIndex, modelData)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8
                
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
                        model: AppContext.slideDeckModel.deck ? AppContext.slideDeckModel.deck.componentNames : []
                        delegate: Item {
                            id: availableWrapper
                            property bool isDragging: false
                            property int calculatedWidth: availableLabel.implicitWidth + 32
                            width: calculatedWidth
                            height: 32
                            property string dragText: modelData
                            
                            Rectangle {
                                id: availablePill
                                width: availableWrapper.calculatedWidth
                                height: 32
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
                                            if (servicesView.activeInsertIndex !== -1) {
                                                AppContext.arrangementModel.removeComponent(servicesView.activeInsertIndex);
                                                servicesView.activeInsertIndex = -1;
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
                    color: window.palette.mid
                }
                
                Label {
                    text: "Current Arrangement Sequence"
                    font.bold: true
                }
                
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.max(40, sequenceFlow.implicitHeight)
                    DropArea {
                        id: sequenceOuterDropArea
                        anchors.fill: parent
                        
                        onEntered: (drag) => {
                            if (drag.source && drag.source.parent === availableFlow) {
                                let text = drag.source.dragText;
                                if (servicesView.activeInsertIndex === -1 && text) {
                                    AppContext.arrangementModel.appendComponent(text);
                                    servicesView.activeInsertIndex = AppContext.arrangementModel.rowCount() - 1;
                                }
                            }
                        }
                        
                        onExited: {
                            // Only remove if we actually left the DropArea entirely.
                            // The release timer handles drops outside the window.
                        }
                        
                        onDropped: (drop) => {
                            if (servicesView.activeInsertIndex !== -1) {
                                servicesView.activeInsertIndex = -1; // Commit!
                                drop.accept();
                                servicesView.triggerAutoSave();
                            } else if (drop.source && drop.source.dragText) {
                                let text = drop.source.dragText;
                                if (text) {
                                    servicesView.triggerAutoSave();
                                    AppContext.arrangementModel.appendComponent(text);
                                }
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
                                model: AppContext.arrangementModel
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
                                        opacity: index === servicesView.activeInsertIndex ? 0 : 1
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
                                            onClicked: {
                                                servicesView.triggerAutoSave();
                                                AppContext.arrangementModel.removeComponent(index);
                                            }
                                        }
                                        
                                        Drag.active: sequenceMouseArea.drag.active
                                        Drag.supportedActions: Qt.MoveAction
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
                                            onReleased: {
                                                cursorShape = Qt.OpenHandCursor
                                                servicesView.triggerAutoSave();
                                            }
                                        }
                                    }
                                    
                                    DropArea {
                                        id: sequenceDropArea
                                        anchors.fill: parent
                                        
                                        onPositionChanged: (drag) => {
                                            let sourceIsAvailable = (drag.source && drag.source.parent === availableFlow);
                                            let sourceIsSequence = (drag.source && drag.source.parent === sequenceFlow);
                                            
                                            if (sourceIsAvailable && servicesView.activeInsertIndex !== -1) {
                                                let insertIndex = drag.x < width / 2 ? index : index + 1;
                                                let fromIndex = servicesView.activeInsertIndex;
                                                
                                                if (fromIndex !== insertIndex) {
                                                    if (fromIndex < insertIndex) insertIndex--;
                                                    if (fromIndex !== insertIndex) {
                                                        servicesView.triggerAutoSave();
                                                        AppContext.arrangementModel.moveComponent(fromIndex, insertIndex);
                                                        servicesView.activeInsertIndex = insertIndex;
                                                    }
                                                }
                                            } else if (sourceIsSequence) {
                                                let fromIndex = drag.source.itemIndex;
                                                let insertIndex = drag.x < width / 2 ? index : index + 1;
                                                
                                                if (fromIndex !== insertIndex) {
                                                    if (fromIndex < insertIndex) insertIndex--;
                                                    if (fromIndex !== insertIndex) {
                                                        servicesView.triggerAutoSave();
                                                        AppContext.arrangementModel.moveComponent(fromIndex, insertIndex);
                                                    }
                                                }
                                            }
                                        }
                                        
                                        onDropped: (drop) => {
                                            if (servicesView.activeInsertIndex !== -1) {
                                                servicesView.activeInsertIndex = -1;
                                                drop.accept();
                                                servicesView.triggerAutoSave();
                                            } else if (drop.source && drop.source.dragText) {
                                                let text = drop.source.dragText;
                                                let insertIndex = drop.x < width / 2 ? index : index + 1;
                                                if (text) {
                                                    servicesView.triggerAutoSave();
                                                    AppContext.arrangementModel.insertComponent(insertIndex, text);
                                                }
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

            // Read-only Arrangement Chips
            Flow {
                id: arrangementList
                Layout.fillWidth: true
                visible: !servicesView.isEditingArrangement
                spacing: 4
                clip: true
                bottomPadding: 17
                
                Repeater {
                    model: AppContext.arrangementModel
                    delegate: Rectangle {
                        width: componentText.implicitWidth + 32
                        height: 32
                        radius: 2
                    color: window.getComponentColor(componentName)
                    border.color: isActive ? window.palette.highlight : Qt.lighter(color, 1.2)
                    border.width: isActive ? 2 : 1
                    
                    Label {
                        id: componentText
                        anchors.centerIn: parent
                        text: componentName
                        font.bold: true
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            AppContext.arrangementModel.setActiveComponentIndex(index)
                            // Jump to the specific occurrence in the arrangement
                            let slideIndex = AppContext.slideDeckModel.firstSlideIndexOfArrangementIndex(index)
                            if (slideIndex !== -1) {
                                slideGrid.positionViewAtIndex(slideIndex, GridView.Beginning)
                                slideGrid.currentIndex = slideIndex
                                AppContext.displayEngine.jumpToSlide(slideIndex)
                            }
                        }
                    }
                }
                }
            }
            GridView {
                id: slideGrid
                Layout.fillWidth: true
                Layout.fillHeight: true
                cellWidth: 320 * zoomSlider.value
                cellHeight: 180 * zoomSlider.value + 20
                clip: true
                focus: true
                keyNavigationEnabled: false
                onCurrentIndexChanged: AppContext.slideDeckModel.selectedSlideIndex = currentIndex
                
                Connections {
                    target: AppContext.slideDeckModel
                    function onSelectedSlideChanged() {
                        if (slideGrid.currentIndex !== AppContext.slideDeckModel.selectedSlideIndex) {
                            slideGrid.currentIndex = AppContext.slideDeckModel.selectedSlideIndex
                        }
                    }
                }
                
                Keys.onSpacePressed: event => { if (!servicesView.isEditingArrangement) { advanceSlide(); event.accepted = true; } }
                Keys.onRightPressed: event => { if (!servicesView.isEditingArrangement) { advanceSlide(); event.accepted = true; } }
                Keys.onDownPressed: event => { if (!servicesView.isEditingArrangement) { advanceSlide(); event.accepted = true; } }
                Keys.onLeftPressed: event => { if (!servicesView.isEditingArrangement) { reverseSlide(); event.accepted = true; } }
                Keys.onUpPressed: event => { if (!servicesView.isEditingArrangement) { reverseSlide(); event.accepted = true; } }
                function advanceSlide() {
                    if (currentIndex === -1) {
                        currentIndex = 0;
                        AppContext.displayEngine.jumpToSlide(0);
                        return;
                    }
                    if (currentIndex < AppContext.slideDeckModel.rowCount() - 1) {
                        currentIndex++;
                        AppContext.displayEngine.jumpToSlide(currentIndex);
                    } else if (AppContext.showModel.activeIndex < AppContext.showModel.rowCount() - 1) {
                        AppContext.showModel.setActiveIndex(AppContext.showModel.activeIndex + 1);
                        slideGrid.currentIndex = 0;
                        AppContext.displayEngine.jumpToSlide(0);
                    }
                }
                
                function reverseSlide() {
                    if (currentIndex > 0) {
                        currentIndex--;
                        AppContext.displayEngine.jumpToSlide(currentIndex);
                    } else if (AppContext.showModel.activeIndex > 0) {
                        AppContext.showModel.setActiveIndex(AppContext.showModel.activeIndex - 1);
                        slideGrid.currentIndex = AppContext.slideDeckModel.rowCount() - 1;
                        AppContext.displayEngine.jumpToSlide(slideGrid.currentIndex);
                    }
                }
                
                model: AppContext.slideDeckModel
                
                delegate: Item {
                    id: slideCardRoot
                    width: slideGrid.cellWidth - 20
                    height: slideGrid.cellHeight - 20

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
                                id: verticalText
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
                            id: slidePreviewItem
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            layoutName: model.layouts ? (model.layouts["Audience"] || "Default") : "Default"
                            slideText: model.slideText
                            showBackground: showBackgroundsSwitch.checked
                            globalBackgroundMedia: AppContext.slideDeckModel.deck ? AppContext.slideDeckModel.deck.globalBackgroundMedia : ""
                            forceOpaqueBackground: true
                            topRightRadius: 11
                            bottomRightRadius: 11
                            quickEditEnabled: servicesView.isEditingArrangement
                            onTextEditFinished: function(newText) {
                                AppContext.slideDeckModel.setSlideText(index, newText)
                            }
                            onEditingStarted: {
                                if (slideGrid.currentIndex !== index) {
                                    slideGrid.currentIndex = index
                                    AppContext.displayEngine.jumpToSlide(index)
                                }
                                slidePreviewItem.forceTextEditFocus()
                            }
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

                    // Selection border overlay
                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        border.color: slideCardRoot.GridView.isCurrentItem ? palette.highlight : Qt.rgba(1,1,1,0.1)
                        border.width: slideCardRoot.GridView.isCurrentItem ? 3 : 1
                        radius: 12
                        visible: true
                        antialiasing: true
                        z: 10
                    }

                    MouseArea {
                        anchors.fill: parent
                        enabled: !servicesView.isEditingArrangement
                        onClicked: {
                            slideGrid.forceActiveFocus()
                            slideGrid.currentIndex = index
                            AppContext.displayEngine.jumpToSlide(index)
                        }
                    }
                }
            }

            // Bottom control bar for Zoom and Settings
            Item {
                Layout.fillWidth: true
                height: 1
                Rectangle {
                    x: -20
                    width: parent.width + 40
                    height: 1
                    color: window.palette.dark
                }
            }
            
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
                    value: 1.0 // Base size multiplier
                }
                Label {
                    text: Math.round(zoomSlider.value * 100) + "%"
                }
                Item {
                    Layout.fillWidth: true // Spacer
                }
                Switch {
                    id: showBackgroundsSwitch
                    text: "Show Backgrounds"
                    checked: false
                    
                    indicator: Rectangle {
                        implicitWidth: 40
                        implicitHeight: 22
                        x: showBackgroundsSwitch.leftPadding
                        y: parent.height / 2 - height / 2
                        radius: 11
                        color: showBackgroundsSwitch.checked ? palette.highlight : palette.mid
                        border.color: Qt.darker(color, 1.2)
                        border.width: 1

                        Rectangle {
                            x: showBackgroundsSwitch.checked ? parent.width - width - 2 : 2
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
                        text: showBackgroundsSwitch.text
                        font: showBackgroundsSwitch.font
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: showBackgroundsSwitch.indicator.width + showBackgroundsSwitch.spacing
                    }
                }
            }
        }
    }
    
    // Right Panel - Monitors & Controls
    Rectangle {
        id: rightPanelRect
        SplitView.preferredWidth: 360
        SplitView.minimumWidth: 150
        color: palette.base
        
        ColumnLayout {
            id: monitorsLayout
            anchors.fill: parent
            anchors.margins: 10
            spacing: 15
            
            Label {
                text: "Monitors"
                font.pixelSize: 16
                font.bold: true
            }
            property var previewData: ({})

            Connections {
                target: AppContext.slideDeckModel
                function onSelectedSlideChanged() {
                    monitorsLayout.previewData = AppContext.slideDeckModel.getSlideDataForPreview(slideGrid.currentIndex)
                }
                function onSlidesRebuilt() {
                    monitorsLayout.previewData = AppContext.slideDeckModel.getSlideDataForPreview(slideGrid.currentIndex)
                }
            }
            
            Connections {
                target: slideGrid
                function onCurrentIndexChanged() {
                    monitorsLayout.previewData = AppContext.slideDeckModel.getSlideDataForPreview(slideGrid.currentIndex)
                }
            }
            
            Component.onCompleted: {
                monitorsLayout.previewData = AppContext.slideDeckModel.getSlideDataForPreview(slideGrid.currentIndex)
            }

            Repeater {
                model: AppContext.screenModel
                delegate: ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5
                    
                    RowLayout {
                        Layout.fillWidth: true
                        Switch {
                            id: monitorSwitch
                            text: model.name
                            checked: model.monitorEnabled
                            onCheckedChanged: model.monitorEnabled = checked

                            indicator: Rectangle {
                                implicitWidth: 40
                                implicitHeight: 22
                                x: monitorSwitch.leftPadding
                                y: parent.height / 2 - height / 2
                                radius: 11
                                color: monitorSwitch.checked ? palette.highlight : palette.mid
                                border.color: Qt.darker(color, 1.2)
                                border.width: 1

                                Rectangle {
                                    x: monitorSwitch.checked ? parent.width - width - 2 : 2
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
                                text: monitorSwitch.text
                                font: monitorSwitch.font
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: monitorSwitch.indicator.width + monitorSwitch.spacing
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: width * (9/16)
                        color: "black"
                        border.color: palette.dark
                        visible: monitorSwitch.checked

                        SlidePreview {
                            anchors.fill: parent
                            layoutName: monitorsLayout.previewData.layouts ? (monitorsLayout.previewData.layouts[model.name] || "Default") : "Default"
                            slideText: monitorsLayout.previewData.slideText || ""
                            nextSlideText: monitorsLayout.previewData.nextSlideText || ""
                            showBackground: true
                            globalBackgroundMedia: AppContext.slideDeckModel.deck ? AppContext.slideDeckModel.deck.globalBackgroundMedia : ""
                            forceOpaqueBackground: true
                            quickEditEnabled: false
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: palette.mid
            }
            
            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: "Timers"
                    font.pixelSize: 16
                    font.bold: true
                    Layout.fillWidth: true
                }
                Button {
                    text: "Add Timer"
                    onClicked: {
                        AppContext.timerManager.addTimer("New Timer", 0, 0, new Date())
                    }
                }
            }
            
            ListView {
                id: timersList
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                clip: true
                spacing: 5
                
                model: AppContext.timerManager
                delegate: Item {
                    id: delegateRoot
                    width: ListView.view.width
                    height: timerCard.height
                    z: timerCard.isDragging ? 10 : 0
                    
                    Behavior on y {
                        enabled: !timerCard.isDragging
                        NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
                    }
                    
                    TimerControlCard {
                        id: timerCard
                        width: parent.width
                        timerIndex: index
                    }
                }
            }
        }
    }
    
    Dialog {
        id: cloneDialog
        title: "Clone Arrangement"
        standardButtons: Dialog.Ok | Dialog.Cancel
        anchors.centerIn: parent
        modal: true
        width: 400
        
        property string sourceName: ""
        property alias nameInput: nameField.text

        background: Rectangle {
            color: palette.window
            border.color: palette.dark
            border.width: 1
            radius: 8
        }

        ColumnLayout {
            anchors.fill: parent
            Label { text: "New Arrangement Name:" }
            TextField {
                id: nameField
                Layout.fillWidth: true
                selectByMouse: true
                onAccepted: cloneDialog.accept()
            }
        }
        
        onAccepted: {
            if (AppContext.slideDeckModel.deck) {
                AppContext.slideDeckModel.deck.cloneArrangement(sourceName, nameField.text.trim())
                AppContext.slideDeckModel.deck.activeArrangementName = nameField.text.trim()
                servicesView.triggerAutoSave()
            }
        }
        
        onOpened: {
            nameField.forceActiveFocus()
            nameField.selectAll()
        }
    }
}
