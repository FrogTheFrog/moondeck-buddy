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

    void setLoggingRules(const QString& rules);

private:
    explicit LogSettings() = default;

    QString m_filename;
};
}  // namespace utils
