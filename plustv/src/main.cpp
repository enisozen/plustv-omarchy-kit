// plustv — Smart TV & Media Hub launcher for Omarchy.

#include <QDir>
#include <QGuiApplication>
#include <QIcon>
#include <QLockFile>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QUrl>

#include "backend.h"
#include "theme.h"
#include "translator.h"

int main(int argc, char *argv[]) {
    QLockFile instanceLock(QDir::tempPath() + "/plustv-" + qEnvironmentVariable("USER", "user") + ".lock");
    instanceLock.setStaleLockTime(0);
    if (!instanceLock.tryLock(0)) {
        QProcess::execute("hyprctl", {"dispatch", "o.workspaces.focus(1)"});
        QProcess::execute("hyprctl", {"dispatch", "hl.dsp.focus({ window = \"class:^plustv$\" })"});
        return 0;
    }

    // Render with the CPU software rasterizer so Plus TV never holds hardware
    // 3D FIFO channels on nouveau when launching browsers or cloud gaming apps.
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);

    QGuiApplication app(argc, argv);
    app.setApplicationName("plustv");
    app.setApplicationVersion("0.1.0");

    // Matches the window to plustv.desktop, so the compositor (Wayland app_id)
    // and the launcher pick up the installed icon.
    app.setDesktopFileName("plustv");
    app.setWindowIcon(QIcon::fromTheme("plustv"));

    QQuickStyle::setStyle("Material");

    Translator translator;
    app.installTranslator(&translator);

    Theme theme;
    Backend backend;

    QQmlApplicationEngine engine;
    QObject::connect(&translator, &Translator::changed, &engine, &QQmlEngine::retranslate);
    engine.rootContext()->setContextProperty("translator", &translator);
    engine.rootContext()->setContextProperty("theme", &theme);
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.load(QUrl("qrc:/Main.qml"));
    if (engine.rootObjects().isEmpty())
        return 1;

    return app.exec();
}
