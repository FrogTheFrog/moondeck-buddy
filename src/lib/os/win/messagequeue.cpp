// header file include
#include "messagequeue.h"

//---------------------------------------------------------------------------------------------------------------------

namespace
{
QString getError(LSTATUS status)
{
    return QString::fromStdString(std::system_category().message(status));
}

//---------------------------------------------------------------------------------------------------------------------

LRESULT QT_WIN_CALLBACK wndProcCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    os::MessageQueue* pThis{nullptr};

    if (message == WM_NCCREATE)
    {
        // NOLINTNEXTLINE(*-reinterpret-cast,*-no-int-to-ptr)
        pThis = static_cast<os::MessageQueue*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);

        SetLastError(0);
        // NOLINTNEXTLINE(*-reinterpret-cast)
        if (SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis)) == 0)
        {
            const auto error{GetLastError()};
            if (error != 0)
            {
                qFatal("Failed to set long ptr in window class! Reason: %s", qUtf8Printable(getError(GetLastError())));
                return FALSE;
            }
        }
    }
    else
    {
        // NOLINTNEXTLINE(*-reinterpret-cast,*-no-int-to-ptr)
        pThis = reinterpret_cast<os::MessageQueue*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pThis == nullptr)
    {
        qDebug("Failed to get long ptr from window class! Message: %u", message);
        return DefWindowProc(hwnd, message, wParam, lParam);
    }

    if (message == WM_DISPLAYCHANGE)
    {
        emit pThis->signalResolutionChanged();
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
MessageQueue::MessageQueue(QString class_name)
    : m_class_name{std::move(class_name)}
{
    WNDCLASSEXW window_class;
    window_class.cbSize        = sizeof(WNDCLASSEXW);
    window_class.style         = 0;
    window_class.lpfnWndProc   = wndProcCallback;
    window_class.cbClsExtra    = 0;
    window_class.cbWndExtra    = 0;
    window_class.hInstance     = GetModuleHandleW(nullptr);
    window_class.hIcon         = nullptr;
    window_class.hCursor       = nullptr;
    window_class.hbrBackground = nullptr;
    window_class.lpszMenuName  = nullptr;
    // NOLINTNEXTLINE(*-reinterpret-cast)
    window_class.lpszClassName = reinterpret_cast<LPCWSTR>(m_class_name.utf16());
    window_class.hIconSm       = nullptr;

    m_window_class_handle = RegisterClassExW(&window_class);
    if (m_window_class_handle == 0)
    {
        qFatal("Failed to register window class! Reason: %s", qUtf8Printable(getError(GetLastError())));
    }

    m_window_handle =
        // NOLINTNEXTLINE(*-reinterpret-cast)
        CreateWindowExW(0, reinterpret_cast<LPCWSTR>(m_class_name.utf16()), nullptr, WS_TILED, CW_USEDEFAULT,
                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, GetModuleHandleW(nullptr), this);
    if (m_window_handle == nullptr)
    {
        qFatal("Failed to create window class! Reason: %s", qUtf8Printable(getError(GetLastError())));
    }
}

//---------------------------------------------------------------------------------------------------------------------

MessageQueue::~MessageQueue()
{
    if (m_window_handle != nullptr)
    {
        if (DestroyWindow(m_window_handle) == FALSE)
        {
            qWarning("Failed to destroy window class! Reason: %s", qUtf8Printable(getError(GetLastError())));
        }
    }

    if (m_window_class_handle != 0)
    {
        // NOLINTNEXTLINE(*-reinterpret-cast)
        if (UnregisterClassW(reinterpret_cast<LPCWSTR>(m_class_name.utf16()), GetModuleHandleW(nullptr)) == FALSE)
        {
            qWarning("Failed to unregister window class! Reason: %s", qUtf8Printable(getError(GetLastError())));
        }
    }
}
}  // namespace os
