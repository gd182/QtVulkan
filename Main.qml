import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: root
    width: 1280
    height: 760
    visible: true
    title: "Qt Vulkan"
    color: "#0b1020"

    property int panelWidth: 320

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#0f172a" }
            GradientStop { position: 1.0; color: "#111827" }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 20
            color: "#020617"
            border.color: "#1e293b"
            border.width: 1
            clip: true

            WindowContainer {
                anchors.fill: parent
                anchors.margins: 1
                window: renderWindow
            }
        }

        WindowContainer {
            Layout.preferredWidth: root.panelWidth
            Layout.fillHeight: true
            window: Window {
                id: panelWindow
                flags: Qt.FramelessWindowHint
                color: "transparent"

                Rectangle {
                    anchors.fill: parent
                    color: "#111827"
                    border.color: "#263245"
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 14

                        Label {
                            text: "Scene Controls"
                            color: "#f8fafc"
                            font.pixelSize: 26
                            font.bold: true
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            radius: 14
                            color: "#0f1b2d"
                            border.color: "#1f9d7a"
                            border.width: 1
                            implicitHeight: 82

                            Column {
                                anchors.centerIn: parent
                                spacing: 4

                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "FPS"
                                    color: "#94a3b8"
                                    font.pixelSize: 12
                                    font.letterSpacing: 1.4
                                }

                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: sceneController.fps.toFixed(1)
                                    color: "#34d399"
                                    font.pixelSize: 32
                                    font.bold: true
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            radius: 14
                            color: "#10192b"
                            border.color: "#334155"
                            border.width: 1
                            implicitHeight: 170

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 14
                                spacing: 10

                                RowLayout {
                                    Layout.fillWidth: true

                                    Label {
                                        text: "Elasticity"
                                        color: "#cbd5e1"
                                        font.pixelSize: 15
                                        font.bold: true
                                    }

                                    Item { Layout.fillWidth: true }

                                    Label {
                                        text: elasticitySlider.value.toFixed(2)
                                        color: "#f8fafc"
                                        font.pixelSize: 15
                                        font.bold: true
                                    }
                                }

                                Slider {
                                    id: elasticitySlider
                                    Layout.fillWidth: true
                                    from: 0.0
                                    to: 1.0
                                    stepSize: 0.01
                                    value: sceneController.elasticity
                                    onMoved: sceneController.setElasticity(value)

                                    background: Rectangle {
                                        x: elasticitySlider.leftPadding
                                        y: elasticitySlider.topPadding + elasticitySlider.availableHeight / 2 - height / 2
                                        width: elasticitySlider.availableWidth
                                        height: 5
                                        radius: 3
                                        color: "#243041"

                                        Rectangle {
                                            width: elasticitySlider.visualPosition * parent.width
                                            height: parent.height
                                            radius: parent.radius
                                            color: "#38bdf8"
                                        }
                                    }

                                    handle: Rectangle {
                                        x: elasticitySlider.leftPadding
                                           + elasticitySlider.visualPosition * (elasticitySlider.availableWidth - width)
                                        y: elasticitySlider.topPadding
                                           + elasticitySlider.availableHeight / 2 - height / 2
                                        width: 18
                                        height: 18
                                        radius: 9
                                        color: elasticitySlider.pressed ? "#7dd3fc" : "#38bdf8"
                                        border.color: "#e0f2fe"
                                        border.width: 1
                                    }
                                }

                                Binding {
                                    target: elasticitySlider
                                    property: "value"
                                    value: sceneController.elasticity
                                    when: !elasticitySlider.pressed
                                    restoreMode: Binding.RestoreBinding
                                }

                                RowLayout {
                                    Layout.fillWidth: true

                                    Label {
                                        text: "0.0 = plastic"
                                        color: "#64748b"
                                        font.pixelSize: 11
                                    }

                                    Item { Layout.fillWidth: true }

                                    Label {
                                        text: "1.0 = elastic"
                                        color: "#64748b"
                                        font.pixelSize: 11
                                    }
                                }
                            }
                        }

                        Button {
                            Layout.fillWidth: true
                            text: "Reset (R / Space)"
                            onClicked: sceneController.reset()

                            background: Rectangle {
                                radius: 12
                                color: parent.down ? "#7f1d1d" : parent.hovered ? "#991b1b" : "#7a2020"
                                border.color: "#ef4444"
                                border.width: 1
                            }

                            contentItem: Label {
                                text: parent.text
                                color: "#ffffff"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: 14
                                font.bold: true
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            radius: 14
                            color: "#0f172a"
                            border.color: "#263245"
                            border.width: 1
                            implicitHeight: 150

                            Column {
                                anchors.fill: parent
                                anchors.margins: 14
                                spacing: 6

                                Label {
                                    text: "Controls"
                                    color: "#94a3b8"
                                    font.pixelSize: 12
                                    font.bold: true
                                }

                                Label {
                                    text: "Drag with left mouse button to orbit the camera"
                                    color: "#cbd5e1"
                                    wrapMode: Text.WordWrap
                                    width: parent.width
                                }

                                Label {
                                    text: "Use the mouse wheel to zoom"
                                    color: "#cbd5e1"
                                    wrapMode: Text.WordWrap
                                    width: parent.width
                                }

                                Label {
                                    text: "Press R or Space to reset physics"
                                    color: "#cbd5e1"
                                    wrapMode: Text.WordWrap
                                    width: parent.width
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }
        }
    }
}
