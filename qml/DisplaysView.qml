import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import fohmedia

Item {
    id: root

    Dialog {
        id: newScreenDialog
        title: "New Display Screen"
        standardButtons: Dialog.Ok | Dialog.Cancel
        anchors.centerIn: parent
        modal: true
        padding: 24
        
        ColumnLayout {
            spacing: 12
            Label { text: "Screen Name:" }
            TextField {
                id: newScreenField
                Layout.fillWidth: true
                onAccepted: newScreenDialog.accept()
            }
        }
        
        onAccepted: {
            if (newScreenField.text.trim() !== "") {
                AppContext.screenModel.addScreen(newScreenField.text.trim())
                newScreenField.text = ""
            }
        }
        onOpened: {
            newScreenField.forceActiveFocus()
        }
    }

    Dialog {
        id: deleteScreenDialog
        title: "Remove Display Screen"
        standardButtons: Dialog.Yes | Dialog.No
        anchors.centerIn: parent
        modal: true
        padding: 24
        property int screenIndex: -1
        property string screenName: ""
        
        Label {
            text: "Are you sure you want to remove the display screen '" + deleteScreenDialog.screenName + "'?"
        }
        
        onAccepted: {
            if (deleteScreenDialog.screenIndex !== -1) {
                AppContext.screenModel.removeScreen(deleteScreenDialog.screenIndex)
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        RowLayout {
            Layout.fillWidth: true
            Layout.maximumWidth: 600
            
            Label {
                text: "Displays"
                font.pixelSize: 16
                font.bold: true
                Layout.fillWidth: true
            }
            ToolButton {
                text: "⊕"
                font.pixelSize: 24
                ToolTip.text: "Add Screen"
                ToolTip.visible: hovered
                onClicked: newScreenDialog.open()
            }
        }
        
        Label {
            text: "Configure physical and logical displays for your presentations."
            color: palette.mid
            font.pixelSize: 14
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Flow {
                width: parent.width
                spacing: 20
                
                Repeater {
                    model: AppContext.screenModel
                    
                    delegate: GroupBox {
                        id: screenDelegate
                        property var screenModel: model
                        width: 600
                    
                    RowLayout {
                        width: parent.width
                        spacing: 20
                        
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            
                            RowLayout {
                                Layout.fillWidth: true
                                Label { text: "Name:" ; Layout.preferredWidth: 80 }
                                TextField {
                                    Layout.fillWidth: true
                                    text: model.name
                                    onEditingFinished: {
                                        if (text.trim() !== "") {
                                            model.name = text.trim()
                                        } else {
                                            text = model.name
                                        }
                                    }
                                }
                            }
                            
                            RowLayout {
                                Layout.fillWidth: true
                                Label { text: "Display:" ; Layout.preferredWidth: 80 }
                                ComboBox {
                                    Layout.fillWidth: true
                                    model: AppContext.screenModel.getHardwareDisplays()
                                    
                                    Component.onCompleted: {
                                        currentIndex = screenDelegate.screenModel.hardwareDisplayIndex + 1
                                    }
                                    
                                    onActivated: function(cbIndex) {
                                        let hwIndex = cbIndex - 1
                                        screenDelegate.screenModel.hardwareDisplayIndex = hwIndex
                                        if (hwIndex >= 0) {
                                            AppContext.screenModel.applyHardwareDisplayGeometry(index, hwIndex)
                                        }
                                    }
                                }
                            }
                            
                            RowLayout {
                                Layout.fillWidth: true
                                Label { text: "Geometry:" ; Layout.preferredWidth: 80 }
                                SpinBox {
                                    from: 1
                                    to: 99999
                                    value: model.width
                                    editable: true
                                    enabled: model.hardwareDisplayIndex < 0
                                    onValueModified: model.width = value
                                }
                                Label { text: "x" }
                                SpinBox {
                                    from: 1
                                    to: 99999
                                    value: model.height
                                    editable: true
                                    enabled: model.hardwareDisplayIndex < 0
                                    onValueModified: model.height = value
                                }
                                
                                Item { Layout.fillWidth: true } // Spacer
                                
                                CheckBox {
                                    text: "Fullscreen"
                                    checked: model.isFullscreen
                                    onCheckedChanged: {
                                        if (model.isFullscreen !== checked) {
                                            model.isFullscreen = checked
                                        }
                                    }
                                }
                            }
                        }
                        
                        ToolButton {
                            text: "❌"
                            ToolTip.text: "Remove Screen"
                            ToolTip.visible: hovered
                            enabled: !model.isLocked
                            Layout.alignment: Qt.AlignTop
                            onClicked: {
                                deleteScreenDialog.screenIndex = index
                                deleteScreenDialog.screenName = model.name
                                deleteScreenDialog.open()
                            }
                        }
                        }
                    }
                }
            }
        }
    }
}
