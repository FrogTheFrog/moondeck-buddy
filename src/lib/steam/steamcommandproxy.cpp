// header file include
#include "steam/steamcommandproxy.h"

// system/Qt includes
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>

// local includes
#include "common/loggingcategories.h"
#include "utils/appsettings.h"

namespace
{
bool setupProcess(const QString& exec, const QStringList& args, const QMap<QString, QString>& env_overrides,
                  QProcess& process)
{
    if (exec.isEmpty())
    {
        return false;
    }

    process.setProgram(exec);
    process.setArguments(args);

    if (!env_overrides.empty())
    {
        auto env{QProcessEnvironment::systemEnvironment()};
        for (const auto& [key, value] : env_overrides.asKeyValueRange())
        {
            env.insert(key, value);
        }

        process.setProcessEnvironment(env);
    }

    qCInfo(lc::steam).nospace() << (env_overrides.empty() ? "" : "[WITH ENV OVERRIDES] ") << "Executing: " << exec
                             << " with args: " << args;
    return true;
}

#if defined(Q_OS_LINUX)
std::optional<QString> execute(const QString& exec, const QStringList& args,
                               const QMap<QString, QString>& env_overrides = {})
{
    QProcess process;
    if (!setupProcess(exec, args, env_overrides, process))
    {
        return std::nullopt;
    }

    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start();
    if (!process.waitForStarted())
    {
        qCWarning(lc::steam) << "waitForStarted() failed with" << process.errorString();
        return std::nullopt;
    }

    process.waitForFinished();
    if (process.state() != QProcess::NotRunning)
    {
        qCWarning(lc::steam) << "waitForFinished() timed out - killing executable!";
        process.close();
        return std::nullopt;
    }

    if (const auto output{process.readAllStandardError().trimmed()}; !output.isEmpty())
    {
        qCInfo(lc::steam).noquote() << "Captured stderr:\n-----\n" << QString{output} << "\n-----";
    }

    if (const auto exit_code{process.exitCode()}; exit_code != 0)
    {
        qCWarning(lc::steam) << "Executable exited with error code" << exit_code;
        return std::nullopt;
    }

    if (auto output{process.readAllStandardOutput().trimmed()}; !output.isEmpty())
    {
        qCDebug(lc::steam).noquote() << "Captured stdout:\n-----\n" << QString{output} << "\n-----";
        return output;
    }

    return QString{};
}
#endif

bool executeDetached(const QString& exec, const QStringList& args, const QMap<QString, QString>& env_overrides = {})
{
    QProcess process;
    if (!setupProcess(exec, args, env_overrides, process))
    {
        return false;
    }

    process.setStandardOutputFile(QProcess::nullDevice());
    process.setStandardErrorFile(QProcess::nullDevice());

    return process.startDetached();
}

QStringList findSteamExecutable()
{
#if defined(Q_OS_WIN)
    const QSettings settings(R"(HKEY_CURRENT_USER\Software\Valve\Steam)", QSettings::NativeFormat);
    if (const auto exec{settings.value("SteamExe").toString()}; !exec.isEmpty())
    {
        return {exec};
    }
    return {};
#elif defined(Q_OS_LINUX)
    if (const auto steam_bin{execute("which", {"steam"})}; steam_bin && !steam_bin->isEmpty())
    {
        return {*steam_bin};
    }
    if (const auto flatpak_bin{execute("which", {"flatpak"})}; flatpak_bin && !flatpak_bin->isEmpty())
    {
        if (execute(*flatpak_bin, {"info", "com.valvesoftware.Steam"}))
        {
            return {*flatpak_bin, "run", "com.valvesoftware.Steam"};
        }
    }
    return {};
#else
    #error OS is not supported!
#endif
}

QStringList getSteamExecutableWithArgs(const utils::AppSettings& app_settings)
{
    if (const auto& exec{app_settings.getSteamExecutablePath()}; !exec.isEmpty())
    {
        if (QFileInfo::exists(exec))
        {
            return {exec};
        }

        // Executable was set, there is no need to look for fallbacks.
        return {};
    }

    return findSteamExecutable();
}

bool executeSteamCommand(const utils::AppSettings& app_settings, const QStringList& steam_args,
                         const QMap<QString, QString>& env_overrides = {})
{
    auto exec_with_args{getSteamExecutableWithArgs(app_settings)};
    if (exec_with_args.isEmpty())
    {
        return false;
    }

    const auto exec{exec_with_args.takeFirst()};
    return executeDetached(exec, exec_with_args + steam_args, env_overrides);
}
}  // namespace

namespace steam
{
SteamCommandProxy::SteamCommandProxy(const utils::AppSettings& app_settings)
    : m_app_settings{app_settings}
{
}

bool SteamCommandProxy::canExecuteCommands() const
{
    return !getSteamExecutableWithArgs(m_app_settings).isEmpty();
}

bool SteamCommandProxy::launchSteam(const bool big_picture_mode, const QMap<QString, QString>& env_overrides)
{
    return executeSteamCommand(
        m_app_settings, big_picture_mode ? QStringList{"steam://open/bigpicture"} : QStringList{}, env_overrides);
}

bool SteamCommandProxy::launchApp(const AppId& app_id, const QMap<QString, QString>& env_overrides)
{
    return executeSteamCommand(m_app_settings,
                               app_id.isGameId()
                                   ? QStringList{"steam://rungameid/" + QString::number(app_id.getId())}
                                   : QStringList{"steam://launch/" + QString::number(app_id.getId()) + "/dialog"},
                               env_overrides);
}

bool SteamCommandProxy::close()
{
    return executeSteamCommand(m_app_settings, {"steam://exit"});
}

bool SteamCommandProxy::closeBigPictureMode()
{
    return executeSteamCommand(m_app_settings, {"steam://close/bigpicture"});
}
}  // namespace steam
