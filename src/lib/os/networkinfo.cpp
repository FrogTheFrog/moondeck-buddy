// header file include
#include "os/networkinfo.h"

// system/Qt includes
// clang-format off
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
// clang-format on
#include <vector>

// local includes
#include "shared/loggingcategories.h"

namespace
{
#if defined(Q_OS_WIN)
QString normalizeIPv4MappedAddress(const QString& ip)
{
    if (ip.startsWith("::ffff:"))
    {
        return ip.mid(7);  // Strip the "::ffff:" prefix
    }
    return ip;
}
#endif
}  // namespace

namespace os
{
QString NetworkInfo::getMacAddress(const QHostAddress& host_address)
{
#if defined(Q_OS_WIN)
    ULONG                 buffer_size{0};
    std::vector<BYTE>     buffer;
    PIP_ADAPTER_ADDRESSES addresses{nullptr};

    ULONG result = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, addresses, &buffer_size);
    if (result == ERROR_BUFFER_OVERFLOW)
    {
        buffer.resize(buffer_size);
        addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());
        result    = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, addresses, &buffer_size);
    }

    if (result != NO_ERROR)
    {
        qCWarning(lc::os) << "GetAdaptersAddresses failed with error:" << lc::getErrorString(result);
        return {};
    }

    const QString target_ip{normalizeIPv4MappedAddress(host_address.toString())};
    while (addresses)
    {
        for (auto unicast_address = addresses->FirstUnicastAddress; unicast_address != nullptr;
             unicast_address      = unicast_address->Next)
        {
            char str_buffer[INET6_ADDRSTRLEN] = {0};
            if (getnameinfo(unicast_address->Address.lpSockaddr, unicast_address->Address.iSockaddrLength, str_buffer,
                            sizeof(str_buffer), nullptr, 0, NI_NUMERICHOST)
                != 0)
            {
                qCWarning(lc::os) << "WSAGetLastError failed with error:" << lc::getErrorString(WSAGetLastError());
                return {};
            }

            const QString adapter_pp{QString::fromLatin1(str_buffer)};
            if (target_ip == adapter_pp)
            {
                const QString mac_address = QString("%1:%2:%3:%4:%5:%6")
                                                .arg(addresses->PhysicalAddress[0], 2, 16, QLatin1Char('0'))
                                                .arg(addresses->PhysicalAddress[1], 2, 16, QLatin1Char('0'))
                                                .arg(addresses->PhysicalAddress[2], 2, 16, QLatin1Char('0'))
                                                .arg(addresses->PhysicalAddress[3], 2, 16, QLatin1Char('0'))
                                                .arg(addresses->PhysicalAddress[4], 2, 16, QLatin1Char('0'))
                                                .arg(addresses->PhysicalAddress[5], 2, 16, QLatin1Char('0'))
                                                .toUpper();

                return mac_address;
            }
        }
        addresses = addresses->Next;
    }

    return {};
#else
    static const QSet<QNetworkInterface::InterfaceType> allowed_types{QNetworkInterface::Ethernet,
                                                                      QNetworkInterface::Wifi};

    for (const QNetworkInterface& iface : QNetworkInterface::allInterfaces())
    {
        if (allowed_types.contains(iface.type()) && (iface.flags() & QNetworkInterface::IsRunning))
        {
            for (const QHostAddress& address_entry : iface.allAddresses())
            {
                if (address_entry.isEqual(address))
                {
                    return iface.hardwareAddress();
                }
            }
        }
    }

    return {};
#endif
}
}  // namespace os
