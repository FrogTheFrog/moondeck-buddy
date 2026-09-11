#include "steam/nonsteamprocesstarget.h"
#include "os/processhandler.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSettings>
#include <QUrl>
#include <QUrlQuery>

namespace
{
#if defined(Q_OS_WIN)
QString unquote(QString value)
{
    value = value.trimmed();
    if (value.startsWith('"') && value.endsWith('"'))
    {
        value = value.mid(1, value.size() - 2);
    }
    return QDir::cleanPath(QDir::fromNativeSeparators(value));
}

std::optional<QString> epicExecutable(QString launch_options)
{
    launch_options = launch_options.trimmed();
    if (launch_options.startsWith('"') && launch_options.endsWith('"'))
    {
        launch_options = launch_options.mid(1, launch_options.size() - 2);
    }
    const QUrl url{launch_options};
    if (!url.isValid() || url.scheme() != "com.epicgames.launcher" || url.host() != "apps"
        || QUrlQuery(url).queryItemValue("action") != "launch")
    {
        return std::nullopt;
    }
    const auto id{url.path(QUrl::FullyDecoded).mid(1)};
    if (id.isEmpty() || id.contains('/'))
    {
        return std::nullopt;
    }
    const auto program_data{qEnvironmentVariable("PROGRAMDATA")};
    if (program_data.isEmpty())
    {
        return std::nullopt;
    }
    const QDir             manifests{program_data + "/Epic/EpicGamesLauncher/Data/Manifests"};
    std::optional<QString> result;
    for (const auto& name : manifests.entryList({"*.item"}, QDir::Files))
    {
        QFile file{manifests.filePath(name)};
        if (!file.open(QIODevice::ReadOnly))
        {
            continue;
        }
        const auto manifest{QJsonDocument::fromJson(file.readAll()).object()};
        const auto app_name{manifest.value("AppName").toString()};
        const auto full_id{manifest.value("CatalogNamespace").toString() + ':'
                           + manifest.value("CatalogItemId").toString() + ':' + app_name};
        if (app_name.isEmpty() || (id != app_name && id != full_id))
        {
            continue;
        }
        const auto      directory{unquote(manifest.value("InstallLocation").toString())};
        const auto      relative_exe{manifest.value("LaunchExecutable").toString()};
        const QFileInfo root{directory};
        const QFileInfo executable{QDir(directory).filePath(relative_exe)};
        const auto      root_path{root.canonicalFilePath()};
        const auto      exe_path{executable.canonicalFilePath()};
        if (!root.isAbsolute() || !root.isDir() || QDir(directory).isRoot() || relative_exe.isEmpty()
            || !QDir::isRelativePath(relative_exe) || !executable.isFile()
            || !exe_path.startsWith(root_path + '/', Qt::CaseInsensitive))
        {
            return std::nullopt;
        }
        if (result && result->compare(exe_path, Qt::CaseInsensitive) != 0)
        {
            return std::nullopt;
        }
        result = exe_path;
    }
    return result;
}
#endif
}  // namespace
namespace steam
{
std::optional<NonSteamProcessTarget> NonSteamProcessTarget::fromShortcut(const ShortcutsVdfEntry& shortcut)
{
#if defined(Q_OS_WIN)
    const auto executable{unquote(shortcut.m_executable)};
    const auto filename{QFileInfo(executable).fileName().toLower()};
    if (filename == "explorer.exe")
    {
        const QRegularExpression pattern{R"(^\s*"?shell:AppsFolder\\([^!"\s]+)![^"\s]+"?\s*$)",
                                         QRegularExpression::CaseInsensitiveOption};
        const auto               match{pattern.match(shortcut.m_launch_options)};
        if (match.hasMatch())
        {
            return NonSteamProcessTarget{Kind::Package, match.captured(1)};
        }
        return std::nullopt;
    }
    if (filename == "ubisoftconnect.exe" || filename == "upc.exe")
    {
        const QRegularExpression pattern{R"(^uplay://launch/(\d+)/\d+$)", QRegularExpression::CaseInsensitiveOption};
        const auto               match{pattern.match(shortcut.m_launch_options.trimmed())};
        if (match.hasMatch())
        {
            const QSettings installs{
                QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Ubisoft\\Launcher\\Installs\\")
                    + match.captured(1),
                QSettings::NativeFormat};
            const auto directory{unquote(installs.value("InstallDir").toString())};
            if (QDir(directory).exists() && !QDir(directory).isRoot() && directory != ".")
            {
                return NonSteamProcessTarget{Kind::Directory, directory};
            }
        }
        return std::nullopt;
    }
    if (filename == "epicgameslauncher.exe")
    {
        if (const auto executable_path{epicExecutable(shortcut.m_launch_options)})
        {
            return NonSteamProcessTarget{Kind::Executable, *executable_path};
        }
        return std::nullopt;
    }
    // Shells and shared launchers cannot identify an individual game.
    if (filename == "steam.exe" || filename == "cmd.exe" || filename == "powershell.exe" || filename == "pwsh.exe"
        || filename == "epicgameslauncher.exe" || filename == "galaxyclient.exe")
    {
        return std::nullopt;
    }
    if (QFileInfo(executable).isAbsolute() && QFileInfo::exists(executable))
    {
        return NonSteamProcessTarget{Kind::Executable, executable};
    }
#else
    Q_UNUSED(shortcut);
#endif
    return std::nullopt;
}

std::map<uint, QDateTime> NonSteamProcessTarget::getProcesses() const
{
    os::ProcessHandler        processes;
    std::map<uint, QDateTime> result;
    for (const auto pid : processes.getPids())
    {
        if (static_cast<qint64>(pid) == QCoreApplication::applicationPid() || !processes.isInCurrentSession(pid))
        {
            continue;
        }
        const auto path{QDir::fromNativeSeparators(processes.getExecPath(pid))};
        const auto filename{QFileInfo(path).fileName().toLower()};
        if (filename == "gamelaunchhelper.exe" || filename.contains("crashhandler") || filename.contains("crashreport"))
        {
            continue;
        }
        bool matches{false};
        switch (m_kind)
        {
            case Kind::Executable:
                matches = path.compare(m_value, Qt::CaseInsensitive) == 0;
                break;
            case Kind::Directory:
                matches = path.startsWith(m_value + '/', Qt::CaseInsensitive);
                break;
            case Kind::Package:
                matches = processes.getPackageFamilyName(pid) == m_value;
                break;
        }
        if (matches)
        {
            const auto started_at{processes.getStartTime(pid)};
            if (started_at.isValid())
            {
                result.emplace(pid, started_at);
            }
        }
    }
    return result;
}
}  // namespace steam
