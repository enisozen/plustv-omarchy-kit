#include "backend.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>

Backend::Backend(const QStringList &applicationDirs, QObject *parent, bool dryRunLaunch)
    : QObject(parent), m_dryRunLaunch(dryRunLaunch) {
    if (!applicationDirs.isEmpty()) {
        m_appDirs = applicationDirs;
    } else {
        m_appDirs = {
            QDir::homePath() + "/.local/share/applications",
            "/usr/share/applications",
            "/var/lib/flatpak/exports/share/applications",
            QDir::homePath() + "/.local/share/flatpak/exports/share/applications"
        };
    }

    for (const auto &dir : m_appDirs) {
        if (QFileInfo::exists(dir))
            m_watcher.addPath(dir);
    }
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &Backend::refresh);

    refresh();
    startRemoteServer();
}

QString Backend::cleanExecCommand(const QString &execLine) {
    QString cleaned = execLine.trimmed();
    static const QRegularExpression fieldCodes(R"(\s+%[fFuUdDnNickvm])");
    cleaned.remove(fieldCodes);
    return cleaned.trimmed();
}

QString Backend::findDesktopExec(const QStringList &candidates, bool *foundOut) const {
    if (foundOut)
        *foundOut = false;

    for (const auto &dirPath : m_appDirs) {
        QDir dir(dirPath);
        if (!dir.exists())
            continue;

        for (const auto &candidate : candidates) {
            const QString filePath = dir.filePath(candidate);
            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
                continue;

            while (!file.atEnd()) {
                const QString line = QString::fromUtf8(file.readLine()).trimmed();
                if (line.startsWith("Exec=")) {
                    if (foundOut)
                        *foundOut = true;
                    return cleanExecCommand(line.mid(5));
                }
            }
        }
    }
    return {};
}

void Backend::refresh() {
    const QList<CatalogEntry> catalog = {
        {
            "bein-connect",
            "beIN CONNECT",
            tr("Live football, Süper Lig, movies and series on demand"),
            "CANLI TV • 4K SPOR",
            "live",
            "qrc:/covers/bein-connect.svg",
            "#9D4EDD",
            "#E30613",
            {"beIN CONNECT.desktop", "bein-connect.desktop"},
            "omarchy-launch-webapp \"https://www.beinconnect.com.tr\""
        },
        {
            "digiturk",
            "Digiturk",
            tr("Official Digiturk satellite & digital broadcast portal"),
            "PREMİUM YAYIN",
            "live",
            "qrc:/covers/digiturk.svg",
            "#E30613",
            "#990012",
            {"Digiturk.desktop", "digiturk.desktop"},
            "omarchy-launch-webapp \"https://www.digiturk.com.tr\""
        },
        {
            "tvplus",
            "TV+",
            tr("Turkcell TV+ live channels, movies and exclusive series"),
            "CANLI TV • SİNEMA",
            "live",
            "qrc:/covers/tvplus.svg",
            "#FFB703",
            "#0077B6",
            {"TV+.desktop", "tvplus.desktop"},
            "omarchy-launch-webapp \"https://tvplus.com.tr\""
        },
        {
            "apple-tv",
            "Apple TV+",
            tr("Apple Originals, prestige series and 4K Dolby Vision cinema"),
            "4K DOLBY VISION",
            "cinema",
            "qrc:/covers/apple-tv.svg",
            "#38BDF8",
            "#1E293B",
            {"Apple TV.desktop", "apple-tv.desktop"},
            "omarchy-launch-webapp \"https://tv.apple.com\""
        },
        {
            "prime-video",
            "Amazon Prime Video",
            tr("Prime Originals, blockbuster movies and popular TV series"),
            "PRIME 4K HDR",
            "cinema",
            "qrc:/covers/prime-video.svg",
            "#00A8E1",
            "#0F172A",
            {"Amazon Prime Video.desktop", "amazon-prime-video.desktop"},
            "omarchy-launch-webapp \"https://www.primevideo.com\""
        },
        {
            "netflix",
            "Netflix",
            tr("Global hit series, original films and documentaries"),
            "ULTRA HD 4K",
            "cinema",
            "qrc:/covers/netflix.svg",
            "#E50914",
            "#450A0A",
            {"Netflix.desktop", "netflix.desktop"},
            "omarchy-launch-webapp \"https://www.netflix.com\""
        },
        {
            "youtube",
            "YouTube",
            tr("Videos, live streams, music channels and 4K creators"),
            "4K 60FPS • CANLI",
            "video",
            "qrc:/covers/youtube.svg",
            "#FF0033",
            "#3F000B",
            {"YouTube.desktop", "youtube.desktop"},
            "omarchy-launch-webapp \"https://youtube.com/\""
        },
        {
            "chrome",
            "Google Chrome",
            tr("Full web browser, media streaming and web extensions"),
            "WEB & MEDYA",
            "web",
            "qrc:/covers/chrome.svg",
            "#4285F4",
            "#1E3A8A",
            {"google-chrome.desktop", "com.google.Chrome.desktop", "chromium.desktop", "omarchy-browser.desktop"},
            "google-chrome-stable"
        },
        {
            "apple-music",
            "Apple Music",
            tr("Millions of songs in lossless audio and spatial sound"),
            "Hİ-RES LOSSLESS",
            "video",
            "qrc:/covers/apple-music.svg",
            "#FA233B",
            "#4C0519",
            {"Apple Music.desktop", "apple-music.desktop"},
            "omarchy-launch-webapp \"https://music.apple.com\""
        },
        {
            "spotify",
            "Spotify",
            tr("Non-stop music, podcasts and personalized playlists"),
            "MÜZİK & PODCAST",
            "video",
            "qrc:/covers/spotify.svg",
            "#1DB954",
            "#064E3B",
            {"Spotify.desktop", "spotify.desktop", "com.spotify.Client.desktop"},
            "omarchy-launch-webapp \"https://open.spotify.com/\""
        },
        {
            "youtube-music",
            "YouTube Music",
            tr("Official albums, music videos, remixes and live performances"),
            "KLİP & MÜZİK",
            "video",
            "qrc:/covers/youtube-music.svg",
            "#FF0033",
            "#881337",
            {"YouTube Music.desktop", "youtube-music.desktop"},
            "omarchy-launch-webapp \"https://music.youtube.com/\""
        },
        {
            "geforce-now",
            "GeForce NOW",
            tr("NVIDIA RTX cloud gaming streaming straight to your screen"),
            "RTX BULUT OYUN",
            "web",
            "qrc:/covers/geforce-now.svg",
            "#76B900",
            "#14532D",
            {"NVIDIA GeForce NOW.desktop", "nvidia-geforce-now.desktop"},
            "omarchy-launch-webapp \"https://play.geforcenow.com\" --disable-gpu"
        }
    };

    m_allApps.clear();
    for (const auto &item : catalog) {
        bool found = false;
        QString execCmd = findDesktopExec(item.desktopCandidates, &found);
        if (execCmd.isEmpty())
            execCmd = item.fallbackExec;

        QVariantMap map;
        map["id"] = item.id;
        map["name"] = item.name;
        map["subtitle"] = item.subtitle;
        map["badge"] = item.badge;
        map["category"] = item.category;
        map["cover"] = item.cover;
        map["accentColor"] = item.accentColor;
        map["secondaryColor"] = item.secondaryColor;
        map["execCommand"] = execCmd;
        map["installed"] = found;
        m_allApps.append(map);
    }

    applyFilter();
}

void Backend::applyFilter() {
    m_filteredApps.clear();
    const QString query = m_searchQuery.trimmed().toLower();

    for (const auto &v : m_allApps) {
        const QVariantMap map = v.toMap();
        if (m_category != "all" && map.value("category").toString() != m_category)
            continue;

        if (!query.isEmpty()) {
            const QString name = map.value("name").toString().toLower();
            const QString sub = map.value("subtitle").toString().toLower();
            const QString badge = map.value("badge").toString().toLower();
            if (!name.contains(query) && !sub.contains(query) && !badge.contains(query))
                continue;
        }
        m_filteredApps.append(map);
    }

    if (m_selectedIndex >= m_filteredApps.size())
        m_selectedIndex = m_filteredApps.isEmpty() ? 0 : m_filteredApps.size() - 1;
    if (m_selectedIndex < 0)
        m_selectedIndex = 0;

    emit appsChanged();
    emit selectedIndexChanged();
}

void Backend::setSelectedIndex(int index) {
    if (m_filteredApps.isEmpty()) {
        if (m_selectedIndex != 0) {
            m_selectedIndex = 0;
            emit selectedIndexChanged();
        }
        return;
    }
    const int clamped = qBound(0, index, static_cast<int>(m_filteredApps.size()) - 1);
    if (clamped == m_selectedIndex)
        return;
    m_selectedIndex = clamped;
    emit selectedIndexChanged();
}

QVariantMap Backend::selectedApp() const {
    if (m_selectedIndex >= 0 && m_selectedIndex < m_filteredApps.size())
        return m_filteredApps.at(m_selectedIndex).toMap();
    return {};
}

void Backend::setCategory(const QString &category) {
    if (m_category == category)
        return;
    m_category = category;
    m_selectedIndex = 0;
    applyFilter();
    emit filterChanged();
}

void Backend::setSearchQuery(const QString &query) {
    if (m_searchQuery == query)
        return;
    m_searchQuery = query;
    m_selectedIndex = 0;
    applyFilter();
    emit filterChanged();
}

void Backend::primaryAction() {
    launchApp(m_selectedIndex);
}

bool Backend::launchApp(int index) {
    if (index < 0 || index >= m_filteredApps.size())
        return false;

    setSelectedIndex(index);
    const QVariantMap app = m_filteredApps.at(index).toMap();
    const QString name = app.value("name").toString();
    const QString execCmd = app.value("execCommand").toString();
    if (execCmd.isEmpty())
        return false;

    m_lastLaunchedCommand = execCmd;
    m_statusMessage = tr("Launching %1...").arg(name);
    emit statusMessageChanged();
    emit appLaunched(name);

    if (m_dryRunLaunch)
        return true;

    const QString safeCmd =
        "target_ws=$(hyprctl workspaces -j 2>/dev/null | jq -r '"
        "[.[] | select(.id >= 2 and (.windows // 0) > 0) | .id] as $used | "
        "[range(2; 11) | select((. as $w | $used | index($w)) | not)][0] // 2"
        "' 2>/dev/null); "
        "hyprctl dispatch \"o.workspaces.focus(${target_ws:-2})\" >/dev/null 2>&1; "
        "pgrep -x chrome >/dev/null 2>&1 || rm -f \"$HOME/.config/google-chrome/SingletonLock\" "
        "\"$HOME/.config/google-chrome/SingletonSocket\" \"$HOME/.config/google-chrome/SingletonCookie\"; " +
        execCmd;
    return QProcess::startDetached("/bin/sh", {"-c", safeCmd});
}

bool Backend::launchById(const QString &id) {
    for (int i = 0; i < m_filteredApps.size(); ++i) {
        if (m_filteredApps.at(i).toMap().value("id").toString() == id)
            return launchApp(i);
    }
    if (m_category != "all") {
        setCategory("all");
        for (int i = 0; i < m_filteredApps.size(); ++i) {
            if (m_filteredApps.at(i).toMap().value("id").toString() == id)
                return launchApp(i);
        }
    }
    return false;
}

QString Backend::detectLanIpAddress() {
    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : interfaces) {
        const auto flags = iface.flags();
        if (!(flags & QNetworkInterface::IsUp) || !(flags & QNetworkInterface::IsRunning))
            continue;
        if (flags & QNetworkInterface::IsLoopBack)
            continue;
        const QString name = iface.name().toLower();
        if (name.startsWith("docker") || name.startsWith("br-") || name.startsWith("veth") || name.startsWith("virbr"))
            continue;
        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            const QHostAddress ip = entry.ip();
            if (ip.protocol() == QAbstractSocket::IPv4Protocol && !ip.isLoopback())
                return ip.toString();
        }
    }
    return QStringLiteral("127.0.0.1");
}

QString Backend::generateQrDataUri(const QString &text) {
    if (text.isEmpty())
        return {};

    QProcess proc;
    proc.start(QStringLiteral("qrencode"), {QStringLiteral("-t"), QStringLiteral("SVG"), QStringLiteral("-m"), QStringLiteral("1"), QStringLiteral("-o"), QStringLiteral("-"), text});
    if (proc.waitForFinished(1500) && proc.exitCode() == 0) {
        const QByteArray svg = proc.readAllStandardOutput();
        if (!svg.isEmpty())
            return QStringLiteral("data:image/svg+xml;base64,") + QString::fromLatin1(svg.toBase64());
    }
    return {};
}

void Backend::startRemoteServer() {
    connect(&m_remoteServer, &QTcpServer::newConnection, this, &Backend::handleRemoteConnection);
    // Port 53317/tcp is pre-opened by Omarchy's default UFW firewall configuration.
    const quint16 preferredPort = m_dryRunLaunch ? 0 : 53317;
    if (!m_remoteServer.listen(QHostAddress::Any, preferredPort)) {
        if (!m_remoteServer.listen(QHostAddress::Any, 8765))
            m_remoteServer.listen(QHostAddress::Any, 0);
    }
    refreshRemoteInfo();
}

void Backend::refreshRemoteInfo() {
    const int port = m_remoteServer.isListening() ? m_remoteServer.serverPort() : 53317;
    const QString ip = detectLanIpAddress();
    const QString url = QStringLiteral("http://%1:%2").arg(ip).arg(port);
    if (url != m_remoteUrl || m_remoteQrDataUri.isEmpty()) {
        m_remoteUrl = url;
        m_remoteQrDataUri = generateQrDataUri(m_remoteUrl);
        emit remoteInfoChanged();
    }
}

