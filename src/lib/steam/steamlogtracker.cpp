// header file include
#include "steam/steamlogtracker.h"

// system/Qt includes
#include <QFile>
#include <QRegularExpression>
#include <utility>

// local includes
#include "common/loggingcategories.h"

namespace
{
bool openForReading(QFile& file)
{
    if (!file.exists())
    {
        qCDebug(lc::steam) << "file" << file.fileName() << "does not exist yet.";
        return false;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qCWarning(lc::steam) << "file" << file.fileName() << "could not be opened!";
        return false;
    }

    return true;
}

QDateTime getDateTimeFromLogLine(const QString& line, const steam::SteamLogTracker::TimeFormat time_format)
{
    using enum steam::SteamLogTracker::TimeFormat;

    switch (time_format)
    {
        case YYYY_MM_DD_hh_mm_ss:
        {
            static const QRegularExpression time_regex{R"(^\[(\d{4})-(\d{2})-(\d{2})\s(\d{2}):(\d{2}):(\d{2})\])"};
            if (const auto match{time_regex.match(line)}; match.hasMatch())
            {
                const QDate date{match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt()};
                const QTime time{match.captured(4).toInt(), match.captured(5).toInt(), match.captured(6).toInt()};
                return QDateTime{date, time};
            }
        }
    }

    return QDateTime{};
}

qint64 readAllLines(std::vector<QString>& lines, QFile& file)
{
    QTextStream stream{&file};
    QString     line;

    stream.seek(0);

    while (stream.readLineInto(&line))
    {
        if (!line.isEmpty())
        {
            lines.push_back(line);
        }
    }

    return stream.pos();
}

bool isLineBeforeDatetime(const QString& line, const QDateTime& datetime,
                          const steam::SteamLogTracker::TimeFormat time_format)
{
    const QDateTime logtime{getDateTimeFromLogLine(line, time_format)};
    if (!logtime.isValid())
    {
        return false;
    }

    return logtime < datetime;
}

bool isLineAtOrAfterDatetime(const QString& line, const QDateTime& datetime,
                             const steam::SteamLogTracker::TimeFormat time_format)
{
    const QDateTime logtime{getDateTimeFromLogLine(line, time_format)};
    if (!logtime.isValid())
    {
        return false;
    }

    return logtime >= datetime;
}

void filterLines(std::vector<QString>& lines, QDateTime& datetime, const steam::SteamLogTracker::TimeFormat time_format)
{
    auto line_rit = std::rbegin(lines);
    for (; line_rit != std::rend(lines); ++line_rit)
    {
        if (isLineBeforeDatetime(*line_rit, datetime, time_format))
        {
            break;
        }
    }

    auto line_it = line_rit == std::rend(lines) ? std::begin(lines) : line_rit.base() /* next element */;
    for (; line_it != std::end(lines); ++line_it)
    {
        if (isLineAtOrAfterDatetime(*line_it, datetime, time_format))
        {
            datetime = {};
            break;
        }
    }

    lines.erase(std::begin(lines), line_it);
}

qint64 readRemainingLines(std::vector<QString>& lines, QFile& file, QDateTime& first_entry_time_filter,
                          const steam::SteamLogTracker::TimeFormat time_format, const qint64 start_offset)
{
    QTextStream stream{&file};
    QString     line;

    // Start reading from the last position we've read.
    stream.seek(start_offset);

    while (stream.readLineInto(&line))
    {
        if (!line.isEmpty())
        {
            if (first_entry_time_filter.isValid())
            {
                if (!isLineAtOrAfterDatetime(line, first_entry_time_filter, time_format))
                {
                    continue;
                }

                first_entry_time_filter = {};
            }

            lines.push_back(line);
        }
    }

    return stream.pos();
}

void tryWatchFileNatively(const QString& filename, QFileSystemWatcher& watcher)
{
    // The file needs to be re-added sometimes if it was deleted or moved (very OS-dependent)
    if (!watcher.files().contains(filename))
    {
        // Adding path may fail sometimes. For example, inotify does not have enough resources on linux.
        if (!watcher.addPath(filename))
        {
            qCDebug(lc::steam) << "could not use native file watcher for" << filename;
        }
    }
}
}  // namespace

