#pragma once

// system/Qt includes
#include <QString>

//---------------------------------------------------------------------------------------------------------------------

namespace utils
{
class LogSettings final
{
    Q_DISABLE_COPY(LogSettings)

public:
    static LogSettings& getInstance();

    void           init(const QString& filename);
    const QString& getFilename() const;

    void enableVerboseMode();
    bool isVerboseModeEnabled() const;

private:
    explicit LogSettings() = default;

    QString m_filename;
    bool    m_is_verbose{false};
};
}  // namespace utils