bool Backend::executeRemoteAction(const QString &action, const QString &payload) {
    const QString act = action.trimmed();
    if (act.isEmpty())
        return false;

    emit remoteCommandReceived(act);

    const bool plusTvFocused = m_dryRunLaunch || (QGuiApplication::focusWindow() != nullptr);

    if (act == QStringLiteral("nav_left")) {
        if (plusTvFocused) {
            setSelectedIndex(qMax(0, m_selectedIndex - 1));
        } else if (!m_dryRunLaunch) {
            QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("-k"), QStringLiteral("Left")});
        }
        return true;
    }
    if (act == QStringLiteral("nav_right")) {
        if (plusTvFocused) {
            setSelectedIndex(qMin(qMax(0, count() - 1), m_selectedIndex + 1));
        } else if (!m_dryRunLaunch) {
            QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("-k"), QStringLiteral("Right")});
        }
        return true;
    }
    if (act == QStringLiteral("nav_up")) {
        if (plusTvFocused) {
            setSelectedIndex(qMax(0, m_selectedIndex - 4));
        } else if (!m_dryRunLaunch) {
            QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("-k"), QStringLiteral("Up")});
        }
        return true;
    }
    if (act == QStringLiteral("nav_down")) {
        if (plusTvFocused) {
            setSelectedIndex(qMin(qMax(0, count() - 1), m_selectedIndex + 4));
        } else if (!m_dryRunLaunch) {
            QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("-k"), QStringLiteral("Down")});
        }
        return true;
    }
    if (act == QStringLiteral("nav_ok")) {
        if (plusTvFocused) {
            primaryAction();
        } else if (!m_dryRunLaunch) {
            QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("-k"), QStringLiteral("Return")});
        }
        return true;
    }
    if (act == QStringLiteral("category_next")) {
        static const QStringList cats = {QStringLiteral("all"), QStringLiteral("live"), QStringLiteral("cinema"), QStringLiteral("video"), QStringLiteral("web")};
        const int idx = qMax(0, cats.indexOf(m_category));
        setCategory(cats.at((idx + 1) % cats.size()));
        return true;
    }
    if (act == QStringLiteral("category_prev")) {
        static const QStringList cats = {QStringLiteral("all"), QStringLiteral("live"), QStringLiteral("cinema"), QStringLiteral("video"), QStringLiteral("web")};
        const int idx = qMax(0, cats.indexOf(m_category));
        setCategory(cats.at((idx - 1 + cats.size()) % cats.size()));
        return true;
    }
    if (act == QStringLiteral("launch")) {
        return launchById(payload.trimmed());
    }
    if (act == QStringLiteral("home")) {
        m_statusMessage = tr("Plus TV Home");
        emit statusMessageChanged();
        if (!m_dryRunLaunch) {
            QProcess::startDetached(QStringLiteral("hyprctl"), {QStringLiteral("dispatch"), QStringLiteral("o.workspaces.focus(1)")});
            QProcess::startDetached(QStringLiteral("hyprctl"), {QStringLiteral("dispatch"), QStringLiteral("hl.dsp.focus({ window = \"class:^plustv$\" })")});
        }
        return true;
    }
    if (act == QStringLiteral("close_app")) {
        m_statusMessage = tr("Returned to Plus TV");
        emit statusMessageChanged();
        if (!m_dryRunLaunch) {
            const QString closeCmd =
                "cls=$(hyprctl activewindow -j 2>/dev/null | jq -r '.class // empty'); "
                "if [ -n \"$cls\" ] && [ \"$cls\" != \"plustv\" ]; then "
                "  hyprctl dispatch 'hl.dsp.window.close()' >/dev/null 2>&1; "
                "fi; "
                "hyprctl dispatch 'o.workspaces.focus(1)' >/dev/null 2>&1";
            QProcess::startDetached(QStringLiteral("/bin/sh"), {QStringLiteral("-c"), closeCmd});
        }
        return true;
    }
    if (act == QStringLiteral("fullscreen")) {
        if (!m_dryRunLaunch) {
            const QString fsCmd =
                "cls=$(hyprctl activewindow -j 2>/dev/null | jq -r '.class // empty'); "
                "if [ \"$cls\" = \"plustv\" ]; then "
                "  hyprctl dispatch 'hl.dsp.window.fullscreen({ mode = \"fullscreen\", action = \"set\", window = \"class:^plustv$\" })' >/dev/null 2>&1; "
                "else "
                "  hyprctl dispatch 'hl.dsp.window.fullscreen({ mode = \"fullscreen\" })' >/dev/null 2>&1; "
                "fi";
            QProcess::startDetached(QStringLiteral("/bin/sh"), {QStringLiteral("-c"), fsCmd});
        }
        return true;
    }
    if (act == QStringLiteral("back")) {
        if (!m_dryRunLaunch)
            QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("-k"), QStringLiteral("Escape")});
        return true;
    }
    if (act == QStringLiteral("vol_up")) {
        if (!m_dryRunLaunch)
            QProcess::startDetached(QStringLiteral("wpctl"), {QStringLiteral("set-volume"), QStringLiteral("-l"), QStringLiteral("1.0"), QStringLiteral("@DEFAULT_AUDIO_SINK@"), QStringLiteral("5%+")});
        return true;
    }
    if (act == QStringLiteral("vol_down")) {
        if (!m_dryRunLaunch)
            QProcess::startDetached(QStringLiteral("wpctl"), {QStringLiteral("set-volume"), QStringLiteral("@DEFAULT_AUDIO_SINK@"), QStringLiteral("5%-")});
        return true;
    }
    if (act == QStringLiteral("vol_mute")) {
        if (!m_dryRunLaunch)
            QProcess::startDetached(QStringLiteral("wpctl"), {QStringLiteral("set-mute"), QStringLiteral("@DEFAULT_AUDIO_SINK@"), QStringLiteral("toggle")});
        return true;
    }
    if (act == QStringLiteral("play_pause")) {
        if (!m_dryRunLaunch)
            QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("-k"), QStringLiteral("space")});
        return true;
    }
    if (act == QStringLiteral("seek_back")) {
        if (!m_dryRunLaunch)
            QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("-k"), QStringLiteral("Left")});
        return true;
    }
    if (act == QStringLiteral("seek_fwd")) {
        if (!m_dryRunLaunch)
            QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("-k"), QStringLiteral("Right")});
        return true;
    }
    if (act == QStringLiteral("type_text")) {
        if (!payload.isEmpty() && !m_dryRunLaunch)
            QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("--"), payload});
        return true;
    }
    if (act == QStringLiteral("type_enter")) {
        if (!m_dryRunLaunch)
            QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("-k"), QStringLiteral("Return")});
        return true;
    }
    if (act == QStringLiteral("type_backspace")) {
        if (!m_dryRunLaunch)
            QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("-k"), QStringLiteral("BackSpace")});
        return true;
    }
    if (act == QStringLiteral("key_press")) {
        QString key = payload.trimmed();
        if (key.isEmpty())
            return true;
        if (key == QStringLiteral("Enter")) key = QStringLiteral("Return");
        else if (key == QStringLiteral("Esc")) key = QStringLiteral("Escape");
        else if (key == QStringLiteral("Space")) key = QStringLiteral("space");
        else if (key == QStringLiteral("Backspace")) key = QStringLiteral("BackSpace");
        else if (key == QStringLiteral("Del")) key = QStringLiteral("Delete");
        else if (key == QStringLiteral("Win") || key == QStringLiteral("Super")) key = QStringLiteral("Super_L");

        static const QSet<QString> namedKeys = {
            QStringLiteral("Return"), QStringLiteral("BackSpace"), QStringLiteral("Tab"),
            QStringLiteral("Escape"), QStringLiteral("Delete"), QStringLiteral("space"),
            QStringLiteral("Up"), QStringLiteral("Down"), QStringLiteral("Left"),
            QStringLiteral("Right"), QStringLiteral("Home"), QStringLiteral("End"),
            QStringLiteral("Page_Up"), QStringLiteral("Page_Down"), QStringLiteral("Insert"),
            QStringLiteral("Super_L"), QStringLiteral("Print"), QStringLiteral("Shift_L"),
            QStringLiteral("Shift_R"), QStringLiteral("Control_L"), QStringLiteral("Control_R"),
            QStringLiteral("Alt_L"), QStringLiteral("Alt_R"),
            QStringLiteral("F1"), QStringLiteral("F2"), QStringLiteral("F3"), QStringLiteral("F4"),
            QStringLiteral("F5"), QStringLiteral("F6"), QStringLiteral("F7"), QStringLiteral("F8"),
            QStringLiteral("F9"), QStringLiteral("F10"), QStringLiteral("F11"), QStringLiteral("F12")
        };

        if (!m_dryRunLaunch) {
            if (namedKeys.contains(key)) {
                QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("-k"), key});
            } else if (key.length() == 1 && ((key.at(0) >= QLatin1Char('a') && key.at(0) <= QLatin1Char('z')) ||
                                             (key.at(0) >= QLatin1Char('0') && key.at(0) <= QLatin1Char('9')))) {
                QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("-k"), key});
            } else {
                QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("--"), key});
            }
        }
        return true;
    }
    if (act == QStringLiteral("key_combo")) {
        const QString combo = payload.trimmed();
        if (combo.isEmpty())
            return true;

        if (combo.compare(QStringLiteral("win"), Qt::CaseInsensitive) == 0 ||
            combo.compare(QStringLiteral("super"), Qt::CaseInsensitive) == 0) {
            if (!m_dryRunLaunch)
                QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("-k"), QStringLiteral("Super_L")});
            return true;
        }

        const QStringList parts = combo.split(QLatin1Char('+'));
        if (parts.size() >= 2 && !m_dryRunLaunch) {
            QStringList args;
            QStringList activeMods;
            for (int i = 0; i < parts.size() - 1; ++i) {
                QString mod = parts.at(i).trimmed().toLower();
                if (mod == QStringLiteral("ctrl") || mod == QStringLiteral("control")) mod = QStringLiteral("ctrl");
                else if (mod == QStringLiteral("alt")) mod = QStringLiteral("alt");
                else if (mod == QStringLiteral("shift")) mod = QStringLiteral("shift");
                else if (mod == QStringLiteral("win") || mod == QStringLiteral("super") || mod == QStringLiteral("logo")) mod = QStringLiteral("logo");
                args << QStringLiteral("-M") << mod;
                activeMods.prepend(mod);
            }
            QString finalKey = parts.last().trimmed();
            if (finalKey.compare(QStringLiteral("tab"), Qt::CaseInsensitive) == 0) finalKey = QStringLiteral("Tab");
            else if (finalKey.compare(QStringLiteral("enter"), Qt::CaseInsensitive) == 0) finalKey = QStringLiteral("Return");
            else if (finalKey.compare(QStringLiteral("esc"), Qt::CaseInsensitive) == 0) finalKey = QStringLiteral("Escape");
            else if (finalKey.compare(QStringLiteral("space"), Qt::CaseInsensitive) == 0) finalKey = QStringLiteral("space");
            args << QStringLiteral("-k") << finalKey;
            for (const QString &mod : activeMods) {
                args << QStringLiteral("-m") << mod;
            }
            QProcess::startDetached(QStringLiteral("wtype"), args);
        }
        return true;
    }
    if (act == QStringLiteral("game_action")) {
        const QString gact = payload.trimmed();
        if (!m_dryRunLaunch) {
            if (gact == QStringLiteral("trigger_r")) {
                QProcess::startDetached(QStringLiteral("hyprctl"), {QStringLiteral("dispatch"), QStringLiteral("hl.dsp.send_shortcut({ mods = \"\", key = \"mouse:272\" })")});
            } else if (gact == QStringLiteral("trigger_l")) {
                QProcess::startDetached(QStringLiteral("hyprctl"), {QStringLiteral("dispatch"), QStringLiteral("hl.dsp.send_shortcut({ mods = \"\", key = \"mouse:273\" })")});
            } else if (gact == QStringLiteral("bumper_r")) {
                QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("-k"), QStringLiteral("e")});
            } else if (gact == QStringLiteral("bumper_l")) {
                QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("-k"), QStringLiteral("q")});
            } else if (gact == QStringLiteral("btn_a") || gact == QStringLiteral("jump")) {
                QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("-k"), QStringLiteral("space")});
            } else if (gact == QStringLiteral("btn_b") || gact == QStringLiteral("sprint")) {
                QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("-k"), QStringLiteral("Shift_L")});
            } else if (gact == QStringLiteral("btn_x") || gact == QStringLiteral("crouch")) {
                QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("-k"), QStringLiteral("Control_L")});
            } else if (gact == QStringLiteral("btn_y")) {
                QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("-k"), QStringLiteral("r")});
            } else if (gact == QStringLiteral("select")) {
                QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("-k"), QStringLiteral("Tab")});
            } else if (gact == QStringLiteral("start")) {
                QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("-k"), QStringLiteral("Escape")});
            }
        }
        return true;
    }
    if (act == QStringLiteral("mouse_move")) {
        const QStringList parts = payload.split(',');
        if (parts.size() == 2) {
            const int dx = qBound(-400, parts.at(0).toInt(), 400);
            const int dy = qBound(-400, parts.at(1).toInt(), 400);
            if (!m_dryRunLaunch && (dx != 0 || dy != 0)) {
                const QString luaCmd = QStringLiteral(
                    "local p = hl.get_cursor_pos(); "
                    "if p then hl.dispatch(hl.dsp.cursor.move({ x = math.floor(p.x + (%1)), y = math.floor(p.y + (%2)) })) end")
                    .arg(dx).arg(dy);
                QProcess::startDetached(QStringLiteral("hyprctl"), {QStringLiteral("eval"), luaCmd});
            }
            return true;
        }
        return false;
    }
    if (act == QStringLiteral("mouse_click")) {
        if (!m_dryRunLaunch) {
            if (payload == QStringLiteral("right")) {
                QProcess::startDetached(QStringLiteral("hyprctl"), {QStringLiteral("dispatch"), QStringLiteral("hl.dsp.send_shortcut({ mods = \"\", key = \"mouse:273\" })")});
            } else if (payload == QStringLiteral("middle")) {
                QProcess::startDetached(QStringLiteral("hyprctl"), {QStringLiteral("dispatch"), QStringLiteral("hl.dsp.send_shortcut({ mods = \"\", key = \"mouse:274\" })")});
            } else if (payload == QStringLiteral("scroll_up")) {
                QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("-k"), QStringLiteral("Page_Up")});
            } else if (payload == QStringLiteral("scroll_down")) {
                QProcess::startDetached(QStringLiteral("wtype"), {QStringLiteral("-k"), QStringLiteral("Page_Down")});
            } else {
                QProcess::startDetached(QStringLiteral("hyprctl"), {QStringLiteral("dispatch"), QStringLiteral("hl.dsp.send_shortcut({ mods = \"\", key = \"mouse:272\" })")});
            }
        }
        return true;
    }
    if (act == QStringLiteral("power_shutdown")) {
        m_statusMessage = tr("Shutting down system...");
        emit statusMessageChanged();
        if (!m_dryRunLaunch) {
            QProcess::startDetached(QStringLiteral("omarchy"), {QStringLiteral("system"), QStringLiteral("shutdown")});
        }
        return true;
    }
    if (act == QStringLiteral("power_reboot")) {
        m_statusMessage = tr("Restarting system...");
        emit statusMessageChanged();
        if (!m_dryRunLaunch) {
            QProcess::startDetached(QStringLiteral("omarchy"), {QStringLiteral("system"), QStringLiteral("reboot")});
        }
        return true;
    }
    if (act == QStringLiteral("power_suspend")) {
        m_statusMessage = tr("Suspending system...");
        emit statusMessageChanged();
        if (!m_dryRunLaunch) {
            QProcess::startDetached(QStringLiteral("systemctl"), {QStringLiteral("suspend")});
        }
        return true;
    }
    if (act == QStringLiteral("screen_toggle")) {
        m_statusMessage = tr("Screen toggled");
        emit statusMessageChanged();
        if (!m_dryRunLaunch) {
            QProcess::startDetached(QStringLiteral("hyprctl"), {QStringLiteral("dispatch"), QStringLiteral("hl.dsp.dpms({ state = \"toggle\" })")});
        }
        return true;
    }
    if (act == QStringLiteral("screen_on")) {
        m_statusMessage = tr("Screen turned on");
        emit statusMessageChanged();
        if (!m_dryRunLaunch) {
            QProcess::startDetached(QStringLiteral("hyprctl"), {QStringLiteral("dispatch"), QStringLiteral("hl.dsp.dpms({ state = \"on\" })")});
        }
        return true;
    }
    if (act == QStringLiteral("screen_off")) {
        m_statusMessage = tr("Screen turned off");
        emit statusMessageChanged();
        if (!m_dryRunLaunch) {
            QProcess::startDetached(QStringLiteral("hyprctl"), {QStringLiteral("dispatch"), QStringLiteral("hl.dsp.dpms({ state = \"off\" })")});
        }
        return true;
    }
    if (act == QStringLiteral("system_lock")) {
        m_statusMessage = tr("Screen locked");
        emit statusMessageChanged();
        if (!m_dryRunLaunch) {
            QProcess::startDetached(QStringLiteral("omarchy-system-lock"), {});
        }
        return true;
    }
    if (act == QStringLiteral("workspace_focus")) {
        const int wsId = payload.trimmed().toInt();
        if (wsId > 0) {
            m_statusMessage = tr("Switched to Desktop %1").arg(wsId);
            emit statusMessageChanged();
            if (!m_dryRunLaunch) {
                QProcess::startDetached(QStringLiteral("hyprctl"), {
                    QStringLiteral("dispatch"),
                    QStringLiteral("o.workspaces.focus(%1)").arg(wsId)
                });
            }
            return true;
        }
        return false;
    }
    if (act == QStringLiteral("workspace_next")) {
        m_statusMessage = tr("Next Desktop");
        emit statusMessageChanged();
        if (!m_dryRunLaunch) {
            QProcess::startDetached(QStringLiteral("hyprctl"), {
                QStringLiteral("dispatch"),
                QStringLiteral("hl.dsp.focus({ workspace = \"m+1\" })")
            });
        }
        return true;
    }
    if (act == QStringLiteral("workspace_prev")) {
        m_statusMessage = tr("Previous Desktop");
        emit statusMessageChanged();
        if (!m_dryRunLaunch) {
            QProcess::startDetached(QStringLiteral("hyprctl"), {
                QStringLiteral("dispatch"),
                QStringLiteral("hl.dsp.focus({ workspace = \"m-1\" })")
            });
        }
        return true;
    }
    return false;
}

