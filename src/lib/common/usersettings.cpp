// header file include
#include "common/usersettings.h"

// local includes
#include "json/json.h"

namespace common
{
UserSettings UserSettings::loadAndValidate(const QString& filepath)
{
    auto settings{json::tryPartialReadFromFile<UserSettings>(filepath)};
    json::saveToFile(filepath, settings);
    return settings;
}
}  // namespace common
