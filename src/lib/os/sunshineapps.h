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

    void load();

    std::optional<std::set<QString>> getAppNames() const;

private:
    QString                          m_filepath;
    std::optional<std::set<QString>> m_apps;
};
}  // namespace os
