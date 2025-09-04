// header file include
#include "shared/appmetadata.h"

// system/Qt includes
#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>

// local includes
#include "shared/enums.h"
#include "shared/loggingcategories.h"

namespace
{
#if defined(Q_OS_LINUX)
QString getConfigDir()
{
    const auto xdg_config_env = qgetenv("XDG_CONFIG_HOME");
    if (!xdg_config_env.isEmpty())
    {
        const QDir xdg_config_dir{xdg_config_env};
        if (xdg_config_dir.exists())
        {
            return xdg_config_dir.absolutePath();
        }
    }

    return QDir::cleanPath(QDir::homePath() + "/.config");
}

QString getAppFilePath()
{
    const auto app_image_env = qgetenv("APPIMAGE");
    if (!app_image_env.isEmpty())
    {
        return QString{app_image_env};
    }

    return QCoreApplication::applicationFilePath();
}
#endif

bool isDisplayAvailable()
{
#if defined(Q_OS_WIN)
    return true;
#elif defined(Q_OS_LINUX)
    const auto qt_platform_env     = qgetenv("QT_QPA_PLATFORM");
    const auto display_env         = qgetenv("DISPLAY");
    const auto wayland_display_env = qgetenv("WAYLAND_DISPLAY");
    return qt_platform_env.toLower().contains("wayland") ? !wayland_display_env.isEmpty() : !display_env.isEmpty();
#else
    #error OS is not supported!
#endif
}
}  // namespace

namespace shared
{
AppMetadata::AppMetadata(App app)
    : m_current_app{app}
{
    // Delay logging until application start
    QTimer::singleShot(0, this,
                       [this]()
                       {
                           qCDebug(lc::shared) << "getAppName() >>" << getAppName();
                           qCDebug(lc::shared) << "getLogDir() >>" << getLogDir();
                           qCDebug(lc::shared) << "getLogName() >>" << getLogName();
                           qCDebug(lc::shared) << "getLogPath() >>" << getLogPath();
                           qCDebug(lc::shared) << "getSettingsDir() >>" << getSettingsDir();
                           qCDebug(lc::shared) << "getSettingsName() >>" << getSettingsName();
                           qCDebug(lc::shared) << "getSettingsPath() >>" << getSettingsPath();

                           for (const auto& value : enums::qEnumValues<AutoStartDelegation>())
                           {
                               qCDebug(lc::shared).nospace() << "getAutoStartDir(" << enums::qEnumToString(value)
                                                             << ") >> " << getAutoStartDir(value);
                               qCDebug(lc::shared).nospace() << "getAutoStartName(" << enums::qEnumToString(value)
                                                             << ") >> " << getAutoStartName(value);
                               qCDebug(lc::shared).nospace() << "getAutoStartPath(" << enums::qEnumToString(value)
                                                             << ") >> " << getAutoStartPath(value);
                           }

                           qCDebug(lc::shared) << "getAutoStartExec() >>" << getAutoStartExec();
                           qCDebug(lc::shared) << "getDefaultSteamExecutable() >>" << getDefaultSteamExecutable();
                           qCDebug(lc::shared) << "getSharedEnvRegexKey() >>" << getSharedEnvRegexKey();
                           qCDebug(lc::shared) << "getSharedEnvMapKey() >>" << getSharedEnvMapKey();
                           qCDebug(lc::shared) << "isGuiEnabled() >>" << isGuiEnabled();
                       });
}

QString AppMetadata::getAppName() const
{
    return getAppName(m_current_app);
}

// NOLINTNEXTLINE(*-static)
QString AppMetadata::getAppName(App app) const
{
    switch (app)
    {
        case App::Buddy:
            return QStringLiteral("MoonDeckBuddy");
        case App::Stream:
            return QStringLiteral("MoonDeckStream");
    }

    Q_ASSERT(false);
    return {};
}

// NOLINTNEXTLINE(*-static)
QString AppMetadata::getLogDir() const
{
#if defined(Q_OS_WIN)
    Q_ASSERT(QCoreApplication::instance() != nullptr);
    return QCoreApplication::applicationDirPath();
#elif defined(Q_OS_LINUX)
    return QStringLiteral("/tmp");
#else
    #error OS is not supported!
#endif
}

QString AppMetadata::getLogName() const
{
    return getAppName().toLower() + QStringLiteral(".log");
}

QString AppMetadata::getLogPath() const
{
    return QDir::cleanPath(getLogDir() + "/" + getLogName());
}

// NOLINTNEXTLINE(*-static)
QString AppMetadata::getSettingsDir() const
{
#if defined(Q_OS_WIN)
    Q_ASSERT(QCoreApplication::instance() != nullptr);
    return QCoreApplication::applicationDirPath();
#elif defined(Q_OS_LINUX)
    return QDir::cleanPath(getConfigDir() + "/" + getAppName(App::Buddy).toLower());
#else
    #error OS is not supported!
#endif
}

// NOLINTNEXTLINE(*-static)
QString AppMetadata::getSettingsName() const
{
    return QStringLiteral("settings.json");
}

QString AppMetadata::getSettingsPath() const
{
    return QDir::cleanPath(getSettingsDir() + "/" + getSettingsName());
}

// NOLINTNEXTLINE(*-static)
QString AppMetadata::getAutoStartDir(const AutoStartDelegation type) const
{
#if defined(Q_OS_WIN)
    Q_UNUSED(type);
    Q_ASSERT(QCoreApplication::instance() != nullptr);
    return QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation) + "/Startup");
