#include <MSTL/core/file/temp_file.hpp>
#include <MSTL/core/time/clocks.hpp>
#include <MSTL/core/async/thread.hpp>
#include <MSTL/core/system/environment.hpp>
#ifdef MSTL_PLATFORM_LINUX__
#include <cstdlib>
#endif
MSTL_BEGIN_NAMESPACE__

path temp_file::generate_temp_path(const string& prefix, const string& suffix) {
    const path temp_dir(environment::temp_directory());

    const auto now = system_clock::now();
    const auto duration = now.time_since_epoch();
    const auto millis = duration_cast<milliseconds>(duration).count();

    const string random_name = prefix
        + "_" + to_string(millis)
        + "_" + to_string(this_thread::get_id().native())
        + suffix;

    return temp_dir / path(_MSTL move(random_name));
}

temp_file::temp_file(const string& prefix, const string& suffix)
    : temp_path_(generate_temp_path(prefix, suffix)) {

    file_.open(temp_path_, false,
               FILE_ACCESS::READ_WRITE,
               FILE_SHARED::NO_SHARE,
               FILE_CREATION::CREATE_FORCE,
               FILE_ATTRI::NORMAL);
}

temp_file::~temp_file() {
    cleanup();
}

temp_file::temp_file(temp_file&& other) noexcept
    : temp_path_(_MSTL move(other.temp_path_)),
      file_(_MSTL move(other.file_)),
      auto_delete_(other.auto_delete_) {
    other.auto_delete_ = false;
}

temp_file& temp_file::operator=(temp_file&& other) noexcept {
    if (this != &other) {
        cleanup();
        temp_path_ = _MSTL move(other.temp_path_);
        file_ = _MSTL move(other.file_);
        auto_delete_ = other.auto_delete_;
        other.auto_delete_ = false;
    }
    return *this;
}

void temp_file::cleanup() {
    if (auto_delete_ && !temp_path_.empty()) {
        file_.close();
        if (temp_path_.exists()) {
            temp_path_.remove();
        }
    }
}

MSTL_END_NAMESPACE__
