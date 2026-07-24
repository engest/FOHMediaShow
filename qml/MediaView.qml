import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Qt.labs.platform as Platform
import QtMultimedia
import fohmedia

Item {
    id: mediaViewRoot
    // anchors.fill: parent is managed by StackView/SwipeView to avoid conflicting anchors

    Component.onCompleted: {
        AppContext.imageMediaModel.refresh()
        AppContext.videoMediaModel.refresh()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        RowLayout {
            Layout.fillWidth: true
            
            Label {
                text: "Media Library"
                font.pixelSize: 16
                font.bold: true
                color: palette.text
            }
            
            Item { Layout.fillWidth: true } // spacer
            
            Row {
                id: mediaTabBar
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: 400
                Layout.preferredHeight: 40
                spacing: 10

                property int currentIndex: 0
                
                Button {
                    width: (parent.width - parent.spacing) / 2
                    height: parent.height
                    checkable: true
                    checked: mediaTabBar.currentIndex === 0
                    onClicked: mediaTabBar.currentIndex = 0
                    contentItem: Text {
                        text: "Images"
                        font.pixelSize: 16
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: parent.checked ? palette.highlight : palette.text
                    }
                    background: Rectangle {
                        color: parent.checked ? palette.button : palette.window
                        border.color: palette.mid
                        border.width: 1
                        radius: 2
                    }
                }
                Button {
                    width: (parent.width - parent.spacing) / 2
                    height: parent.height
                    checkable: true
                    checked: mediaTabBar.currentIndex === 1
                    onClicked: mediaTabBar.currentIndex = 1
                    contentItem: Text {
                        text: "Videos"
                        font.pixelSize: 16
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: parent.checked ? palette.highlight : palette.text
                    }
                    background: Rectangle {
                        color: parent.checked ? palette.button : palette.window
                        border.color: palette.mid
                        border.width: 1
                        radius: 2
                    }
                }
            }
            
            Item { Layout.fillWidth: true } // spacer

            ToolButton {
                text: "📥 Import Media"
                padding: 10
                onClicked: {
                    if (mediaTabBar.currentIndex === 0) {
                        importDialog.nameFilters = ["Image files (*.png *.jpg *.jpeg *.gif *.bmp *.webp)"]
                        importDialog.title = "Import Images"
                    } else {
                        importDialog.nameFilters = ["Video files (*.mp4 *.mov *.avi *.mkv *.webm)"]
                        importDialog.title = "Import Videos"
                    }
                    importDialog.open()
                }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: mediaTabBar.currentIndex

            // Images View
            GridView {
                id: imageGrid
                model: AppContext.imageMediaModel
                cellWidth: 220
                cellHeight: 220
                clip: true

                delegate: Rectangle {
                    width: imageGrid.cellWidth - 20
                    height: imageGrid.cellHeight - 20
                    color: "transparent"
                    border.color: hoverAreaImage.containsMouse ? palette.highlight : palette.mid
                    border.width: hoverAreaImage.containsMouse ? 2 : 1
                    radius: 5

                    Image {
                        anchors.fill: parent
                        anchors.margins: 5
                        source: fileUrl
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.leftMargin: parent.border.width
                        anchors.rightMargin: parent.border.width
                        anchors.bottomMargin: parent.border.width
                        height: 40
                        color: "#cc000000"
                        radius: Math.max(0, parent.radius - parent.border.width)
                        
                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            height: parent.radius
                            color: "#cc000000"
                        }
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 5
                            
                            Label {
                                text: fileName
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                color: "white"
                                font.pixelSize: 12
                            }
                            
                            ToolButton {
                                text: "✏️"
                                font.pixelSize: 12
                                ToolTip.text: "Rename"
                                ToolTip.visible: hovered
                                onClicked: {
                                    renameDialog.isImage = true
                                    renameDialog.fileIndex = index
                                    renameDialog.currentName = fileName
                                    renameDialog.open()
                                }
                            }
                            ToolButton {
                                text: "🗑️"
                                font.pixelSize: 12
                                ToolTip.text: "Delete"
                                ToolTip.visible: hovered
                                onClicked: {
                                    deleteDialog.isImage = true
                                    deleteDialog.fileIndex = index
                                    deleteDialog.fileName = fileName
                                    deleteDialog.open()
                                }
                            }
                        }
                    }

                    MouseArea {
                        id: hoverAreaImage
                        anchors.fill: parent
                        hoverEnabled: true
                        // Pass clicks through to buttons
                        acceptedButtons: Qt.NoButton
                    }
                }
            }

            // Videos View
            GridView {
                id: videoGrid
                model: AppContext.videoMediaModel
                cellWidth: 320
                cellHeight: 240
                clip: true

                delegate: Rectangle {
                    width: videoGrid.cellWidth - 20
                    height: videoGrid.cellHeight - 20
                    color: "transparent"
                    border.color: hoverAreaVideo.containsMouse ? palette.highlight : palette.mid
                    border.width: hoverAreaVideo.containsMouse ? 2 : 1
                    radius: 5

                    VideoOutput {
                        id: videoOut
                        anchors.fill: parent
                        anchors.margins: 5
                        fillMode: VideoOutput.PreserveAspectFit
                    }

                    MediaPlayer {
                        id: player
                        source: fileUrl
                        audioOutput: null // Mute audio
                        loops: MediaPlayer.Infinite
                        autoPlay: false
                        videoOutput: videoOut

                        Component.onCompleted: {
                            // Start and immediately pause to load the first frame
                            player.play()
                            player.pause()
                            player.position = 0
                        }
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.leftMargin: parent.border.width
                        anchors.rightMargin: parent.border.width
                        anchors.bottomMargin: parent.border.width
                        height: 40
                        color: "#cc000000"
                        radius: Math.max(0, parent.radius - parent.border.width)
                        
                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            height: parent.radius
                            color: "#cc000000"
                        }
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 5
                            
                            Label {
                                text: "🎥 " + fileName
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                color: "white"
                                font.pixelSize: 12
                            }
                            
                            ToolButton {
                                text: "✏️"
                                font.pixelSize: 12
                                ToolTip.text: "Rename"
                                ToolTip.visible: hovered
                                onClicked: {
                                    renameDialog.isImage = false
                                    renameDialog.fileIndex = index
                                    renameDialog.currentName = fileName
                                    renameDialog.open()
                                }
                            }
                            ToolButton {
                                text: "🗑️"
                                font.pixelSize: 12
                                ToolTip.text: "Delete"
                                ToolTip.visible: hovered
                                onClicked: {
                                    deleteDialog.isImage = false
                                    deleteDialog.fileIndex = index
                                    deleteDialog.fileName = fileName
                                    deleteDialog.open()
                                }
                            }
                        }
                    }

                    MouseArea {
                        id: hoverAreaVideo
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.NoButton
                        onEntered: {
                            player.play()
                        }
                        onExited: {
                            player.pause()
                            player.position = 0
                        }
                    }
                }
            }
        }
    }

    // Dialogs
    Platform.FileDialog {
        id: importDialog
        fileMode: Platform.FileDialog.OpenFiles
        onAccepted: {
            if (mediaTabBar.currentIndex === 0) {
                AppContext.imageMediaModel.importFiles(files)
            } else {
                AppContext.videoMediaModel.importFiles(files)
            }
        }
    }

    Dialog {
        id: renameDialog
        title: "Rename Media"
        standardButtons: Dialog.Ok | Dialog.Cancel
        
        property bool isImage: true
        property int fileIndex: -1
        property string currentName: ""

        ColumnLayout {
            spacing: 10
            Label {
                text: "New name for " + renameDialog.currentName + ":"
            }
            TextField {
                id: renameField
                Layout.fillWidth: true
                text: renameDialog.currentName
            }
        }

        onOpened: {
            renameField.text = currentName
            renameField.selectAll()
            renameField.forceActiveFocus()
        }

        onAccepted: {
            if (renameField.text.trim() !== "" && renameField.text !== currentName) {
                if (isImage) {
                    AppContext.imageMediaModel.renameFile(fileIndex, renameField.text.trim())
                } else {
                    AppContext.videoMediaModel.renameFile(fileIndex, renameField.text.trim())
                }
            }
        }
    }

    MessageDialog {
        id: deleteDialog
        title: "Confirm Delete"
        buttons: MessageDialog.Yes | MessageDialog.No
        
        property bool isImage: true
        property int fileIndex: -1
        property string fileName: ""
        
        text: "Are you sure you want to delete '" + fileName + "' from the library?"
        informativeText: "This action cannot be undone."

        onButtonClicked: function(button, role) {
            if (button === MessageDialog.Yes) {
                if (isImage) {
                    AppContext.imageMediaModel.deleteFile(fileIndex)
                } else {
                    AppContext.videoMediaModel.deleteFile(fileIndex)
                }
            }
        }
    }
}
