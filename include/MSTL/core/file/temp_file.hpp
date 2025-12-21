#ifndef MSTL_CORE_FILE_TEMP_FILE_HPP__
#define MSTL_CORE_FILE_TEMP_FILE_HPP__
#include "file.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API temp_file {
public:
    enum class DELETE_POLICY {
        AUTO_DELETE,
        MANUAL_DELETE,
        KEEP_ON_EXIT
    };

    explicit temp_file(
        const string& prefix = "tmp", const string& suffix = ".tmp",
        FILE_CREATION mode = FILE_CREATION::CREATE_FORCE,
        DELETE_POLICY policy = DELETE_POLICY::AUTO_DELETE
    );

    explicit temp_file(
        const path& existing_path,
        DELETE_POLICY policy = DELETE_POLICY::AUTO_DELETE
    );

    ~temp_file();

    temp_file(const temp_file&) = delete;
    temp_file& operator =(const temp_file&) = delete;
    temp_file(temp_file&& other) noexcept;
    temp_file& operator =(temp_file&& other) noexcept;

    _MSTL file& file() noexcept { return file_; }
    const _MSTL file& file() const noexcept { return file_; }

    void keep() noexcept { delete_policy_ = DELETE_POLICY::KEEP_ON_EXIT; }
    void set_delete_policy(const DELETE_POLICY policy) noexcept { delete_policy_ = policy; }
    DELETE_POLICY delete_policy() const noexcept { return delete_policy_; }

    void cleanup();
    void release();

    static temp_file create_temp_file(
            const string& prefix = "tmp",
            const string& suffix = ".tmp",
            FILE_CREATION mode = FILE_CREATION::CREATE_FORCE
    );

    static void cleanup_all_temp_files();
    static void register_for_cleanup(const path& temp_path);

private:
    _MSTL file file_;
    DELETE_POLICY delete_policy_ = DELETE_POLICY::AUTO_DELETE;

    static vector<path>& get_temp_registry();
    static mutex& get_registry_mutex();

    static path generate_unique_path(const string& prefix, const string& suffix);
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_TEMP_FILE_HPP__
