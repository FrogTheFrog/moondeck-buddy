#pragma once

// system/Qt includes
#include <QString>
#include <set>

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class SunshineApps
{
    Q_DISABLE_COPY(SunshineApps)

public:
    explicit SunshineApps(QString filepath);
    virtual ~SunshineApps() = default;

    std::optional<std::set<QString>> load();

private:
    QString m_filepath;
};
}  // namespace os
