import QtQuick
import fohmedia
import QtQuick.Window
import QtQuick.Controls
import QtMultimedia

Rectangle {
    id: root
    color: "transparent"
    focus: true

    Component.onCompleted: forceActiveFocus()

    Keys.onPressed: (event) => {
        let win = root.Window.window
        if (win) {
            if (event.key === Qt.Key_F) {
                if (win.visibility === Window.FullScreen) {
                    win.visibility = Window.Windowed
                } else {
                    win.visibility = Window.FullScreen
                }
                event.accepted = true
            } else if (event.key === Qt.Key_Escape) {
                if (win.visibility === Window.FullScreen) {
                    win.visibility = Window.Windowed
                    event.accepted = true
                }
            }
        }
    }

    property string layoutName: "Default"
    property string slideText: ""
    property string nextSlideText: ""
    property bool isStage: false
    property string globalBackgroundMedia: ""
    property string transitionType: "Cut"
    property int transitionDurationMs: 0
    property int currentSlideIndex: -1

    function isColorTransparent(c) {
        if (!c) return false
        if (typeof c === "string") {
            let s = c.trim().toLowerCase()
            if (s === "transparent" || s === "#00000000" || s === "rgba(0,0,0,0)" || s === "rgba(0, 0, 0, 0)") return true
            if (s.startsWith("#") && s.length === 9) {
                let alpha = parseInt(s.substring(1, 3), 16)
                if (alpha === 0) return true
            }
        }
        return Qt.colorEqual(c, "transparent") || Qt.colorEqual(c, "#00000000")
    }

    property var layoutProps: AppContext.showModel.getLayoutProperties(layoutName)
    property bool isTransparentLayout: {
        if (!layoutProps || layoutProps.backgroundColor === undefined) return false
        return isColorTransparent(layoutProps.backgroundColor)
    }

    // The text/layout currently painted on the animated layer. These are updated
    // at the mid-point of a transition (once the outgoing slide is fully hidden)
    // so that the next slide's text NEVER appears on top of the current slide.
    property string displayedLayoutName: layoutName
    property string displayedSlideText: slideText
    property string displayedNextSlideText: nextSlideText

    onSlideTextChanged: {
        if (!transitionAnim.running) {
            root.displayedSlideText = root.slideText
        }
    }
    onNextSlideTextChanged: {
        if (!transitionAnim.running) {
            root.displayedNextSlideText = root.nextSlideText
        }
    }
    onLayoutNameChanged: {
        if (!transitionAnim.running) {
            root.displayedLayoutName = root.layoutName
        }
    }

    function runTransition() {
        // Silent shift: If text and layout are completely identical, do not animate.
        if (root.displayedSlideText === root.slideText && root.displayedLayoutName === root.layoutName) {
            transitionAnim.stop();
            slideLayer.opacity = 1.0;
            slideLayer.x = 0;
            slideLayer.y = 0;
            return;
        }

        // Cut / no-duration / stage: swap instantly, no animation.
        if (root.isStage || root.transitionType === "Cut" || root.transitionDurationMs <= 0) {
            transitionAnim.stop();
            slideLayer.opacity = 1.0;
            slideLayer.x = 0;
            slideLayer.y = 0;
            root.displayedLayoutName    = root.layoutName;
            root.displayedSlideText     = root.slideText;
            root.displayedNextSlideText = root.nextSlideText;
            return;
        }

        // Kick off a fresh two-phase transition: exit the current text,
        // swap the text, then enter the new text.
        transitionAnim.stop();
        transitionAnim.start();
    }

    onCurrentSlideIndexChanged: runTransition()

    function isVideo(path) {
        if (!path) return false
        var lower = path.toString().toLowerCase()
        return lower.endsWith(".mp4") || lower.endsWith(".mov") || lower.endsWith(".avi") || lower.endsWith(".mkv") || lower.endsWith(".webm")
    }

    property bool hasGlobalVideo: root.globalBackgroundMedia !== "" && isVideo(root.globalBackgroundMedia) && !root.isStage && !root.isTransparentLayout && root.layoutName !== "Disabled"

    // Layer 0: Global Background Media (Video)
    MediaPlayer {
        id: backgroundPlayer
        source: root.hasGlobalVideo ? AppContext.getLocalFileUrl(root.globalBackgroundMedia) : ""
        audioOutput: null
        loops: MediaPlayer.Infinite
        videoOutput: backgroundVideo

        onSourceChanged: {
            if (source.toString() !== "") {
                backgroundPlayer.play()
            } else {
                backgroundPlayer.stop()
            }
        }

        Component.onCompleted: {
            if (source.toString() !== "") {
                backgroundPlayer.play()
            }
        }
    }

    VideoOutput {
        id: backgroundVideo
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectCrop
        visible: root.hasGlobalVideo
    }

    // Layer 1: Stationary Slide Content (Background and Timers)
    SlidePreview {
        id: stationarySlide
        anchors.fill: parent
        layoutName: root.layoutName
        slideText: ""
        nextSlideText: ""
        globalBackgroundMedia: root.globalBackgroundMedia
        showBackground: !root.isTransparentLayout && !root.hasGlobalVideo

        renderBackground: !root.isTransparentLayout && !root.hasGlobalVideo
        renderTimers: true
        renderText: false
    }

    // Layer 2: Animated Slide Content (Text only). A SINGLE SlidePreview whose
    // text is swapped at the mid-point of a two-phase animation. This guarantees
    // the outgoing text is fully gone before the incoming text becomes visible,
    // so we never see the next slide's text painted on top of the current one.
    SlidePreview {
        id: slideLayer
        // Don't use anchors.fill here: we need x/y to be free so the slide
        // transitions can move this layer around.
        x: 0
        y: 0
        width: root.width
        height: root.height

        layoutName: root.displayedLayoutName
        slideText: root.displayedSlideText
        nextSlideText: root.displayedNextSlideText
        showBackground: false
        globalBackgroundMedia: root.globalBackgroundMedia

        renderBackground: false
        renderTimers: false
        renderText: true

        quickEditEnabled: false
        textRenderType: TextEdit.QtRendering
    }

    // Two-phase transition:
    //  1. Exit the currently-displayed text (fade out / slide off screen).
    //  2. Swap the displayed text/layout to the new slide's values while invisible/off-screen.
    //  3. Enter the new text (fade in / slide on screen).
    SequentialAnimation {
        id: transitionAnim

        // ── Phase 1: exit ─────────────────────────────────────────────
        ParallelAnimation {
            NumberAnimation {
                target: slideLayer
                property: "opacity"
                to: (root.transitionType === "Fade") ? 0.0 : 1.0
                duration: (root.transitionType === "Fade") ? Math.max(1, root.transitionDurationMs / 2) : 0
            }
            NumberAnimation {
                target: slideLayer
                property: "x"
                to: {
                    if (root.transitionType === "Slide Left")  return -root.width;
                    if (root.transitionType === "Slide Right") return  root.width;
                    return 0;
                }
                duration: (root.transitionType === "Slide Left" || root.transitionType === "Slide Right")
                    ? Math.max(1, root.transitionDurationMs / 2) : 0
                easing.type: Easing.InCubic
            }
            NumberAnimation {
                target: slideLayer
                property: "y"
                to: {
                    if (root.transitionType === "Slide Up")   return -root.height;
                    if (root.transitionType === "Slide Down") return  root.height;
                    return 0;
                }
                duration: (root.transitionType === "Slide Up" || root.transitionType === "Slide Down")
                    ? Math.max(1, root.transitionDurationMs / 2) : 0
                easing.type: Easing.InCubic
            }
        }

        // ── Phase 2: swap content while the layer is off-screen / invisible ──
        ScriptAction {
            script: {
                root.displayedLayoutName    = root.layoutName;
                root.displayedSlideText     = root.slideText;
                root.displayedNextSlideText = root.nextSlideText;

                // Reposition the (now hidden) layer to the entering start position.
                if (root.transitionType === "Slide Left") {
                    slideLayer.x = root.width;
                    slideLayer.y = 0;
                } else if (root.transitionType === "Slide Right") {
                    slideLayer.x = -root.width;
                    slideLayer.y = 0;
                } else if (root.transitionType === "Slide Up") {
                    slideLayer.x = 0;
                    slideLayer.y = root.height;
                } else if (root.transitionType === "Slide Down") {
                    slideLayer.x = 0;
                    slideLayer.y = -root.height;
                } else {
                    // Fade / anything else: stay in place, remain at opacity 0.
                    slideLayer.x = 0;
                    slideLayer.y = 0;
                }
            }
        }

        // ── Phase 3: enter ────────────────────────────────────────────
        ParallelAnimation {
            NumberAnimation {
                target: slideLayer
                property: "opacity"
                to: 1.0
                duration: (root.transitionType === "Fade") ? Math.max(1, root.transitionDurationMs / 2) : 0
            }
            NumberAnimation {
                target: slideLayer
                property: "x"
                to: 0
                duration: (root.transitionType === "Slide Left" || root.transitionType === "Slide Right")
                    ? Math.max(1, root.transitionDurationMs / 2) : 0
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                target: slideLayer
                property: "y"
                to: 0
                duration: (root.transitionType === "Slide Up" || root.transitionType === "Slide Down")
                    ? Math.max(1, root.transitionDurationMs / 2) : 0
                easing.type: Easing.OutCubic
            }
        }
    }
}