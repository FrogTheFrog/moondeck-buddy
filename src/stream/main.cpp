#include <QBuffer>
#include <csignal>
#include <iostream>
#include <unistd.h>

const size_t max_rss_memory = 500 * 1024 * 1024;

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
    QBuffer buffer;

    size_t rss = getCurrentRSS();
    std::cout << "Initial RSS size = " << rss << std::endl;
    for (size_t i = 0; i < 2000000; ++i)
    {
        QTextStream out{&buffer};

        rss = getCurrentRSS();
        if (rss > max_rss_memory)
        {
            std::cout << "Current RSS size = " << rss << " exceeds 500 Mb, quit application";
            break;
        }
    }

    std::cout << "Current RSS size = " << rss << std::endl;
    if (rss <= max_rss_memory)
    {
        std::cout << "Mem. leak" << std::endl;
    }
    else
    {
        std::cout << "No mem. leak" << std::endl;
    }

    return 0;
}
