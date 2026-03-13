#include "PathUtils.h"
#include <cstdlib>

namespace tim2tox::path {

std::filesystem::path GetDefaultDataDir() {
#if defined(_WIN32) || defined(_WIN64)
    const char* appdata = std::getenv("APPDATA");
    if (appdata && appdata[0]) {
        return std::filesystem::path(appdata) / "tim2tox";
    }
    return std::filesystem::path("./tim2tox_data");
#elif defined(__APPLE__)
    const char* home = std::getenv("HOME");
    if (home && home[0]) {
        return std::filesystem::path(home) / "Library" / "Application Support" / "tim2tox";
    }
    return std::filesystem::path("./tim2tox_data");
#else
    const char* xdg = std::getenv("XDG_DATA_HOME");
    if (xdg && xdg[0]) {
        return std::filesystem::path(xdg) / "tim2tox";
    }
    const char* home = std::getenv("HOME");
    if (home && home[0]) {
        return std::filesystem::path(home) / ".local" / "share" / "tim2tox";
    }
    return std::filesystem::path("./tim2tox_data");
#endif
}

std::filesystem::path BuildProfilePath(const std::filesystem::path& base_dir, int64_t instance_id) {
    if (instance_id == 0) {
        return base_dir / "tox_profile.tox";
    }
    return base_dir / ("tox_profile_" + std::to_string(instance_id) + ".tox");
}

bool EnsureDirectoryExists(const std::filesystem::path& dir, std::string* err) {
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        if (err) {
            *err = ec.message();
        }
        return false;
    }
    return true;
}

}  // namespace tim2tox::path
