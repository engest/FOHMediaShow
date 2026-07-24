import QtQuick
import fohmedia
import QtQuick.Controls
import QtMultimedia

Rectangle {
    id: root

    property string layoutName: "Default"
    property string slideText: ""
    property string nextSlideText: ""
    property bool showBackground: false
    property bool forceOpaqueBackground: false
    property string globalBackgroundMedia: ""
    
    property bool renderBackground: true
    property bool renderText: true
    property bool renderTimers: true

    // We fetch properties from the C++ ShowViewModel via context property
    property var layoutProps: AppContext.showModel.getLayoutProperties(layoutName)
    
    Connections {
        target: AppContext.layoutEditorModel
        function onLayoutSaved(savedLayoutName) {
            if (savedLayoutName === root.layoutName) {
                // Force re-fetch of layout properties from disk via ShowViewModel
                root.layoutProps = AppContext.showModel.getLayoutProperties(root.layoutName)
            }
        }
    }

    color: (root.renderBackground && (root.showBackground || root.forceOpaqueBackground)) ? (layoutProps && layoutProps.backgroundColor && layoutProps.backgroundColor !== "#00000000" ? layoutProps.backgroundColor : "#000000") : "transparent"
    clip: true
    antialiasing: true

    property bool quickEditEnabled: false
    property int textRenderType: TextEdit.QtRendering
    
    signal textEditFinished(string newText)
    signal editingStarted()

    // Internal mapping of text alignment enum values from Qt::Alignment
    function getHAlign(alignment) {
        if (!alignment) return TextEdit.AlignHCenter;
        if (alignment & Qt.AlignLeft) return TextEdit.AlignLeft;
        if (alignment & Qt.AlignRight) return TextEdit.AlignRight;
        return TextEdit.AlignHCenter; // Default for Qt.AlignHCenter
    }

    function getVAlign(alignment) {
        if (!alignment) return TextEdit.AlignVCenter;
        if (alignment & Qt.AlignTop) return TextEdit.AlignTop;
        if (alignment & Qt.AlignBottom) return TextEdit.AlignBottom;
        return TextEdit.AlignVCenter; // Default for Qt.AlignVCenter
    }

    Item {
        id: layoutContainer
        // Original layout dimensions
        width: layoutProps.width ? layoutProps.width : 1920
        height: layoutProps.height ? layoutProps.height : 1080

        anchors.centerIn: parent
        // Scale to fit the bounds of the SlidePreview while preserving aspect ratio
        transform: Scale {
            origin.x: layoutContainer.width / 2
            origin.y: layoutContainer.height / 2
            xScale: Math.min(root.width / layoutContainer.width, root.height / layoutContainer.height)
            yScale: Math.min(root.width / layoutContainer.width, root.height / layoutContainer.height)
        }
        
        Image {
            anchors.fill: parent
            source: {
                if (!root.showBackground) return "";
                
                var url = "";
                if (root.globalBackgroundMedia !== "") {
                    // Check if it's already a full URL or needs resolution
                    if (root.globalBackgroundMedia.startsWith("file://") || root.globalBackgroundMedia.startsWith("qrc:/")) {
                        url = root.globalBackgroundMedia;
                    } else {
                        url = AppContext.getLocalFileUrl(root.globalBackgroundMedia);
                    }
                } else if (layoutProps && layoutProps.backgroundUrl) {
                    url = layoutProps.backgroundUrl;
                }
                
                if (url !== "") {
                    var lower = url.toString().toLowerCase();
                    if (lower.endsWith(".mp4") || lower.endsWith(".mov") || lower.endsWith(".avi") || lower.endsWith(".mkv") || lower.endsWith(".webm")) {
                        return ""; // Do not try to load videos as images
                    }
                    return url.toString();
                }
                
                return "";
            }
            fillMode: Image.PreserveAspectCrop
            visible: root.renderBackground && source.toString() !== ""
        }

        TextEdit {
            id: textEditItem
            visible: root.renderText && root.quickEditEnabled
            x: layoutProps.textBoundsX !== undefined ? layoutProps.textBoundsX : 0
            y: layoutProps.textBoundsY !== undefined ? layoutProps.textBoundsY : 0
            width: layoutProps.textBoundsWidth !== undefined ? layoutProps.textBoundsWidth : layoutContainer.width
            height: layoutProps.textBoundsHeight !== undefined ? layoutProps.textBoundsHeight : layoutContainer.height

            text: root.slideText
            color: layoutProps.fontColor ? layoutProps.fontColor : "#ffffff"
            font.family: layoutProps.fontFamily ? layoutProps.fontFamily : "Bebas Neue"
            font.pixelSize: layoutProps.fontSize ? layoutProps.fontSize : 90
            font.bold: layoutProps.isBold ? true : false
            font.italic: layoutProps.isItalic ? true : false
            font.capitalization: layoutProps.allCaps ? Font.AllUppercase : Font.MixedCase
            
            horizontalAlignment: root.getHAlign(layoutProps.alignment)
            verticalAlignment: root.getVAlign(layoutProps.alignment)
            wrapMode: TextEdit.WordWrap
            textFormat: TextEdit.AutoText
            renderType: root.textRenderType
            readOnly: false
            selectByMouse: true
            cursorVisible: activeFocus
            
            cursorDelegate: Component {
                Rectangle {
                    // Compensate for the massive scale-down so the cursor is always at least ~2 physical pixels wide
                    width: Math.max(2, 2 * (layoutContainer.width / root.width))
                    color: layoutProps.fontColor ? layoutProps.fontColor : "#ffffff"
                    visible: textEditItem.cursorVisible
                }
            }
            
            onTextChanged: {
                if (root.quickEditEnabled && activeFocus) {
                    root.textEditFinished(text)
                }
            }
            
            onActiveFocusChanged: {
                if (activeFocus && root.quickEditEnabled) {
                    root.editingStarted()
                }
            }
        }

        Text {
            id: textDisplayItem
            visible: root.renderText && !root.quickEditEnabled
            x: textEditItem.x
            y: textEditItem.y
            width: textEditItem.width
            height: textEditItem.height
            text: textEditItem.text
            color: textEditItem.color
            font: textEditItem.font
            horizontalAlignment: textEditItem.horizontalAlignment
            verticalAlignment: textEditItem.verticalAlignment
            wrapMode: Text.WordWrap
            textFormat: Text.AutoText
            renderType: root.textRenderType
            renderTypeQuality: Text.HighRenderTypeQuality
        }
        
        Text {
            id: nextTextEditItem
            x: layoutProps.nextTextBoundsX !== undefined ? layoutProps.nextTextBoundsX : 0
            y: layoutProps.nextTextBoundsY !== undefined ? layoutProps.nextTextBoundsY : layoutContainer.height / 2
            width: layoutProps.nextTextBoundsWidth !== undefined ? layoutProps.nextTextBoundsWidth : layoutContainer.width
            height: layoutProps.nextTextBoundsHeight !== undefined ? layoutProps.nextTextBoundsHeight : layoutContainer.height / 2

            text: root.nextSlideText
            color: layoutProps.nextFontColor ? layoutProps.nextFontColor : "#ffffff"
            font.family: layoutProps.nextFontFamily ? layoutProps.nextFontFamily : "Bebas Neue"
            font.pixelSize: layoutProps.nextFontSize ? layoutProps.nextFontSize : 60
            font.bold: layoutProps.nextIsBold ? true : false
            font.italic: layoutProps.nextIsItalic ? true : false
            font.capitalization: layoutProps.nextAllCaps ? Font.AllUppercase : Font.MixedCase
            
            horizontalAlignment: root.getHAlign(layoutProps.nextAlignment)
            verticalAlignment: root.getVAlign(layoutProps.nextAlignment)
            wrapMode: Text.WordWrap
            textFormat: Text.AutoText
            renderType: root.textRenderType
            renderTypeQuality: Text.HighRenderTypeQuality
            visible: root.renderText && root.nextSlideText !== "" && layoutProps.hasNextText === true
        }
        
        // Render Timers on Live Slide / Preview
        Item {
            anchors.fill: parent
            visible: root.renderTimers
            
            Repeater {
                model: layoutProps.timers ? layoutProps.timers : []
                
                delegate: Text {
                    property var timerData: modelData
                    property var timerObj: AppContext.timerManager.getTimer(timerData.timerId)
                    
                    x: timerData.bounds ? timerData.bounds.x : 0
                    y: timerData.bounds ? timerData.bounds.y : 0
                    width: timerData.bounds ? timerData.bounds.width : 0
                    height: timerData.bounds ? timerData.bounds.height : 0
                    
                    text: AppContext.timerManager.getTimerString(timerData.timerId)
                    
                    Connections {
                        target: AppContext.timerManager
                        function onTimerTicked() {
                            text = AppContext.timerManager.getTimerString(timerData.timerId)
                        }
                        function onTimersChanged() {
                            text = AppContext.timerManager.getTimerString(timerData.timerId)
                        }
                    }
                    
                    color: timerData.fontColor ? timerData.fontColor : "#ffffff"
                    font.family: timerData.fontFamily ? timerData.fontFamily : "Bebas Neue"
                    font.pixelSize: timerData.fontSize ? timerData.fontSize : 60
                    font.bold: timerData.isBold ? true : false
                    font.italic: timerData.isItalic ? true : false
                    
                    horizontalAlignment: root.getHAlign(timerData.alignment)
                    verticalAlignment: root.getVAlign(timerData.alignment)
                    wrapMode: Text.WordWrap
                    textFormat: Text.AutoText
                    renderType: root.textRenderType
                    renderTypeQuality: Text.HighRenderTypeQuality
                }
            }
        }
        
        MouseArea {
            anchors.fill: parent
            enabled: root.quickEditEnabled
            z: -1 // Behind the TextEdit
            onClicked: {
                textEditItem.forceActiveFocus()
            }
        }
    }
    
    function forceTextEditFocus() {
        textEditItem.forceActiveFocus()
    }
}
