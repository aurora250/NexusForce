#include <NeForce/core/file/temp_file.hpp>
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
    lock<mutex> lock(get_registry_mutex());
    get_temp_registry().push_back(temp_path);
}

void temp_file::cleanup_all_temp_files() {
    lock<mutex> lock(get_registry_mutex());
    auto& registry = get_temp_registry();

    for (const auto& temp_path : registry) {
        if (temp_path.exists()) {
            try {
                if (temp_path.is_directory()) {
                    NEFORCE_IGNORE temp_path.remove_all();
                } else {
                    NEFORCE_IGNORE temp_path.remove();
                }
            } catch (...) {}
        }
    }

    registry.clear();
}

temp_file::temp_file(const string& prefix, const string& suffix, const FILE_CREATION mode, const DELETE_POLICY policy)
: file_(generate_unique_path(prefix, suffix), FILE_ACCESS::READ_WRITE, FILE_SHARED::SHARE_READ, mode),
  delete_policy_(policy) {
    try {
        if (file_.is_opened()) {
            if (delete_policy_ == DELETE_POLICY::AUTO_DELETE) {
                register_for_cleanup(file_.path());
            }
        } else {
            NEFORCE_THROW_EXCEPTION(system_exception("Failed to create temporary file"));
        }
    } catch (...) {
        if (file_.path().exists()) {
            NEFORCE_IGNORE file_.path().remove();
        }
        throw;
    }
}

temp_file::temp_file(const path& existing_path, const DELETE_POLICY policy)
: file_(existing_path, FILE_ACCESS::READ_WRITE, FILE_SHARED::SHARE_READ_WRITE, FILE_CREATION::OPEN_EXIST),
  delete_policy_(policy) {
    if (delete_policy_ == DELETE_POLICY::AUTO_DELETE) {
        register_for_cleanup(file_.path());
    }
}

temp_file::~temp_file() {
    cleanup();
}

temp_file::temp_file(temp_file&& other) noexcept
: file_(_NEFORCE move(other.file_)), delete_policy_(other.delete_policy_) {
    other.delete_policy_ = DELETE_POLICY::KEEP_ON_EXIT;
}

temp_file& temp_file::operator =(temp_file&& other) noexcept {
    if (this != addressof(other)) {
        cleanup();

        file_ = _NEFORCE move(other.file_);

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
        lock<mutex> lock(get_registry_mutex());
        auto& registry = get_temp_registry();
        const auto it = _NEFORCE find(registry.begin(), registry.end(), file_.path());
        if (it != registry.end()) {
            registry.erase(it);
        }
    }
    file_.close();
}

void temp_file::release() {
    delete_policy_ = DELETE_POLICY::MANUAL_DELETE;

    lock<mutex> lock(get_registry_mutex());
    auto& registry = get_temp_registry();
    const auto it = _NEFORCE find(registry.begin(), registry.end(), file_.path());
    if (it != registry.end()) {
        registry.erase(it);
    }
}

temp_file temp_file::create_temp_file(const string& prefix,
    const string& suffix, const FILE_CREATION mode) {
    return temp_file(prefix, suffix, mode, DELETE_POLICY::AUTO_DELETE);
}

NEFORCE_END_NAMESPACE__
