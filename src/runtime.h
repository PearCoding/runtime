#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "anydsl_runtime.h"
#include "platform.h"

namespace AnyDSLInternal {
class Runtime {
public:
    Runtime();
    ~Runtime();

    /// Registers the given platform into the runtime.
    template <typename T, typename... Args>
    inline void register_platform(Args&&... args)
    {
        Platform* platform = mPlatforms.emplace_back(new T(this, std::forward<Args&&>(args)...)).get();
        if (platform->type() == AnyDSL_DEVICE_HOST)
            mHost = platform;
    }

    void init();

    inline const std::vector<std::unique_ptr<Platform>>& platforms() const { return mPlatforms; }
    std::optional<Platform*> query_platform(AnyDSLDeviceType type);

    inline Platform* host() const { return mHost; }

    static void* aligned_malloc(size_t, size_t);
    static void aligned_free(void*);

    static Runtime& instance();

private:
    std::vector<std::unique_ptr<Platform>> mPlatforms;
    Platform* mHost;
};
} // namespace AnyDSLInternal