// header file include
#include "os/steamlogtracker.h"

// system/Qt includes
#include <QFile>

// local includes
#include "shared/loggingcategories.h"

namespace
{
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
SteamLogTracker::SteamLogTracker(const QString& main_filename, const QString& backup_filename)
    : m_main_filename{main_filename}
    , m_backup_filename{backup_filename}
{
    connect(&m_watch_timer, &QTimer::timeout, this, &SteamLogTracker::slotOnTimeout);
    m_watch_timer.setInterval(1000);
    m_watch_timer.setSingleShot(true);

    slotOnTimeout();
}

void SteamLogTracker::slotOnTimeout()
{
    const auto start_timer_on_exit{qScopeGuard([this]() { m_watch_timer.start(); })};

    QFile main_file{m_main_filename};
    if (!main_file.exists())
    {
        qCDebug(lc::os) << "file" << m_main_filename << "does not exist yet.";
        return;
    }

    if (!main_file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qCWarning(lc::os) << "file" << m_main_filename << "could not be opened!";
        return;
    }

    const auto current_main_file_size{main_file.size()};
    const bool was_main_file_appended{current_main_file_size > m_last_prev_size};
    const bool was_main_file_switched_with_backup{current_main_file_size < m_last_prev_size};

    if (was_main_file_appended)
    {
        qCWarning(lc::os) << "file" << m_main_filename << "was appended.";

        std::vector<QString> lines;
        m_last_read_pos  = readRemainingLines(lines, main_file, m_last_read_pos);
        m_last_prev_size = current_main_file_size;

        onLogChanged(lines);
    }
    else if (was_main_file_switched_with_backup)
    {
        qCWarning(lc::os) << "file" << m_main_filename << "was switched with" << m_backup_filename << ".";

        QFile backup_file{m_backup_filename};
        if (!backup_file.exists())
        {
            qCDebug(lc::os) << "file" << m_backup_filename << "does not exist yet.";
            return;
        }

        if (!backup_file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qCWarning(lc::os) << "file" << m_backup_filename << "could not be opened!";
            return;
        }

        std::vector<QString> lines;
        readRemainingLines(lines, backup_file, m_last_read_pos);
        m_last_read_pos  = readRemainingLines(lines, main_file, 0);
        m_last_prev_size = current_main_file_size;

        onLogChanged(lines);
    }
    else
    {
        qCWarning(lc::os) << "file" << m_main_filename << "did not change.";
    }
}
}  // namespace os
