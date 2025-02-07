// header file include
#include "os/steam/steamwebhelperlogtracker.h"

// system/Qt includes
#include <QRegularExpression>

// local includes
#include "shared/loggingcategories.h"

namespace os
{
SteamWebHelperLogTracker::SteamWebHelperLogTracker(const std::filesystem::path& logs_dir,
                                                   QDateTime                    first_entry_time_filter)
    : SteamLogTracker(logs_dir / "webhelper.txt", logs_dir / "webhelper.previous.txt",
                      std::move(first_entry_time_filter))
{
}

SteamWebHelperLogTracker::UiMode SteamWebHelperLogTracker::getUiMode() const
{
    return m_ui_mode;
}

void SteamWebHelperLogTracker::onLogChanged(const std::vector<QString>& new_lines)
{
    UiMode new_ui_mode{m_ui_mode};
    for (const QString& line : new_lines)
    {
        static const QRegularExpression mode_regex{R"(SP\s(?:(Desktop)|(BPM))_.+?WasHidden\s0)"};
        const auto                      match{mode_regex.match(line)};
        if (match.hasMatch())
        {
            new_ui_mode = match.hasCaptured(1) ? UiMode::Desktop : UiMode::BigPicture;
        }
    }

    if (new_ui_mode != m_ui_mode)
    {
        qCInfo(lc::os()) << "Steam UI mode change:" << lc::qEnumToString(m_ui_mode) << "->"
                         << lc::qEnumToString(new_ui_mode);
        m_ui_mode = new_ui_mode;
    }
}
}  // namespace os
