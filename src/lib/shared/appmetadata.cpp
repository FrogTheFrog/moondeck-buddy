// header file include
#include "shared/appmetadata.h"

// system/Qt includes
#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>

// local includes
#include "shared/loggingcategories.h"

namespace
{
#if defined(Q_OS_LINUX)
QString getConfigDir()
{
    const auto xdg_config_env = qgetenv("XDG_CONFIG_HOME");
    if (!xdg_config_env.isEmpty())
    {
        const QDir xdg_cnnfig_dir{xdg_config_env};
        if (xdg_cnnfig_dir.exists())
        {
            return xdg_cnnfig_dir.absolutePath();
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
                           qCDebug(lc::shared) << "getAppName() >> " << getAppName();
                           qCDebug(lc::shared) << "getLogDir() >> " << getLogDir();
                           qCDebug(lc::shared) << "getLogName() >> " << getLogName();
                           qCDebug(lc::shared) << "getLogPath() >> " << getLogPath();
                           qCDebug(lc::shared) << "getSettingsDir() >> " << getSettingsDir();
                           qCDebug(lc::shared) << "getSettingsName() >> " << getSettingsName();
                           qCDebug(lc::shared) << "getSettingsPath() >> " << getSettingsPath();
                           qCDebug(lc::shared) << "getAutoStartDir() >> " << getAutoStartDir();
                           qCDebug(lc::shared) << "getAutoStartPath() >> " << getAutoStartPath();
                           qCDebug(lc::shared) << "getAutoStartExec() >> " << getAutoStartExec();
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
QString AppMetadata::getAutoStartDir() const
{
#if defined(Q_OS_WIN)
    Q_ASSERT(QCoreApplication::instance() != nullptr);
    return QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation) + "/Startup");
#elif defined(Q_OS_LINUX)
    return QDir::cleanPath(getConfigDir() + "/autostart");
#else
    #error OS is not supported!
#endif
}

QString AppMetadata::getAutoStartName() const
{
#if defined(Q_OS_WIN)
    return getAppName() + ".lnk";
#elif defined(Q_OS_LINUX)
    return getAppName().toLower() + ".desktop";
#else
    #error OS is not supported!
#endif
}

QString AppMetadata::getAutoStartPath() const
{
    return QDir::cleanPath(getAutoStartDir() + "/" + getAutoStartName());
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
}  // namespace shared