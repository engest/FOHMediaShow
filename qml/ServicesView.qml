import QtQuick
import fohmedia
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Effects
import QtQuick.Shapes
import com.company.TimerManager 1.0

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
                        FohToolTip {

                            visible: parent.hovered

                            text: "New Service"

                        }
                        onClicked: {
                            newServiceInput.text = ""
                            newServiceDialog.open()
                        }
                    }
                    ToolButton {
                        text: "🎵" // Lyrics icon
                        font.pixelSize: 20
                        FohToolTip {

                            visible: parent.hovered

                            text: "Load Lyrics into Service"

                        }
                        onClicked: loadLyricsDialog.open()
                    }
                    ToolButton {
                        text: "⊕" // Plus/Load icon
                        font.pixelSize: 20
                        FohToolTip {

                            visible: parent.hovered

                            text: "Load Service from Library"

                        }
                        onClicked: loadServiceDialog.open()
                    }
                    ToolButton {
                        text: "⎘" // Import icon
                        font.pixelSize: 20
                        FohToolTip {

                            visible: parent.hovered

                            text: "Import Service from File"

                        }
                        onClicked: serviceFileDialog.open()
                    }
                    ToolButton {
                        text: "🗑️" // Trash icon
                        font.pixelSize: 20
                        FohToolTip {

                            visible: parent.hovered

                            text: "Remove Services"

                        }
                        onClicked: removeServicesDialog.open()
                    }
                }

                // The show title field has been removed and replaced by section headers in the list view.
            }

            // File Dialog for loading a service
            FileDialog {
                id: serviceFileDialog
                title: "Open Service File"
                nameFilters: ["FOHMedia Service Files (*.fohs *.yaml)"]
                onAccepted: {
                    AppContext.showModel.importShow(serviceFileDialog.selectedFile)
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
                standardButtons: Dialog.Close
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

            ScrollView {
                id: showScrollView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: availableWidth
                
                ScrollBar.vertical: ScrollBar {
                    id: showScrollBar
                    policy: ScrollBar.AsNeeded
                    width: 8
                    hoverEnabled: true
                    active: hovered || pressed

                    contentItem: Rectangle {
                        implicitWidth: 6
                        radius: width / 2
                        color: showScrollBar.pressed ? palette.highlight : (showScrollBar.hovered ? "#888888" : "#555555")
                        opacity: showScrollBar.active ? 0.8 : (showScrollBar.size < 1.0 ? 0.35 : 0.0)
                        Behavior on opacity { NumberAnimation { duration: 200 } }
                    }
                    background: Rectangle {
                        implicitWidth: 8
                        color: "transparent"
                    }
                }

                Column {
                    id: showsContainer
                    width: showScrollView.availableWidth
                    spacing: 12

                    Repeater {
                        id: showSectionsRepeater
                        model: AppContext.showModel.shows

                        delegate: Column {
                            id: showSection
                            width: showsContainer.width
                            spacing: 4

                            property var showObj: modelData

                            // Section Header (Fixed and static above the decks)
                            Rectangle {
                                id: sectionHeader
                                width: parent.width
                                height: 40
                                color: "transparent"

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 4

                                    TextField {
                                        id: sectionTitleField
                                        text: showObj ? showObj.name : ""
                                        font.bold: true
                                        font.pixelSize: 16
                                        Layout.fillWidth: true
                                        padding: 4
                                        leftPadding: 2

                                        background: Rectangle {
                                            color: sectionTitleField.activeFocus ? "#333333" : "transparent"
                                            radius: 4
                                        }

                                        readOnly: !activeFocus
                                        selectByMouse: activeFocus

                                        onEditingFinished: {
                                            if (showObj && text.trim() !== "" && text.trim() !== showObj.name) {
                                                AppContext.showModel.renameShow(showObj.name, text.trim());
                                            } else if (showObj) {
                                                text = Qt.binding(() => showObj.name);
                                            }
                                            focus = false;
                                        }

                                        Keys.onEscapePressed: {
                                            if (showObj) text = Qt.binding(() => showObj.name);
                                            focus = false;
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            acceptedButtons: Qt.LeftButton
                                            onDoubleClicked: {
                                                sectionTitleField.forceActiveFocus();
                                                sectionTitleField.selectAll();
                                            }
                                            onPressed: (mouse) => {
                                                if (!sectionTitleField.activeFocus) {
                                                    mouse.accepted = true;
                                                } else {
                                                    mouse.accepted = false;
                                                }
                                            }
                                        }
                                    }

                                    ToolButton {
                                        text: "✕"
                                        font.pixelSize: 14
                                        onClicked: {
                                            if (showObj) AppContext.showModel.unloadShow(showObj.name);
                                        }
                                        FohToolTip {
                                            visible: parent.hovered
                                            text: "Unload Service"
                                        }
                                    }
                                }
                            }

                            // Placeholder if empty show
                            Rectangle {
                                visible: showObj ? showObj.deckCount === 0 : true
                                width: parent.width
                                height: 32
                                color: "transparent"
                                Label {
                                    anchors.centerIn: parent
                                    text: "No slide decks in service."
                                    color: palette.mid
                                    font.pixelSize: 13
                                }
                            }

                            // The draggable deck list for this show
                            Column {
                                id: deckColumn
                                width: parent.width
                                spacing: 2

                                move: Transition {
                                    NumberAnimation { properties: "y"; duration: 250; easing.type: Easing.OutQuad }
                                }
                                add: Transition {
                                    NumberAnimation { properties: "y"; duration: 250; easing.type: Easing.OutQuad }
                                }

                                Repeater {
                                    id: deckRepeater
                                    model: showObj

                                    delegate: Item {
                                        id: deckCardContainer
                                        width: deckColumn.width
                                        height: 40

                                        property int itemFlatIndex: flatIndex
                                        property int itemDeckIndex: index
                                        property string itemDeckName: deckName
                                        property bool itemIsActive: isActive
                                        property bool itemHasBgVideo: hasBackgroundVideo

                                        Rectangle {
                                            id: deckCard
                                            width: deckCardContainer.width
                                            height: 40
                                            radius: 4
                                            color: deckCardContainer.itemIsActive ? palette.highlight : (deckMouseArea.containsMouse ? palette.midlight : "transparent")
                                            border.color: deckMouseArea.drag.active ? palette.highlight : (deckCardContainer.itemIsActive ? Qt.lighter(palette.highlight, 1.2) : "transparent")
                                            border.width: deckMouseArea.drag.active || deckCardContainer.itemIsActive ? 1 : 0
                                            z: deckMouseArea.drag.active ? 100 : 1
                                            opacity: deckMouseArea.drag.active ? 0.85 : 1.0

                                            Drag.active: deckMouseArea.drag.active
                                            Drag.supportedActions: Qt.MoveAction
                                            Drag.dragType: Drag.Internal
                                            Drag.source: deckCardContainer
                                            Drag.hotSpot.x: width / 2
                                            Drag.hotSpot.y: height / 2

                                            states: [
                                                State {
                                                    when: deckMouseArea.drag.active
                                                    ParentChange { target: deckCard; parent: leftPanelRect }
                                                }
                                            ]

                                            MouseArea {
                                                id: deckMouseArea
                                                anchors.fill: parent
                                                anchors.rightMargin: 36
                                                hoverEnabled: true
                                                drag.target: deckCard
                                                cursorShape: drag.active ? Qt.ClosedHandCursor : Qt.OpenHandCursor

                                                onClicked: {
                                                    AppContext.showModel.setActiveIndex(deckCardContainer.itemFlatIndex);
                                                }
                                            }

                                            RowLayout {
                                                anchors.fill: parent
                                                anchors.leftMargin: 8
                                                anchors.rightMargin: 8
                                                spacing: 10

                                                Label {
                                                    text: "≡"
                                                    color: deckMouseArea.drag.active ? palette.highlight : (deckCardContainer.itemIsActive ? palette.highlightedText : palette.mid)
                                                    font.pixelSize: 18
                                                    Layout.alignment: Qt.AlignVCenter
                                                }

                                                Label {
                                                    text: deckCardContainer.itemDeckName
                                                    Layout.fillWidth: true
                                                    elide: Text.ElideRight
                                                    color: deckCardContainer.itemIsActive ? palette.highlightedText : palette.text
                                                }

                                                Label {
                                                    text: "🎥"
                                                    font.pixelSize: 14
                                                    visible: deckCardContainer.itemHasBgVideo
                                                    color: deckCardContainer.itemIsActive ? palette.highlightedText : palette.text
                                                    FohToolTip {
                                                        visible: bgVideoMouseArea.containsMouse
                                                        text: "Has Background Video"
                                                    }

                                                    MouseArea {
                                                        id: bgVideoMouseArea
                                                        anchors.fill: parent
                                                        hoverEnabled: true
                                                    }
                                                }

                                                ToolButton {
                                                    text: "🗑️"
                                                    font.pixelSize: 14
                                                    FohToolTip {
                                                        visible: parent.hovered
                                                        text: "Remove Lyrics"
                                                    }
                                                    onClicked: {
                                                        confirmRemoveDeckDialog.indexToRemove = deckCardContainer.itemFlatIndex;
                                                        confirmRemoveDeckDialog.nameToRemove = deckCardContainer.itemDeckName;
                                                        confirmRemoveDeckDialog.open();
                                                    }
                                                }
                                            }
                                        }

                                        DropArea {
                                            id: deckDropArea
                                            anchors.fill: parent

                                            onPositionChanged: (drag) => {
                                                let sourceIsDeck = (drag.source && drag.source.itemDeckIndex !== undefined);
                                                if (sourceIsDeck) {
                                                    let fromIndex = drag.source.itemDeckIndex;
                                                    let targetIndex = drag.y < height / 2 ? index : index + 1;
                                                    if (fromIndex !== targetIndex) {
                                                        if (fromIndex < targetIndex) targetIndex--;
                                                        if (fromIndex !== targetIndex && showObj) {
                                                            showObj.moveDeck(fromIndex, targetIndex);
                                                            triggerAutoSave();
                                                        }
                                                    }
                                                }
                                            }

                                            onDropped: (drop) => {
                                                triggerAutoSave();
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
                    FohToolTip {

                        visible: parent.hovered

                        text: "Edit Arrangement"

                    }
                    checkable: true
                    checked: servicesView.isEditingArrangement
                    onClicked: {
                        servicesView.isEditingArrangement = !servicesView.isEditingArrangement
                        if (!servicesView.isEditingArrangement) {
                            AppContext.slideDeckModel.saveAllEdits()
                            if (AppContext.displayEngine) {
                                let liveIndex = AppContext.displayEngine.currentSlideNumber - 1
                                if (liveIndex >= 0 && slideGrid.currentIndex !== liveIndex) {
                                    slideGrid.currentIndex = liveIndex
                                }
                            }
                        }
                    }
                }
                ToolButton {
                    id: cloneArrangementBtn
                    text: "⧉" // Two joined squares (Clone)
                    font.pixelSize: 20
                    FohToolTip {

                        visible: parent.hovered

                        text: "Clone Arrangement"

                    }
                    onClicked: {
                        if (AppContext.slideDeckModel.deck && AppContext.slideDeckModel.deck.activeArrangementName) {
                            cloneDialog.sourceName = AppContext.slideDeckModel.deck.activeArrangementName
                            cloneDialog.nameInput = cloneDialog.sourceName + " (Copy)"
                            cloneDialog.open()
                        }
                    }
                }
                ToolButton {
                    id: renameArrangementBtn
                    text: "Ab" // Text for rename
                    font.pixelSize: 20
                    FohToolTip {
                        visible: parent.hovered
                        text: "Rename Arrangement"
                    }
                    onClicked: {
                        if (AppContext.slideDeckModel.deck && AppContext.slideDeckModel.deck.activeArrangementName) {
                            renameDialog.sourceName = AppContext.slideDeckModel.deck.activeArrangementName
                            renameDialog.nameInput = renameDialog.sourceName
                            renameDialog.open()
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
                        FohToolTip {

                            visible: parent.hovered

                            text: "Insert Blank Slide"

                        }
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
                        FohToolTip {

                            visible: parent.hovered

                            text: "Remove Slide"

                        }
                        onClicked: {
                            if (slideGrid.currentIndex >= 0) {
                                AppContext.slideDeckModel.removeSlideAndSave(slideGrid.currentIndex)
                            }
                        }
                    }
                    
                    Button {
                        id: changeComponentBtn
                        text: "Change Component ▾"
                        FohToolTip {

                            visible: parent.hovered

                            text: "Split into new component group"

                        }
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
                                    color: availablePill.color.hslLightness > 0.5 ? "black" : "white"
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
                                            color: sequencePill.color.hslLightness > 0.5 ? "black" : "white"
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
                        color: parent.color.hslLightness > 0.5 ? "black" : "white"
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
                currentIndex: AppContext.slideDeckModel.selectedSlideIndex

                ScrollBar.vertical: ScrollBar {
                    id: slideGridScrollBar
                    policy: ScrollBar.AsNeeded
                    width: 8
                    hoverEnabled: true
                    active: hovered || pressed

                    contentItem: Rectangle {
                        implicitWidth: slideGridScrollBar.hovered || slideGridScrollBar.pressed ? 8 : 6
                        radius: width / 2
                        color: slideGridScrollBar.pressed ? palette.highlight : (slideGridScrollBar.hovered ? "#aaaaaa" : "#666666")
                        opacity: slideGridScrollBar.active ? 0.9 : (slideGridScrollBar.size < 1.0 ? 0.45 : 0.0)
                        Behavior on opacity { NumberAnimation { duration: 150 } }
                        Behavior on implicitWidth { NumberAnimation { duration: 150 } }
                    }
                    background: Rectangle {
                        implicitWidth: 8
                        color: "transparent"
                    }
                }
                onCurrentIndexChanged: {
                    if (currentIndex !== -1) {
                        if (currentIndex !== AppContext.slideDeckModel.selectedSlideIndex) {
                            AppContext.slideDeckModel.selectedSlideIndex = currentIndex
                        }
                        let arrIndex = AppContext.slideDeckModel.arrangementIndexForSlideIndex(currentIndex)
                        if (arrIndex !== -1) {
                            AppContext.arrangementModel.setActiveComponentIndex(arrIndex)
                        }
                    }
                }
                
                Component.onCompleted: {
                    positionViewTimer.start()
                }
                
                Timer {
                    id: positionViewTimer
                    interval: 100
                    onTriggered: {
                        let cols = Math.max(1, Math.floor(slideGrid.width / slideGrid.cellWidth))
                        let lookAheadIndex = Math.min(AppContext.slideDeckModel.rowCount() - 1, slideGrid.currentIndex + cols)
                        if (lookAheadIndex >= 0) slideGrid.positionViewAtIndex(lookAheadIndex, GridView.Contain)
                        if (slideGrid.currentIndex >= 0) slideGrid.positionViewAtIndex(slideGrid.currentIndex, GridView.Contain)
                    }
                }
                
                Connections {
                    target: AppContext.slideDeckModel
                    function onSelectedSlideChanged() {
                        if (slideGrid.currentIndex !== AppContext.slideDeckModel.selectedSlideIndex) {
                            slideGrid.currentIndex = AppContext.slideDeckModel.selectedSlideIndex
                        }
                        let cols = Math.max(1, Math.floor(slideGrid.width / slideGrid.cellWidth))
                        let lookAheadIndex = Math.min(AppContext.slideDeckModel.rowCount() - 1, slideGrid.currentIndex + cols)
                        if (lookAheadIndex >= 0) slideGrid.positionViewAtIndex(lookAheadIndex, GridView.Contain)
                        if (slideGrid.currentIndex >= 0) slideGrid.positionViewAtIndex(slideGrid.currentIndex, GridView.Contain)
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
                        if (AppContext.showModel.isSameShow(AppContext.showModel.activeIndex, AppContext.showModel.activeIndex + 1)) {
                            AppContext.showModel.setActiveIndex(AppContext.showModel.activeIndex + 1);
                            slideGrid.currentIndex = 0;
                            AppContext.displayEngine.jumpToSlide(0);
                        }
                    }
                }
                
                function reverseSlide() {
                    if (currentIndex > 0) {
                        currentIndex--;
                        AppContext.displayEngine.jumpToSlide(currentIndex);
                    } else if (AppContext.showModel.activeIndex > 0) {
                        if (AppContext.showModel.isSameShow(AppContext.showModel.activeIndex, AppContext.showModel.activeIndex - 1)) {
                            AppContext.showModel.setActiveIndex(AppContext.showModel.activeIndex - 1);
                            slideGrid.currentIndex = AppContext.slideDeckModel.rowCount() - 1;
                            AppContext.displayEngine.jumpToSlide(slideGrid.currentIndex);
                        }
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
                                color: parent.color.hslLightness > 0.5 ? "black" : "white"
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
                                color: parent.color.hslLightness > 0.5 ? "black" : "white"
                                font.bold: true
                            }
                        }
                        
                        // Slide Preview
                        SlidePreview {
                            id: slidePreviewItem
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            layoutName: (model.cardLayout !== undefined && model.cardLayout !== "") ? model.cardLayout : (AppContext.slideDeckModel ? AppContext.slideDeckModel.getCardLayout(index) : "Default")
                            slideText: model.slideText
                            showBackground: showBackgroundsSwitch.checked
                            globalBackgroundMedia: AppContext.slideDeckModel.deck ? AppContext.slideDeckModel.deck.globalBackgroundMedia : ""
                            topRightRadius: 11
                            bottomRightRadius: 11
                            quickEditEnabled: servicesView.isEditingArrangement
                            onTextEditFinished: function(newText) {
                                AppContext.slideDeckModel.setSlideText(index, newText)
                            }
                            onEditingStarted: {
                                if (slideGrid.currentIndex !== index) {
                                    slideGrid.currentIndex = index
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
                target: AppContext.displayEngine
                function onCurrentSlideChanged() {
                    if (AppContext.displayEngine) {
                        let liveIndex = AppContext.displayEngine.currentSlideNumber - 1
                        if (liveIndex >= 0) {
                            monitorsLayout.previewData = AppContext.slideDeckModel.getSlideDataForPreview(liveIndex)
                        }
                    }
                }
            }
            
            Connections {
                target: AppContext.slideDeckModel
                function onSlidesUpdated() {
                    if (AppContext.displayEngine) {
                        let liveIndex = AppContext.displayEngine.currentSlideNumber - 1
                        if (liveIndex >= 0) {
                            monitorsLayout.previewData = AppContext.slideDeckModel.getSlideDataForPreview(liveIndex)
                        }
                    }
                }
                function onSlidesRebuilt() {
                    if (AppContext.displayEngine) {
                        let liveIndex = AppContext.displayEngine.currentSlideNumber - 1
                        if (liveIndex >= 0) {
                            monitorsLayout.previewData = AppContext.slideDeckModel.getSlideDataForPreview(liveIndex)
                        }
                    }
                }
            }
            
            Component.onCompleted: {
                if (AppContext.displayEngine) {
                    let liveIndex = AppContext.displayEngine.currentSlideNumber - 1
                    if (liveIndex >= 0) {
                        monitorsLayout.previewData = AppContext.slideDeckModel.getSlideDataForPreview(liveIndex)
                    }
                }
            }

            Repeater {
                model: AppContext.screenModel
                delegate: ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5
                    visible: !model.disabled
                    
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
                            layoutName: monitorsLayout.previewData.layouts ? (monitorsLayout.previewData.layouts[model.name] || (model.name === "Stage" ? "StageDefault" : "Default")) : (model.name === "Stage" ? "StageDefault" : "Default")
                            slideText: monitorsLayout.previewData.slideText || ""
                            nextSlideText: monitorsLayout.previewData.nextSlideText || ""
                            showBackground: true
                            globalBackgroundMedia: AppContext.slideDeckModel.deck ? AppContext.slideDeckModel.deck.globalBackgroundMedia : ""
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
        id: renameDialog
        title: "Rename Arrangement"
        standardButtons: Dialog.Ok | Dialog.Cancel
        anchors.centerIn: parent
        modal: true
        width: 400
        
        property string sourceName: ""
        property alias nameInput: renameNameField.text

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
                id: renameNameField
                Layout.fillWidth: true
                selectByMouse: true
                onAccepted: renameDialog.accept()
            }
        }
        
        onAccepted: {
            if (AppContext.slideDeckModel.deck) {
                AppContext.slideDeckModel.deck.renameArrangement(sourceName, renameNameField.text.trim())
                servicesView.triggerAutoSave()
            }
        }
        
        onOpened: {
            renameNameField.forceActiveFocus()
            renameNameField.selectAll()
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
