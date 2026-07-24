import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import fohmedia

Dialog {
    id: mediaPicker
    
    property string mediaType: "image" // "image" or "video"
    property var mediaModel: mediaType === "image" ? AppContext.imageMediaModel : AppContext.videoMediaModel
    
    title: mediaType === "image" ? "Select Background Image" : "Select Background Video"
    
    width: 800
    height: 600
    modal: true
    anchors.centerIn: Overlay.overlay

    property string selectedFilePath: ""
    property string selectedFileUrl: ""
    property int selectedIndex: -1
    
    onOpened: {
        selectedFilePath = ""
        selectedFileUrl = ""
        selectedIndex = -1
        if (mediaModel) {
            mediaModel.refresh()
        }
    }

    onAccepted: {
        if (selectedFilePath !== "") {
            fileSelected(selectedFilePath, selectedFileUrl)
        }
    }

    signal fileSelected(string filePath, string fileUrl)
    
    footer: DialogButtonBox {
        Button {
            text: "Cancel"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        Button {
            text: "OK"
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            enabled: mediaPicker.selectedFilePath !== ""
        }
    }

    contentItem: Item {
        GridView {
            id: grid
            anchors.fill: parent
            anchors.margins: 10
            model: mediaPicker.mediaModel
            clip: true
            
            cellWidth: mediaPicker.mediaType === "image" ? 220 : 320
            cellHeight: mediaPicker.mediaType === "image" ? 220 : 240
            
            delegate: Rectangle {
                id: delegateRect
                width: grid.cellWidth - 20
                height: grid.cellHeight - 20
                color: "transparent"
                radius: 5
                
                property bool isSelected: mediaPicker.selectedIndex === index
                
                border.color: isSelected ? palette.highlight : (hoverArea.containsMouse ? palette.midlight : palette.mid)
                border.width: isSelected ? 3 : (hoverArea.containsMouse ? 2 : 1)
                
                Loader {
                    anchors.fill: parent
                    anchors.margins: isSelected ? 6 : (hoverArea.containsMouse ? 5 : 4)
                    active: mediaPicker.visible
                    sourceComponent: mediaPicker.mediaType === "image" ? imageComp : videoComp
                }
                
                Component {
                    id: imageComp
                    Image {
                        source: fileUrl
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                    }
                }
                
                Component {
                    id: videoComp
                    Item {
                        VideoOutput {
                            id: videoOut
                            anchors.fill: parent
                            fillMode: VideoOutput.PreserveAspectFit
                        }
                        
                        MediaPlayer {
                            id: player
                            source: fileUrl
                            audioOutput: null
                            loops: MediaPlayer.Infinite
                            autoPlay: false
                            videoOutput: videoOut
                            
                            Component.onCompleted: {
                                player.play()
                                player.pause()
                                player.position = 0
                            }
                        }
                        
                        Connections {
                            target: hoverArea
                            function onEntered() { player.play() }
                            function onExited() { 
                                player.pause() 
                                player.position = 0
                            }
                        }
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
                            text: (mediaPicker.mediaType === "video" ? "🎥 " : "") + fileName
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            color: "white"
                            font.pixelSize: 12
                            font.bold: isSelected
                        }
                    }
                }
                
                MouseArea {
                    id: hoverArea
                    anchors.fill: parent
                    hoverEnabled: true
                    
                    onClicked: {
                        mediaPicker.selectedIndex = index
                        mediaPicker.selectedFilePath = filePath
                        mediaPicker.selectedFileUrl = fileUrl.toString()
                    }
                    
                    onDoubleClicked: {
                        mediaPicker.selectedIndex = index
                        mediaPicker.selectedFilePath = filePath
                        mediaPicker.selectedFileUrl = fileUrl.toString()
                        mediaPicker.accept()
                    }
                }
            }
        }
    }
}
