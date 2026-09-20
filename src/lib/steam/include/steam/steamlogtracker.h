#pragma once

// system/Qt includes
#include <QDateTime>
#include <QFileSystemWatcher>
#include <QTimer>
#include <filesystem>

// local includes
#include "common/statelesssignaldebouncer.h"

namespace steam
{
class SteamLogTracker : public QObject
{
    Q_OBJECT

public:
    enum class TimeFormat
    {
        YYYY_MM_DD_hh_mm_ss,
    };
    Q_ENUM(TimeFormat)

    struct LogLine
    {
        QDateTime parseTimestamp(const TimeFormat& time_format) const;

        QString m_text;
    };

    explicit SteamLogTracker(std::filesystem::path main_filename, std::filesystem::path backup_filename,
                             QDateTime  first_entry_time_filter,
                             TimeFormat time_format = TimeFormat::YYYY_MM_DD_hh_mm_ss);
    ~SteamLogTracker() override = default;

signals:
    void signalStateChanged();

public slots:
    void slotCheckLog();

protected:
    virtual void onLogChanged(const std::vector<LogLine>& new_lines) = 0;
    TimeFormat   getDefaultTimeFormat() const;

private:
    QFileSystemWatcher               m_file_watcher;
    common::StatelessSignalDebouncer m_debouncer;

    std::filesystem::path m_main_filename;
    std::filesystem::path m_backup_filename;
    QDateTime             m_first_entry_time_filter;
    TimeFormat            m_time_format;
    qint64                m_last_prev_size{0};
    qint64                m_last_read_pos{0};
    bool                  m_initialized{false};
};
}  // namespace steam