namespace steam
{
SteamLogTracker::SteamLogTracker(std::filesystem::path main_filename, std::filesystem::path backup_filename,
                                 QDateTime first_entry_time_filter, TimeFormat time_format)
    : m_main_filename{std::move(main_filename)}
    , m_backup_filename{std::move(backup_filename)}
    , m_first_entry_time_filter{std::move(first_entry_time_filter)}
    , m_time_format{time_format}
{
    connect(&m_file_watcher, &QFileSystemWatcher::fileChanged, this,
            [this]()
            {
                if (!m_pending_file_changed_check)
                {
                    constexpr auto debouce_time_ms{50};

                    m_pending_file_changed_check = true;
                    QTimer::singleShot(debouce_time_ms, this,
                                       [this]()
                                       {
                                           m_pending_file_changed_check = false;
                                           slotCheckLog();
                                       });
                }
            });
}

void SteamLogTracker::slotCheckLog()
{
    QFile      main_file{m_main_filename};
    const auto file_watcher_restart_guard{qScopeGuard(
        [this, &main_file]()
        {
            // It should be enough to only monitor the main file as the backup changes very rarely, worst case there
            // will be a delay of 1 second...
            tryWatchFileNatively(main_file.fileName(), m_file_watcher);
        })};

    if (!openForReading(main_file))
    {
        return;
    }

    const auto current_main_file_size{main_file.size()};
    const bool was_main_file_appended{current_main_file_size > m_last_prev_size};
    const bool was_main_file_switched_with_backup{current_main_file_size < m_last_prev_size};

    if (!m_initialized)
    {
        qCInfo(lc::steam) << "performing initial log read for files" << m_main_filename.generic_string() << "and"
                          << m_backup_filename.generic_string();

        QFile backup_file{m_backup_filename};
        if (!openForReading(backup_file))
        {
            if (backup_file.exists())
            {
                // Warning already logged.
                return;
            }

            qCInfo(lc::steam) << "skipping file" << m_backup_filename.generic_string()
                              << "for initial read, because it does not exist.";
        }

        std::vector<QString> lines;
        if (backup_file.isOpen())
        {
            readAllLines(lines, backup_file);
        }

        m_last_read_pos  = readAllLines(lines, main_file);
        m_last_prev_size = current_main_file_size;

        filterLines(lines, m_first_entry_time_filter, m_time_format);
        onLogChanged(lines);
        m_initialized = true;
        return;
    }

    if (was_main_file_appended)
    {
        qCDebug(lc::steam) << "file" << m_main_filename.generic_string() << "was appended.";

        std::vector<QString> lines;
        m_last_read_pos =
            readRemainingLines(lines, main_file, m_first_entry_time_filter, m_time_format, m_last_read_pos);
        m_last_prev_size = current_main_file_size;

        onLogChanged(lines);
        return;
    }

    if (was_main_file_switched_with_backup)
    {
        qCDebug(lc::steam) << "file" << m_main_filename.generic_string() << "was switched with"
                           << m_backup_filename.generic_string();

        QFile backup_file{m_backup_filename};
        if (!openForReading(backup_file))
        {
            return;
        }

        std::vector<QString> lines;
        readRemainingLines(lines, backup_file, m_first_entry_time_filter, m_time_format, m_last_read_pos);
        m_last_read_pos  = readRemainingLines(lines, main_file, m_first_entry_time_filter, m_time_format, 0);
        m_last_prev_size = current_main_file_size;

        onLogChanged(lines);
        return;
    }

    qCDebug(lc::steam) << "file" << m_main_filename.generic_string() << "did not change.";
}
}  // namespace steam
