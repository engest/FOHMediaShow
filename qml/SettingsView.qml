import QtQuick
import fohmedia
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20
        width: Math.min(parent.width * 0.8, 600)

        Label {
            text: "Global Settings"
            font.pixelSize: 24
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        GroupBox {
            title: "Transitions"
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

        GroupBox {
            title: "Performance"
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
            }
        }
    }
}
