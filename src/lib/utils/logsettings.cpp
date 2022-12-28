// header file include
#include "logsettings.h"

// system/Qt includes
#include <QDateTime>
#include <QFile>
#include <QLoggingCategory>

// local includes
#include "helpers.h"

//---------------------------------------------------------------------------------------------------------------------

namespace
{
void removeLogFile(const QString& filename)
{
    QFile file(filename);
    if (file.exists())
    {
        file.remove();
    }
}

//---------------------------------------------------------------------------------------------------------------------

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    const QString formatted_msg{qFormatLogMessage(type, context, msg)};
    {
        QTextStream stream(stdout);
        stream << formatted_msg << Qt::endl;
    }

    QFile file(utils::LogSettings::getInstance().getFilename());
    if (file.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        QTextStream stream(&file);
        stream << formatted_msg << Qt::endl;
    }
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace utils
{
LogSettings& LogSettings::getInstance()
{
    static LogSettings instance;
    return instance;
}

//---------------------------------------------------------------------------------------------------------------------

void LogSettings::init(const QString& filename)
{
    if (!m_filename.isEmpty())
    {
        return;
    }

    m_filename = getExecDir() + filename;
    removeLogFile(m_filename);

    qSetMessagePattern("[%{time hh:mm:ss.zzz}] "
                       "%{if-debug}DEBUG    %{endif}"
                       "%{if-info}INFO     %{endif}"
                       "%{if-warning}WARNING  %{endif}"
                       "%{if-critical}CRITICAL %{endif}"
                       "%{if-fatal}FATAL    %{endif}"
                       "%{if-category}%{category}: %{endif}%{message}");
    qInstallMessageHandler(messageHandler);
}

//---------------------------------------------------------------------------------------------------------------------

const QString& LogSettings::getFilename() const
{
    return m_filename;
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-to-static)
void LogSettings::setLoggingRules(const QString& rules)
{
    if (!rules.isEmpty())
    {
        QLoggingCategory::setFilterRules(rules);
    }
}
}  // namespace utils
