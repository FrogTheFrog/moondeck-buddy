// header file include
#include "waylandresolutionhandler.h"

// local includes
#include "shared/loggingcategories.h"
#include "wayland-wlr-output-management-unstable-v1-client-protocol.h"

//---------------------------------------------------------------------------------------------------------------------

namespace
{
struct HandlerData;
struct HeadData;
struct ModeData;

//---------------------------------------------------------------------------------------------------------------------

struct HandlerData final
{
    ~HandlerData()
    {
        destroyDisplay();
    }

    void destroyDisplay()
    {
        destroyRegistry();
        if (m_display)
        {
            wl_display_disconnect(m_display);
            m_display = nullptr;
        }
    }

    void destroyRegistry()
    {
        destroyOutputManager();
        if (m_registry)
        {
            wl_registry_destroy(m_registry);
            m_registry = nullptr;
        }
    }

    void destroyOutputManager()
    {
        m_heads.clear();
        if (m_output_manager)
        {
            zwlr_output_manager_v1_destroy(m_output_manager);
            m_output_manager = nullptr;
        }
    }

    void destroyHead(const HeadData* data_ptr)
    {
        auto head_it = std::find_if(std::cbegin(m_heads), std::cend(m_heads),
                                    [data_ptr](const auto& head) { return head.get() == data_ptr; });

        Q_ASSERT(head_it != std::cend(m_heads));
        m_heads.erase(head_it);
    }

    wl_display* m_display;

    wl_registry*         m_registry;
    wl_registry_listener m_registry_listener;

    zwlr_output_manager_v1*         m_output_manager;
    zwlr_output_manager_v1_listener m_output_manager_listener;
    uint32_t                        m_output_serial;

    std::vector<std::unique_ptr<HeadData>> m_heads;
    zwlr_output_head_v1_listener           m_output_head_listener;

    zwlr_output_mode_v1_listener m_output_mode_listener;
};

//---------------------------------------------------------------------------------------------------------------------

struct HeadData final
{
    explicit HeadData(HandlerData* const parent)
        : m_parent{parent}
    {
    }

    ~HeadData()
    {
        if (m_head == nullptr)
        {
            return;
        }

        if (zwlr_output_head_v1_get_version(m_head) >= 3)
        {
            zwlr_output_head_v1_release(m_head);
        }
        else
        {
            zwlr_output_head_v1_destroy(m_head);
        }
    }

    void destroyMode(const ModeData* data_ptr)
    {
        if (m_current_mode.get() == data_ptr)
        {
            m_current_mode = nullptr;
            return;
        }

        auto mode_it = std::find_if(std::cbegin(m_modes), std::cend(m_modes),
                                    [data_ptr](const auto& mode) { return mode.get() == data_ptr; });

        Q_ASSERT(mode_it != std::cend(m_modes));
        m_modes.erase(mode_it);
    }

    HandlerData* const   m_parent;
    zwlr_output_head_v1* m_head{nullptr};

    QString                                m_name{};
    std::vector<std::unique_ptr<ModeData>> m_modes{};
    std::unique_ptr<ModeData>              m_current_mode{nullptr};
};

//---------------------------------------------------------------------------------------------------------------------

struct ModeData final
{
    explicit ModeData(HeadData* const parent)
        : m_parent{parent}
    {
    }

    ~ModeData()
    {
        if (m_mode == nullptr)
        {
            return;
        }

        if (zwlr_output_mode_v1_get_version(m_mode) >= 3)
        {
            zwlr_output_mode_v1_release(m_mode);
        }
        else
        {
            zwlr_output_mode_v1_destroy(m_mode);
        }
    }

    HeadData* const      m_parent;
    zwlr_output_mode_v1* m_mode{nullptr};

