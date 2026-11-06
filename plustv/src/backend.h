#pragma once

#include <QFileSystemWatcher>
#include <QJsonObject>
#include <QObject>
#include <QStringList>
#include <QTcpServer>
#include <QVariantList>
#include <QVariantMap>

class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList apps READ apps NOTIFY appsChanged)
    Q_PROPERTY(int count READ count NOTIFY appsChanged)
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedIndexChanged)
    Q_PROPERTY(QVariantMap selectedApp READ selectedApp NOTIFY selectedIndexChanged)
    Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY filterChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY filterChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString remoteUrl READ remoteUrl NOTIFY remoteInfoChanged)
    Q_PROPERTY(QString remoteQrDataUri READ remoteQrDataUri NOTIFY remoteInfoChanged)
    Q_PROPERTY(int remotePort READ remotePort NOTIFY remoteInfoChanged)

public:
    explicit Backend(const QStringList &applicationDirs = {}, QObject *parent = nullptr, bool dryRunLaunch = false);

    QVariantList apps() const { return m_filteredApps; }
    int count() const { return m_filteredApps.size(); }

    int selectedIndex() const { return m_selectedIndex; }
    void setSelectedIndex(int index);
    QVariantMap selectedApp() const;

    QString category() const { return m_category; }
    void setCategory(const QString &category);

    QString searchQuery() const { return m_searchQuery; }
    void setSearchQuery(const QString &query);

    QString statusMessage() const { return m_statusMessage; }
    QString lastLaunchedCommand() const { return m_lastLaunchedCommand; }

    QString remoteUrl() const { return m_remoteUrl; }
    QString remoteQrDataUri() const { return m_remoteQrDataUri; }
    int remotePort() const { return m_remoteServer.isListening() ? m_remoteServer.serverPort() : 0; }

    Q_INVOKABLE void primaryAction();
    Q_INVOKABLE bool launchApp(int index);
    Q_INVOKABLE bool launchById(const QString &id);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool executeRemoteAction(const QString &action, const QString &payload = QString());
    Q_INVOKABLE void refreshRemoteInfo();
    Q_INVOKABLE QJsonObject getWorkspacesInfo() const;

    static QString cleanExecCommand(const QString &execLine);

signals:
    void appsChanged();
    void selectedIndexChanged();
    void filterChanged();
    void statusMessageChanged();
    void appLaunched(const QString &name);
    void remoteInfoChanged();
    void remoteCommandReceived(const QString &action);

private:
    struct CatalogEntry {
        QString id;
        QString name;
        QString subtitle;
        QString badge;
        QString category;
        QString cover;
        QString accentColor;
        QString secondaryColor;
        QStringList desktopCandidates;
        QString fallbackExec;
    };

    void applyFilter();
    QString findDesktopExec(const QStringList &candidates, bool *foundOut = nullptr) const;
    void startRemoteServer();
    void handleRemoteConnection();
    QByteArray buildRemoteHtmlPage() const;
    static QString detectLanIpAddress();
    static QString generateQrDataUri(const QString &text);

    QStringList m_appDirs;
    bool m_dryRunLaunch = false;
    QVariantList m_allApps;
    QVariantList m_filteredApps;
    int m_selectedIndex = 0;
    QString m_category = "all";
    QString m_searchQuery;
    QString m_statusMessage;
    QString m_lastLaunchedCommand;
    QString m_remoteUrl;
    QString m_remoteQrDataUri;
    QTcpServer m_remoteServer;
    QFileSystemWatcher m_watcher;
};
