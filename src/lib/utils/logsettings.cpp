// header file include
#include "utils/logsettings.h"

// system/Qt includes
#include <QDateTime>
#include <QFile>

// local includes
#include "shared/loggingcategories.h"

namespace
{
void removeLogFile(const QString& filepath)
{
    QFile file(filepath);
    if (file.exists())
    {
        file.remove();
    }
}

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    const QString formatted_msg{qFormatLogMessage(type, context, msg)};
    {
        QTextStream stream(stdout);
        stream << formatted_msg << Qt::endl;
    }

    const auto& filepath{utils::LogSettings::getInstance().getFilepath()};
    QFile       file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        qFatal("File could not be opened for writing: \"%s\".", qUtf8Printable(filepath));
    }

    QTextStream stream(&file);
    stream << formatted_msg << Qt::endl;
}
}  // namespace

namespace utils
{
LogSettings& LogSettings::getInstance()
{
    static LogSettings instance;
    return instance;
}

void LogSettings::init(const QString& filepath)
{
    qSetMessagePattern("[%{time hh:mm:ss.zzz}] "
                       "%{if-debug}DEBUG    %{endif}"
                       "%{if-info}INFO     %{endif}"
                       "%{if-warning}WARNING  %{endif}"
                       "%{if-critical}CRITICAL %{endif}"
                       "%{if-fatal}FATAL    %{endif}"
                       "%{if-category}%{category}: %{endif}%{message}");

    if (filepath.isEmpty())
    {
        return;
    }

    m_filepath = filepath;
    removeLogFile(filepath);
    qInstallMessageHandler(messageHandler);

    qCInfo(lc::utils) << "Log location:" << m_filepath;
}

const QString& LogSettings::getFilepath() const
{
    return m_filepath;
}

// NOLINTNEXTLINE(*-to-static)
void LogSettings::setLoggingRules(const QString& rules)
{
    if (!rules.isEmpty())
    {
        QLoggingCategory::setFilterRules(rules);
    }
}
}  // namespace utils
