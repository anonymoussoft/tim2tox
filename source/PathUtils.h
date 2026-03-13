#pragma once

#include <filesystem>
#include <string>

namespace tim2tox::path {

/** Default data directory for the current platform (no instance-specific suffix). */
std::filesystem::path GetDefaultDataDir();

/** Profile file path under base_dir for the given instance_id (0 => tox_profile.tox, else tox_profile_<id>.tox). */
std::filesystem::path BuildProfilePath(const std::filesystem::path& base_dir, int64_t instance_id);

/** Create directory and all parents; return true on success, else set *err and return false. */
bool EnsureDirectoryExists(const std::filesystem::path& dir, std::string* err);

}  // namespace tim2tox::path
