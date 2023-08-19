// header file include
#include "sunshineapps.h"

// system/Qt includes
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

// local includes
#include "shared/loggingcategories.h"

// os-specific includes
#if defined(Q_OS_WIN)
    #include "os/win/regkey.h"
#endif

//---------------------------------------------------------------------------------------------------------------------

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
#endif
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
SunshineApps::SunshineApps(QString filepath)
    : m_filepath{std::move(filepath)}
{
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-cognitive-complexity)
std::optional<std::set<QString>> SunshineApps::load()
{
    QString filepath{m_filepath};
    if (filepath.isEmpty())  // Fallback to places where we could expect the file to exist
    {
#if defined(Q_OS_WIN)
        RegKey reg_key;
        reg_key.open(R"(HKEY_LOCAL_MACHINE\Software\LizardByte\Sunshine)");
        const auto reg_value{reg_key.getValue(/* Default key */)};
        if (!reg_value.isNull())
        {
            filepath = QDir::cleanPath(reg_value.toString() + "/config/apps.json");
        }
#elif defined(Q_OS_LINUX)
        filepath = QDir::cleanPath(getConfigDir() + "/sunshine/apps.json");
#else
    #error OS is not supported!
#endif
    }

    qCDebug(lc::os) << "selected filepath for Sunshine apps:" << filepath;
    if (filepath.isEmpty())
    {
        qCWarning(lc::os) << "filepath for Sunshine apps is empty!";
        return std::nullopt;
    }

    QFile file{filepath};
    if (!file.open(QFile::ReadOnly))
    {
        qCWarning(lc::os) << "file" << filepath << "could not be opened! Reason:" << file.errorString();
        return std::nullopt;
    }

    const auto data{file.readAll()};

    QJsonParseError     parser_error;
    const QJsonDocument json_data{QJsonDocument::fromJson(data, &parser_error)};
    if (json_data.isNull())
    {
        qCWarning(lc::os) << "failed to decode JSON data! Reason:" << parser_error.errorString() << "| data:" << data;
        return std::nullopt;
    }

    qCDebug(lc::os).noquote() << "Sunshine apps file content:" << Qt::endl << json_data.toJson(QJsonDocument::Indented);
    if (json_data.isObject())
    {
        const auto json_object = json_data.object();
        const auto apps_it{json_object.find("apps")};
        if (apps_it != json_object.end() && apps_it->isArray())
        {
            std::set<QString> parsed_apps{};

            const auto apps = apps_it->toArray();
            if (apps.isEmpty())
            {
                qCDebug(lc::os) << "there are no Sunshine apps to parse.";
                return parsed_apps;
            }

            for (const auto& app : apps)
            {
                if (!app.isObject())
                {
                    qCDebug(lc::os) << "skipping entry as it's not an object:" << app;
                    continue;
                }

                const auto app_object = app.toObject();
                const auto name_it{app_object.find("name")};
                if (name_it == app_object.end())
                {
                    qCDebug(lc::os) << "skipping entry as it does not contain \"name\" field:" << app_object;
                    continue;
                }

                if (!name_it->isString())
                {
                    qCDebug(lc::os) << "skipping entry as the \"name\" field does not contain a string:" << *name_it;
                    continue;
                }

                parsed_apps.insert(name_it->toString());
            }

            qCDebug(lc::os) << "parsed the following Sunshine apps:"
                            << QSet<QString>{std::begin(parsed_apps), std::end(parsed_apps)};
            return parsed_apps;
        }
    }

    qCWarning(lc::os) << "file" << m_filepath << "could not be parsed!";
    return std::nullopt;
}
}  // namespace os