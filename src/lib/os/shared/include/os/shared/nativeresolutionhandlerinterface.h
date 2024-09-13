#pragma once

// system/Qt includes
#include <QtGlobal>
#include <functional>
#include <map>

namespace os
{
class NativeResolutionHandlerInterface
{
public:
    struct Resolution
    {
        uint m_width;
        uint m_height;
    };
    using ChangedResMap = std::map<QString, std::optional<Resolution>>;
    using DisplayPredicate =
        std::function<std::optional<Resolution>(const QString& /*display_name*/, bool /*is_primary*/)>;

    virtual ~NativeResolutionHandlerInterface()                               = default;
    virtual ChangedResMap changeResolution(const DisplayPredicate& predicate) = 0;
};
}  // namespace os
