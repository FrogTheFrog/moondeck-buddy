// header file include
#include "steam/steamwebhelperlogtracker.h"

// system/Qt includes
#include <QRegularExpression>

// local includes
#include "common/loggingcategories.h"

namespace steam
{
SteamWebHelperLogTracker::SteamWebHelperLogTracker(const std::filesystem::path& logs_dir,
                                                   QDateTime                    first_entry_time_filter)
    : SteamLogTracker(logs_dir / "webhelper.txt", logs_dir / "webhelper.previous.txt",
                      std::move(first_entry_time_filter))
{
}

SteamWebHelperLogTracker::~SteamWebHelperLogTracker()
{
    if (m_ui_mode != enums::SteamUiMode::Unknown)
    {
        m_ui_mode = enums::SteamUiMode::Unknown;
        emit signalStateChanged();
        emit signalSteamUiModeChanged();
    }
}

enums::SteamUiMode SteamWebHelperLogTracker::getSteamUiMode() const
{
    return m_ui_mode;
}

void SteamWebHelperLogTracker::onLogChanged(const std::vector<LogLine>& new_lines)
{
    enums::SteamUiMode new_ui_mode{m_ui_mode};
    for (const auto& line : new_lines)
    {
        static const QRegularExpression mode_regex{R"(SP\s(?:(Desktop)|(BPM))_)"};
        if (const auto match{mode_regex.match(line.m_text)}; match.hasMatch())
        {
            if (constexpr int desktop_group{1}; match.hasCaptured(desktop_group))
            {
                new_ui_mode = enums::SteamUiMode::Desktop;
            }
            else
            {
                new_ui_mode = enums::SteamUiMode::BigPicture;
            }
        }
    }

    if (new_ui_mode != m_ui_mode)
    {
        qCInfo(lc::steam) << "Steam UI mode change:" << enums::qEnumToString(m_ui_mode) << "->"
                          << enums::qEnumToString(new_ui_mode);
        m_ui_mode = new_ui_mode;
        emit signalStateChanged();
        emit signalSteamUiModeChanged();
    }
}
}  // namespace steam
