#include "cache.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <direct.h>
#else
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

#include <cstring>
#include <fstream>

#include "log.h"

namespace AnyDSLInternal {
#if _XOPEN_SOURCE >= 500 || _POSIX_C_SOURCE >= 200112L || /* Glibc versions <= 2.19: */ _BSD_SOURCE
static std::filesystem::path get_self_directory()
{
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0';
        return std::filesystem::path(path).parent_path();
    }
    return std::filesystem::current_path();
}
#elif defined(__APPLE__)
static std::filesystem::path get_self_directory()
{
    char path[PATH_MAX];
    uint32_t size = (uint32_t)sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        char resolved[PATH_MAX];
        if (realpath(path, resolved))
            return std::filesystem::path(resolved).parent_path();
    }
    return std::filesystem::current_path();
}
#elif defined(_WIN32)
static std::filesystem::path get_self_directory()
{
    CHAR path[MAX_PATH];
    DWORD nSize  = (DWORD)sizeof(path);
    DWORD length = GetModuleFileNameA(NULL, path, nSize);
    if ((length == 0) || (length == MAX_PATH))
        return std::filesystem::current_path();

    return std::filesystem::path(path).parent_path();
}
#else
static std::filesystem::path get_self_directory()
{
    return std::filesystem::current_path();
}
#endif

std::filesystem::path Cache::get_directory() const
{
    if (mCacheDir.empty()) {
        std::filesystem::path cache_path = get_self_directory();
        return cache_path / "cache";
    } else {
        return mCacheDir;
    }
}

std::filesystem::path Cache::get_user_directory() const
{
    return mCacheDir;
}

std::filesystem::path Cache::get_filename(const std::string& str, const std::string& ext) const
{
    size_t key = std::hash<std::string>{}(str);
    std::stringstream hex_stream;
    hex_stream << std::hex << key;
    return get_directory() / (hex_stream.str() + ext);
}

inline std::string read_stream(std::istream& stream)
{
    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

std::string Cache::load_file(const std::filesystem::path& filename) const
{
    {
        std::lock_guard _guard(mLock);
        if (auto file_it = mFiles.find(filename.generic_string()); file_it != mFiles.end())
            return file_it->second;
    }

    std::ifstream src_file(filename);
    if (!src_file)
        error("Can't open source file '%s'", filename.c_str());
    return read_stream(src_file);
}

void Cache::store_file(const std::filesystem::path& filename, const std::string& str) const
{
    store_file(filename, reinterpret_cast<const std::byte*>(str.data()), str.length());
}

void Cache::store_file(const std::filesystem::path& filename, const std::byte* data, size_t size) const
{
    std::ofstream dst_file(filename, std::ofstream::binary);
    if (!dst_file)
        error("Can't open destination file '%s'", filename.c_str());
    dst_file.write(reinterpret_cast<const char*>(data), size);
}

std::string Cache::load_from_cache(const std::string& key, const std::string& ext) const
{
    std::filesystem::path filename = get_filename(key, ext);
    std::ifstream src_file(filename, std::ifstream::binary);
    if (!src_file.is_open())
        return std::string();
    // prevent collision by storing the key in the cached file
    size_t size = 0;
    if (!src_file.read(reinterpret_cast<char*>(&size), sizeof(size_t)))
        return std::string();
    auto buf = std::make_unique<char[]>(size);
    if (!src_file.read(buf.get(), size))
        return std::string();
    if (std::memcmp(key.data(), buf.get(), size))
        return std::string();
    debug("Loading from cache: %s", filename.c_str());
    return read_stream(src_file);
}

void Cache::store_to_cache(const std::string& key, const std::string& str, const std::string ext) const
{
    std::filesystem::create_directory(get_directory());
    std::filesystem::path filename = get_filename(key, ext);
    debug("Storing to cache: %s", filename.c_str());

    std::ofstream dst_file(filename, std::ofstream::binary);
    size_t size = key.size();
    dst_file.write(reinterpret_cast<char*>(&size), sizeof(size_t));
    dst_file.write(key.data(), size);
    dst_file.write(str.data(), str.size());
}
} // namespace AnyDSLInternal