void Backend::handleRemoteConnection() {
    while (m_remoteServer.hasPendingConnections()) {
        QTcpSocket *socket = m_remoteServer.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            const QByteArray requestData = socket->readAll();
            const int lineEnd = requestData.indexOf("\r\n");
            const QByteArray firstLine = lineEnd > 0 ? requestData.left(lineEnd) : requestData;
            const QList<QByteArray> tokens = firstLine.split(' ');
            const QString target = tokens.size() >= 2 ? QString::fromUtf8(tokens.at(1)) : QStringLiteral("/");

            const QUrl url(QStringLiteral("http://localhost") + target);
            const QString path = url.path();

            if (path == QStringLiteral("/api/cmd")) {
                const QUrlQuery query(url);
                const QString action = query.queryItemValue(QStringLiteral("action"), QUrl::FullyDecoded);
                const QString payload = query.queryItemValue(QStringLiteral("payload"), QUrl::FullyDecoded);
                const bool ok = executeRemoteAction(action, payload);

                QJsonObject resp;
                resp[QStringLiteral("ok")] = ok;
                resp[QStringLiteral("selected")] = selectedApp().value(QStringLiteral("name")).toString();
                resp[QStringLiteral("category")] = m_category;
                resp[QStringLiteral("status")] = m_statusMessage;
                const QByteArray body = QJsonDocument(resp).toJson(QJsonDocument::Compact);

                QByteArray header = "HTTP/1.1 200 OK\r\n"
                                    "Content-Type: application/json; charset=utf-8\r\n"
                                    "Cache-Control: no-store\r\n"
                                    "Access-Control-Allow-Origin: *\r\n"
                                    "Content-Length: " + QByteArray::number(body.size()) + "\r\n"
                                    "Connection: close\r\n\r\n";
                socket->write(header + body);
                socket->disconnectFromHost();
                return;
            }

            if (path == QStringLiteral("/api/workspaces")) {
                const QJsonObject info = getWorkspacesInfo();
                const QByteArray body = QJsonDocument(info).toJson(QJsonDocument::Compact);

                QByteArray header = "HTTP/1.1 200 OK\r\n"
                                    "Content-Type: application/json; charset=utf-8\r\n"
                                    "Cache-Control: no-store\r\n"
                                    "Access-Control-Allow-Origin: *\r\n"
                                    "Content-Length: " + QByteArray::number(body.size()) + "\r\n"
                                    "Connection: close\r\n\r\n";
                socket->write(header + body);
                socket->disconnectFromHost();
                return;
            }

            const QByteArray html = buildRemoteHtmlPage();
            QByteArray header = "HTTP/1.1 200 OK\r\n"
                                "Content-Type: text/html; charset=utf-8\r\n"
                                "Cache-Control: no-cache\r\n"
                                "Content-Length: " + QByteArray::number(html.size()) + "\r\n"
                                "Connection: close\r\n\r\n";
            socket->write(header + html);
            socket->disconnectFromHost();
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    }
}

QJsonObject Backend::getWorkspacesInfo() const {
    QJsonObject result;
    if (m_dryRunLaunch) {
        result[QStringLiteral("active")] = 1;
        QJsonArray list;
        QJsonObject ws1;
        ws1[QStringLiteral("id")] = 1;
        ws1[QStringLiteral("name")] = QStringLiteral("1");
        ws1[QStringLiteral("windows")] = 1;
        ws1[QStringLiteral("apps")] = QStringLiteral("Plus TV");
        list.append(ws1);
        result[QStringLiteral("workspaces")] = list;
        return result;
    }

    int activeId = 1;
    QProcess pActive;
    pActive.start(QStringLiteral("hyprctl"), {QStringLiteral("activeworkspace"), QStringLiteral("-j")});
    if (pActive.waitForFinished(350)) {
        const QJsonDocument doc = QJsonDocument::fromJson(pActive.readAllStandardOutput());
        if (doc.isObject()) {
            activeId = doc.object().value(QStringLiteral("id")).toInt(1);
        }
    }
    result[QStringLiteral("active")] = activeId;

    QMap<int, QStringList> wsApps;
    QProcess pClients;
    pClients.start(QStringLiteral("hyprctl"), {QStringLiteral("clients"), QStringLiteral("-j")});
    if (pClients.waitForFinished(350)) {
        const QJsonDocument cDoc = QJsonDocument::fromJson(pClients.readAllStandardOutput());
        if (cDoc.isArray()) {
            const QJsonArray arr = cDoc.array();
            for (const QJsonValue &v : arr) {
                const QJsonObject obj = v.toObject();
                const int wid = obj.value(QStringLiteral("workspace")).toObject().value(QStringLiteral("id")).toInt();
                QString title = obj.value(QStringLiteral("title")).toString().trimmed();
                const QString cls = obj.value(QStringLiteral("class")).toString().trimmed();
                if (title.isEmpty())
                    title = cls;
                if (!title.isEmpty()) {
                    if (title.length() > 28)
                        title = title.left(26) + QStringLiteral("…");
                    if (!wsApps[wid].contains(title))
                        wsApps[wid].append(title);
                }
            }
        }
    }

    QJsonArray list;
    QProcess pWs;
    pWs.start(QStringLiteral("hyprctl"), {QStringLiteral("workspaces"), QStringLiteral("-j")});
    if (pWs.waitForFinished(350)) {
        const QJsonDocument wsDoc = QJsonDocument::fromJson(pWs.readAllStandardOutput());
        if (wsDoc.isArray()) {
            const QJsonArray arr = wsDoc.array();
            QList<QJsonObject> sorted;
            for (const QJsonValue &v : arr) {
                sorted.append(v.toObject());
            }
            std::sort(sorted.begin(), sorted.end(), [](const QJsonObject &a, const QJsonObject &b) {
                return a.value(QStringLiteral("id")).toInt() < b.value(QStringLiteral("id")).toInt();
            });

            for (const QJsonObject &obj : sorted) {
                const int id = obj.value(QStringLiteral("id")).toInt();
                if (id <= 0)
                    continue;
                QJsonObject item;
                item[QStringLiteral("id")] = id;
                item[QStringLiteral("name")] = obj.value(QStringLiteral("name")).toString();
                item[QStringLiteral("windows")] = obj.value(QStringLiteral("windows")).toInt();
                QString appStr = wsApps.value(id).join(QStringLiteral(", "));
                if (id == 1 && (appStr.isEmpty() || !appStr.contains(QStringLiteral("Plus TV")))) {
                    appStr = appStr.isEmpty() ? QStringLiteral("Plus TV") : (QStringLiteral("Plus TV, ") + appStr);
                }
                item[QStringLiteral("apps")] = appStr;
                list.append(item);
            }
        }
    }

    if (list.isEmpty()) {
        QJsonObject def;
        def[QStringLiteral("id")] = 1;
        def[QStringLiteral("name")] = QStringLiteral("1");
        def[QStringLiteral("windows")] = 1;
        def[QStringLiteral("apps")] = QStringLiteral("Plus TV");
        list.append(def);
    }

    result[QStringLiteral("workspaces")] = list;
    return result;
}

