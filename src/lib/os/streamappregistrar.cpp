// header file include
#include "os/streamappregistrar.h"

// system/Qt includes
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>
#include <algorithm>
#include <optional>

// local includes
#include "common/appmetadata.h"
#include "common/loggingcategories.h"

namespace
{
QStringList getConfigDirs()
{
#if defined(Q_OS_WIN)
    // Vibepollo installs into the same directory as Apollo
    const QDir program_files{qEnvironmentVariable("ProgramFiles", "C:/Program Files")};
    return {program_files.filePath("Sunshine/config"), program_files.filePath("Apollo/config")};
#elif defined(Q_OS_LINUX)
    // Apollo and Vibepollo use the same directory as Sunshine
    const QDir config_dir{QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)};
    return {config_dir.filePath("sunshine"),
            QDir::home().filePath(".var/app/dev.lizardbyte.app.Sunshine/config/sunshine")};
#else
    #error OS is not supported!
#endif
}

QString getAppsFile(const QString& config_dir)
{
    // The path can be overridden in the main config, where relative paths start from the config dir
    QString apps_file{"apps.json"};

    QFile config_file{QDir{config_dir}.filePath("sunshine.conf")};
    if (config_file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        while (!config_file.atEnd())
        {
            const auto line{QString::fromUtf8(config_file.readLine()).trimmed()};
            const auto separator{line.indexOf('=')};
            if (line.startsWith('#') || separator < 0 || line.left(separator).trimmed() != "file_apps")
            {
                continue;
            }

            // Only the first occurrence is used by Sunshine
            if (const auto value{line.mid(separator + 1).trimmed()}; !value.isEmpty())
            {
                apps_file = value;
            }
            break;
        }
    }

    return QDir::cleanPath(QDir{config_dir}.absoluteFilePath(apps_file));
}

std::optional<QJsonObject> readAppsFile(const QString& apps_file)
{
    QFile file{apps_file};
    if (!file.open(QIODevice::ReadOnly))
    {
        qCWarning(lc::os) << "Failed to open" << apps_file;
        return std::nullopt;
    }

    QJsonParseError error;
    const auto      doc{QJsonDocument::fromJson(file.readAll(), &error)};
    if (error.error != QJsonParseError::NoError || !doc.isObject())
    {
        qCWarning(lc::os) << "Failed to parse" << apps_file << "-" << error.errorString();
        return std::nullopt;
    }

    return doc.object();
}

bool isStreamApp(const QJsonValue& app, const QString& stream_name)
{
    // The name can be changed in MoonDeck, so only the command is reliable
    return app.toObject().value("cmd").toString().contains(stream_name);
}

QString getStreamCommand(const common::AppMetadata& app_meta, const QString& apps_file)
{
    auto args{app_meta.getStreamExec()};
#if defined(Q_OS_LINUX)
    // Sunshine running as a flatpak has to escape the sandbox to start Stream
    if (apps_file.startsWith(QDir::home().filePath(".var/app/")))
    {
        args = QStringList{"flatpak-spawn", "--host", "--"} + args;
    }
#else
    Q_UNUSED(apps_file);
#endif

    for (auto& arg : args)
    {
        if (arg.contains(' '))
        {
            arg = '"' + arg + '"';
        }
    }

    return args.join(' ');
}
}  // namespace

namespace os
{
StreamAppRegistrar::StreamAppRegistrar(const common::AppMetadata& app_meta)
    : m_app_meta{app_meta}
{
}

QStringList StreamAppRegistrar::findAppsFiles()
{
    QStringList apps_files;
    for (const auto& config_dir : getConfigDirs())
    {
        if (const auto apps_file{getAppsFile(config_dir)}; QFile::exists(apps_file) && !apps_files.contains(apps_file))
        {
            apps_files.append(apps_file);
        }
    }

    return apps_files;
}

bool StreamAppRegistrar::isRegistered(const QString& apps_file) const
{
    const auto root{readAppsFile(apps_file)};
    if (!root)
    {
        return false;
    }

    // Braces would wrap the array in another array
    const auto apps = root->value("apps").toArray();
    const auto stream_name{m_app_meta.getAppName(common::AppMetadata::App::Stream)};
    return std::any_of(apps.cbegin(), apps.cend(),
                       [&stream_name](const QJsonValue& app) { return isStreamApp(app, stream_name); });
}

bool StreamAppRegistrar::registerApp(const QString& apps_file) const
{
    auto root{readAppsFile(apps_file)};
    if (!root)
    {
        return false;
    }

    // Braces would wrap the array in another array
    const auto original_apps = root->value("apps").toArray();
    const auto stream_name{m_app_meta.getAppName(common::AppMetadata::App::Stream)};
    const auto command{getStreamCommand(m_app_meta, apps_file)};

    // Apollo and Vibepollo fail to load apps without a UUID, while Sunshine has no such thing
    const bool needs_uuid{root->contains("version")
                          || std::any_of(original_apps.cbegin(), original_apps.cend(),
                                         [](const QJsonValue& app) { return app.toObject().contains("uuid"); })};

    const auto update_app{[&](QJsonObject app)
                          {
                              app["cmd"] = command;
                              // Otherwise the stream is not ended when Stream exits
                              app["auto-detach"] = false;
                              if (needs_uuid && app.value("uuid").toString().isEmpty())
                              {
                                  app["uuid"] = QUuid::createUuid().toString(QUuid::WithoutBraces).toUpper();
                              }
                              return app;
                          }};

    auto apps = original_apps;
    bool found{false};
    for (qsizetype i = 0; i < apps.size(); ++i)
    {
        if (isStreamApp(apps[i], stream_name))
        {
            found   = true;
            apps[i] = update_app(apps[i].toObject());
        }
    }

    if (!found)
    {
        apps.append(update_app(QJsonObject{{"name", stream_name}}));
    }

    if (apps == original_apps)
    {
        qCInfo(lc::os) << stream_name << "is already up to date in" << apps_file;
        return true;
    }

    // Keep the previous version around, as the file belongs to another app
    const auto backup_file{apps_file + ".moondeck.bak"};
    QFile::remove(backup_file);
    if (!QFile::copy(apps_file, backup_file))
    {
        qCWarning(lc::os) << "Failed to back up" << apps_file;
        return false;
    }

    root->insert("apps", apps);
    const auto data{QJsonDocument{*root}.toJson()};

    QSaveFile file{apps_file};
    // The config dir might only allow changing existing files
    file.setDirectWriteFallback(true);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit())
    {
        qCWarning(lc::os) << "Failed to write" << apps_file;
        return false;
    }

    qCInfo(lc::os) << (found ? "Updated" : "Added") << stream_name << "in" << apps_file;
    if (needs_uuid)
    {
        qCInfo(lc::os) << "Apollo and Vibepollo only load new apps on restart, please restart it.";
    }

    return true;
}
}  // namespace os
