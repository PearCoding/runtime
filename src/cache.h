#pragma once

#include "anydsl_runtime_internal_config.h"

#include <cstddef>
#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_map>

namespace AnyDSLInternal {

/// @brief A singleton managing the internal caching system
class Cache {
private:
    inline Cache()
    {
    }

    std::filesystem::path mCacheDir;
    std::unordered_map<std::string, std::string> mFiles;
    mutable std::mutex mLock;

public:
    inline void set_directory(const std::filesystem::path& dir) { mCacheDir = dir; }
    std::filesystem::path get_directory() const;
    std::filesystem::path get_user_directory() const;

    std::filesystem::path get_filename(const std::string& str, const std::string& ext) const;
    std::string load_file(const std::filesystem::path& filename) const;

    inline void register_file(const std::filesystem::path& filename, const std::string& program_string)
    {
        std::lock_guard _guard(mLock);
        mFiles[filename.generic_string()] = program_string;
    }

    void store_file(const std::filesystem::path& filename, const std::string& str) const;
    void store_file(const std::filesystem::path& filename, const std::byte* data, size_t size) const;

    std::string load_from_cache(const std::string& key, const std::string& ext = ".bin") const;
    void store_to_cache(const std::string& key, const std::string& str, const std::string ext = ".bin") const;

    inline static Cache& instance()
    {
        static Cache sCache;
        return sCache;
    }
};

} // namespace AnyDSLInternal