QByteArray Backend::buildRemoteHtmlPage() const {
    static const char kHtml[] = R"HTML(<!DOCTYPE html>
<html lang="tr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>Plus TV • Akıllı Kumanda</title>
<style>
  :root {
    --bg: #080a10;
    --card: #131724;
    --border: #252b40;
    --accent: #e30613;
    --purple: #9d4edd;
    --text: #f8fafc;
    --muted: #94a3b8;
    --blue: #38bdf8;
    --green: #22c55e;
    --yellow: #eab308;
    --red: #ef4444;
  }
  * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; user-select: none; }
  body {
    margin: 0; padding: 10px;
    background: radial-gradient(circle at top, #171b2e 0%, var(--bg) 70%);
    color: var(--text);
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
    min-height: 100vh;
    display: flex; flex-direction: column; gap: 8px;
    max-width: 500px; margin-left: auto; margin-right: auto;
  }
  .header {
    display: flex; align-items: center; justify-content: space-between;
    background: var(--card); border: 1px solid var(--border);
    padding: 8px 12px; border-radius: 14px;
  }
  .brand {
    display: flex; align-items: center; gap: 8px;
    font-weight: 900; font-size: 14px; letter-spacing: 0.5px;
  }
  .brand-badge {
    background: linear-gradient(135deg, var(--accent), var(--purple));
    width: 26px; height: 26px; border-radius: 7px;
    display: inline-flex; align-items: center; justify-content: center;
    font-weight: 900;
  }
  .header-actions { display: flex; align-items: center; gap: 10px; }
  .status { font-size: 11px; color: var(--blue); font-weight: 600; text-align: right; max-width: 140px; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
  .btn-power-hdr {
    background: linear-gradient(135deg, #e30613, #b91c1c); color: #fff; border: 1px solid #f87171;
    border-radius: 10px; width: 34px; height: 34px; min-height: 34px;
    font-size: 17px; font-weight: 900; display: inline-flex;
    align-items: center; justify-content: center; cursor: pointer;
    box-shadow: 0 0 12px rgba(227, 6, 19, 0.45); padding: 0;
  }
  .btn-power-hdr:active { transform: scale(0.92); }

  .top-bar {
    display: grid; grid-template-columns: repeat(4, 1fr); gap: 6px;
  }
  .tabs {
    display: grid; grid-template-columns: repeat(4, 1fr); gap: 6px;
  }
  button {
    background: var(--card); color: var(--text);
    border: 1px solid var(--border); border-radius: 12px;
    padding: 10px 4px; font-size: 11px; font-weight: 700;
    cursor: pointer; transition: transform 0.08s, background 0.12s;
    display: flex; flex-direction: column; align-items: center; justify-content: center; gap: 3px;
  }
  button:active { transform: scale(0.95); background: #222940; }
  .btn-home { border-color: var(--blue); color: var(--blue); }
  .btn-danger { border-color: #f43f5e; color: #fda4af; }
  .tab-btn { position: relative; font-size: 11px; padding: 10px 4px; }
  .tab-btn.active {
    background: linear-gradient(135deg, var(--accent), var(--purple));
    border-color: transparent; color: #fff;
  }
  .tab-badge {
    position: absolute; top: 3px; right: 3px;
    background: var(--blue); color: #000; font-size: 8px; font-weight: 900;
    padding: 1px 4px; border-radius: 8px; line-height: 1.1;
  }
  .panel { display: none; flex-direction: column; gap: 10px; }
  .panel.active { display: flex; }

  /* D-Pad in Remote Tab */
  .dpad-wrap {
    background: var(--card); border: 1px solid var(--border);
    border-radius: 22px; padding: 14px;
    display: grid; grid-template-columns: repeat(3, 1fr); gap: 8px;
    aspect-ratio: 1 / 0.88; max-width: 310px; width: 100%; margin: 0 auto;
  }
  .dpad-btn { font-size: 22px; border-radius: 16px; background: #1a2033; }
  .dpad-ok {
    background: linear-gradient(135deg, var(--accent), var(--purple));
    font-size: 18px; font-weight: 900; border-radius: 50%; border: 2px solid #fff;
  }
  .dpad-sub { font-size: 10px; color: var(--muted); font-weight: 600; }
  .row-4 { display: grid; grid-template-columns: repeat(4, 1fr); gap: 6px; }
  .row-3 { display: grid; grid-template-columns: repeat(3, 1fr); gap: 6px; }
  .row-2 { display: grid; grid-template-columns: repeat(2, 1fr); gap: 6px; }
  .section-lbl { font-size: 11px; color: var(--muted); text-transform: uppercase; letter-spacing: 0.8px; font-weight: 700; margin-top: 2px; }

  /* Quick Desktop Strip */
  .ws-strip {
    background: #0f1422; border: 1px solid #1e283d;
    border-radius: 14px; padding: 6px;
  }
  .btn-strip-ws {
    background: #192238; border-color: #38bdf8;
  }

  /* Desktops Tab */
  .ws-banner {
    background: linear-gradient(135deg, #101c36, #161a29);
    border: 1px solid #233554; border-radius: 14px;
    padding: 12px 14px; display: flex; flex-direction: column; gap: 4px;
  }
  .ws-list { display: flex; flex-direction: column; gap: 8px; }
  .ws-card {
    background: var(--card); border: 2px solid var(--border);
    border-radius: 14px; padding: 12px 14px; display: flex;
    flex-direction: column; gap: 6px; cursor: pointer; text-align: left;
    transition: all 0.12s ease;
  }
  .ws-card:active { transform: scale(0.98); }
  .ws-card.active {
    border-color: #38bdf8; background: #101d36;
    box-shadow: 0 0 14px rgba(56, 189, 248, 0.25);
  }
  .ws-head {
    display: flex; align-items: center; justify-content: space-between;
  }
  .ws-title { font-size: 14px; font-weight: 800; color: #fff; }
  .ws-pill-active {
    font-size: 10px; font-weight: 900; letter-spacing: 0.5px;
    padding: 3px 8px; border-radius: 6px; background: #0284c7; color: #fff;
  }
  .ws-pill-switch {
    font-size: 11px; font-weight: 700; color: var(--blue);
  }
  .ws-desc { font-size: 12px; color: var(--muted); line-height: 1.3; }

  /* Channels Grid */
  .ch-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 8px; }
  .ch-btn {
    padding: 12px 8px; border-radius: 14px; text-align: center;
    font-size: 13px; font-weight: 800; border-width: 2px;
  }

  /* ==============================================================
     SADE KONTROLCÜLER MENÜSÜ (MINIMAL CONTROLLERS HUB)
     ============================================================== */
  .ctrl-simple-grid {
    display: flex; flex-direction: column; gap: 10px; margin-top: 2px;
  }
  .ctrl-simple-card {
    background: var(--card); border: 1.5px solid var(--border);
    border-radius: 16px; padding: 16px 14px; display: flex;
    flex-direction: row; align-items: center; justify-content: space-between;
    gap: 12px; cursor: pointer; transition: all 0.12s ease; text-align: left;
  }
  .ctrl-simple-card:active { transform: scale(0.97); }
  .ctrl-card-icon {
    font-size: 26px; width: 46px; height: 46px; border-radius: 12px;
    display: flex; align-items: center; justify-content: center;
    background: #171f34; border: 1px solid #283756; flex-shrink: 0;
  }
  .ctrl-card-info {
    flex: 1; display: flex; flex-direction: column; gap: 3px;
  }
  .ctrl-card-title {
    font-size: 15px; font-weight: 900; color: #fff;
  }
  .ctrl-card-sub {
    font-size: 11px; color: var(--muted); font-weight: 600;
  }
  .ctrl-card-arrow {
    font-size: 18px; color: var(--blue); font-weight: 900; padding-right: 4px;
  }
  .ctrl-card-gp:active { border-color: #38bdf8; box-shadow: 0 0 16px rgba(56, 189, 248, 0.3); }
  .ctrl-card-kbd:active { border-color: #a855f7; box-shadow: 0 0 16px rgba(168, 85, 247, 0.3); }
  .ctrl-card-mouse:active { border-color: #22c55e; box-shadow: 0 0 16px rgba(34, 197, 94, 0.3); }

  /* ==============================================================
     DEDICATED FULLSCREEN SCREENS (DOĞRUDAN YATAY EKRAN DÜZENİ)
     ============================================================== */
  .screen-overlay {
    display: none; background: radial-gradient(circle at top, #141b33 0%, #060812 85%);
    z-index: 3000; flex-direction: column; padding: clamp(2px, 0.8vh, 6px); gap: 3px;
    box-sizing: border-box; overflow: hidden;
  }
  .screen-overlay.open { display: flex; }

  /* Otomatik Yatay: Dikey telefonda doğrudan 90 derece yatay gösterir */
  @media (orientation: portrait) {
    .auto-landscape.open {
      position: fixed !important;
      top: 0 !important;
      left: 100% !important;
      left: 100vw !important;
      left: 100dvw !important;
      width: 100vh !important;
      width: 100dvh !important;
      width: var(--app-h, 100dvh) !important;
      height: 100vw !important;
      height: 100dvw !important;
      height: var(--app-w, 100dvw) !important;
      transform: rotate(90deg);
      transform-origin: top left;
    }
  }
  /* Yatay telefonda tam ekran yayılır */
  @media (orientation: landscape) {
    .auto-landscape.open {
      position: fixed !important;
      inset: 0 !important;
      width: 100% !important;
      width: 100vw !important;
      width: 100dvw !important;
      width: var(--app-w, 100dvw) !important;
      height: 100% !important;
      height: 100vh !important;
      height: 100dvh !important;
      height: var(--app-h, 100dvh) !important;
      transform: none !important;
    }
  }

  /* Fare dikeyde tam ekran açılır */
  #overlay-mouse.open {
    position: fixed; inset: 0; width: 100%; width: 100vw; width: 100dvw; height: 100%; height: 100vh; height: 100dvh;
    padding: 10px; gap: 8px;
  }

  .screen-topbar {
    display: flex; align-items: center; justify-content: space-between;
    background: rgba(16, 21, 36, 0.95); border: 1px solid #27344f;
    border-radius: 8px; padding: 2px 6px; gap: 6px; flex-shrink: 0;
    height: clamp(24px, 5.5vh, 30px);
  }
  .btn-screen-back {
    background: #1e263d; border: 1px solid #38bdf8; color: #38bdf8;
    padding: 2px 8px; font-size: clamp(9px, 2.2vh, 11px); font-weight: 800; border-radius: 6px;
    display: flex; flex-direction: row; align-items: center; justify-content: center; gap: 4px; height: 80%;
  }
  .btn-screen-back:active { transform: scale(0.93); background: #38bdf8; color: #000; }

  /* HUD GERİ TEPKİ GÖSTERGESİ (CANLI TUŞ BİLGİSİ) */
  .hud-feedback {
    background: #0d1222; border: 1.5px solid #2b3a5b; border-radius: 6px;
    padding: 2px 8px; font-size: clamp(9px, 2.2vh, 11px); font-weight: 800; color: #38bdf8;
    letter-spacing: 0.5px; transition: all 0.08s ease; min-width: 100px;
    text-align: center; white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
    height: 80%; display: flex; align-items: center; justify-content: center;
  }

  /* ==============================================================
     GAMEPAD LANDSCAPE LAYOUT
     ============================================================== */
  .gamepad-body {
    display: grid; grid-template-columns: 1.15fr 0.85fr 1.15fr;
    gap: clamp(4px, 1.2vw, 8px); flex: 1; min-height: 0; align-items: stretch; overflow: hidden;
    padding: 1px 0;
  }
  .gamepad-wing {
    display: flex; flex-direction: column; justify-content: space-between;
    background: rgba(12, 16, 28, 0.7); border: 1px solid #1f2a44;
    border-radius: 12px; padding: clamp(2px, 0.8vh, 6px); min-height: 0; overflow: hidden;
  }
  .gamepad-center {
    display: flex; flex-direction: column; justify-content: space-between;
    align-items: center; padding: clamp(2px, 0.8vh, 4px); min-width: 0; min-height: 0;
    gap: clamp(2px, 0.8vh, 6px);
  }
  .gp-shoulder-row {
    display: flex; gap: 4px; width: 100%;
    height: clamp(24px, 6.5vh, 32px); flex-shrink: 0;
  }
  .btn-trigger {
    flex: 1.25; height: 100%; padding: 0 4px; border-radius: 8px;
    background: #141f38; border: 1.5px solid #38bdf8; color: #fff;
    display: flex; flex-direction: row; align-items: center; justify-content: center; gap: 4px;
  }
  .btn-trigger-primary { border-color: #ef4444; background: #261217; }
  .btn-bumper {
    flex: 1; height: 100%; padding: 0 4px; border-radius: 8px;
    background: #171d30; border: 1px solid #334155;
    display: flex; flex-direction: row; align-items: center; justify-content: center; gap: 4px;
  }
  .btn-tag { font-size: clamp(10px, 2.4vh, 12px); font-weight: 900; }
  .btn-sub { font-size: clamp(7px, 1.6vh, 9px); color: #94a3b8; font-weight: 600; }

  /* D-Pad Container */
  .gp-dpad-center-wrap {
    display: flex; align-items: center; justify-content: center; flex: 1; min-height: 0; padding: 1px 0;
  }
  .dpad-grid-large {
    display: grid;
    --dpad-k: clamp(26px, 8.2vh, 42px);
    grid-template-columns: repeat(3, var(--dpad-k));
    grid-template-rows: repeat(3, var(--dpad-k));
    gap: clamp(2px, 0.5vh, 4px);
    align-items: center; justify-items: center;
  }
  .g-pad-btn {
    width: 100%; height: 100%; border-radius: clamp(7px, 1.8vh, 11px);
    background: #162035; border: 1.5px solid #2b3a5b;
    font-size: clamp(12px, 3.2vh, 17px); font-weight: 900; color: #38bdf8; padding: 0;
    display: flex; align-items: center; justify-content: center;
    transition: transform 0.05s, background 0.08s, box-shadow 0.08s;
  }
  .dpad-center {
    width: 100%; height: 100%; display: flex; align-items: center; justify-content: center;
    font-size: clamp(11px, 2.8vh, 15px);
  }

  /* ABXY Diamond Container */
  .gp-abxy-center-wrap {
    display: flex; align-items: center; justify-content: center; flex: 1; min-height: 0; padding: 1px 0;
  }
  .abxy-diamond-large {
    display: flex; flex-direction: column; align-items: center; gap: clamp(2px, 0.5vh, 4px);
  }
  .abxy-mid-row { display: flex; gap: clamp(12px, 3.2vw, 22px); }
  .abxy-btn {
    --abxy-k: clamp(26px, 8.2vh, 42px);
    width: var(--abxy-k); height: var(--abxy-k); border-radius: 50%; font-weight: 900;
    border: 2px solid #fff; display: flex; flex-direction: column;
    align-items: center; justify-content: center; padding: 0;
    transition: transform 0.05s, filter 0.08s, box-shadow 0.08s;
  }
  .abxy-letter { font-size: clamp(11px, 2.8vh, 15px); line-height: 1; }
  .abxy-sub { font-size: clamp(6px, 1.4vh, 8px); opacity: 0.9; line-height: 1; margin-top: 1px; pointer-events: none; }
  .btn-a { background: #15803d; border-color: #22c55e; color: #fff; box-shadow: 0 0 10px rgba(34, 197, 94, 0.45); }
  .btn-b { background: #b91c1c; border-color: #ef4444; color: #fff; box-shadow: 0 0 10px rgba(239, 68, 68, 0.45); }
  .btn-x { background: #1d4ed8; border-color: #3b82f6; color: #fff; box-shadow: 0 0 10px rgba(59, 130, 246, 0.45); }
  .btn-y { background: #a16207; border-color: #eab308; color: #fff; box-shadow: 0 0 10px rgba(234, 179, 8, 0.45); }

  /* TUŞ GERİ TEPKİSİ / PARLAMA (ACTIVE FEEDBACK EFFECTS) */
  .active-press {
    transform: scale(0.88) !important;
    filter: brightness(1.7) !important;
    box-shadow: 0 0 24px #38bdf8 !important;
  }
  .btn-a.active-press { box-shadow: 0 0 28px #22c55e, inset 0 0 12px #fff !important; }
  .btn-b.active-press { box-shadow: 0 0 28px #ef4444, inset 0 0 12px #fff !important; }
  .btn-x.active-press { box-shadow: 0 0 28px #38bdf8, inset 0 0 12px #fff !important; }
  .btn-y.active-press { box-shadow: 0 0 28px #facc15, inset 0 0 12px #fff !important; }
  .g-pad-btn.active-press { background: #0284c7 !important; color: #fff !important; box-shadow: 0 0 24px #38bdf8 !important; }

  /* Slots Row */
  .gp-slots-row {
    display: flex; gap: 4px; width: 100%;
    height: clamp(20px, 5.5vh, 26px); flex-shrink: 0;
  }
  .game-slot {
    flex: 1; height: 100%; padding: 0; font-size: clamp(9px, 2.2vh, 11px); font-weight: 900;
    background: #111624; border: 1px solid #222d47; border-radius: 6px;
    display: flex; align-items: center; justify-content: center; gap: 3px;
  }
  .slot-sub { font-size: clamp(6.5px, 1.5vh, 8.5px); font-weight: 600; opacity: 0.75; }

  /* Center Column Elements */
  .gp-sys-row {
    display: flex; gap: 3px; width: 100%;
    height: clamp(22px, 5.5vh, 26px); flex-shrink: 0;
  }
  .btn-sys {
    flex: 1; height: 100%; padding: 0; border-radius: 6px;
    background: #13182a; border: 1px solid #27334d; font-size: clamp(8px, 1.9vh, 9.5px);
    display: flex; flex-direction: row; align-items: center; justify-content: center; gap: 3px;
  }
  .sys-sub { font-size: clamp(6.5px, 1.4vh, 8px); color: #94a3b8; }
  .btn-sys-home {
    width: clamp(28px, 7vh, 38px); height: clamp(28px, 7vh, 38px); border-radius: 50%;
    background: linear-gradient(135deg, var(--accent), var(--purple));
    border: 2px solid #fff; font-size: clamp(12px, 2.8vh, 16px); padding: 0; flex-shrink: 0;
    display: flex; align-items: center; justify-content: center;
    box-shadow: 0 0 10px rgba(227, 6, 19, 0.4);
  }
  .gp-actions-wrap {
    display: flex; flex-direction: column; gap: clamp(2px, 0.5vh, 3px);
    width: 100%; flex: 1; min-height: 0; justify-content: center;
  }
  .game-action-pill {
    width: 100%; flex: 1; min-height: 0; max-height: clamp(20px, 5.5vh, 28px);
    padding: 0; font-size: clamp(8px, 1.9vh, 9.5px); font-weight: 800;
    background: #141b2f; border: 1px solid #2a375a; border-radius: 6px;
    display: flex; align-items: center; justify-content: center;
  }

  /* ==============================================================
     KEYBOARD LANDSCAPE LAYOUT
     ============================================================== */
  .kbd-screen-body {
    display: flex; flex-direction: column; gap: 2px; flex: 1; min-height: 0;
    justify-content: stretch;
  }
  .win-shortcuts-bar {
    display: flex; flex-wrap: nowrap; gap: 3px; overflow-x: auto; flex-shrink: 0;
    height: clamp(19px, 4.8vh, 24px); align-items: center;
    scrollbar-width: none;
  }
  .win-shortcuts-bar::-webkit-scrollbar { display: none; }
  .win-chip {
    padding: 0 6px; font-size: clamp(8px, 1.9vh, 9.5px); border-radius: 4px;
    background: #151a2e; border: 1px solid #28334e; flex-shrink: 0;
    text-align: center; font-weight: 700; height: 100%;
    display: flex; align-items: center; justify-content: center;
  }
  .win-btn-logo { color: #38bdf8; border-color: #38bdf8; background: #0f2038; }
  .fkeys-bar {
    display: flex; gap: 2px; width: 100%;
    height: clamp(17px, 4.4vh, 22px); flex-shrink: 0;
  }
  .fkey {
    flex: 1; padding: 0; font-size: clamp(7.5px, 1.8vh, 9.5px); min-width: 0;
    background: #161c30; border: 1px solid #27344f; border-radius: 4px; font-weight: 800;
    height: 100%; display: flex; align-items: center; justify-content: center;
  }
  .kbd-line {
    display: flex; gap: 2px; width: 100%; flex: 1; min-height: 0;
  }
  .k-key {
    flex: 1; min-width: 0; height: 100%; padding: 0;
    font-size: clamp(9.5px, 2.7vh, 13px); font-weight: 700;
    border-radius: 4px; background: #171d32; border: 1px solid #283554;
    color: #f8fafc; display: flex; flex-direction: column; align-items: center; justify-content: center;
    line-height: 1; transition: transform 0.05s, background 0.08s, box-shadow 0.08s;
  }
  .k-key.active-press {
    background: #38bdf8 !important; color: #000 !important;
    box-shadow: 0 0 18px rgba(56, 189, 248, 0.9) !important;
    transform: scale(0.88) !important;
  }
  .k-sub {
    font-size: clamp(6px, 1.5vh, 8px); color: #64748b; line-height: 1; margin-bottom: 1px; pointer-events: none;
  }
  .k-w15 { flex: 1.4; }
  .k-w2 { flex: 1.8; }
  .k-space { flex: 5.2; background: #1e2640; font-size: clamp(9px, 2.2vh, 11px); }
  .k-enter {
    flex: 1.8; background: linear-gradient(135deg, #0284c7, #0369a1);
    border-color: #38bdf8; color: #fff; font-size: clamp(9px, 2.2vh, 11px);
  }
  .k-spec { background: #14192b; color: #94a3b8; font-size: clamp(8px, 1.9vh, 9.5px); }
  .k-spec.active { background: #0284c7; color: #fff; border-color: #38bdf8; }
  .k-arrow { background: #1a2138; color: #38bdf8; font-size: clamp(9.5px, 2.4vh, 12px); flex: 1.1; }

  /* ==============================================================
     MOUSE / TOUCHPAD SCREEN
     ============================================================== */
  .mouse-screen-body {
    display: flex; flex-direction: column; gap: 8px; flex: 1;
  }
  .touchpad-large {
    background: #101422; border: 2px dashed #2b3852;
    border-radius: 16px; flex: 1; min-height: 200px;
    display: flex; flex-direction: column; align-items: center; justify-content: center;
    color: var(--muted); font-size: 13px; text-align: center; padding: 14px;
    touch-action: none;
  }
  .kbd-row { display: flex; gap: 6px; }
  .kbd-input {
    flex: 1; background: var(--card); border: 1px solid var(--border);
    border-radius: 12px; padding: 10px 12px; color: #fff; font-size: 14px;
  }

  /* Power Modal */
  .modal-overlay {
    display: none; position: fixed; inset: 0;
    background: rgba(0, 0, 0, 0.78); backdrop-filter: blur(8px);
    z-index: 4000; align-items: flex-end; justify-content: center;
  }
  .modal-overlay.open { display: flex; }
  .modal-card {
    background: #0f1320; border: 1px solid #2a344d;
    border-radius: 24px 24px 0 0; width: 100%; max-width: 480px;
    padding: 20px 18px 30px 18px; display: flex; flex-direction: column; gap: 12px;
    box-shadow: 0 -12px 40px rgba(0,0,0,0.85);
    animation: slideUp 0.18s cubic-bezier(0.16, 1, 0.3, 1);
  }
  @keyframes slideUp { from { transform: translateY(100%); } to { transform: translateY(0); } }
  .modal-header { display: flex; align-items: center; justify-content: space-between; }
  .modal-title { font-weight: 900; font-size: 17px; }
  .modal-close {
    background: #1b2135; border: none; color: #94a3b8;
    border-radius: 50%; width: 30px; height: 30px; font-size: 14px;
    padding: 0; display: inline-flex; align-items: center; justify-content: center;
  }
  .pwr-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 10px; }
  .pwr-btn {
    padding: 14px 12px; border-radius: 14px; border: 1px solid var(--border);
    background: #161b2b; text-align: left; display: flex; flex-direction: column; gap: 3px;
    align-items: flex-start;
  }
  .pwr-icon { font-size: 22px; margin-bottom: 2px; }
  .pwr-text { font-size: 13px; font-weight: 800; color: #fff; }
  .pwr-sub { font-size: 10px; color: var(--muted); font-weight: 500; }
  .pwr-off { border-color: #ef4444; background: #261114; }
  .pwr-restart { border-color: #f59e0b; background: #231c12; }
  .pwr-sleep { border-color: #a855f7; background: #1f162c; }
  .pwr-screen { border-color: #38bdf8; background: #102133; }
  .confirm-box {
    background: #2b1115; border: 1px solid #f43f5e; border-radius: 14px;
    padding: 14px; display: flex; flex-direction: column; gap: 10px; margin-top: 4px;
  }
  .confirm-msg { font-size: 13px; font-weight: 700; color: #fda4af; text-align: center; }
</style>
</head>
<body>
  <div class="header">
    <div class="brand"><span class="brand-badge">+</span> PLUS TV KUMANDA</div>
    <div class="header-actions">
      <div class="status" id="st">Bağlı • Hazır</div>
      <button class="btn-power-hdr" onclick="openPowerModal()" title="Güç Yönetimi">⏻</button>
    </div>
  </div>

  <div class="top-bar">
    <button class="btn-home" onclick="cmd('home')">🏠<span>Plus TV</span></button>
    <button onclick="showTab('ctrl', document.querySelectorAll('.tab-btn')[3])">🎮<span>Kontroller</span></button>
    <button onclick="showTab('ws', document.querySelectorAll('.tab-btn')[2])">🖥️<span>Masaüstü</span></button>
    <button class="btn-danger" onclick="cmd('close_app')">❌<span>Kapat</span></button>
  </div>

  <div class="tabs">
    <button class="tab-btn active" onclick="showTab('pad', this)">🎛️ Kumanda</button>
    <button class="tab-btn" onclick="showTab('ch', this)">📺 Kanallar</button>
    <button class="tab-btn" onclick="showTab('ws', this)">🖥️ Masaüstü <span id="ws-badge" class="tab-badge">1</span></button>
    <button class="tab-btn" onclick="showTab('ctrl', this)">🎮 Kontrolcüler</button>
  </div>

  <!-- TAB 1: D-PAD & MEDIA -->
  <div id="tab-pad" class="panel active">
    <div class="dpad-wrap">
      <button onclick="cmd('back')">🔙<span class="dpad-sub">Geri</span></button>
      <button class="dpad-btn" onclick="cmd('nav_up')">▲</button>
      <button onclick="cmd('category_next')">🗂️<span class="dpad-sub">Kategori</span></button>

      <button class="dpad-btn" onclick="cmd('nav_left')">◀</button>
      <button class="dpad-ok" onclick="cmd('nav_ok')">OK</button>
      <button class="dpad-btn" onclick="cmd('nav_right')">▶</button>

      <button onclick="cmd('category_prev')">⏮<span class="dpad-sub">Önceki Sekme</span></button>
      <button class="dpad-btn" onclick="cmd('nav_down')">▼</button>
      <button onclick="cmd('home')">🏠<span class="dpad-sub">Ana Menü</span></button>
    </div>

    <!-- Quick Desktop Switcher -->
    <div class="section-lbl">Masaüstü Geçişi</div>
    <div class="row-3 ws-strip">
      <button onclick="cmd('workspace_prev')">◀<span>Önceki</span></button>
      <button class="btn-strip-ws" onclick="showTab('ws', document.querySelectorAll('.tab-btn')[2])">
        <span id="quick-ws-lbl" style="font-size:13px;font-weight:900;color:var(--blue)">Masaüstü 1</span>
        <span style="font-size:10px;color:var(--muted)">Tüm Masaüstleri</span>
      </button>
      <button onclick="cmd('workspace_next')">▶<span>Sonraki</span></button>
    </div>

    <div class="section-lbl">Ses Kontrolü</div>
    <div class="row-3">
      <button onclick="cmd('vol_down')">🔉<span>Ses -</span></button>
      <button onclick="cmd('vol_mute')">🔇<span>Sessiz</span></button>
      <button onclick="cmd('vol_up')">🔊<span>Ses +</span></button>
    </div>

    <div class="section-lbl">Medya Oynatma</div>
    <div class="row-3">
      <button onclick="cmd('seek_back')">⏪<span>-10 sn</span></button>
      <button onclick="cmd('play_pause')">⏯️<span>Oynat / Durdur</span></button>
      <button onclick="cmd('seek_fwd')">⏩<span>+10 sn</span></button>
    </div>
  </div>

  <!-- TAB 2: DIRECT CHANNELS -->
  <div id="tab-ch" class="panel">
    <div class="ch-grid">
      <button class="ch-btn" style="border-color:#9D4EDD" onclick="cmd('launch','bein-connect')">beIN CONNECT</button>
      <button class="ch-btn" style="border-color:#E30613" onclick="cmd('launch','digiturk')">Digiturk</button>
      <button class="ch-btn" style="border-color:#FFB703" onclick="cmd('launch','tvplus')">Turkcell TV+</button>
      <button class="ch-btn" style="border-color:#38BDF8" onclick="cmd('launch','apple-tv')">Apple TV+</button>
      <button class="ch-btn" style="border-color:#00A8E1" onclick="cmd('launch','prime-video')">Prime Video</button>
      <button class="ch-btn" style="border-color:#E50914" onclick="cmd('launch','netflix')">Netflix</button>
      <button class="ch-btn" style="border-color:#FF0033" onclick="cmd('launch','youtube')">YouTube</button>
      <button class="ch-btn" style="border-color:#4285F4" onclick="cmd('launch','chrome')">Google Chrome</button>
      <button class="ch-btn" style="border-color:#FA233B" onclick="cmd('launch','apple-music')">Apple Music</button>
      <button class="ch-btn" style="border-color:#1DB954" onclick="cmd('launch','spotify')">Spotify</button>
      <button class="ch-btn" style="border-color:#FF0033" onclick="cmd('launch','youtube-music')">YouTube Music</button>
      <button class="ch-btn" style="border-color:#76B900" onclick="cmd('launch','geforce-now')">GeForce NOW</button>
    </div>
  </div>

  <!-- TAB 3: WORKSPACES (DESKTOPS) -->
  <div id="tab-ws" class="panel">
    <div class="ws-banner">
      <div style="font-weight:900;font-size:15px;color:#fff">🖥️ Açık Masaüstleri (Workspaces)</div>
      <div style="font-size:12px;color:var(--muted)">Açık olan masaüstleri arasında anında geçiş yapın:</div>
    </div>

    <div class="row-2">
      <button style="padding:12px;font-size:13px" onclick="cmd('workspace_prev')">◀ Önceki Masaüstü</button>
      <button style="padding:12px;font-size:13px" onclick="cmd('workspace_next')">Sonraki Masaüstü ▶</button>
    </div>

    <div class="section-lbl">Aktif Masaüstleri Listesi</div>
    <div class="ws-list" id="ws-container">
      <!-- Injected dynamically via JS -->
    </div>

    <button class="btn-home" style="margin-top:6px;padding:12px" onclick="cmd('workspace_focus', '1'); cmd('home');">
      🏠 Masaüstü 1 • Plus TV Ana Ekranına Odaklan
    </button>
  </div>

  <!-- TAB 4: SADE KONTROLCÜLER MENÜSÜ -->
  <div id="tab-ctrl" class="panel">
    <div class="ctrl-simple-grid">
      <div class="ctrl-simple-card ctrl-card-gp" onclick="openOverlay('gamepad')">
        <div class="ctrl-card-icon" style="color:#38bdf8">🎮</div>
        <div class="ctrl-card-info">
          <div class="ctrl-card-title">Oyun Kumandası</div>
          <div class="ctrl-card-sub">Yatay Tam Ekran Oyun Kolu</div>
        </div>
        <div class="ctrl-card-arrow">➔</div>
      </div>

      <div class="ctrl-simple-card ctrl-card-kbd" onclick="openOverlay('keyboard')">
        <div class="ctrl-card-icon" style="color:#a855f7">⌨️</div>
        <div class="ctrl-card-info">
          <div class="ctrl-card-title">Windows Klavyesi</div>
          <div class="ctrl-card-sub">Yatay Geniş Ekran PC Klavyesi</div>
        </div>
        <div class="ctrl-card-arrow">➔</div>
      </div>

      <div class="ctrl-simple-card ctrl-card-mouse" onclick="openOverlay('mouse')">
        <div class="ctrl-card-icon" style="color:#22c55e">🖱️</div>
        <div class="ctrl-card-info">
          <div class="ctrl-card-title">Fare & Touchpad</div>
          <div class="ctrl-card-sub">Air Mouse & TV Metin Arama</div>
        </div>
        <div class="ctrl-card-arrow">➔</div>
      </div>
    </div>
  </div>

  <!-- ==============================================================
       SCREEN OVERLAY 1: DEDICATED AUTO-LANDSCAPE GAMEPAD
       ============================================================== -->
  <div id="overlay-gamepad" class="screen-overlay auto-landscape">
    <div class="screen-topbar">
      <button class="btn-screen-back" onclick="closeOverlay('gamepad')">
        ◀ <span>Kumandaya Dön</span>
      </button>

      <!-- Canlı Geri Tepki Bildirimi -->
      <div class="hud-feedback" id="gp-hud">🎮 Oyun Kolu • Hazır</div>

      <button id="btn-gp-mode-toggle" class="btn-screen-back" style="padding:5px 8px" onclick="toggleGameMode()">
        <span id="gp-mode-lbl">Mod: WASD</span>
      </button>
    </div>

    <div class="gamepad-body">
      <!-- Left Wing: Shoulders, D-Pad, Slots -->
      <div class="gamepad-wing">
        <div class="gp-shoulder-row">
          <button class="btn-trigger" data-hold="game_action:trigger_l" data-label="LT: Nişan (Sağ Tık)" data-color="#38bdf8">
            <span class="btn-tag">LT</span>
            <span class="btn-sub">Nişan (Sağ Tık)</span>
          </button>
          <button class="btn-bumper" data-hold="game_action:bumper_l" data-label="LB: Sol El (Q)" data-color="#38bdf8">
            <span class="btn-tag">LB</span>
            <span class="btn-sub">Sol El (Q)</span>
          </button>
        </div>

        <div class="gp-dpad-center-wrap">
          <div class="dpad-grid-large">
            <div></div>
            <button class="g-pad-btn" data-hold="game_dir:up" data-label="▲ Yukarı (W)" data-color="#38bdf8">▲</button>
            <div></div>

            <button class="g-pad-btn" data-hold="game_dir:left" data-label="◀ Sol (A)" data-color="#38bdf8">◀</button>
            <div class="dpad-center">🕹️</div>
            <button class="g-pad-btn" data-hold="game_dir:right" data-label="▶ Sağ (D)" data-color="#38bdf8">▶</button>

            <div></div>
            <button class="g-pad-btn" data-hold="game_dir:down" data-label="▼ Aşağı (S)" data-color="#38bdf8">▼</button>
            <div></div>
          </div>
        </div>

        <div class="gp-slots-row">
          <button class="game-slot" onclick="triggerKeyFeedback(this, '1', 'Silah 1', '#38bdf8')">1 <span class="slot-sub">Silah 1</span></button>
          <button class="game-slot" onclick="triggerKeyFeedback(this, '2', 'Silah 2', '#38bdf8')">2 <span class="slot-sub">Silah 2</span></button>
        </div>
      </div>

      <!-- Center Column: System & Action Pills -->
      <div class="gamepad-center">
        <div class="gp-sys-row">
          <button class="btn-sys" onclick="triggerActionFeedback(this, 'select', 'BACK (Tab / Skor)', '#facc15')">
            <span>◂ BACK</span>
            <span class="sys-sub">Tab</span>
          </button>
          <button class="btn-sys" onclick="triggerActionFeedback(this, 'start', 'START (Esc / Menü)', '#facc15')">
            <span>START ▸</span>
            <span class="sys-sub">Esc</span>
          </button>
        </div>

        <button class="btn-sys-home" onclick="cmd('home'); showFeedback('🏠 Plus TV Ana Menü', '#ff0055', 'gp-hud'); flashKey(this);" title="Plus TV Ana Menü">
          <span>🏠</span>
        </button>

        <div class="gp-actions-wrap">
          <button class="game-action-pill" data-hold="game_action:jump" data-label="␣ Zıpla (Space)" data-color="#22c55e">␣ Zıpla (Space)</button>
          <button class="game-action-pill" data-hold="game_action:sprint" data-label="⚡ Koş (Shift)" data-color="#ef4444">⚡ Koş (Shift)</button>
          <button class="game-action-pill" data-hold="game_action:crouch" data-label="🛡️ Eğil (Ctrl)" data-color="#38bdf8">🛡️ Eğil (Ctrl)</button>
        </div>
      </div>

      <!-- Right Wing: Shoulders, ABXY Diamond, Slots -->
      <div class="gamepad-wing">
        <div class="gp-shoulder-row">
          <button class="btn-bumper" data-hold="game_action:bumper_r" data-label="RB: Sağ El (E)" data-color="#38bdf8">
            <span class="btn-tag">RB</span>
            <span class="btn-sub">Sağ El (E)</span>
          </button>
          <button class="btn-trigger btn-trigger-primary" data-hold="game_action:trigger_r" data-label="RT: Ateş (Sol Tık)" data-color="#ef4444">
            <span class="btn-tag">RT</span>
            <span class="btn-sub">Ateş (Sol Tık)</span>
          </button>
        </div>

        <div class="gp-abxy-center-wrap">
          <div class="abxy-diamond-large">
            <button class="abxy-btn btn-y" data-hold="game_action:btn_y" data-label="🟡 Y: R / Şarjör" data-color="#facc15">
              <span class="abxy-letter">Y</span>
              <span class="abxy-sub" id="lbl-y">R / Şarjör</span>
            </button>
            <div class="abxy-mid-row">
              <button class="abxy-btn btn-x" data-hold="game_action:btn_x" data-label="🔵 X: E / Etkileşim" data-color="#38bdf8">
                <span class="abxy-letter">X</span>
                <span class="abxy-sub" id="lbl-x">E / Etkileşim</span>
              </button>
              <button class="abxy-btn btn-b" data-hold="game_action:btn_b" data-label="🔴 B: Shift / Koş" data-color="#ef4444">
                <span class="abxy-letter">B</span>
                <span class="abxy-sub" id="lbl-b">Shift / Koş</span>
              </button>
            </div>
            <button class="abxy-btn btn-a" data-hold="game_action:btn_a" data-label="🟢 A: Space / Zıpla" data-color="#22c55e">
              <span class="abxy-letter">A</span>
              <span class="abxy-sub" id="lbl-a">Space / Zıpla</span>
            </button>
          </div>
        </div>

        <div class="gp-slots-row">
          <button class="game-slot" onclick="triggerKeyFeedback(this, '3', 'Silah 3', '#38bdf8')">3 <span class="slot-sub">Silah 3</span></button>
          <button class="game-slot" onclick="triggerKeyFeedback(this, '4', 'Bomba (4)', '#38bdf8')">4 <span class="slot-sub">Bomba</span></button>
        </div>
      </div>
    </div>
  </div>

  <!-- ==============================================================
       SCREEN OVERLAY 2: DEDICATED AUTO-LANDSCAPE WINDOWS KEYBOARD
       ============================================================== -->
  <div id="overlay-keyboard" class="screen-overlay auto-landscape">
    <div class="screen-topbar">
      <button class="btn-screen-back" onclick="closeOverlay('keyboard')">
        ◀ <span>Kumandaya Dön</span>
      </button>

      <!-- Canlı Klavye Tuş Geri Bildirimi -->
      <div class="hud-feedback" id="kbd-hud">⌨️ Klavye • Hazır</div>

      <button class="btn-screen-back" style="padding:5px 8px;border-color:#ef4444;color:#ef4444" onclick="triggerKeyFeedback(this, 'BackSpace', '⌫ Sil', '#ef4444')">
        ⌫ <span>Sil</span>
      </button>
    </div>

    <!-- Windows Shortcuts Row -->
    <div class="win-shortcuts-bar">
      <button class="win-chip win-btn-logo" onclick="triggerComboFeedback(this, 'win', '⊞ Windows Başlat', '#38bdf8')">⊞ Win</button>
      <button class="win-chip" onclick="triggerComboFeedback(this, 'win+d', 'Win+D (Masaüstü)', '#38bdf8')">Win+D</button>
      <button class="win-chip" onclick="triggerComboFeedback(this, 'alt+Tab', 'Alt+Tab (Geçiş)', '#38bdf8')">Alt+Tab</button>
      <button class="win-chip btn-danger" onclick="triggerComboFeedback(this, 'alt+F4', 'Alt+F4 (Kapat)', '#ef4444')">Alt+F4</button>
      <button class="win-chip" onclick="triggerComboFeedback(this, 'ctrl+c', 'Ctrl+C (Kopyala)', '#38bdf8')">Ctrl+C</button>
      <button class="win-chip" onclick="triggerComboFeedback(this, 'ctrl+v', 'Ctrl+V (Yapıştır)', '#38bdf8')">Ctrl+V</button>
      <button class="win-chip" onclick="triggerComboFeedback(this, 'ctrl+z', 'Ctrl+Z (Geri Al)', '#38bdf8')">Ctrl+Z</button>
      <button class="win-chip" onclick="triggerComboFeedback(this, 'ctrl+a', 'Ctrl+A (Tümü)', '#38bdf8')">Ctrl+A</button>
    </div>

    <!-- Wide Horizontal Keyboard Body -->
    <div class="kbd-screen-body">
      <!-- F-Keys Row -->
      <div class="fkeys-bar">
        <button class="fkey" onclick="triggerKeyFeedback(this, 'Escape', 'Esc', '#ef4444')">Esc</button>
        <button class="fkey" onclick="triggerKeyFeedback(this, 'F1', 'F1', '#38bdf8')">F1</button>
        <button class="fkey" onclick="triggerKeyFeedback(this, 'F2', 'F2', '#38bdf8')">F2</button>
        <button class="fkey" onclick="triggerKeyFeedback(this, 'F3', 'F3', '#38bdf8')">F3</button>
        <button class="fkey" onclick="triggerKeyFeedback(this, 'F4', 'F4', '#38bdf8')">F4</button>
        <button class="fkey" onclick="triggerKeyFeedback(this, 'F5', 'F5', '#38bdf8')">F5</button>
        <button class="fkey" onclick="triggerKeyFeedback(this, 'F6', 'F6', '#38bdf8')">F6</button>
        <button class="fkey" onclick="triggerKeyFeedback(this, 'F7', 'F7', '#38bdf8')">F7</button>
        <button class="fkey" onclick="triggerKeyFeedback(this, 'F8', 'F8', '#38bdf8')">F8</button>
        <button class="fkey" onclick="triggerKeyFeedback(this, 'F9', 'F9', '#38bdf8')">F9</button>
        <button class="fkey" onclick="triggerKeyFeedback(this, 'F10', 'F10', '#38bdf8')">F10</button>
        <button class="fkey" onclick="triggerKeyFeedback(this, 'F11', 'F11', '#38bdf8')">F11</button>
        <button class="fkey" onclick="triggerKeyFeedback(this, 'F12', 'F12', '#38bdf8')">F12</button>
        <button class="fkey btn-danger" onclick="triggerKeyFeedback(this, 'Delete', 'Del', '#ef4444')">Del</button>
      </div>

      <!-- Number Row -->
      <div class="kbd-line">
        <button class="k-key" onclick="typeNumKey(this, '\"', 'é')"><span class="k-sub">é</span><span class="k-main">"</span></button>
        <button class="k-key" onclick="typeNumKey(this, '1', '!')"><span class="k-sub">!</span><span class="k-main">1</span></button>
        <button class="k-key" onclick="typeNumKey(this, '2', '\'')"><span class="k-sub">'</span><span class="k-main">2</span></button>
        <button class="k-key" onclick="typeNumKey(this, '3', '^')"><span class="k-sub">^</span><span class="k-main">3</span></button>
        <button class="k-key" onclick="typeNumKey(this, '4', '+')"><span class="k-sub">+</span><span class="k-main">4</span></button>
        <button class="k-key" onclick="typeNumKey(this, '5', '%')"><span class="k-sub">%</span><span class="k-main">5</span></button>
        <button class="k-key" onclick="typeNumKey(this, '6', '&')"><span class="k-sub">&</span><span class="k-main">6</span></button>
        <button class="k-key" onclick="typeNumKey(this, '7', '/')"><span class="k-sub">/</span><span class="k-main">7</span></button>
        <button class="k-key" onclick="typeNumKey(this, '8', '(')"><span class="k-sub">(</span><span class="k-main">8</span></button>
        <button class="k-key" onclick="typeNumKey(this, '9', ')')"><span class="k-sub">)</span><span class="k-main">9</span></button>
        <button class="k-key" onclick="typeNumKey(this, '0', '=')"><span class="k-sub">=</span><span class="k-main">0</span></button>
        <button class="k-key" onclick="typeNumKey(this, '*', '?')"><span class="k-sub">?</span><span class="k-main">*</span></button>
        <button class="k-key" onclick="typeNumKey(this, '-', '_')"><span class="k-sub">_</span><span class="k-main">-</span></button>
        <button class="k-key k-w2 k-spec" onclick="triggerKeyFeedback(this, 'BackSpace', '⌫ Sil', '#ef4444')">⌫ Sil</button>
      </div>

      <!-- Row 1: Q W E R T Y U I O P Ğ Ü -->
      <div class="kbd-line">
        <button class="k-key k-w15 k-spec" onclick="triggerKeyFeedback(this, 'Tab', 'Tab ⇥', '#38bdf8')">Tab ⇥</button>
        <button class="k-key letter" onclick="typeLetter(this, 'q')">Q</button>
        <button class="k-key letter" onclick="typeLetter(this, 'w')">W</button>
        <button class="k-key letter" onclick="typeLetter(this, 'e')">E</button>
        <button class="k-key letter" onclick="typeLetter(this, 'r')">R</button>
        <button class="k-key letter" onclick="typeLetter(this, 't')">T</button>
        <button class="k-key letter" onclick="typeLetter(this, 'y')">Y</button>
        <button class="k-key letter" onclick="typeLetter(this, 'u')">U</button>
        <button class="k-key letter" onclick="typeLetter(this, 'ı')">I</button>
        <button class="k-key letter" onclick="typeLetter(this, 'o')">O</button>
        <button class="k-key letter" onclick="typeLetter(this, 'p')">P</button>
        <button class="k-key letter" onclick="typeLetter(this, 'ğ')">Ğ</button>
        <button class="k-key letter" onclick="typeLetter(this, 'ü')">Ü</button>
      </div>

      <!-- Row 2: A S D F G H J K L Ş İ Enter -->
      <div class="kbd-line">
        <button class="k-key k-w15 k-spec" id="caps-btn" onclick="toggleCaps(this)">Caps ⇪</button>
        <button class="k-key letter" onclick="typeLetter(this, 'a')">A</button>
        <button class="k-key letter" onclick="typeLetter(this, 's')">S</button>
        <button class="k-key letter" onclick="typeLetter(this, 'd')">D</button>
        <button class="k-key letter" onclick="typeLetter(this, 'f')">F</button>
        <button class="k-key letter" onclick="typeLetter(this, 'g')">G</button>
        <button class="k-key letter" onclick="typeLetter(this, 'h')">H</button>
        <button class="k-key letter" onclick="typeLetter(this, 'j')">J</button>
        <button class="k-key letter" onclick="typeLetter(this, 'k')">K</button>
        <button class="k-key letter" onclick="typeLetter(this, 'l')">L</button>
        <button class="k-key letter" onclick="typeLetter(this, 'ş')">Ş</button>
        <button class="k-key letter" onclick="typeLetter(this, 'i')">İ</button>
        <button class="k-key k-w2 k-enter" onclick="triggerKeyFeedback(this, 'Return', '↵ Enter', '#22c55e')">↵ Enter</button>
      </div>

      <!-- Row 3: Shift Z X C V B N M Ö Ç . ▲ -->
      <div class="kbd-line">
        <button class="k-key k-w2 k-spec" id="shift-btn" onclick="toggleShift(this)">⇧ Shift</button>
        <button class="k-key letter" onclick="typeLetter(this, 'z')">Z</button>
        <button class="k-key letter" onclick="typeLetter(this, 'x')">X</button>
        <button class="k-key letter" onclick="typeLetter(this, 'c')">C</button>
        <button class="k-key letter" onclick="typeLetter(this, 'v')">V</button>
        <button class="k-key letter" onclick="typeLetter(this, 'b')">B</button>
        <button class="k-key letter" onclick="typeLetter(this, 'n')">N</button>
        <button class="k-key letter" onclick="typeLetter(this, 'm')">M</button>
        <button class="k-key letter" onclick="typeLetter(this, 'ö')">Ö</button>
        <button class="k-key letter" onclick="typeLetter(this, 'ç')">Ç</button>
        <button class="k-key" onclick="triggerKeyFeedback(this, '.', 'Nokta (.)', '#38bdf8')">.</button>
        <button class="k-key" onclick="triggerKeyFeedback(this, ',', 'Virgül (,)', '#38bdf8')">,</button>
        <button class="k-key k-arrow" onclick="triggerKeyFeedback(this, 'Up', '▲ Yukarı', '#38bdf8')">▲</button>
      </div>

      <!-- Row 4: Ctrl Win Alt Space AltGr ← ▼ ▶ -->
      <div class="kbd-line">
        <button class="k-key k-spec" onclick="triggerKeyFeedback(this, 'Control_L', 'Ctrl', '#38bdf8')">Ctrl</button>
        <button class="k-key k-spec win-btn-logo" onclick="triggerComboFeedback(this, 'win', '⊞ Win', '#38bdf8')">⊞</button>
        <button class="k-key k-spec" onclick="triggerKeyFeedback(this, 'Alt_L', 'Alt', '#38bdf8')">Alt</button>
        <button class="k-key k-space" onclick="triggerKeyFeedback(this, 'space', '␣ Boşluk (Space)', '#38bdf8')">␣ Boşluk (Space)</button>
        <button class="k-key k-spec" onclick="triggerKeyFeedback(this, 'Alt_R', 'AltGr', '#38bdf8')">AltGr</button>
        <button class="k-key k-arrow" onclick="triggerKeyFeedback(this, 'Left', '◀ Sol', '#38bdf8')">◀</button>
        <button class="k-key k-arrow" onclick="triggerKeyFeedback(this, 'Down', '▼ Aşağı', '#38bdf8')">▼</button>
        <button class="k-key k-arrow" onclick="triggerKeyFeedback(this, 'Right', '▶ Sağ', '#38bdf8')">▶</button>
      </div>
    </div>
  </div>

  <!-- ==============================================================
       SCREEN OVERLAY 3: DEDICATED MOUSE & TOUCHPAD SCREEN
       ============================================================== -->
  <div id="overlay-mouse" class="screen-overlay">
    <div class="screen-topbar">
      <button class="btn-screen-back" onclick="closeOverlay('mouse')">
        ◀ <span>Kumandaya Dön</span>
      </button>
      <div class="screen-topbar-title">
        <span>🖱️ FARE & TOUCHPAD</span>
      </div>
      <div class="hud-feedback" id="mouse-hud" style="min-width:120px">İmleç Hazır</div>
    </div>

    <div class="mouse-screen-body">
      <div class="touchpad-large" id="pad">
        <span style="font-size:26px;margin-bottom:6px">🖱️</span>
        <b style="font-size:14px;color:#fff">Dokunmatik Fare Alanı (Air Touchpad)</b>
        <span style="font-size:11px;color:var(--muted);margin-top:4px">İmleci hareket ettirmek için sürükleyin.<br>Tıklamak için hafifçe dokunun.</span>
      </div>

      <div class="row-3">
        <button style="padding:12px" onclick="cmd('mouse_click','left'); showFeedback('🖱️ Sol Tık', '#22c55e', 'mouse-hud'); flashKey(this);">🖱️<span>Sol Tık</span></button>
        <button style="padding:12px" onclick="cmd('mouse_click','middle'); showFeedback('🔘 Orta Tık', '#38bdf8', 'mouse-hud'); flashKey(this);">🔘<span>Orta Tık</span></button>
        <button style="padding:12px" onclick="cmd('mouse_click','right'); showFeedback('📑 Sağ Tık', '#38bdf8', 'mouse-hud'); flashKey(this);">📑<span>Sağ Tık</span></button>
      </div>
      <div class="row-4">
        <button style="padding:10px" onclick="cmd('back'); showFeedback('🔙 Geri (Esc)', '#ef4444', 'mouse-hud'); flashKey(this);">🔙<span>Esc</span></button>
        <button style="padding:10px" onclick="cmd('mouse_click','scroll_up'); showFeedback('⬆️ Yukarı Kaydır', '#38bdf8', 'mouse-hud'); flashKey(this);">⬆️<span>Yukarı</span></button>
        <button style="padding:10px" onclick="cmd('mouse_click','scroll_down'); showFeedback('⬇️ Aşağı Kaydır', '#38bdf8', 'mouse-hud'); flashKey(this);">⬇️<span>Aşağı</span></button>
        <button style="padding:10px" onclick="cmd('key_press','BackSpace'); showFeedback('⌫ Sil', '#ef4444', 'mouse-hud'); flashKey(this);">⌫<span>Sil</span></button>
      </div>

      <div class="section-lbl">TV'ye Yazı / Arama Gönder</div>
      <div class="kbd-row">
        <input class="kbd-input" id="txt" type="text" placeholder="Film, kanal veya şarkı yazın..." autocomplete="off">
        <button style="padding:10px 14px;background:var(--accent);border:none" onclick="sendText()">Ara ↵</button>
      </div>
    </div>
  </div>

  <!-- POWER MODAL -->
  <div id="pwr-modal" class="modal-overlay" onclick="closePowerModal(event)">
    <div class="modal-card" onclick="event.stopPropagation()">
      <div class="modal-header">
        <div class="modal-title">⏻ Güç ve Sistem Yönetimi</div>
        <button class="modal-close" onclick="closePowerModal()">✕</button>
      </div>
      <div style="font-size:12px;color:var(--muted)">TV ve bilgisayarın durumunu uzaktan yönetin:</div>

      <div class="pwr-grid">
        <button class="pwr-btn pwr-screen" onclick="cmd('screen_toggle'); closePowerModal();">
          <span class="pwr-icon">🖥️</span>
          <span class="pwr-text">Ekran Aç / Kapat</span>
          <span class="pwr-sub">TV ekranını uyutur veya açar</span>
        </button>

        <button class="pwr-btn pwr-sleep" onclick="cmd('power_suspend'); closePowerModal();">
          <span class="pwr-icon">🌙</span>
          <span class="pwr-text">Uyku Modu</span>
          <span class="pwr-sub">Bilgisayarı askıya alır</span>
        </button>

        <button class="pwr-btn pwr-restart" onclick="confirmPower('restart')">
          <span class="pwr-icon">🔄</span>
          <span class="pwr-text">Yeniden Başlat</span>
          <span class="pwr-sub">Sistemi baştan başlatır</span>
        </button>

        <button class="pwr-btn pwr-off" onclick="confirmPower('shutdown')">
          <span class="pwr-icon">⏻</span>
          <span class="pwr-text">Bilgisayarı Kapat</span>
          <span class="pwr-sub">Sistemi tamamen kapatır</span>
        </button>
      </div>

      <div id="pwr-confirm" class="confirm-box" style="display:none">
        <div id="confirm-msg" class="confirm-msg">Bilgisayarı kapatmak istediğinize emin misiniz?</div>
        <div class="row-2">
          <button class="btn-danger" id="confirm-yes-btn">Evet, Devam Et</button>
          <button onclick="hideConfirm()">Vazgeç</button>
        </div>
      </div>

      <button style="padding:10px;border-color:#334155" onclick="cmd('system_lock'); closePowerModal();">
        🔒 Ekranı Kilitle
      </button>
    </div>
  </div>

<script>
function vib(ms) { if (navigator.vibrate) navigator.vibrate(ms || 18); }

function showTab(id, btn) {
  vib(10);
  closeAllOverlays();
  document.querySelectorAll('.panel').forEach(p => p.classList.remove('active'));
  document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
  const p = document.getElementById('tab-' + id);
  if (p) p.classList.add('active');
  if (btn) btn.classList.add('active');
  if (id === 'ws') refreshWorkspaces();
}

// Canlı Tuş Geri Bildirim Göstergesi
function showFeedback(text, color, hudId) {
  const el = document.getElementById(hudId || 'gp-hud');
  if (!el) return;
  el.textContent = text;
  el.style.borderColor = color || '#38bdf8';
  el.style.color = color || '#38bdf8';
  el.style.boxShadow = '0 0 16px ' + (color || '#38bdf8');
  clearTimeout(el._timer);
  el._timer = setTimeout(() => {
    el.style.boxShadow = 'none';
  }, 400);
}

function flashKey(el) {
  if (!el) return;
  el.classList.add('active-press');
  setTimeout(() => el.classList.remove('active-press'), 130);
}

function triggerKeyFeedback(btn, key, label, color) {
  flashKey(btn);
  showFeedback(label || key, color || '#38bdf8', 'kbd-hud');
  cmd('key_press', key);
}

function triggerComboFeedback(btn, combo, label, color) {
  flashKey(btn);
  showFeedback(label || combo, color || '#38bdf8', 'kbd-hud');
  cmd('key_combo', combo);
}

function triggerActionFeedback(btn, act, label, color) {
  flashKey(btn);
  showFeedback(label || act, color || '#facc15', 'gp-hud');
  cmd('game_action', act);
}

function updateViewportSize() {
  const h = window.innerHeight;
  const w = window.innerWidth;
  document.documentElement.style.setProperty('--app-h', h + 'px');
  document.documentElement.style.setProperty('--app-w', w + 'px');
}
window.addEventListener('resize', updateViewportSize);
window.addEventListener('orientationchange', () => {
  setTimeout(updateViewportSize, 100);
  setTimeout(updateViewportSize, 300);
});
updateViewportSize();

async function openOverlay(name) {
  vib(15);
  closeAllOverlays();
  const el = document.getElementById('overlay-' + name);
  if (!el) return;
  el.classList.add('open');
  updateViewportSize();
  setTimeout(updateViewportSize, 100);
  if (name === 'gamepad' || name === 'keyboard') {
    try {
      if (document.documentElement.requestFullscreen) {
        await document.documentElement.requestFullscreen();
      }
      if (screen.orientation && screen.orientation.lock) {
        await screen.orientation.lock('landscape').catch(() => {});
      }
    } catch(e) {}
    setTimeout(updateViewportSize, 250);
  }
}

function closeOverlay(name) {
  vib(12);
  const el = document.getElementById('overlay-' + name);
  if (el) el.classList.remove('open');
  try {
    if (document.fullscreenElement && document.exitFullscreen) {
      document.exitFullscreen().catch(() => {});
    }
  } catch(e) {}
}

function closeAllOverlays() {
  document.querySelectorAll('.screen-overlay').forEach(el => el.classList.remove('open'));
}

let pendingPowerAction = null;
function openPowerModal() {
  vib(15);
  hideConfirm();
  document.getElementById('pwr-modal').classList.add('open');
}
function closePowerModal(e) {
  if (e && e.target !== e.currentTarget && !e.target.classList.contains('modal-close')) return;
  vib(10);
  hideConfirm();
  document.getElementById('pwr-modal').classList.remove('open');
}
function confirmPower(act) {
  vib(20);
  pendingPowerAction = act;
  const box = document.getElementById('pwr-confirm');
  const msg = document.getElementById('confirm-msg');
  const btn = document.getElementById('confirm-yes-btn');
  if (act === 'shutdown') {
    msg.textContent = 'TV ve bilgisayar tamamen kapatılacak. Emin misiniz?';
    btn.textContent = 'Evet, Kapat';
    btn.onclick = () => { cmd('power_shutdown'); closePowerModal(); };
  } else if (act === 'restart') {
    msg.textContent = 'Bilgisayar yeniden başlatılacak. Emin misiniz?';
    btn.textContent = 'Evet, Yeniden Başlat';
    btn.onclick = () => { cmd('power_reboot'); closePowerModal(); };
  }
  box.style.display = 'flex';
}
function hideConfirm() {
  const box = document.getElementById('pwr-confirm');
  if (box) box.style.display = 'none';
  pendingPowerAction = null;
}

let currentActiveWs = 1;
async function refreshWorkspaces() {
  try {
    const res = await fetch('/api/workspaces');
    const data = await res.json();
    if (data.active !== undefined) {
      currentActiveWs = data.active;
      const qLbl = document.getElementById('quick-ws-lbl');
      if (qLbl) qLbl.textContent = 'Masaüstü ' + currentActiveWs;
      const bEl = document.getElementById('ws-badge');
      if (bEl) bEl.textContent = currentActiveWs;
    }
    const container = document.getElementById('ws-container');
    if (container && Array.isArray(data.workspaces)) {
      container.innerHTML = data.workspaces.map(ws => {
        const isActive = ws.id === currentActiveWs;
        const apps = ws.apps || (ws.id === 1 ? 'Plus TV' : 'Masaüstü Pencereleri');
        return `
          <div class="ws-card ${isActive ? 'active' : ''}" onclick="cmd('workspace_focus', '${ws.id}')">
            <div class="ws-head">
              <span class="ws-title">🖥️ Masaüstü ${ws.id}</span>
              ${isActive ? '<span class="ws-pill-active">● AKTİF</span>' : '<span class="ws-pill-switch">Geçiş Yap ➔</span>'}
            </div>
            <div class="ws-desc">${apps}</div>
          </div>
        `;
      }).join('');
    }
  } catch(e) {}
}

async function cmd(action, payload) {
  vib(12);
  const st = document.getElementById('st');
  try {
    const q = '/api/cmd?action=' + encodeURIComponent(action) + (payload !== undefined && payload !== null ? '&payload=' + encodeURIComponent(payload) : '');
    const r = await fetch(q);
    const j = await r.json();
    if (j.status) st.textContent = j.status;
    else if (j.selected) st.textContent = j.selected;
    if (action.startsWith('workspace') || action === 'home') {
      setTimeout(refreshWorkspaces, 150);
    }
  } catch (e) {
    st.textContent = 'Bağlantı hatası';
  }
}

function sendText() {
  const el = document.getElementById('txt');
  const val = el.value;
  if (!val) { cmd('key_press', 'Return'); return; }
  cmd('type_text', val).then(() => setTimeout(() => cmd('key_press', 'Return'), 120));
  el.value = '';
}
document.getElementById('txt').addEventListener('keydown', e => {
  if (e.key === 'Enter') { e.preventDefault(); sendText(); }
});

// Touchpad handling
const pad = document.getElementById('pad');
let lx = 0, ly = 0, moved = false, accX = 0, accY = 0, timer = null;
if (pad) {
  pad.addEventListener('touchstart', e => {
    if (e.touches.length === 1) {
      lx = e.touches[0].clientX;
      ly = e.touches[0].clientY;
      moved = false;
    }
  }, {passive: false});
  pad.addEventListener('touchmove', e => {
    e.preventDefault();
    if (e.touches.length !== 1) return;
    const x = e.touches[0].clientX, y = e.touches[0].clientY;
    const dx = (x - lx) * 1.9, dy = (y - ly) * 1.9;
    if (Math.abs(dx) > 2 || Math.abs(dy) > 2) moved = true;
    lx = x; ly = y;
    accX += dx; accY += dy;
    if (!timer) {
      timer = setTimeout(() => {
        const mx = Math.round(accX), my = Math.round(accY);
        accX = 0; accY = 0; timer = null;
        if (mx !== 0 || my !== 0) fetch('/api/cmd?action=mouse_move&payload=' + mx + ',' + my);
      }, 35);
    }
  }, {passive: false});
  pad.addEventListener('touchend', e => {
    if (!moved) {
      cmd('mouse_click', 'left');
      showFeedback('🖱️ Tıklandı', '#22c55e', 'mouse-hud');
    }
  });
}

// ==============================================================
// WINDOWS KEYBOARD STATE & ACTIONS
// ==============================================================
let capsLockActive = false;
let shiftActive = false;

function toggleCaps(btn) {
  flashKey(btn);
  capsLockActive = !capsLockActive;
  btn.classList.toggle('active', capsLockActive);
  showFeedback(capsLockActive ? 'Caps Lock: AÇIK' : 'Caps Lock: KAPALI', '#facc15', 'kbd-hud');
  updateKeyboardLetters();
}

function toggleShift(btn) {
  flashKey(btn);
  shiftActive = !shiftActive;
  btn.classList.toggle('active', shiftActive);
  showFeedback(shiftActive ? '⇧ Shift: AÇIK' : '⇧ Shift: KAPALI', '#a855f7', 'kbd-hud');
  updateKeyboardLetters();
}

function updateKeyboardLetters() {
  const isUpper = (capsLockActive !== shiftActive);
  document.querySelectorAll('.k-key.letter').forEach(btn => {
    const raw = btn.textContent.trim().toLowerCase();
    btn.textContent = isUpper ? raw.toUpperCase() : raw.toLowerCase();
  });
}

function typeLetter(btn, char) {
  flashKey(btn);
  const isUpper = (capsLockActive !== shiftActive);
  let out = isUpper ? char.toUpperCase() : char.toLowerCase();
  if (char === 'i') out = isUpper ? 'İ' : 'i';
  else if (char === 'ı') out = isUpper ? 'I' : 'ı';
  showFeedback('Tuş: ' + out, '#38bdf8', 'kbd-hud');
  cmd('key_press', out);
  if (shiftActive) {
    shiftActive = false;
    const sBtn = document.getElementById('shift-btn');
    if (sBtn) sBtn.classList.remove('active');
    updateKeyboardLetters();
  }
}

function typeNumKey(btn, normal, shifted) {
  flashKey(btn);
  const char = shiftActive ? shifted : normal;
  showFeedback('Tuş: ' + char, '#38bdf8', 'kbd-hud');
  cmd('key_press', char);
  if (shiftActive) {
    shiftActive = false;
    const sBtn = document.getElementById('shift-btn');
    if (sBtn) sBtn.classList.remove('active');
    updateKeyboardLetters();
  }
}

// ==============================================================
// GAME CONTROLLER (HOLD & CONTINUOUS INPUT LOGIC)
// ==============================================================
let currentGameMode = 'wasd'; // 'wasd' or 'arrows'
let holdTimer = null;
let activeHoldAction = null;

function toggleGameMode() {
  setGameMode(currentGameMode === 'wasd' ? 'arrows' : 'wasd');
}

function setGameMode(mode) {
  vib(15);
  currentGameMode = mode;
  const lbl = document.getElementById('gp-mode-lbl');
  if (lbl) lbl.textContent = mode === 'wasd' ? 'Mod: WASD' : 'Mod: Oklar';
  showFeedback(mode === 'wasd' ? '🕹️ WASD (PC Modu)' : '🏹 Ok Tuşları (Retro)', '#38bdf8', 'gp-hud');

  const ly = document.getElementById('lbl-y');
  const lx = document.getElementById('lbl-x');
  const lb = document.getElementById('lbl-b');
  const la = document.getElementById('lbl-a');
  if (mode === 'wasd') {
    if (ly) ly.textContent = 'R / Şarjör';
    if (lx) lx.textContent = 'E / Etkileşim';
    if (lb) lb.textContent = 'Shift / Koş';
    if (la) la.textContent = 'Space / Zıpla';
  } else {
    if (ly) ly.textContent = 'X / Sil';
    if (lx) lx.textContent = 'Z / Özel';
    if (lb) lb.textContent = 'Esc / Geri';
    if (la) la.textContent = 'Enter / Onay';
  }
}

function startHold(action, el) {
  stopHold();
  vib(22);
  activeHoldAction = action;
  if (el) {
    const lbl = el.getAttribute('data-label') || action;
    const clr = el.getAttribute('data-color') || '#38bdf8';
    showFeedback(lbl, clr, 'gp-hud');
  }
  executeHoldAction(action);
  holdTimer = setInterval(() => {
    executeHoldAction(action);
  }, 65);
}

function stopHold() {
  if (holdTimer) {
    clearInterval(holdTimer);
    holdTimer = null;
  }
  activeHoldAction = null;
}

function executeHoldAction(action) {
  if (!action) return;
  const parts = action.split(':');
  const type = parts[0];
  const val = parts[1];
  if (type === 'game_dir') {
    if (currentGameMode === 'wasd') {
      const map = { up: 'w', down: 's', left: 'a', right: 'd' };
      cmd('key_press', map[val] || val);
    } else {
      const map = { up: 'Up', down: 'Down', left: 'Left', right: 'Right' };
      cmd('key_press', map[val] || val);
    }
  } else if (type === 'game_action') {
    if (currentGameMode === 'arrows') {
      if (val === 'btn_a') { cmd('key_press', 'Return'); return; }
      if (val === 'btn_b') { cmd('key_press', 'Escape'); return; }
      if (val === 'btn_x') { cmd('key_press', 'z'); return; }
      if (val === 'btn_y') { cmd('key_press', 'x'); return; }
    }
    cmd('game_action', val);
  }
}

function initHoldListeners() {
  document.querySelectorAll('[data-hold]').forEach(el => {
    const act = el.getAttribute('data-hold');
    el.addEventListener('touchstart', e => {
      e.preventDefault();
      startHold(act, el);
      el.classList.add('active-press');
    }, {passive: false});
    el.addEventListener('touchend', e => {
      e.preventDefault();
      stopHold();
      el.classList.remove('active-press');
    }, {passive: false});
    el.addEventListener('touchcancel', () => {
      stopHold();
      el.classList.remove('active-press');
    });
    el.addEventListener('mousedown', e => {
      e.preventDefault();
      startHold(act, el);
      el.classList.add('active-press');
    });
    el.addEventListener('mouseup', () => {
      stopHold();
      el.classList.remove('active-press');
    });
    el.addEventListener('mouseleave', () => {
      stopHold();
      el.classList.remove('active-press');
    });
  });
}

initHoldListeners();
setInterval(refreshWorkspaces, 2500);
refreshWorkspaces();
</script>
</body>
</html>)HTML";
    return QByteArray(kHtml);
}
