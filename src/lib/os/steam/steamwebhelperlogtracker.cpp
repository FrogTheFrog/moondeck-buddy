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
        static const QRegularExpression initial_regex{R"(SP\s(?:(Desktop)|(BPM))_)"};
        static const QRegularExpression default_regex{R"(SP\s(?:(Desktop)|(BPM))_.+?WasHidden\s(?:(0)|(1)))"};
        const auto match{(new_ui_mode == UiMode::Unknown ? initial_regex : default_regex).match(line)};
        if (match.hasMatch())
        {
            constexpr int desktop_group{1};
            constexpr int was_hidden_group{4};
            if (match.hasCaptured(desktop_group))
            {
                if (match.hasCaptured(was_hidden_group))
                {
                    // Desktop was hidden, but unless BPM is visible we assume it's Desktop mode still
                }
                else
                {
                    new_ui_mode = UiMode::Desktop;
                }
            }
            else
            {
                if (match.hasCaptured(was_hidden_group))
                {
                    // BPM was hidden, we will fall back to Desktop by default
                    new_ui_mode = UiMode::Desktop;
                }
                else
                {
                    new_ui_mode = UiMode::BigPicture;
                }
            }
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
