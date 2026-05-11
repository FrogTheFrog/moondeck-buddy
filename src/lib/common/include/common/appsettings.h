#pragma once

// local includes
#include "appmetadata.h"
#include "usersettings.h"

namespace common
{
struct AppSettings
{
    const AppMetadata&  m_app_metadata;
    const UserSettings& m_user_settings;
};
}  // namespace common
