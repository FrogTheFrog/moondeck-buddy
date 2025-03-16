#pragma once

// system/Qt includes
#include <QDateTime>
#include <QTimer>
#include <filesystem>

namespace os
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

    explicit SteamLogTracker(std::filesystem::path main_filename, std::filesystem::path backup_filename,
                             QDateTime  first_entry_time_filter,
                             TimeFormat time_format = TimeFormat::YYYY_MM_DD_hh_mm_ss);
    ~SteamLogTracker() override = default;

public slots:
    void slotCheckLog();

protected:
    virtual void onLogChanged(const std::vector<QString>& new_lines) = 0;

private:
    std::filesystem::path m_main_filename;
    std::filesystem::path m_backup_filename;
    QDateTime             m_first_entry_time_filter;
    TimeFormat            m_time_format;
    qint64                m_last_prev_size{0};
    qint64                m_last_read_pos{0};
    bool                  m_initialized{false};
};
}  // namespace os
