import QtQuick
import fohmedia
import QtQuick.Controls

Item {
    id: root

    property color strokeColor: "#ff8c00"
    property int handleSize: 10
    property bool isSelected: false

    // Signals for parent to update the backend model
    signal boundsChanged(real newX, real newY, real newWidth, real newHeight)
    signal clicked()

    // Emit boundsChanged only on user drag/resize release

    Rectangle {
        id: selectionBox
        anchors.fill: parent
        color: "transparent"
        border.color: root.strokeColor
        border.width: root.isSelected ? 4 : 2
        
        // Drag surface
        MouseArea {
            anchors.fill: parent
            anchors.margins: root.handleSize
            cursorShape: Qt.SizeAllCursor
            drag.target: root
            drag.axis: Drag.XAndYAxis
            drag.minimumX: 0
            drag.minimumY: 0
            drag.maximumX: root.parent.width - root.width
            drag.maximumY: root.parent.height - root.height

            onPressed: root.clicked()
            
            onReleased: {
                root.boundsChanged(root.x, root.y, root.width, root.height)
            }
        }
        
        // Component to spawn handles
        component ResizeHandle : Rectangle {
            width: root.handleSize
            height: root.handleSize
            color: root.strokeColor
            border.color: "#ffffff"
            border.width: 1
            property int dragAxis
            property bool topEdge: false
            property bool bottomEdge: false
            property bool leftEdge: false
            property bool rightEdge: false

            MouseArea {
                anchors.fill: parent
                cursorShape: {
                    if ((parent.topEdge && parent.leftEdge) || (parent.bottomEdge && parent.rightEdge)) return Qt.SizeFDiagCursor
                    if ((parent.topEdge && parent.rightEdge) || (parent.bottomEdge && parent.leftEdge)) return Qt.SizeBDiagCursor
                    if (parent.topEdge || parent.bottomEdge) return Qt.SizeVerCursor
                    return Qt.SizeHorCursor
                }

                property real startX: 0
                property real startY: 0
                property real initialX: 0
                property real initialY: 0
                property real initialWidth: 0
                property real initialHeight: 0

                onPressed: function(mouse) {
                    root.clicked()
                    let mappedPoint = mapToItem(root.parent, mouse.x, mouse.y)
                    startX = mappedPoint.x
                    startY = mappedPoint.y
                    initialX = root.x
                    initialY = root.y
                    initialWidth = root.width
                    initialHeight = root.height
                }

                onPositionChanged: function(mouse) {
                    if (!pressed) return;

                    let mappedPoint = mapToItem(root.parent, mouse.x, mouse.y)
                    let dx = mappedPoint.x - startX
                    let dy = mappedPoint.y - startY
                    
                    let newX = initialX
                    let newY = initialY
                    let newWidth = initialWidth
                    let newHeight = initialHeight

                    if (parent.leftEdge) {
                        newX += dx
                        newWidth -= dx
                    } else if (parent.rightEdge) {
                        newWidth += dx
                    }

                    if (parent.topEdge) {
                        newY += dy
                        newHeight -= dy
                    } else if (parent.bottomEdge) {
                        newHeight += dy
                    }

                    // Constrain minimum size
                    if (newWidth >= 20) {
                        root.x = newX
                        root.width = newWidth
                    }
                    if (newHeight >= 20) {
                        root.y = newY
                        root.height = newHeight
                    }
                }
                
                onReleased: {
                    root.boundsChanged(root.x, root.y, root.width, root.height)
                }
            }
        }

        // Top Left
        ResizeHandle {
            anchors.verticalCenter: parent.top
            anchors.horizontalCenter: parent.left
            topEdge: true; leftEdge: true
        }
        // Top Center
        ResizeHandle {
            anchors.verticalCenter: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            topEdge: true
        }
        // Top Right
        ResizeHandle {
            anchors.verticalCenter: parent.top
            anchors.horizontalCenter: parent.right
            topEdge: true; rightEdge: true
        }
        // Right Center
        ResizeHandle {
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.right
            rightEdge: true
        }
        // Bottom Right
        ResizeHandle {
            anchors.verticalCenter: parent.bottom
            anchors.horizontalCenter: parent.right
            bottomEdge: true; rightEdge: true
        }
        // Bottom Center
        ResizeHandle {
            anchors.verticalCenter: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            bottomEdge: true
        }
        // Bottom Left
        ResizeHandle {
            anchors.verticalCenter: parent.bottom
            anchors.horizontalCenter: parent.left
            bottomEdge: true; leftEdge: true
        }
        // Left Center
        ResizeHandle {
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.left
            leftEdge: true
        }
    }
}
