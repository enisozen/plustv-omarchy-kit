#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "backend.h"
#include "theme.h"
#include "translator.h"

class PlusTvTests : public QObject {
    Q_OBJECT

private slots:
    void backendLoadsCatalogAndParsesDesktopExec()
    {
        QTemporaryDir dir;
        write(dir.path() + "/beIN CONNECT.desktop",
              "[Desktop Entry]\nName=beIN CONNECT\nExec=omarchy-launch-webapp \"https://www.beinconnect.com.tr\" %u\n");

        Backend backend({dir.path()}, nullptr, true);
        QVERIFY(backend.count() >= 7);
        QCOMPARE(backend.selectedApp().value("id").toString(), QString("bein-connect"));
        QCOMPARE(backend.selectedApp().value("installed").toBool(), true);
        QCOMPARE(backend.selectedApp().value("execCommand").toString(),
                 QString("omarchy-launch-webapp \"https://www.beinconnect.com.tr\""));

        QVERIFY(backend.launchById("bein-connect"));
        QCOMPARE(backend.lastLaunchedCommand(),
                 QString("omarchy-launch-webapp \"https://www.beinconnect.com.tr\""));

        QVERIFY(backend.launchById("spotify"));
        QCOMPARE(backend.lastLaunchedCommand(),
                 QString("omarchy-launch-webapp \"https://open.spotify.com/\""));

        QVERIFY(backend.launchById("youtube-music"));
        QCOMPARE(backend.lastLaunchedCommand(),
                 QString("omarchy-launch-webapp \"https://music.youtube.com/\""));

        QVERIFY(backend.launchById("geforce-now"));
        QCOMPARE(backend.lastLaunchedCommand(),
                 QString("omarchy-launch-webapp \"https://play.geforcenow.com\" --disable-gpu"));
    }

    void backendFiltersByCategory()
    {
        QTemporaryDir dir;
        Backend backend({dir.path()}, nullptr, true);
        const int totalCount = backend.count();
        backend.setCategory("live");
        QVERIFY(backend.count() > 0);
        QVERIFY(backend.count() < totalCount);
        backend.setCategory("all");
        QCOMPARE(backend.count(), totalCount);
    }

    void themeFallsBack()
    {
        QTemporaryDir dir;
        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#FFD60A"));
        QCOMPARE(theme.accentForeground(), QString("black"));
    }

    void themeReadsAccent()
    {
        QTemporaryDir dir;
        QDir(dir.path()).mkpath("theme");
        QFile colors(dir.path() + "/theme/colors.toml");
        QVERIFY(colors.open(QIODevice::WriteOnly));
        colors.write("accent = \"#112233\"\n");
        colors.close();

        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#112233"));
        QCOMPARE(theme.accentForeground(), QString("white"));
    }

    void themeFollowsHyprlandRounding()
    {
        QTemporaryDir dir;
        const QString response = dir.filePath("response.json"), command = dir.filePath("hyprctl");
        auto writeExec = [](const QString &path, const QByteArray &data) {
            QFile file(path);
            return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
        };
        QVERIFY(writeExec(command, "#!/bin/sh\ncat '" + response.toUtf8() + "'\n"));
        QVERIFY(QFile::setPermissions(command, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        QVERIFY(writeExec(response, "{\"int\":0}"));

        Theme theme(dir.path(), nullptr, command);
        QCOMPARE(theme.radius(), 0);
        QVERIFY(writeExec(response, "{\"int\":6}"));
        QTRY_COMPARE(theme.radius(), 6);
    }

    void translatorFollowsTheLanguageFile()
    {
        QTemporaryDir dir;
        write(dir.path() + "/language", "tr\n");
        write(dir.path() + "/tr.json", R"({"Launch Now": "Şimdi Aç"})");

        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("tr"));
        QCOMPARE(translator.translate("Main", "Launch Now"), QString("Şimdi Aç"));
    }

    void remoteServerListensAndHandlesActions()
    {
        QTemporaryDir dir;
        Backend backend({dir.path()}, nullptr, true);
        QVERIFY(backend.remotePort() > 0);
        QVERIFY(backend.remoteUrl().startsWith("http://"));

        QCOMPARE(backend.selectedIndex(), 0);
        QVERIFY(backend.executeRemoteAction("nav_right"));
        QCOMPARE(backend.selectedIndex(), 1);
        QVERIFY(backend.executeRemoteAction("nav_left"));
        QCOMPARE(backend.selectedIndex(), 0);

        QVERIFY(backend.executeRemoteAction("category_next"));
        QCOMPARE(backend.category(), QString("live"));

        QVERIFY(backend.executeRemoteAction("launch", "netflix"));
        QCOMPARE(backend.category(), QString("all"));
        QCOMPARE(backend.lastLaunchedCommand(), QString("omarchy-launch-webapp \"https://www.netflix.com\""));

        // Power and Screen Actions
        QVERIFY(backend.executeRemoteAction("screen_toggle"));
        QCOMPARE(backend.statusMessage(), QString("Screen toggled"));
        QVERIFY(backend.executeRemoteAction("power_suspend"));
        QCOMPARE(backend.statusMessage(), QString("Suspending system..."));
        QVERIFY(backend.executeRemoteAction("power_reboot"));
        QCOMPARE(backend.statusMessage(), QString("Restarting system..."));
        QVERIFY(backend.executeRemoteAction("power_shutdown"));
        QCOMPARE(backend.statusMessage(), QString("Shutting down system..."));

        // Workspace Actions
        QVERIFY(backend.executeRemoteAction("workspace_next"));
        QCOMPARE(backend.statusMessage(), QString("Next Desktop"));
        QVERIFY(backend.executeRemoteAction("workspace_prev"));
        QCOMPARE(backend.statusMessage(), QString("Previous Desktop"));
        QVERIFY(backend.executeRemoteAction("workspace_focus", "2"));
        QCOMPARE(backend.statusMessage(), QString("Switched to Desktop 2"));

        // Windows Keyboard & Game Actions
        QVERIFY(backend.executeRemoteAction("key_press", "Return"));
        QVERIFY(backend.executeRemoteAction("key_press", "a"));
        QVERIFY(backend.executeRemoteAction("key_press", "Delete"));
        QVERIFY(backend.executeRemoteAction("key_combo", "win"));
        QVERIFY(backend.executeRemoteAction("key_combo", "ctrl+c"));
        QVERIFY(backend.executeRemoteAction("key_combo", "win+d"));
        QVERIFY(backend.executeRemoteAction("game_action", "btn_a"));
        QVERIFY(backend.executeRemoteAction("game_action", "trigger_r"));
        QVERIFY(backend.executeRemoteAction("game_action", "trigger_l"));
        QVERIFY(backend.executeRemoteAction("mouse_click", "middle"));

        // Workspaces Info API
        const QJsonObject wsInfo = backend.getWorkspacesInfo();
        QCOMPARE(wsInfo.value("active").toInt(), 1);
        QVERIFY(wsInfo.value("workspaces").isArray());
        QVERIFY(!wsInfo.value("workspaces").toArray().isEmpty());
    }

private:
    static void write(const QString &path, const QByteArray &contents)
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(contents);
    }
};

QTEST_MAIN(PlusTvTests)
#include "plustv_tests.moc"