    int32_t m_width{0};
    int32_t m_height{0};
};

//---------------------------------------------------------------------------------------------------------------------

template<typename... Args>
void noopFunction(Args...)
{
    // Does nothing
}

//---------------------------------------------------------------------------------------------------------------------

zwlr_output_mode_v1_listener makeOutputModeListener()
{
    return {.size =
                [](void* data, zwlr_output_mode_v1*, int32_t width, int32_t height)
            {
                auto* mode_data = reinterpret_cast<ModeData*>(data);
                Q_ASSERT(mode_data != nullptr);

                mode_data->m_width  = width;
                mode_data->m_height = height;
            },
            .refresh   = noopFunction,
            .preferred = noopFunction,
            .finished =
                [](void* data, zwlr_output_mode_v1*)
            {
                auto* mode_data = reinterpret_cast<ModeData*>(data);
                Q_ASSERT(mode_data != nullptr);
                Q_ASSERT(mode_data->m_parent != nullptr);

                mode_data->m_parent->destroyMode(mode_data);
            }};
}

//---------------------------------------------------------------------------------------------------------------------

zwlr_output_head_v1_listener makeOutputHeadListener()
{
    return {
        .name =
            [](void* data, zwlr_output_head_v1*, const char* name)
        {
            auto* head_data = reinterpret_cast<HeadData*>(data);
            Q_ASSERT(head_data != nullptr);

            head_data->m_name = name;
        },
        .description   = noopFunction,
        .physical_size = noopFunction,
        .mode =
            [](void* data, zwlr_output_head_v1*, zwlr_output_mode_v1* mode)
        {
            auto* head_data = reinterpret_cast<HeadData*>(data);
            Q_ASSERT(head_data != nullptr);
            Q_ASSERT(head_data->m_parent != nullptr);

            head_data->m_modes.push_back(std::make_unique<ModeData>(head_data));
            auto* mode_data{head_data->m_modes.back().get()};

            if (zwlr_output_mode_v1_add_listener(mode, &head_data->m_parent->m_output_mode_listener, mode_data) == -1)
            {
                qCWarning(lc::os) << "Failed to add output mode listener!";
                head_data->destroyMode(mode_data);
                return;
            }

            mode_data->m_mode = mode;
        },
        .enabled = noopFunction,
        .current_mode =
            [](void* data, zwlr_output_head_v1*, zwlr_output_mode_v1* mode)
        {
            auto* head_data = reinterpret_cast<HeadData*>(data);
            Q_ASSERT(head_data != nullptr);
            Q_ASSERT(head_data->m_parent != nullptr);

            head_data->m_current_mode = std::make_unique<ModeData>(head_data);
            auto* mode_data{head_data->m_current_mode.get()};

            if (zwlr_output_mode_v1_add_listener(mode, &head_data->m_parent->m_output_mode_listener, mode_data) == -1)
            {
                qCWarning(lc::os) << "Failed to add output current mode listener!";
                head_data->destroyMode(mode_data);
                return;
            }

            mode_data->m_mode = mode;
        },
        .position  = noopFunction,
        .transform = noopFunction,
        .scale     = noopFunction,
        .finished =
            [](void* data, zwlr_output_head_v1*)
        {
            auto* head_data = reinterpret_cast<HeadData*>(data);
            Q_ASSERT(head_data != nullptr);
            Q_ASSERT(head_data->m_parent != nullptr);

            head_data->m_parent->destroyHead(head_data);
        },
        .make          = noopFunction,
        .model         = noopFunction,
        .serial_number = noopFunction,
        .adaptive_sync = noopFunction};
}

//---------------------------------------------------------------------------------------------------------------------

zwlr_output_manager_v1_listener makeOutputManagerListener()
{
    return {.head =
                [](void* data, zwlr_output_manager_v1*, zwlr_output_head_v1* head)
            {
                auto* handler_data = reinterpret_cast<HandlerData*>(data);
                Q_ASSERT(handler_data != nullptr);

                handler_data->m_heads.push_back(std::make_unique<HeadData>(handler_data));
                auto* head_data{handler_data->m_heads.back().get()};

                if (zwlr_output_head_v1_add_listener(head, &handler_data->m_output_head_listener, head_data) == -1)
                {
                    qCWarning(lc::os) << "Failed to add output head listener!";
                    handler_data->destroyHead(head_data);
                    return;
                }

                head_data->m_head = head;
            },
            .done =
                [](void* data, zwlr_output_manager_v1*, uint32_t serial)
            {
                auto* handler_data = reinterpret_cast<HandlerData*>(data);
                Q_ASSERT(handler_data != nullptr);

                handler_data->m_output_serial = serial;
            },
            .finished =
                [](void* data, struct zwlr_output_manager_v1*)
            {
                auto* handler_data = reinterpret_cast<HandlerData*>(data);
                Q_ASSERT(handler_data != nullptr);

                handler_data->destroyOutputManager();
            }};
}

//---------------------------------------------------------------------------------------------------------------------

wl_registry_listener makeRegistryListener()
{
    return {
        .global{[](void* data, wl_registry* wl_registry, uint32_t name, const char* interface, uint32_t version)
                {
                    auto* handler_data = reinterpret_cast<HandlerData*>(data);
                    Q_ASSERT(handler_data != nullptr);

                    if (strcmp(interface, zwlr_output_manager_v1_interface.name) == 0)
                    {
                        uint32_t supported_version     = version <= 4 ? version : 4;
                        handler_data->m_output_manager = reinterpret_cast<zwlr_output_manager_v1*>(
                            wl_registry_bind(wl_registry, name, &zwlr_output_manager_v1_interface, supported_version));

                        if (handler_data->m_output_manager == nullptr)
                        {
                            qCDebug(lc::os) << "Failed to bind output manager.";
                            return;
                        }

                        if (zwlr_output_manager_v1_add_listener(handler_data->m_output_manager,
                                                                &handler_data->m_output_manager_listener, handler_data)
                            == -1)
                        {
                            qCWarning(lc::os) << "Failed to add output manager listener!";
                            return;
                        }
                    }
                }},
        .global_remove{[](void* data, struct wl_registry*, uint32_t)
                       {
                           auto* handler_data = reinterpret_cast<HandlerData*>(data);
                           Q_ASSERT(handler_data != nullptr);

                           handler_data->destroyRegistry();
                       }}};
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
WaylandResolutionHandler::ChangedResMap WaylandResolutionHandler::changeResolution(const DisplayPredicate& predicate)
{
    Q_UNUSED(predicate)
    const uint32_t initial_serial{0};
    HandlerData    data{.m_display = nullptr,

                        .m_registry = nullptr,
                        .m_registry_listener{makeRegistryListener()},

                        .m_output_manager = nullptr,
                        .m_output_manager_listener{makeOutputManagerListener()},
                        .m_output_serial = initial_serial,

                        .m_heads{},
                        .m_output_head_listener{makeOutputHeadListener()},

                        .m_output_mode_listener{makeOutputModeListener()}};

    data.m_display = wl_display_connect(nullptr);
    if (data.m_display == nullptr)
    {
        qCDebug(lc::os) << "No wayland display is available.";
        return {};
    }

    data.m_registry = wl_display_get_registry(data.m_display);
    if (data.m_display == nullptr)
    {
        qCWarning(lc::os) << "Failed to get display registry!";
        return {};
    }

    if (wl_registry_add_listener(data.m_registry, &data.m_registry_listener, &data) == -1)
    {
        qCWarning(lc::os) << "Failed to add registry listener!";
        return {};
    }

    wl_display_dispatch(data.m_display);
    wl_display_roundtrip(data.m_display);

    if (data.m_output_manager == nullptr)
    {
        qCDebug(lc::os) << "Wayland protocol" << zwlr_output_manager_v1_interface.name << "is not supported.";
        return {};
    }

    // TODO: finish once compositors supports the interface
    // while (data.m_output_serial == initial_serial)
    // {
    //     if (wl_display_dispatch(data.m_display) == -1)
    //     {
    //         qCWarning(lc::os) << "Failed to dispatch events!";
    //         return {};
    //     }
    // }

    return {};
}
}  // namespace os
