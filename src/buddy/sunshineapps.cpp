// header file include
#include "sunshineapps.h"

// system/Qt includes
#include <QDir>
#include <QSettings>

// local includes
#include "common/loggingcategories.h"
#include "json/json.h"

namespace
{
#ifdef Q_OS_LINUX
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
#endif
}  // namespace

namespace ext_linkage_for_glaze
{
struct AppsJson
{
    struct AppsEntry
    {
        std::optional<QString> name;
    };

    std::optional<std::vector<AppsEntry>> apps;
};
}  // namespace ext_linkage_for_glaze

SunshineApps::SunshineApps(QString filepath)
    : m_filepath{std::move(filepath)}
{
}

std::optional<std::set<QString>> SunshineApps::load()
{
    QString filepath{m_filepath};
    if (filepath.isEmpty())  // Fallback to places where we could expect the file to exist
    {
#ifdef Q_OS_WIN
        const QSettings settings(R"(HKEY_LOCAL_MACHINE\Software\LizardByte\Sunshine)", QSettings::NativeFormat);
        filepath = settings.value("Default").toString();
        if (!filepath.isEmpty())
        {
            filepath = QDir::cleanPath(filepath + "/config/apps.json");
        }
#elifdef Q_OS_LINUX
        filepath = QDir::cleanPath(getConfigDir() + "/sunshine/apps.json");
#else
    #error OS is not supported!
#endif
    }

    qCDebug(lc::buddyMain) << "Selected filepath for Sunshine apps:" << filepath;
    if (filepath.isEmpty())
    {
        qCWarning(lc::buddyMain) << "Filepath for Sunshine apps is empty!";
        return std::nullopt;
    }

    const auto [apps]{json::tryPartialReadFromFile<ext_linkage_for_glaze::AppsJson>(filepath)};
    if (!apps)
    {
        qCWarning(lc::buddyMain) << "Sunshine apps file contains no apps!";
        return std::nullopt;
    }

    std::set<QString> parsed_apps{};
    for (const auto& [name] : *apps)
    {
        if (!name || name->isEmpty())
        {
            continue;
        }

        parsed_apps.emplace(*name);
    }

    return parsed_apps;
}