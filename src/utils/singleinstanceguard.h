#pragma once

// system/Qt includes
#include <QSharedMemory>
#include <QSystemSemaphore>

//---------------------------------------------------------------------------------------------------------------------

namespace utils
{
// From https://stackoverflow.com/questions/5006547/qt-best-practice-for-a-single-instance-app-protection
class SingleInstanceGuard final
{
    Q_DISABLE_COPY(SingleInstanceGuard)

public:
    explicit SingleInstanceGuard(const QString& key);
    ~SingleInstanceGuard();

    bool isAnotherRunning();
    bool tryToRun();
    void release();

private:
    const QString m_mem_lock_key;
    const QString m_shared_mem_key;

    QSystemSemaphore m_mem_lock;
    QSharedMemory    m_shared_mem;
};
}  // namespace utils
