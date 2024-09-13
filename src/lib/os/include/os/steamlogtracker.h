#pragma once

// system/Qt includes
#include <QFileSystemWatcher>

namespace os
{
class SteamLogTracker final
{
public:
    explicit SteamLogTracker(const QString& log_dir);
    ~SteamLogTracker();

private:
    QFileSystemWatcher m_file_watcher;
};
}  // namespace os