#elif defined(Q_OS_LINUX)
    switch (type)
    {
        case AutoStartDelegation::Desktop:
            return QDir::cleanPath(getConfigDir() + "/autostart");
        case AutoStartDelegation::ServiceMain:
        case AutoStartDelegation::ServiceHelper:
            return QDir::cleanPath(getConfigDir() + "/systemd/user");
    }

    Q_UNREACHABLE();
#else
    #error OS is not supported!
#endif
}

QString AppMetadata::getAutoStartName(const AutoStartDelegation type) const
{
#if defined(Q_OS_WIN)
    Q_UNUSED(type);
    return getAppName() + ".lnk";
#elif defined(Q_OS_LINUX)
    switch (type)
    {
        case AutoStartDelegation::Desktop:
            return getAppName().toLower() + ".desktop";
        case AutoStartDelegation::ServiceMain:
            return getAppName().toLower() + ".service";
        case AutoStartDelegation::ServiceHelper:
            return getAppName().toLower() + "-session-restart.service";
    }

    Q_UNREACHABLE();
#else
    #error OS is not supported!
#endif
}

QString AppMetadata::getAutoStartPath(const AutoStartDelegation type) const
{
    return QDir::cleanPath(getAutoStartDir(type) + "/" + getAutoStartName(type));
}

// NOLINTNEXTLINE(*-static)
QString AppMetadata::getAutoStartExec() const
{
    Q_ASSERT(QCoreApplication::instance() != nullptr);
#if defined(Q_OS_WIN)
    return QCoreApplication::applicationFilePath();
#elif defined(Q_OS_LINUX)
    return getAppFilePath();
#else
    #error OS is not supported!
#endif
}

QString AppMetadata::getDefaultSteamExecutable() const
{
#if defined(Q_OS_WIN)
    const QSettings settings(R"(HKEY_CURRENT_USER\Software\Valve\Steam)", QSettings::NativeFormat);
    return settings.value("SteamExe").toString();
#elif defined(Q_OS_LINUX)
    return "/usr/bin/steam";
#else
    #error OS is not supported!
#endif
}

QString AppMetadata::getSharedEnvRegexKey() const
{
    return QStringLiteral("MoonDeck_EnvRegex_Key");
}

QString AppMetadata::getSharedEnvMapKey() const
{
    return QStringLiteral("MoonDeck_EnvMap_Key");
}

bool AppMetadata::isGuiEnabled() const
{
    const auto no_gui_env = qgetenv("NO_GUI");
    if (!no_gui_env.isEmpty())
    {
        const QString env_str{no_gui_env.toLower().trimmed()};
        if (env_str == "auto")
        {
            return isDisplayAvailable();
        }

        return env_str != "1" && env_str != "true";
    }

    return true;
}
}  // namespace shared