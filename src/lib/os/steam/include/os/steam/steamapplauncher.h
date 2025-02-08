#pragma once

// system/Qt includes
#include <QObject>

namespace os
{
class SteamAppLauncher : public QObject
{
    Q_OBJECT

public:
    explicit SteamAppLauncher();
    ~SteamAppLauncher() override = default;
};
}  // namespace os
