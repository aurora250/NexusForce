#ifndef NEFORCE_CORE_FILE_TEMP_FILE_HPP__
#define NEFORCE_CORE_FILE_TEMP_FILE_HPP__

/**
 * @file temp_file.hpp
 * @brief 临时文件管理类
 *
 * 此文件提供了临时文件的创建、管理和自动清理功能。
 * 支持自定义文件名前缀和后缀，以及多种删除策略。
 */

#include "NeForce/core/file/file.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup File 文件
 * @brief 文件操作
 * @{
 */

/**
 * @class temp_file
 * @brief 临时文件管理类
 *
 * 自动创建和销毁临时文件。
 * 所有创建的临时文件会在程序退出时通过全局清理函数尝试删除。
 */
class NEFORCE_API temp_file {
public:
    /**
     * @enum delete_policy
     * @brief 临时文件删除策略
     */
    enum class delete_policy {
        AUTO_DELETE,   ///< 析构自动删除
        MANUAL_DELETE, ///< 手动删除
        KEEP_ON_EXIT   ///< 程序退出时保留
    };

private:
    _NEFORCE file file_;                                       ///< 内部文件对象
    delete_policy delete_policy_ = delete_policy::AUTO_DELETE; ///< 删除策略

public:
    /**
     * @brief 构造函数：创建新的临时文件
     * @param prefix 文件名前缀（默认 "tmp"）
     * @param suffix 文件名后缀（默认 ".tmp"）
     * @param mode 文件创建模式（默认 CREATE_FORCE）
     * @param policy 删除策略（默认 AUTO_DELETE）
     * @throws system_exception 文件创建失败时抛出
     *
     * 在系统临时目录中生成一个唯一文件名，并打开文件。
     * 生成规则：{prefix}_{纳秒时间}_{进程ID}_{线程ID}_{随机数}{suffix}
     */
    explicit temp_file(const string& prefix = "tmp", const string& suffix = ".tmp",
                       file_creation mode = file_creation::CREATE_FORCE,
                       delete_policy policy = delete_policy::AUTO_DELETE);

    /**
     * @brief 构造函数：接管已有的文件作为临时文件
     * @param existing_path 已有文件的路径
     * @param policy 删除策略（默认 AUTO_DELETE）
     * @throws system_exception 文件打开失败时抛出
     *
     * 将指定的现有文件作为临时文件管理，不会生成新文件。
     */
    explicit temp_file(const path& existing_path, delete_policy policy = delete_policy::AUTO_DELETE);

    /**
     * @brief 析构函数
     *
     * 根据删除策略决定是否删除临时文件：
     * - AUTO_DELETE: 立即删除文件并从全局注册表中移除
     * - MANUAL_DELETE/KEEP_ON_EXIT: 仅关闭文件，不删除
     */
    ~temp_file();

    temp_file(const temp_file&) = delete;
    temp_file& operator=(const temp_file&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 被移动的对象
     *
     * 转移文件所有权，原对象的删除策略被设置为 KEEP_ON_EXIT。
     */
    temp_file(temp_file&& other) noexcept;

    /**
     * @brief 移动赋值运算符
     * @param other 被移动的对象
     * @return 自身引用
     *
     * 先清理当前资源，再转移所有权。
     */
    temp_file& operator=(temp_file&& other) noexcept;

    /**
     * @brief 获取内部文件对象
     * @return 文件对象引用
     */
    NEFORCE_NODISCARD _NEFORCE file& file() noexcept { return file_; }

    /**
     * @brief 获取内部常量文件对象
     * @return 文件对象常量引用
     */
    NEFORCE_NODISCARD const _NEFORCE file& file() const noexcept { return file_; }

    /**
     * @brief 设置删除策略为 KEEP_ON_EXIT
     *
     * 调用后，析构时不会删除文件，也不从全局注册表中移除。
     */
    void keep() noexcept { delete_policy_ = delete_policy::KEEP_ON_EXIT; }

    /**
     * @brief 设置删除策略
     * @param policy 新的删除策略
     */
    void set_delete_policy(const delete_policy policy) noexcept { delete_policy_ = policy; }

    /**
     * @brief 获取当前删除策略
     * @return 删除策略
     */
    NEFORCE_NODISCARD delete_policy policy() const noexcept { return delete_policy_; }

    /**
     * @brief 手动清理临时文件
     *
     * 根据当前删除策略执行清理：
     * - 如果策略为 AUTO_DELETE，则删除文件并从注册表中移除
     * - 其他策略仅关闭文件
     * 此操作后文件对象将处于关闭状态。
     */
    void cleanup();

    /**
     * @brief 释放临时文件的所有权
     *
     * 将删除策略设置为 MANUAL_DELETE，并从全局注册表中移除，
     * 但保持文件打开状态，析构时不会自动删除。
     */
    void release();

    /**
     * @brief 创建临时文件（工厂函数）
     * @param prefix 文件名前缀（默认 "tmp"）
     * @param suffix 文件名后缀（默认 ".tmp"）
     * @param mode 文件创建模式（默认 CREATE_FORCE）
     * @return 新创建的临时文件对象
     *
     * 等价于调用带 AUTO_DELETE 策略的构造函数。
     */
    NEFORCE_NODISCARD static temp_file create_temp_file(const string& prefix = "tmp", const string& suffix = ".tmp",
                                                        file_creation mode = file_creation::CREATE_FORCE);

    /**
     * @brief 清理所有已注册的临时文件
     *
     * 遍历全局注册表，删除所有记录的临时文件。
     * 通常在程序退出时由atexit注册的函数调用。
     */
    static void cleanup_all_temp_files();

    /**
     * @brief 注册一个路径以供程序退出时清理
     * @param temp_path 临时文件路径
     *
     * 将路径添加到全局注册表，在 cleanup_all_temp_files 中被清理。
     * 通常由 temp_file 构造函数自动调用。
     */
    static void register_for_cleanup(const path& temp_path);
};

/** @} */ // File

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_TEMP_FILE_HPP__
