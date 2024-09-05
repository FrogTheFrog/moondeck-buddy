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

    void           init(const QString& filepath);
    const QString& getFilepath() const;

    void setLoggingRules(const QString& rules);

private:
    explicit LogSettings() = default;

    QString m_filepath;
};
}  // namespace utils
