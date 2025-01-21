#pragma once

// system/Qt includes
#include <QTimer>
#include <filesystem>

namespace os
{
class SteamLogTracker : public QObject
{
    Q_OBJECT

public:
    explicit SteamLogTracker(const std::filesystem::path& main_filename, const std::filesystem::path& backup_filename);
    ~SteamLogTracker() override = default;

private slots:
    void slotOnTimeout();

protected:
    virtual void onLogChanged(const std::vector<QString>& new_lines) = 0;

private:
    QTimer                m_watch_timer;
    std::filesystem::path m_main_filename;
    std::filesystem::path m_backup_filename;
    qint64                m_last_prev_size{0};
    qint64                m_last_read_pos{0};
    bool                  m_initialized{false};
};
}  // namespace os
