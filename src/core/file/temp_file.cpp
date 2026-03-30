#include <NeForce/core/file/temp_file.hpp>
#include <NeForce/core/file/filesystem.hpp>
#include <NeForce/core/time/clocks.hpp>
#include <NeForce/core/async/thread.hpp>
#include <NeForce/core/async/atomic.hpp>
#include <NeForce/core/system/environment.hpp>
#include <NeForce/core/system/process.hpp>
#include <NeForce/core/numeric/random.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    vector<path>& get_temp_registry() {
        static vector<path> registry;
        return registry;
    }

    mutex& get_registry_mutex() {
        static mutex mutex;
        return mutex;
    }

    path generate_unique_path(const string& prefix, const string& suffix) {
        const path temp_dir{environment::temp_directory()};
        static atomic<uint64_t> counter{0};
        counter.fetch_add(1, memory_order_relaxed);

        const auto nanos = system_clock::now().since_epoch().to_nano();
        const auto pid = process::current_id();
        random_mt rand;
        const uint64_t random_part = rand.next_uint64();

        const string filename = format(
            "{}_{}_{}_{}_{}{}",
            prefix,
            nanos.count(),
            pid,
            this_thread::id().native_handle(),
            random_part,
            suffix
        );
        return temp_dir / path(filename);
    }
}


void temp_file::register_for_cleanup(const path& temp_path) {
    lock<mutex> lk(get_registry_mutex());
    auto& registry = get_temp_registry();
    if (find(registry.begin(), registry.end(), temp_path) == registry.end()) {
        registry.push_back(temp_path);
    }
}

void temp_file::cleanup_all_temp_files() {
    vector<path> to_delete;
    {
        lock<mutex> lk(get_registry_mutex());
        auto& registry = get_temp_registry();
        to_delete = registry;
        registry.clear();
    }

    for (const auto& temp_path : to_delete) {
        if (temp_path.exists()) {
            try {
                if (temp_path.is_directory()) {
                    filesystem::remove_all(temp_path);
                } else {
                    filesystem::remove(temp_path);
                }
            } catch (...) { /* ignore */ }
        }
    }
}

temp_file::temp_file(const string& prefix, const string& suffix, const file_creation mode, const delete_policy policy)
: file_(generate_unique_path(prefix, suffix), false, file_access::READ_WRITE, file_shared::SHARE_READ, mode),
  delete_policy_(policy) {
    constexpr int max_retries = 8;
    bool opened = false;

    for (int i = 0; i < max_retries; ++i) {
        const path candidate = generate_unique_path(prefix, suffix);
        file_ = _NEFORCE file(candidate, false,
                              file_access::READ_WRITE,
                              file_shared::SHARE_READ,
                              mode);
        if (file_.is_opened()) {
            opened = true;
            break;
        }
    }

    if (!opened) {
        NEFORCE_THROW_EXCEPTION(system_exception("Failed to create temporary file"));
    }

    if (delete_policy_ == delete_policy::AUTO_DELETE) {
        register_for_cleanup(file_.file_path());
    }
}

temp_file::temp_file(const path& existing_path, const delete_policy policy)
: file_(existing_path, false, file_access::READ_WRITE, file_shared::SHARE_READ_WRITE, file_creation::OPEN_EXIST),
  delete_policy_(policy) {
    if (!file_.is_opened()) {
        NEFORCE_THROW_EXCEPTION(system_exception("Failed to open existing file as temporary file"));
    }
    if (delete_policy_ == delete_policy::AUTO_DELETE) {
        register_for_cleanup(file_.file_path());
    }
}

temp_file::~temp_file() {
    cleanup();
}

temp_file::temp_file(temp_file&& other) noexcept
: file_(move(other.file_)), delete_policy_(other.delete_policy_) {
    other.delete_policy_ = delete_policy::KEEP_ON_EXIT;
}

temp_file& temp_file::operator =(temp_file&& other) noexcept {
    if (addressof(other) == this) return *this;

    cleanup();

    const path other_path = other.file_.file_path();
    const delete_policy other_policy = other.delete_policy_;

    file_ = move(other.file_);
    delete_policy_ = other_policy;
    other.delete_policy_ = delete_policy::KEEP_ON_EXIT;

    return *this;
}

void temp_file::cleanup() {
    if (delete_policy_ == delete_policy::KEEP_ON_EXIT) {
        file_.close();
        return;
    }

    const path file_path = file_.file_path();
    file_.close();

    {
        lock<mutex> lk(get_registry_mutex());
        auto& registry = get_temp_registry();
        const auto it = find(registry.begin(), registry.end(), file_path);
        if (it != registry.end()) {
            registry.erase(it);
        }
    }

    if (delete_policy_ == delete_policy::AUTO_DELETE) {
        if (file_path.exists()) {
            try {
                if (file_path.is_directory()) {
                    filesystem::remove_all(file_path);
                } else {
                    filesystem::remove(file_path);
                }
            } catch (...) {}
        }
    }
}

void temp_file::release() {
    {
        lock<mutex> lk(get_registry_mutex());
        auto& registry = get_temp_registry();
        const auto it = find(registry.begin(), registry.end(), file_.file_path());
        if (it != registry.end()) {
            registry.erase(it);
        }
    }
    delete_policy_ = delete_policy::MANUAL_DELETE;
}

temp_file temp_file::create_temp_file(const string& prefix,
    const string& suffix, const file_creation mode) {
    return temp_file(prefix, suffix, mode, delete_policy::AUTO_DELETE);
}

NEFORCE_END_NAMESPACE__
