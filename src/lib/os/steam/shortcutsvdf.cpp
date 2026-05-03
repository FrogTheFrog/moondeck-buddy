// class header include
#include "os/steam/shortcutsvdf.h"

// system/Qt includes
#include <QFile>

// local includes
#include "shared/loggingcategories.h"

namespace
{
qsizetype indexOfInsensitive(const QByteArray& data, const QByteArrayView view, qsizetype from)
{
    const auto needle{view.at(0)};
    while (true)
    {
        from = data.indexOf(needle, from);
        if (from == -1)
        {
            break;
        }

        if (const char* needle_ptr{data.data() + from};
            qstrnicmp(needle_ptr, view.data(), qMin(data.size() - from, view.size())) == 0)
        {
            break;
        }
        ++from;
    }

    return from;
}

std::optional<std::vector<std::uint32_t>> scrapeAppIds(const QByteArray& contents)
{
    std::vector<std::uint32_t> app_ids;
    {
        constexpr QByteArrayView app_id_view("\x02"
                                             "appid"
                                             "\x00");
        qsizetype                from{0};
        while (true)
        {
            from = indexOfInsensitive(contents, app_id_view, from);
            if (from == -1)
            {
                break;
            }
            from += app_id_view.size() + 1;

            if (from + 4 > contents.size())
            {
                qCWarning(lc::os) << "Out of range error while scraping shortcuts.vdf for appid!";
                return std::nullopt;
            }

            const std::uint32_t app_id{
                static_cast<std::uint32_t>(static_cast<std::uint8_t>(contents.at(from)))
                | static_cast<std::uint32_t>(static_cast<std::uint8_t>(contents.at(from + 1))) << 8U
                | static_cast<std::uint32_t>(static_cast<std::uint8_t>(contents.at(from + 2))) << 16U
                | static_cast<std::uint32_t>(static_cast<std::uint8_t>(contents.at(from + 3))) << 24U};
            app_ids.emplace_back(app_id);
        }
    }
    return app_ids;
}

std::optional<std::vector<QString>> scrapeAppNames(const QByteArray& contents)
{
    std::vector<QString> app_names;
    {
        constexpr QByteArrayView app_name_view("\x01"
                                               "appname"
                                               "\x00");
        qsizetype                from{0};
        while (true)
        {
            from = indexOfInsensitive(contents, app_name_view, from);
            if (from == -1)
            {
                break;
            }
            from += app_name_view.size() + 1;

            const auto to = contents.indexOf('\0', from);
            if (to == -1)
            {
                qCWarning(lc::os) << "Out of range error while scraping shortcuts.vdf for appname!";
                return std::nullopt;
            }

            app_names.emplace_back(QString::fromUtf8(contents.data() + from, to - from));
        }
    }
    return app_names;
}

std::optional<std::vector<QString>> scrapeStartDirs(const QByteArray& contents)
{
    std::vector<QString> app_names;
    {
        constexpr QByteArrayView app_name_view("\x01"
                                               "startdir"
                                               "\x00");
        qsizetype                from{0};
        while (true)
        {
            from = indexOfInsensitive(contents, app_name_view, from);
            if (from == -1)
            {
                break;
            }
            from += app_name_view.size() + 1;

            const auto to = contents.indexOf('\0', from);
            if (to == -1)
            {
                qCWarning(lc::os) << "Out of range error while scraping shortcuts.vdf for start dirs!";
                return std::nullopt;
            }

            app_names.emplace_back(QString::fromUtf8(contents.data() + from, to - from));
        }
    }
    return app_names;
}
}  // namespace

namespace os
{
// This is a very "son, we have a parser at home" kind of parser, very basic, but gets the job done...
std::optional<std::vector<ShortcutsVdfEntry>> ShortcutsVdfEntry::scrapeShortcutsVdf(const QByteArray& contents)
{
    const auto app_ids{scrapeAppIds(contents)};
    const auto app_names{scrapeAppNames(contents)};
    const auto start_dirs{scrapeStartDirs(contents)};

    if (!app_ids || !app_names || !start_dirs)
    {
        return std::nullopt;
    }

    if (app_names->size() != app_ids->size() || app_ids->size() != start_dirs->size())
    {
        qCWarning(lc::os) << "Failed to scrape shortcuts.vdf - list size mismatch!";
        return std::nullopt;
    }

    std::vector<ShortcutsVdfEntry> data;
    for (std::size_t i{0}; i < app_ids->size(); ++i)
    {
        data.emplace_back(shared::AppId{(*app_ids)[i]}, (*app_names)[i], (*start_dirs)[i]);
    }

    return data;
}

std::optional<std::vector<ShortcutsVdfEntry>>
    ShortcutsVdfEntry::scrapeShortcutsVdf(const std::filesystem::path& steam_dir, const shared::SteamId& user_id)
{
    if (steam_dir.empty())
    {
        qCWarning(lc::os) << "Steam directory is not available yet!";
        return std::nullopt;
    }

    const auto shortcuts_file{steam_dir / "userdata" / user_id.toSteamId32().toStdString() / "config"
                              / "shortcuts.vdf"};
    qDebug(lc::os) << "Mapped user id to shortcuts file:" << user_id.toSteamId64() << "->"
                   << shortcuts_file.generic_string();

    QFile file{shortcuts_file};
    if (!file.exists())
    {
        qCWarning(lc::os) << "file" << shortcuts_file.generic_string() << "does not exist!";
        return std::nullopt;
    }

    if (!file.open(QIODevice::ReadOnly))
    {
        qCWarning(lc::os) << "file" << shortcuts_file.generic_string() << "could not be opened!";
        return std::nullopt;
    }

    return scrapeShortcutsVdf(file.readAll());
}
}  // namespace os
