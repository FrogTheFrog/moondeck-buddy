#pragma once

// system/Qt includes
#include <QString>

namespace utils
{
class LogSettings final
{
    Q_DISABLE_COPY(LogSettings)

public:
    static LogSettings& getInstance();

    void init(const QString& filepath);
    void writeToStdOut(QStringView view);
    void writeToStdErr(QStringView view);
    void writeToFile(QStringView view);

    void logSignalBeforeExit(int code);
    void setLoggingRules(const QString& rules);

private:
    explicit LogSettings() = default;

    QString m_filepath;
    bool    m_file_appended{false};
};
}  // namespace utils
