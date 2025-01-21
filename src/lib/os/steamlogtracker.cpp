// header file include
#include "os/steamlogtracker.h"

// system/Qt includes
#include <QFile>

// local includes
#include "shared/loggingcategories.h"

namespace
{
bool openForReading(QFile& file)
{
    if (!file.exists())
    {
        qCDebug(lc::os) << "file" << file.fileName() << "does not exist yet.";
        return false;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qCWarning(lc::os) << "file" << file.fileName() << "could not be opened!";
        return false;
    }

    return true;
}

qint64 readRemainingLines(std::vector<QString>& lines, QFile& file, const qint64 start_offset)
{
    QTextStream stream{&file};
    QString     line;

    // Start reading from the last position we've read.
    stream.seek(start_offset);

    while (stream.readLineInto(&line))
    {
        if (!line.isEmpty())
        {
            lines.push_back(line);
        }
    }

    return stream.pos();
}
}  // namespace

namespace os
{
SteamLogTracker::SteamLogTracker(const std::filesystem::path& main_filename,
                                 const std::filesystem::path& backup_filename)
    : m_main_filename{main_filename}
    , m_backup_filename{backup_filename}
{
    connect(&m_watch_timer, &QTimer::timeout, this, &SteamLogTracker::slotOnTimeout);
    m_watch_timer.setInterval(1000);
    m_watch_timer.setSingleShot(true);

    QTimer::singleShot(0, this, &SteamLogTracker::slotOnTimeout);
}

void SteamLogTracker::slotOnTimeout()
{
    const auto start_timer_on_exit{qScopeGuard([this]() { m_watch_timer.start(); })};

    QFile main_file{m_main_filename};
    if (!openForReading(main_file))
    {
        return;
    }

    const auto current_main_file_size{main_file.size()};
    const bool was_main_file_appended{current_main_file_size > m_last_prev_size};
    const bool was_main_file_switched_with_backup{current_main_file_size < m_last_prev_size};

    if (!m_initialized)
    {
        qCInfo(lc::os) << "performing initial log read for files" << m_main_filename.generic_string() << "and"
                       << m_backup_filename.generic_string() << ".";

        QFile backup_file{m_backup_filename};
        if (!openForReading(backup_file))
        {
            if (backup_file.exists())
            {
                // Warning already logged.
                return;
            }

            qCInfo(lc::os) << "skipping file" << m_backup_filename.generic_string()
                           << "for initial read, because it does not exist.";
        }

        std::vector<QString> lines;
        if (backup_file.isOpen())
        {
            readRemainingLines(lines, backup_file, 0);
        }

        m_last_read_pos  = readRemainingLines(lines, main_file, 0);
        m_last_prev_size = current_main_file_size;
        m_initialized    = true;

        onLogChanged(lines);
        return;
    }

    if (was_main_file_appended)
    {
        qCDebug(lc::os) << "file" << m_main_filename.generic_string() << "was appended.";

        std::vector<QString> lines;
        m_last_read_pos  = readRemainingLines(lines, main_file, m_last_read_pos);
        m_last_prev_size = current_main_file_size;

        onLogChanged(lines);
        return;
    }

    if (was_main_file_switched_with_backup)
    {
        qCDebug(lc::os) << "file" << m_main_filename.generic_string() << "was switched with"
                        << m_backup_filename.generic_string() << ".";

        QFile backup_file{m_backup_filename};
        if (!openForReading(backup_file))
        {
            return;
        }

        std::vector<QString> lines;
        readRemainingLines(lines, backup_file, m_last_read_pos);
        m_last_read_pos  = readRemainingLines(lines, main_file, 0);
        m_last_prev_size = current_main_file_size;

        onLogChanged(lines);
        return;
    }

    qCDebug(lc::os) << "file" << m_main_filename.generic_string() << "did not change.";
}
}  // namespace os
