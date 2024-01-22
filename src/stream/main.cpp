#include <QBuffer>
#include <QTextStream>
#include <csignal>
#include <iostream>
#include <unistd.h>

const size_t max_rss_memory = 500 * 1024 * 1024;

class QDeviceClosedNotifier : public QObject
{
    Q_OBJECT
public:
    inline QDeviceClosedNotifier()
    {
    }

    inline void setupDevice(QTextStream* stream, QIODevice* device)
    {
        disconnect();
        if (device)
        {
            // Force direct connection here so that QTextStream can be used
            // from multiple threads when the application code is handling
            // synchronization (see also QTBUG-12055).
            connect(device, SIGNAL(aboutToClose()), this, SLOT(flushStream()), Qt::DirectConnection);
        }
        this->stream = stream;
    }

public Q_SLOTS:
    inline void flushStream()
    {
        stream->flush();
    }

private:
    QTextStream* stream;
};

size_t getCurrentRSS()
{
    long  rss = 0L;
    FILE* fp  = NULL;
    if ((fp = fopen("/proc/self/statm", "r")) == NULL)
    {
        return (size_t)0L;
    }

    if (fscanf(fp, "%*s%ld", &rss) != 1)
    {
        fclose(fp);
        return (size_t)0L;
    }

    fclose(fp);
    return (size_t)rss * (size_t)sysconf(_SC_PAGESIZE);
}

int main()
{
    QBuffer     buffer;
    QTextStream out;

    size_t rss = getCurrentRSS();
    std::cout << "Initial RSS size = " << rss << std::endl;
    for (size_t i = 0; i < 2000000; ++i)
    {
        QDeviceClosedNotifier notifier;
        notifier.setupDevice(&out, &buffer);
        // out.setDevice(&buffer);

        rss = getCurrentRSS();
        if (rss > max_rss_memory)
        {
            std::cout << "Current RSS size = " << rss << " exceeds 500 Mb, breaking" << std::endl;
            break;
        }
    }

    std::cout << "Current RSS size = " << rss << std::endl;
    if (rss <= max_rss_memory)
    {
        std::cout << "No mem. leak" << std::endl;
    }
    else
    {
        std::cout << "Mem. leak" << std::endl;
    }

    return 0;
}

#include "main.moc"
