#include <MSTL/core/file/temp_file.hpp>
#include <MSTL/core/time/clocks.hpp>
#include <MSTL/core/async/thread.hpp>
#include <MSTL/core/async/atomic.hpp>
#include <MSTL/core/system/environment.hpp>
#include <MSTL/core/system/process.hpp>
#include <MSTL/core/numeric/random.hpp>
#ifdef MSTL_PLATFORM_LINUX__
#include <cstdlib>
#endif
MSTL_BEGIN_NAMESPACE__

vector<path>& temp_file::get_temp_registry() {
    static vector<path> registry;
    return registry;
}

mutex& temp_file::get_registry_mutex() {
    static mutex mutex;
    return mutex;
}

void temp_file::register_for_cleanup(const path& temp_path) {
    lock_guard<mutex> lock(get_registry_mutex());
    get_temp_registry().push_back(temp_path);
}

void temp_file::cleanup_all_temp_files() {
    lock_guard<mutex> lock(get_registry_mutex());
    auto& registry = get_temp_registry();

    for (const auto& temp_path : registry) {
        if (temp_path.exists()) {
            try {
                if (temp_path.is_directory()) {
                    temp_path.remove_all();
                } else {
                    temp_path.remove();
                }
            } catch (...) {}
        }
    }

    registry.clear();
}

path temp_file::generate_unique_path(const string& prefix, const string& suffix) {
    const path temp_dir{environment::temp_directory()};
    static atomic<uint64_t> counter{0};
    counter.fetch_add(1, memory_order_relaxed);

    const auto now = system_clock::now();
    const auto duration = now.time_since_epoch();
    const auto nanos = duration_cast<nanoseconds>(duration).count();

    const int pid = process::current_process_id();
    const uint64_t random_part = random_mt::next_int();
    const string filename = format(
        "{}_{}_{}_{}_{}{}",
        prefix,
        nanos,
        pid,
        this_thread::get_id().native(),
        random_part,
        suffix
    );
    return temp_dir / path(filename);
}

temp_file::temp_file(
    const string& prefix, const string& suffix,
    const FILE_CREATION mode, const DELETE_POLICY policy)
: file_(
    generate_unique_path(prefix, suffix),
    FILE_ACCESS::READ_WRITE,
    FILE_SHARED::SHARE_READ,
    mode)
, delete_policy_(policy) {
    try {
        if (file_.is_opened()) {
            if (delete_policy_ == DELETE_POLICY::AUTO_DELETE) {
                register_for_cleanup(file_.path());
            }
        } else {
            throw_exception(system_exception("Failed to create temporary file"));
        }
    } catch (...) {
        if (file_.path().exists()) {
            file_.path().remove();
        }
        throw;
    }
}

temp_file::temp_file(const path& existing_path, const DELETE_POLICY policy)
: file_(existing_path,
    FILE_ACCESS::READ_WRITE,
    FILE_SHARED::SHARE_READ_WRITE,
    FILE_CREATION::OPEN_EXIST)
, delete_policy_(policy) {
    if (delete_policy_ == DELETE_POLICY::AUTO_DELETE) {
        register_for_cleanup(file_.path());
    }
}

temp_file::~temp_file() {
    cleanup();
}

temp_file::temp_file(temp_file&& other) noexcept
: file_(move(other.file_)), delete_policy_(other.delete_policy_) {
    other.delete_policy_ = DELETE_POLICY::KEEP_ON_EXIT;
}

temp_file& temp_file::operator =(temp_file&& other) noexcept {
    if (this != addressof(other)) {
        cleanup();

        file_ = _MSTL move(other.file_);

        delete_policy_ = other.delete_policy_;
        other.delete_policy_ = DELETE_POLICY::KEEP_ON_EXIT;
    }
    return *this;
}

void temp_file::cleanup() {
    if (delete_policy_ == DELETE_POLICY::KEEP_ON_EXIT) {
        return;
    }

    if (delete_policy_ == DELETE_POLICY::AUTO_DELETE) {
        lock_guard<mutex> lock(get_registry_mutex());
        auto& registry = get_temp_registry();
        const auto it = _MSTL find(registry.begin(), registry.end(), file_.path());
        if (it != registry.end()) {
            registry.erase(it);
        }
    }
    file_.close();
}

void temp_file::release() {
    delete_policy_ = DELETE_POLICY::MANUAL_DELETE;

    lock_guard<mutex> lock(get_registry_mutex());
    auto& registry = get_temp_registry();
    const auto it = _MSTL find(registry.begin(), registry.end(), file_.path());
    if (it != registry.end()) {
        registry.erase(it);
    }
}

temp_file temp_file::create_temp_file(const string& prefix,
    const string& suffix, const FILE_CREATION mode) {
    return temp_file(prefix, suffix, mode, DELETE_POLICY::AUTO_DELETE);
}

MSTL_END_NAMESPACE__
