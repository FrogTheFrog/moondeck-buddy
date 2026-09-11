#include "steam/nonsteamprocesstarget.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition)
    {
        qFatal("%s", message);
    }
}

auto target(const QString& executable, const QString& options = {})
{
    return steam::NonSteamProcessTarget::fromShortcut({steam::AppId{42}, "Test", {}, executable, options});
}
}  // namespace

int main(int argc, char** argv)
{
    QCoreApplication app{argc, argv};
#if defined(Q_OS_WIN)
    using Kind = steam::NonSteamProcessTarget::Kind;
    const auto package{target("C:/Windows/explorer.exe", "shell:AppsFolder\\Publisher.Game_123!Game")};
    require(package && package->m_kind == Kind::Package && package->m_value == "Publisher.Game_123",
            "Game Pass shortcut should resolve to its package family");
    require(!target("C:/Windows/explorer.exe", "shell:AppsFolder\\Publisher.Game_123!Game extra"),
            "Trailing shell arguments must not be accepted");
    require(!target("C:/Windows/explorer.exe", "C:/Games"), "Explorer itself must never become a game target");
    for (const QString executable : {"steam.exe", "cmd.exe", "powershell.exe", "pwsh.exe", "epicgameslauncher.exe",
                                     "galaxyclient.exe", "upc.exe", "ubisoftconnect.exe"})
    {
        require(!target(executable), "A shared launcher must not become an executable target");
    }

    const auto self{target('"' + QCoreApplication::applicationFilePath() + '"')};
    require(self && self->m_kind == Kind::Executable, "An existing absolute executable should resolve");
    require(self->getProcesses().empty(), "Buddy's own process must be excluded from targets");
    require(!target("missing-game.exe"), "A relative executable should not resolve");

    QTemporaryDir temp;
    require(temp.isValid(), "Could not create isolated manifest fixture");
    qputenv("PROGRAMDATA", temp.path().toUtf8());
    const auto manifests{temp.path() + "/Epic/EpicGamesLauncher/Data/Manifests"};
    const auto game_dir{temp.path() + "/Game"};
    require(QDir{}.mkpath(manifests) && QDir{}.mkpath(game_dir), "Could not create fixture directories");
    QFile game{game_dir + "/Game.exe"};
    require(game.open(QIODevice::WriteOnly), "Could not create fixture executable");
    game.close();
    QJsonObject manifest{{"AppName", "app"},
                         {"CatalogNamespace", "namespace"},
                         {"CatalogItemId", "item"},
                         {"InstallLocation", game_dir},
                         {"LaunchExecutable", "Game.exe"}};
    const auto  write_manifest = [&manifests, &manifest]()
    {
        QFile file{manifests + "/game.item"};
        require(file.open(QIODevice::WriteOnly | QIODevice::Truncate), "Could not write fixture manifest");
        require(file.write(QJsonDocument{manifest}.toJson()) > 0, "Could not serialize fixture manifest");
    };
    write_manifest();
    const QString full_url{"com.epicgames.launcher://apps/namespace%3Aitem%3Aapp?action=launch&silent=true"};
    const auto    epic{target("EpicGamesLauncher.exe", full_url)};
    require(epic && epic->m_kind == Kind::Executable && epic->m_value == QFileInfo{game.fileName()}.canonicalFilePath(),
            "Full Steam Ahead Epic URI should resolve to the manifest executable");
    require(target("EpicGamesLauncher.exe", "com.epicgames.launcher://apps/app?action=launch").has_value(),
            "Epic's short app URI should resolve when unambiguous");
    require(!target("EpicGamesLauncher.exe", "com.epicgames.launcher://apps/other?action=launch"),
            "An unrelated manifest must not match");
    require(!target("EpicGamesLauncher.exe", "com.epicgames.launcher://apps/app?action=uninstall"),
            "Only launch URIs should resolve");
    manifest["LaunchExecutable"] = "../outside.exe";
    QFile outside{temp.path() + "/outside.exe"};
    require(outside.open(QIODevice::WriteOnly), "Could not create traversal fixture");
    outside.close();
    write_manifest();
    require(!target("EpicGamesLauncher.exe", full_url),
            "A manifest executable outside its install directory is unsafe");
    qInfo("PASS: package identity, shared launcher exclusion, self exclusion, Epic metadata and path confinement");
#else
    require(!target("/bin/sh"), "Windows non-Steam tracking must not activate on Linux");
    qInfo("PASS: Windows-only non-Steam target resolution stays disabled");
#endif
    return 0;
}
