import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: win
    width: 1280
    height: 800
    minimumWidth: 520
    minimumHeight: 360
    visible: true
    visibility: Window.FullScreen
    title: qsTr("Plus TV")
    color: "#090a10"

    onVisibilityChanged: function() {
        if (win.visibility !== Window.FullScreen && win.visibility !== Window.Hidden)
            win.visibility = Window.FullScreen
    }

    Material.theme: Material.Dark
    Material.accent: theme.accent
    Material.roundedScale: theme.radius

    property bool helpVisible: false
    property bool remoteVisible: false
    property string currentTime: Qt.formatTime(new Date(), "HH:mm")
    readonly property bool compactMode: win.height < 620 || win.width < 860

    Timer {
        interval: 10000
        running: true
        repeat: true
        onTriggered: win.currentTime = Qt.formatTime(new Date(), "HH:mm")
    }

    Timer {
        id: toastTimer
        interval: 3200
        repeat: false
        onTriggered: launchToast.opacity = 0
    }

    Connections {
        target: backend
        function onAppLaunched(name) {
            launchToast.opacity = 1
            toastTimer.restart()
        }
    }

    readonly property var categories: [
        { key: "all", label: qsTr("All Channels") },
        { key: "live", label: qsTr("Live TV & Sports") },
        { key: "cinema", label: qsTr("Movies & Series") },
        { key: "video", label: qsTr("Video & Music") },
        { key: "web", label: qsTr("Web & Gaming") }
    ]

    function cycleCategory(delta) {
        var idx = 0
        for (var i = 0; i < categories.length; ++i) {
            if (categories[i].key === backend.category) {
                idx = i
                break
            }
        }
        var next = (idx + delta + categories.length) % categories.length
        backend.category = categories[next].key
    }

    Shortcut { sequence: "?"; context: Qt.ApplicationShortcut; onActivated: { win.remoteVisible = false; win.helpVisible = !win.helpVisible } }
    Shortcut {
        sequence: "R"
        context: Qt.ApplicationShortcut
        onActivated: {
            win.helpVisible = false
            backend.refreshRemoteInfo()
            win.remoteVisible = !win.remoteVisible
        }
    }
    onClosing: function(close) { close.accepted = false }
    Shortcut {
        sequence: "Escape"
        context: Qt.ApplicationShortcut
        onActivated: {
            if (win.remoteVisible)
                win.remoteVisible = false
            else if (win.helpVisible)
                win.helpVisible = false
        }
    }
    Shortcut { sequence: "Space"; context: Qt.ApplicationShortcut; onActivated: backend.primaryAction() }
    Shortcut { sequence: "Return"; context: Qt.ApplicationShortcut; onActivated: backend.primaryAction() }
    Shortcut { sequence: "Enter"; context: Qt.ApplicationShortcut; onActivated: backend.primaryAction() }
    Shortcut { sequence: "Tab"; context: Qt.ApplicationShortcut; onActivated: win.cycleCategory(1) }
    Shortcut { sequence: "Backtab"; context: Qt.ApplicationShortcut; onActivated: win.cycleCategory(-1) }

    Shortcut { sequence: "Left"; context: Qt.ApplicationShortcut; onActivated: backend.selectedIndex = Math.max(0, backend.selectedIndex - 1) }
    Shortcut { sequence: "Right"; context: Qt.ApplicationShortcut; onActivated: backend.selectedIndex = Math.min(backend.count - 1, backend.selectedIndex + 1) }
    Shortcut {
        sequence: "Up"
        context: Qt.ApplicationShortcut
        onActivated: {
            var cols = Math.max(1, Math.floor(coverGrid.width / coverGrid.cellWidth))
            backend.selectedIndex = Math.max(0, backend.selectedIndex - cols)
        }
    }
    Shortcut {
        sequence: "Down"
        context: Qt.ApplicationShortcut
        onActivated: {
            var cols = Math.max(1, Math.floor(coverGrid.width / coverGrid.cellWidth))
            backend.selectedIndex = Math.min(backend.count - 1, backend.selectedIndex + cols)
        }
    }

    Shortcut { sequence: "1"; context: Qt.ApplicationShortcut; onActivated: backend.launchApp(0) }
    Shortcut { sequence: "2"; context: Qt.ApplicationShortcut; onActivated: backend.launchApp(1) }
    Shortcut { sequence: "3"; context: Qt.ApplicationShortcut; onActivated: backend.launchApp(2) }
    Shortcut { sequence: "4"; context: Qt.ApplicationShortcut; onActivated: backend.launchApp(3) }
    Shortcut { sequence: "5"; context: Qt.ApplicationShortcut; onActivated: backend.launchApp(4) }
    Shortcut { sequence: "6"; context: Qt.ApplicationShortcut; onActivated: backend.launchApp(5) }
    Shortcut { sequence: "7"; context: Qt.ApplicationShortcut; onActivated: backend.launchApp(6) }
    Shortcut { sequence: "8"; context: Qt.ApplicationShortcut; onActivated: backend.launchApp(7) }
    Shortcut { sequence: "9"; context: Qt.ApplicationShortcut; onActivated: backend.launchApp(8) }

    Item {
        anchors.fill: parent
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        // Ambient background gradient reactive to currently selected media platform
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: backend.selectedApp.secondaryColor ? Qt.darker(backend.selectedApp.secondaryColor, 2.2) : "#11131f"
                    Behavior on color { ColorAnimation { duration: 260 } }
                }
                GradientStop { position: 0.45; color: "#0b0d14" }
                GradientStop { position: 1.0; color: "#07080c" }
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: win.compactMode ? 16 : 24
            spacing: win.compactMode ? 12 : 18

            // Top Header Bar
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                // Plus TV Brand Badge
                Rectangle {
                    Layout.preferredWidth: win.compactMode ? 144 : 164
                    Layout.preferredHeight: win.compactMode ? 38 : 44
                    radius: Math.max(8, theme.radius)
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#e30613" }
                        GradientStop { position: 1.0; color: "#9d4edd" }
                    }

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 8

                        Rectangle {
                            width: 22
                            height: 22
                            radius: 5
                            color: "#ffffff"
                            Text {
                                anchors.centerIn: parent
                                text: "+"
                                color: "#e30613"
                                font.pixelSize: 17
                                font.bold: true
                            }
                        }

                        Text {
                            text: "PLUS TV"
                            color: "#ffffff"
                            font.pixelSize: win.compactMode ? 16 : 18
                            font.weight: Font.Black
                            font.letterSpacing: 1.2
                        }
                    }
                }

                // Category Pills
                RowLayout {
                    spacing: 8
                    Repeater {
                        model: win.categories
                        delegate: Rectangle {
                            readonly property bool active: backend.category === modelData.key
                            height: win.compactMode ? 34 : 38
                            width: catLabel.implicitWidth + (win.compactMode ? 20 : 26)
                            radius: Math.max(8, theme.radius)
                            color: active ? theme.accent : "#181b28"
                            border.width: active ? 0 : 1
                            border.color: "#2b3044"

                            Text {
                                id: catLabel
                                anchors.centerIn: parent
                                text: modelData.label
                                color: active ? theme.accentForeground : "#cbd5e1"
                                font.pixelSize: win.compactMode ? 12 : 13
                                font.bold: active
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: backend.category = modelData.key
                            }
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                // Live Clock, Smart Remote QR & Help Trigger
                RowLayout {
                    spacing: 8

                    Rectangle {
                        height: win.compactMode ? 34 : 38
                        width: remoteBtnRow.implicitWidth + (win.compactMode ? 20 : 26)
                        radius: Math.max(8, theme.radius)
                        color: win.remoteVisible ? theme.accent : "#181d2e"
                        border.width: 1
                        border.color: win.remoteVisible ? theme.accent : "#38bdf8"

                        RowLayout {
                            id: remoteBtnRow
                            anchors.centerIn: parent
                            spacing: 6

                            Text {
                                text: "📱"
                                font.pixelSize: win.compactMode ? 13 : 14
                            }

                            Text {
                                text: qsTr("Remote")
                                color: win.remoteVisible ? theme.accentForeground : "#f8fafc"
                                font.pixelSize: win.compactMode ? 12 : 13
                                font.bold: true
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                win.helpVisible = false
                                backend.refreshRemoteInfo()
                                win.remoteVisible = !win.remoteVisible
                            }
                        }
                    }

                    Rectangle {
                        height: win.compactMode ? 34 : 38
                        width: clockText.implicitWidth + 22
                        radius: Math.max(8, theme.radius)
                        color: "#151824"
                        border.width: 1
                        border.color: "#262b3d"

                        Text {
                            id: clockText
                            anchors.centerIn: parent
                            text: win.currentTime
                            color: "#f8fafc"
                            font.pixelSize: win.compactMode ? 13 : 15
                            font.bold: true
                        }
                    }

                    Rectangle {
                        width: win.compactMode ? 34 : 38
                        height: win.compactMode ? 34 : 38
                        radius: Math.max(8, theme.radius)
                        color: win.helpVisible ? theme.accent : "#151824"
                        border.width: 1
                        border.color: "#262b3d"

                        Text {
                            anchors.centerIn: parent
                            text: "?"
                            color: win.helpVisible ? theme.accentForeground : "#94a3b8"
                            font.pixelSize: 15
                            font.bold: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                win.remoteVisible = false
                                win.helpVisible = !win.helpVisible
                            }
                        }
                    }
                }
            }

            // Featured Hero Stage Banner
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: win.compactMode ? 148 : 205
                radius: Math.max(12, theme.radius)
                color: "#121521"
                border.width: 1
                border.color: backend.selectedApp.accentColor ? backend.selectedApp.accentColor : theme.accent
                clip: true

                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: "#111420" }
                    GradientStop {
                        position: 0.65
                        color: backend.selectedApp.secondaryColor ? Qt.darker(backend.selectedApp.secondaryColor, 1.65) : "#1a1f35"
                        Behavior on color { ColorAnimation { duration: 220 } }
                    }
                    GradientStop {
                        position: 1.0
                        color: backend.selectedApp.accentColor ? Qt.darker(backend.selectedApp.accentColor, 1.35) : "#252b48"
                        Behavior on color { ColorAnimation { duration: 220 } }
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: win.compactMode ? 16 : 22
                    spacing: 20

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: win.compactMode ? 4 : 8

                        Rectangle {
                            height: 24
                            width: heroBadgeText.implicitWidth + 20
                            radius: 6
                            color: backend.selectedApp.accentColor ? backend.selectedApp.accentColor : theme.accent

                            Text {
                                id: heroBadgeText
                                anchors.centerIn: parent
                                text: backend.selectedApp.badge ? backend.selectedApp.badge : "4K ULTRA HD"
                                color: "#ffffff"
                                font.pixelSize: 11
                                font.bold: true
                                font.letterSpacing: 1.1
                            }
                        }

                        Text {
                            text: backend.selectedApp.name ? backend.selectedApp.name : "Plus TV"
                            color: "#ffffff"
                            font.pixelSize: win.compactMode ? 26 : 36
                            font.weight: Font.Black
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Text {
                            text: backend.selectedApp.subtitle ? backend.selectedApp.subtitle : ""
                            color: "#cbd5e1"
                            font.pixelSize: win.compactMode ? 13 : 15
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Item { Layout.fillHeight: true }

                        RowLayout {
                            spacing: 12

                            Rectangle {
                                id: heroLaunchBtn
                                height: win.compactMode ? 36 : 42
                                width: launchBtnText.implicitWidth + 36
                                radius: Math.max(8, theme.radius)
                                color: backend.selectedApp.accentColor ? backend.selectedApp.accentColor : theme.accent

                                Text {
                                    id: launchBtnText
                                    anchors.centerIn: parent
                                    text: "▶  " + qsTr("Launch Now")
                                    color: "#ffffff"
                                    font.pixelSize: win.compactMode ? 13 : 15
                                    font.bold: true
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: backend.primaryAction()
                                }
                            }

                            Text {
                                text: qsTr("Click any cover card or press Enter / Space to open")
                                color: "#94a3b8"
                                font.pixelSize: 12
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }
                    }

                    // Widescreen Hero Cover Art Preview on Right
                    Rectangle {
                        visible: win.width >= 680
                        Layout.preferredWidth: win.compactMode ? 210 : 288
                        Layout.preferredHeight: win.compactMode ? 118 : 162
                        radius: Math.max(10, theme.radius)
                        color: "#090b10"
                        border.width: 2
                        border.color: "#ffffff"
                        clip: true

                        Image {
                            anchors.fill: parent
                            anchors.margins: 2
                            source: backend.selectedApp.cover ? backend.selectedApp.cover : ""
                            fillMode: Image.PreserveAspectCrop
                            sourceSize.width: 640
                            sourceSize.height: 360
                            smooth: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: backend.primaryAction()
                        }
                    }
                }
            }

            // Section Title
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: qsTr("Media Applications")
                    color: "#f8fafc"
                    font.pixelSize: win.compactMode ? 15 : 17
                    font.bold: true
                }
                Item { Layout.fillWidth: true }
                Text {
                    readonly property string _lang: translator.language
                    text: qsTr("%n platforms ready", "", backend.count)
                    color: "#94a3b8"
                    font.pixelSize: 13
                }
            }

            // Showy Cover Art Grid
            GridView {
                id: coverGrid
                Layout.fillWidth: true
                Layout.fillHeight: true
                readonly property int columns: Math.max(2, Math.min(5, Math.floor(width / 210)))
                readonly property int rows: Math.max(1, Math.ceil(backend.count / columns))
                cellWidth: Math.floor(width / columns)
                cellHeight: Math.min(Math.round(cellWidth * 0.66), Math.max(130, Math.floor(height / rows)))
                clip: true
                model: backend.apps
                currentIndex: backend.selectedIndex

                delegate: Item {
                    width: coverGrid.cellWidth
                    height: coverGrid.cellHeight

                    readonly property bool isCurrent: index === backend.selectedIndex

                    Rectangle {
                        id: cardRect
                        anchors.fill: parent
                        anchors.margins: 7
                        radius: Math.max(10, theme.radius)
                        color: "#131724"
                        border.width: isCurrent ? 3 : 1
                        border.color: isCurrent ? (modelData.accentColor ? modelData.accentColor : theme.accent) : "#242a3d"
                        scale: isCurrent ? 1.025 : 1.0
                        Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutQuad } }

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 0

                            // Cover Image Banner
                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                Image {
                                    anchors.fill: parent
                                    anchors.margins: isCurrent ? 3 : 1
                                    source: modelData.cover
                                    fillMode: Image.PreserveAspectCrop
                                    sourceSize.width: 640
                                    sourceSize.height: 360
                                    smooth: true
                                }

                                // Quick-launch number badge (top-left)
                                Rectangle {
                                    visible: index < 9
                                    anchors.top: parent.top
                                    anchors.left: parent.left
                                    anchors.margins: 9
                                    width: 24
                                    height: 24
                                    radius: 6
                                    color: "#cc090b10"
                                    border.width: 1
                                    border.color: "#475569"

                                    Text {
                                        anchors.centerIn: parent
                                        text: (index + 1).toString()
                                        color: "#f8fafc"
                                        font.pixelSize: 11
                                        font.bold: true
                                    }
                                }

                                // Quality / Category pill (top-right)
                                Rectangle {
                                    anchors.top: parent.top
                                    anchors.right: parent.right
                                    anchors.margins: 9
                                    height: 22
                                    width: cardBadge.implicitWidth + 14
                                    radius: 5
                                    color: modelData.accentColor ? modelData.accentColor : "#e30613"

                                    Text {
                                        id: cardBadge
                                        anchors.centerIn: parent
                                        text: modelData.badge
                                        color: "#ffffff"
                                        font.pixelSize: 10
                                        font.bold: true
                                    }
                                }
                            }

                            // Bottom Info Strip
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 44
                                color: isCurrent ? "#1d2236" : "#111420"
                                radius: Math.max(8, theme.radius)

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    spacing: 8

                                    Rectangle {
                                        width: 9
                                        height: 9
                                        radius: 5
                                        color: modelData.accentColor ? modelData.accentColor : theme.accent
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 1

                                        Text {
                                            text: modelData.name
                                            color: "#ffffff"
                                            font.pixelSize: 14
                                            font.bold: true
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }

                                        Text {
                                            text: modelData.subtitle
                                            color: "#94a3b8"
                                            font.pixelSize: 11
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                    }

                                    Rectangle {
                                        visible: isCurrent
                                        height: 24
                                        width: openLabel.implicitWidth + 14
                                        radius: 5
                                        color: modelData.accentColor ? modelData.accentColor : theme.accent

                                        Text {
                                            id: openLabel
                                            anchors.centerIn: parent
                                            text: qsTr("OPEN") + " ▶"
                                            color: "#ffffff"
                                            font.pixelSize: 10
                                            font.bold: true
                                        }
                                    }
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            preventStealing: true
                            onEntered: backend.selectedIndex = index
                            onClicked: backend.launchApp(index)
                        }
                    }
                }
            }
        }

        // Launch notification toast
        Rectangle {
            id: launchToast
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottomMargin: 24
            height: 44
            width: toastLabel.implicitWidth + 40
            radius: Math.max(8, theme.radius)
            color: "#1e293b"
            border.width: 2
            border.color: theme.accent
            opacity: 0
            visible: opacity > 0
            Behavior on opacity { NumberAnimation { duration: 180 } }

            Text {
                id: toastLabel
                anchors.centerIn: parent
                text: backend.statusMessage
                color: "#ffffff"
                font.pixelSize: 14
                font.bold: true
            }
        }

        // Keyboard Shortcuts Overlay (?)
        Rectangle {
            anchors.fill: parent
            color: "#d907080d"
            visible: win.helpVisible

            MouseArea {
                anchors.fill: parent
                onClicked: win.helpVisible = false
            }

            Rectangle {
                anchors.centerIn: parent
                width: 460
                height: 320
                radius: Math.max(12, theme.radius)
                color: "#131724"
                border.width: 1
                border.color: theme.accent

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 26
                    spacing: 12

                    Text {
                        text: qsTr("Keyboard Shortcuts")
                        color: "#ffffff"
                        font.pixelSize: 20
                        font.bold: true
                    }

                    Repeater {
                        model: [
                            { k: "← → ↑ ↓", d: qsTr("Navigate media cards") },
                            { k: "Enter / Space", d: qsTr("Launch selected application") },
                            { k: "1 - 9", d: qsTr("Quick launch channel 1 - 9") },
                            { k: "Tab", d: qsTr("Switch category tab") },
                            { k: "R", d: qsTr("Open mobile Smart Remote QR") },
                            { k: "?", d: qsTr("Show / hide shortcuts") }
                        ]
                        delegate: RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: modelData.k
                                color: theme.accent
                                font.pixelSize: 13
                                font.bold: true
                                Layout.preferredWidth: 130
                            }
                            Text {
                                text: modelData.d
                                color: "#e2e8f0"
                                font.pixelSize: 13
                                Layout.fillWidth: true
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }
        }

        // Smart TV Phone Remote QR Overlay (R)
        Rectangle {
            anchors.fill: parent
            color: "#de07080d"
            visible: win.remoteVisible

            MouseArea {
                anchors.fill: parent
                onClicked: win.remoteVisible = false
            }

            Rectangle {
                anchors.centerIn: parent
                width: Math.min(win.width - 40, 620)
                height: Math.min(win.height - 40, 390)
                radius: Math.max(14, theme.radius)
                color: "#121624"
                border.width: 2
                border.color: "#38bdf8"

                MouseArea { anchors.fill: parent }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 26
                    spacing: 24

                    // Left: Crisp QR Code Box
                    Rectangle {
                        Layout.preferredWidth: 220
                        Layout.preferredHeight: 220
                        radius: Math.max(12, theme.radius)
                        color: "#ffffff"
                        border.width: 4
                        border.color: "#38bdf8"

                        Image {
                            anchors.fill: parent
                            anchors.margins: 10
                            source: backend.remoteQrDataUri
                            fillMode: Image.PreserveAspectFit
                            sourceSize.width: 440
                            sourceSize.height: 440
                            smooth: false
                        }
                    }

                    // Right: Instructions & URL
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 10

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "📱 " + qsTr("Smart TV Mobile Remote")
                                color: "#ffffff"
                                font.pixelSize: 22
                                font.bold: true
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                        }

                        Text {
                            text: qsTr("Scan this QR code with your phone camera on the same Wi-Fi network to control Plus TV, switch channels, adjust volume, or use your phone as a touchpad & keyboard.")
                            color: "#cbd5e1"
                            font.pixelSize: 13
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 42
                            radius: Math.max(8, theme.radius)
                            color: "#090b12"
                            border.width: 1
                            border.color: "#334155"

                            Text {
                                anchors.centerIn: parent
                                text: backend.remoteUrl
                                color: "#38bdf8"
                                font.pixelSize: 16
                                font.bold: true
                            }
                        }

                        ColumnLayout {
                            spacing: 4
                            Layout.fillWidth: true

                            Text { text: "• " + qsTr("D-Pad, OK, Home & Close Channel buttons"); color: "#94a3b8"; font.pixelSize: 12 }
                            Text { text: "• " + qsTr("Remote Power: Shutdown, restart, sleep & screen toggle"); color: "#94a3b8"; font.pixelSize: 12 }
                            Text { text: "• " + qsTr("Desktop Switcher: Jump between active workspaces"); color: "#94a3b8"; font.pixelSize: 12 }
                            Text { text: "• " + qsTr("Volume +/-/Mute & 10s Media Seek controls"); color: "#94a3b8"; font.pixelSize: 12 }
                            Text { text: "• " + qsTr("Air Touchpad mouse & mobile keyboard search"); color: "#94a3b8"; font.pixelSize: 12 }
                        }

                        Item { Layout.fillHeight: true }

                        RowLayout {
                            Layout.fillWidth: true
                            Item { Layout.fillWidth: true }
                            Rectangle {
                                height: 34
                                width: closeRemoteText.implicitWidth + 28
                                radius: Math.max(8, theme.radius)
                                color: theme.accent

                                Text {
                                    id: closeRemoteText
                                    anchors.centerIn: parent
                                    text: qsTr("Close") + " (Esc)"
                                    color: theme.accentForeground
                                    font.pixelSize: 13
                                    font.bold: true
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: win.remoteVisible = false
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
