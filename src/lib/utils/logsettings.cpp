// header file include
#include "logsettings.h"

// system/Qt includes
#include <QDateTime>
#include <QFile>

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

void messageHandler(QtMsgType type, const QMessageLogContext& /*unused*/, const QString& msg)
{
    QString formatted_msg;
    {
        const QString now{QDateTime::currentDateTime().toString("hh:mm:ss.zzz")};
        QTextStream   stream(&formatted_msg);
        switch (type)
        {
            case QtDebugMsg:
                if (!utils::LogSettings::getInstance().isVerboseModeEnabled())
                {
                    return;
                }

                stream << "[" << now << "] DEBUG:    " << msg;
                break;
            case QtInfoMsg:
                stream << "[" << now << "] INFO:     " << msg;
                break;
            case QtWarningMsg:
                stream << "[" << now << "] WARNING:  " << msg;
                break;
            case QtCriticalMsg:
                stream << "[" << now << "] CRITICAL: " << msg;
                break;
            case QtFatalMsg:
                stream << "[" << now << "] FATAL:    " << msg;
                break;
        }
    }

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
    qInstallMessageHandler(messageHandler);
}

//---------------------------------------------------------------------------------------------------------------------

const QString& LogSettings::getFilename() const
{
    return m_filename;
}

//---------------------------------------------------------------------------------------------------------------------

void LogSettings::enableVerboseMode()
{
    m_is_verbose = true;
}

//---------------------------------------------------------------------------------------------------------------------

bool LogSettings::isVerboseModeEnabled() const
{
    return m_is_verbose;
}
}  // namespace utils
