// system/Qt includes
#include <QCoreApplication>
#include <QProcessEnvironment>
#include <QRegularExpression>

// local includes
#include "os/sleepinhibitor.h"
#include "shared/appmetadata.h"
#include "shared/loggingcategories.h"
#include "utils/heartbeat.h"
#include "utils/logsettings.h"
#include "utils/shmserialization.h"
#include "utils/singleinstanceguard.h"
#include "utils/unixsignalhandler.h"

#include "avfromtojson.h"

struct nested_struct
{
    QMap<QString, int>      m_qt_map;
    QHash<QString, QString> m_ugh;
};

struct my_struct
{
    Q_GADGET

public:
    enum class Who
    {
        Mario,
        Luigi
    };
    Q_ENUM(Who)

    enum class SimpleWho
    {
        SimpleMario,
        SimpleLuigi
    };

    Who                              m_its_a_me;
    SimpleWho                        m_simple;
    QHash<Who, SimpleWho>            m_asd;
    QMap<SimpleWho, Who>             m_asdf;
    QString                          m_test;
    QVector<QString>                 m_vec;
    std::map<QString, nested_struct> m_std_map;
    std::optional<int>               opt_int_1;
    std::optional<int>               opt_int_2;
};

void test_glaze()
{
    const QString buffer = R"(
    {
        "its_a_me": "Luigi",
        "simple": 1,
        "asd" : {
            "Mario": 0,
            "Luigi": 1
        },
        "asdf" : {
            "0": "Mario",
            "1": "Luigi"
        },
        "test": "MEH",
        "vec": ["1", "2", "3"],
        "std_map": {
            "m_key1": {
                "ugh": {
                    "1": "on\"e",
                    "2": "twO"
                },
                "qt_map": {
                    "on\"e": 1,
                    "two": 2
                }
            }
        },
        "opt_int_1": null,
        "opt_int_2": 123
    })";
    qInfo().noquote().nospace() << "INITIAL! " << buffer;
    auto parsed{AVFromJson<my_struct>(buffer)};
    if (parsed)
    {
        auto value{parsed.value()};

        qInfo().noquote().nospace() << "YAY 1!";
        auto json_string{AVToJson(value)};
        if (json_string)
        {
            qInfo().noquote().nospace() << "YAY 2!" << json_string.value();
        }
        else
        {
            qInfo().noquote().nospace() << "NAY 2!" << json_string.error();
        }
    }
    else
    {
        qInfo().noquote().nospace() << "NAY 1!" << parsed.error();
    }
}

QMap<QString, QString> getMatchingEnv(const QRegularExpression& regex)
{
    QMap<QString, QString> captured_env;
    for (const auto env{QProcessEnvironment::systemEnvironment()}; const QString& key : env.keys())
    {
        if (const auto match = regex.match(key); match.hasMatch())
        {
            const auto value{env.value(key)};
            qCDebug(lc::streamMain) << "Captured environment variable:" << key << "=" << value;
            captured_env[key] = value;
        }
    }

    return captured_env;
}

// NOLINTNEXTLINE(*-avoid-c-arrays)
int main(int argc, char* argv[])
{
    const shared::AppMetadata  app_meta{shared::AppMetadata::App::Stream};
    utils::SingleInstanceGuard guard{app_meta.getAppName()};

    QCoreApplication app{argc, argv};
    QCoreApplication::setApplicationName(app_meta.getAppName());
    QCoreApplication::setApplicationVersion(EXEC_VERSION);

    if (!guard.tryToRun())
    {
        qCWarning(lc::streamMain) << "Another instance of" << app_meta.getAppName() << "is already running!";
        return EXIT_FAILURE;
    }

    test_glaze();

    utils::LogSettings::getInstance().init(app_meta.getLogPath());
    utils::installSignalHandler();
    qCInfo(lc::streamMain) << "Startup. Version:" << EXEC_VERSION;

    // Capture and store environment variables for Buddy to use when launching games
    utils::ShmSerializer env_map_serializer{app_meta.getSharedEnvMapKey()};
    {
        utils::ShmDeserializer env_regex_deserializer{app_meta.getSharedEnvRegexKey()};
        if (const auto regex = env_regex_deserializer.read<QRegularExpression>())
        {
            qCInfo(lc::streamMain) << "Got the following ENV regex from Buddy:" << *regex;
            if (!env_map_serializer.write(getMatchingEnv(*regex)))
            {
                qCWarning(lc::streamMain) << "Failed to capture environment variables, still continuing...";
            }
        }
        else
        {
            qCWarning(lc::streamMain) << "Failed to read ENV regex from shared memory!";
        }
    }

    const os::SleepInhibitor sleep_inhibitor{app_meta.getAppName()};
    utils::Heartbeat         heartbeat{app_meta.getAppName()};
    QObject::connect(&heartbeat, &utils::Heartbeat::signalShouldTerminate, &app, &QCoreApplication::quit);
    heartbeat.startBeating();

    QObject::connect(&app, &QCoreApplication::aboutToQuit, []() { qCInfo(lc::streamMain) << "Shutdown."; });
    qCInfo(lc::streamMain) << "Startup finished.";
    return QCoreApplication::exec();
}

#include "main.moc"
