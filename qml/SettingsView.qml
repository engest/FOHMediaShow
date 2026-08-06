import QtQuick
import fohmedia
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    id: root

    ColorDialog {
        id: introColorDialog
        title: "Select Intro Component Color"
        options: ColorDialog.ShowAlphaChannel
        onAccepted: AppContext.settingsManager.introComponentColor = selectedColor
    }
    ColorDialog {
        id: verseColorDialog
        title: "Select Verse Component Color"
        options: ColorDialog.ShowAlphaChannel
        onAccepted: AppContext.settingsManager.verseComponentColor = selectedColor
    }
    ColorDialog {
        id: prechorusColorDialog
        title: "Select PreChorus Component Color"
        options: ColorDialog.ShowAlphaChannel
        onAccepted: AppContext.settingsManager.prechorusComponentColor = selectedColor
    }
    ColorDialog {
        id: chorusColorDialog
        title: "Select Chorus Component Color"
        options: ColorDialog.ShowAlphaChannel
        onAccepted: AppContext.settingsManager.chorusComponentColor = selectedColor
    }
    ColorDialog {
        id: bridgeColorDialog
        title: "Select Bridge Component Color"
        options: ColorDialog.ShowAlphaChannel
        onAccepted: AppContext.settingsManager.bridgeComponentColor = selectedColor
    }
    ColorDialog {
        id: tagColorDialog
        title: "Select Tag Component Color"
        options: ColorDialog.ShowAlphaChannel
        onAccepted: AppContext.settingsManager.tagComponentColor = selectedColor
    }
    ColorDialog {
        id: interludeColorDialog
        title: "Select Interlude Component Color"
        options: ColorDialog.ShowAlphaChannel
        onAccepted: AppContext.settingsManager.interludeComponentColor = selectedColor
    }
    ColorDialog {
        id: refrainColorDialog
        title: "Select Refrain Component Color"
        options: ColorDialog.ShowAlphaChannel
        onAccepted: AppContext.settingsManager.refrainComponentColor = selectedColor
    }
    ColorDialog {
        id: instrumentalColorDialog
        title: "Select Instrumental Component Color"
        options: ColorDialog.ShowAlphaChannel
        onAccepted: AppContext.settingsManager.instrumentalComponentColor = selectedColor
    }
    ColorDialog {
        id: vampColorDialog
        title: "Select Vamp Component Color"
        options: ColorDialog.ShowAlphaChannel
        onAccepted: AppContext.settingsManager.vampComponentColor = selectedColor
    }
    ColorDialog {
        id: turnaroundColorDialog
        title: "Select Turnaround Component Color"
        options: ColorDialog.ShowAlphaChannel
        onAccepted: AppContext.settingsManager.turnaroundComponentColor = selectedColor
    }
    ColorDialog {
        id: passthroughColorDialog
        title: "Select Passthrough Component Color"
        options: ColorDialog.ShowAlphaChannel
        onAccepted: AppContext.settingsManager.passthroughComponentColor = selectedColor
    }
    ColorDialog {
        id: endingColorDialog
        title: "Select Ending Component Color"
        options: ColorDialog.ShowAlphaChannel
        onAccepted: AppContext.settingsManager.endingComponentColor = selectedColor
    }
    ColorDialog {
        id: outroColorDialog
        title: "Select Outro Component Color"
        options: ColorDialog.ShowAlphaChannel
        onAccepted: AppContext.settingsManager.outroComponentColor = selectedColor
    }
    ColorDialog {
        id: blankColorDialog
        title: "Select Blank Component Color"
        options: ColorDialog.ShowAlphaChannel
        onAccepted: AppContext.settingsManager.blankComponentColor = selectedColor
    }

    ColumnLayout {
        anchors.left: parent.left
        spacing: 20
        width: Math.min(parent.width * 0.8, 600)

        Label {
            text: "Global Settings"
            font.pixelSize: 16
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: "Transitions";
            font.bold: true;
            color: palette.highlight;
            padding: 10;
        }

        GroupBox {
            Layout.fillWidth: true

            GridLayout {
                columns: 2
                rowSpacing: 15
                columnSpacing: 20
                anchors.fill: parent

                Label {
                    text: "Default Transition Type:"
                }

                ComboBox {
                    id: transitionTypeCombo
                    Layout.fillWidth: true
                    model: ["Cut", "Fade", "Slide Left", "Slide Right", "Slide Up", "Slide Down"]
                    currentIndex: model.indexOf(AppContext.showModel.defaultTransitionType)
                    
                    onActivated: {
                        AppContext.showModel.defaultTransitionType = currentValue
                    }
                }

                Label {
                    text: "Transition Duration (ms):"
                }

                RowLayout {
                    Layout.fillWidth: true
                    
                    Slider {
                        id: transitionDurationSlider
                        Layout.fillWidth: true
                        from: 0
                        to: 3000
                        stepSize: 100
                        value: AppContext.showModel.defaultTransitionDurationMs
                        
                        onMoved: {
                            AppContext.showModel.defaultTransitionDurationMs = value
                        }
                    }
                    
                    Label {
                        text: Math.round(transitionDurationSlider.value) + " ms"
                        Layout.minimumWidth: 60
                    }
                }
            }
        }

        Label {
            text: "Performance";
            font.bold: true;
            color: palette.highlight;
            padding: 10;
            bottomPadding: 0
        }

        GroupBox {
            padding: 0
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent

                CheckBox {
                    text: "Disable Hardware Video Decoding (Requires Restart)"
                    checked: AppContext.settingsManager.disableHwVideo
                    onCheckedChanged: {
                        if (AppContext.settingsManager.disableHwVideo !== checked) {
                            AppContext.settingsManager.disableHwVideo = checked
                        }
                    }
                }

                CheckBox {
                    text: "Disable Splash Screen"
                    checked: AppContext.settingsManager.disableSplash
                    onCheckedChanged: {
                        if (AppContext.settingsManager.disableSplash !== checked) {
                            AppContext.settingsManager.disableSplash = checked
                        }
                    }
                }
            }
        }

        Label {
            text: "Practice / Simulation";
            font.bold: true;
            color: palette.highlight;
            padding: 10;
            bottomPadding: 0
        }

        GroupBox {
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                GridLayout {
                    columns: 2
                    rowSpacing: 10
                    columnSpacing: 15
                    Layout.fillWidth: true

                    Label {
                        text: "Practice Delay:"
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Slider {
                            id: practiceDelaySlider
                            Layout.fillWidth: true
                            from: 0
                            to: 1000
                            stepSize: 1
                            value: AppContext.settingsManager.practiceDelayMs

                            onMoved: {
                                AppContext.settingsManager.practiceDelayMs = Math.round(value)
                            }
                        }

                        SpinBox {
                            id: practiceDelaySpin
                            from: 0
                            to: 5000
                            stepSize: 10
                            value: AppContext.settingsManager.practiceDelayMs
                            editable: true
                            onValueModified: {
                                AppContext.settingsManager.practiceDelayMs = value
                            }
                        }

                        Label {
                            text: "ms"
                        }
                    }
                }

                Label {
                    text: "Simulates hardware output pipeline delay (e.g. switcher/LED wall) on the audience display only."
                    font.pixelSize: 11
                    color: palette.placeholderText
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }

        Label {
            text: "Lyrics Settings"
            font.pixelSize: 16
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: "Component Colors";
            font.bold: true;
            color: palette.highlight;
            padding: 10;
        }

        GridLayout {
            columns: 2
            rowSpacing: 15
            columnSpacing: 10

            Button {
                Layout.fillWidth: true
                text: "Intro Color"
                padding: 10
                leftInset: 20
                palette.button: AppContext.settingsManager.introComponentColor
                palette.buttonText: AppContext.settingsManager.introComponentColor.hslLightness > 0.5 ? "black" : "white"
                onClicked: {
                    introColorDialog.selectedColor = AppContext.settingsManager.introComponentColor
                    introColorDialog.open()
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Verse Color"
                padding: 10
                leftInset: 20
                palette.button: AppContext.settingsManager.verseComponentColor
                palette.buttonText: AppContext.settingsManager.verseComponentColor.hslLightness > 0.5 ? "black" : "white"
                onClicked: {
                    verseColorDialog.selectedColor = AppContext.settingsManager.verseComponentColor
                    verseColorDialog.open()
                }
            }

            Button {
                Layout.fillWidth: true
                text: "PreChorus Color"
                padding: 10
                leftInset: 20
                palette.button: AppContext.settingsManager.prechorusComponentColor
                palette.buttonText: AppContext.settingsManager.prechorusComponentColor.hslLightness > 0.5 ? "black" : "white"
                onClicked: {
                    prechorusColorDialog.selectedColor = AppContext.settingsManager.prechorusComponentColor
                    prechorusColorDialog.open()
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Chorus Color"
                padding: 10
                leftInset: 20
                palette.button: AppContext.settingsManager.chorusComponentColor
                palette.buttonText: AppContext.settingsManager.chorusComponentColor.hslLightness > 0.5 ? "black" : "white"
                onClicked: {
                    chorusColorDialog.selectedColor = AppContext.settingsManager.chorusComponentColor
                    chorusColorDialog.open()
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Bridge Color"
                padding: 10
                leftInset: 20
                palette.button: AppContext.settingsManager.bridgeComponentColor
                palette.buttonText: AppContext.settingsManager.bridgeComponentColor.hslLightness > 0.5 ? "black" : "white"
                onClicked: {
                    bridgeColorDialog.selectedColor = AppContext.settingsManager.bridgeComponentColor
                    bridgeColorDialog.open()
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Tag Color"
                padding: 10
                leftInset: 20
                palette.button: AppContext.settingsManager.tagComponentColor
                palette.buttonText: AppContext.settingsManager.tagComponentColor.hslLightness > 0.5 ? "black" : "white"
                onClicked: {
                    tagColorDialog.selectedColor = AppContext.settingsManager.tagComponentColor
                    tagColorDialog.open()
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Interlude Color"
                padding: 10
                leftInset: 20
                palette.button: AppContext.settingsManager.interludeComponentColor
                palette.buttonText: AppContext.settingsManager.interludeComponentColor.hslLightness > 0.5 ? "black" : "white"
                onClicked: {
                    interludeColorDialog.selectedColor = AppContext.settingsManager.interludeComponentColor
                    interludeColorDialog.open()
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Refrain Color"
                padding: 10
                leftInset: 20
                palette.button: AppContext.settingsManager.refrainComponentColor
                palette.buttonText: AppContext.settingsManager.refrainComponentColor.hslLightness > 0.5 ? "black" : "white"
                onClicked: {
                    refrainColorDialog.selectedColor = AppContext.settingsManager.refrainComponentColor
                    refrainColorDialog.open()
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Instrumental Color"
                padding: 10
                leftInset: 20
                palette.button: AppContext.settingsManager.instrumentalComponentColor
                palette.buttonText: AppContext.settingsManager.instrumentalComponentColor.hslLightness > 0.5 ? "black" : "white"
                onClicked: {
                    instrumentalColorDialog.selectedColor = AppContext.settingsManager.instrumentalComponentColor
                    instrumentalColorDialog.open()
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Vamp Color"
                padding: 10
                leftInset: 20
                palette.button: AppContext.settingsManager.vampComponentColor
                palette.buttonText: AppContext.settingsManager.vampComponentColor.hslLightness > 0.5 ? "black" : "white"
                onClicked: {
                    vampColorDialog.selectedColor = AppContext.settingsManager.vampComponentColor
                    vampColorDialog.open()
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Turnaround Color"
                padding: 10
                leftInset: 20
                palette.button: AppContext.settingsManager.turnaroundComponentColor
                palette.buttonText: AppContext.settingsManager.turnaroundComponentColor.hslLightness > 0.5 ? "black" : "white"
                onClicked: {
                    turnaroundColorDialog.selectedColor = AppContext.settingsManager.turnaroundComponentColor
                    turnaroundColorDialog.open()
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Passthrough Color"
                padding: 10
                leftInset: 20
                palette.button: AppContext.settingsManager.passthroughComponentColor
                palette.buttonText: AppContext.settingsManager.passthroughComponentColor.hslLightness > 0.5 ? "black" : "white"
                onClicked: {
                    passthroughColorDialog.selectedColor = AppContext.settingsManager.passthroughComponentColor
                    passthroughColorDialog.open()
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Ending Color"
                padding: 10
                leftInset: 20
                palette.button: AppContext.settingsManager.endingComponentColor
                palette.buttonText: AppContext.settingsManager.endingComponentColor.hslLightness > 0.5 ? "black" : "white"
                onClicked: {
                    endingColorDialog.selectedColor = AppContext.settingsManager.endingComponentColor
                    endingColorDialog.open()
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Outro Color"
                padding: 10
                leftInset: 20
                palette.button: AppContext.settingsManager.outroComponentColor
                palette.buttonText: AppContext.settingsManager.outroComponentColor.hslLightness > 0.5 ? "black" : "white"
                onClicked: {
                    outroColorDialog.selectedColor = AppContext.settingsManager.outroComponentColor
                    outroColorDialog.open()
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Blank Color"
                padding: 10
                leftInset: 20
                palette.button: AppContext.settingsManager.blankComponentColor
                palette.buttonText: AppContext.settingsManager.blankComponentColor.hslLightness > 0.5 ? "black" : "white"
                onClicked: {
                    blankColorDialog.selectedColor = AppContext.settingsManager.blankComponentColor
                    blankColorDialog.open()
                }
            }
        }
    }
}
