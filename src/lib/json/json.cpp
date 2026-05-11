// header file include
#include "json/json.h"

// system/Qt includes
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace json
{
namespace internal
{
QString tryPartialReadFromFile(const QString& filepath)
{
    if (QFile ids_file{filepath}; ids_file.exists())
    {
        if (!ids_file.open(QFile::ReadOnly))
        {
            qFatal("File exists, but could not be opened: \"%s\"", qUtf8Printable(filepath));
        }

        return ids_file.readAll();
    }

    return {};
}

void saveToFile(const QString& filepath, const QString& value)
{
    QFile file{filepath};
    if (!file.exists())
    {
        const QFileInfo info(filepath);
        if (const QDir dir; !dir.mkpath(info.absolutePath()))
        {
            qFatal("Failed at mkpath \"%s\"!", qUtf8Printable(filepath));
        }
    }

    if (!file.open(QFile::WriteOnly))
    {
        qFatal("File could not be opened for writing \"%s\"!", qUtf8Printable(filepath));
    }

    if (file.write(value.toUtf8()) == -1)
    {
        qFatal("Failed to write to file \"%s\"! Reason:\n%s", qUtf8Printable(filepath),
               qUtf8Printable(file.errorString()));
    }
}
}  // namespace internal
}  // namespace json