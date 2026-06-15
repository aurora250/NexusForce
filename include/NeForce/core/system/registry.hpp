#ifndef NEFORCE_CORE_SYSTEM_REGISTRY_HPP__
#define NEFORCE_CORE_SYSTEM_REGISTRY_HPP__

/**
 * @file registry.hpp
 * @brief 系统注册表操作
 *
 * 此文件提供了Windows系统注册表操作，
 * 支持系统注册表项的创建、打开、删除、枚举以及值的读写操作。
 */

#include "NeForce/core/config/windef.hpp"
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include "NeForce/core/string/string.hpp"
#    include <windef.h>
#    include <winreg.h>
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct registry_key_exception
 * @brief 系统注册表操作异常
 */
struct registry_key_exception final : system_exception {
    /**
     * @brief 构造函数
     * @param info 异常信息
     * @param type 异常类型名称
     * @param code 错误码
     */
    explicit registry_key_exception(const char* info = "Registry Key Operation Failed.", const char* type = static_type,
                                    const int code = 0) noexcept :
    system_exception(info, type, code) {}

    explicit registry_key_exception(const exception& e) :
    system_exception(e) {}

    ~registry_key_exception() override = default;
    static constexpr auto static_type = "registry_key_exception";
};

/** @} */ // Exceptions

/**
 * @defgroup WindowsSystemRegistry 系统注册表
 * @brief 系统注册表操作
 * @{
 */

/**
 * @class registry_key
 * @brief 系统注册表项类
 *
 * 系统注册表项的打开、创建、读写和枚举操作。
 */
class NEFORCE_API registry_key {
public:
    /**
     * @struct root_key
     * @brief 系统注册表根键常量
     */
    struct root_key {
        /**
         * @brief HKEY_CLASSES_ROOT根键
         * @return HKEY_CLASSES_ROOT句柄
         */
        static ::HKEY classes_root() noexcept { return HKEY_CLASSES_ROOT; }

        /**
         * @brief HKEY_CURRENT_USER根键
         * @return HKEY_CURRENT_USER句柄
         */
        static ::HKEY current_user() noexcept { return HKEY_CURRENT_USER; }

        /**
         * @brief HKEY_LOCAL_MACHINE根键
         * @return HKEY_LOCAL_MACHINE句柄
         */
        static ::HKEY local_machine() noexcept { return HKEY_LOCAL_MACHINE; }

        /**
         * @brief HKEY_USERS根键
         * @return HKEY_USERS句柄
         */
        static ::HKEY users() noexcept { return HKEY_USERS; }

        /**
         * @brief HKEY_CURRENT_CONFIG根键
         * @return HKEY_CURRENT_CONFIG句柄
         */
        static ::HKEY current_config() noexcept { return HKEY_CURRENT_CONFIG; }
    };

    /**
     * @enum value_type
     * @brief 系统注册表值类型枚举
     *
     * 定义系统注册表支持的值类型。
     */
    enum class value_type : ::DWORD {
        none = REG_NONE,               ///< 无类型
        string = REG_SZ,               ///< 字符串类型
        expand_string = REG_EXPAND_SZ, ///< 可扩展字符串
        binary = REG_BINARY,           ///< 二进制数据
        dword = REG_DWORD,             ///< 32位无符号整数
        qword = REG_QWORD,             ///< 64位无符号整数
        multi_string = REG_MULTI_SZ    ///< 多字符串类型
    };

    /**
     * @enum wow64_view
     * @brief 注册表 32/64 位视图控制
     */
    enum wow64_view : ::REGSAM {
        view_default = 0,             /**< 默认视图 */
        view_32bit = KEY_WOW64_32KEY, /**< 32 位视图 */
        view_64bit = KEY_WOW64_64KEY, /**< 64 位视图 */
    };

    /**
     * @struct value_info
     * @brief 系统注册表值信息结构
     */
    struct NEFORCE_API value_info {
        wstring name;        ///< 值名称
        value_type type;     ///< 值类型
        vector<byte_t> data; ///< 原始数据

        /**
         * @brief 将值数据转换为字符串
         * @return 字符串值
         * @throws registry_key_exception 当类型不是字符串类型时抛出
         */
        NEFORCE_NODISCARD wstring to_string() const;

        /**
         * @brief 将值数据转换为32位整数
         * @return DWORD值
         */
        NEFORCE_NODISCARD ::DWORD to_dword() const noexcept;

        /**
         * @brief 将值数据转换为64位整数
         * @return QWORD值
         */
        NEFORCE_NODISCARD ::ULONGLONG to_qword() const noexcept;

        /**
         * @brief 将值数据转换为多字符串列表
         * @return 字符串向量
         * @throws registry_key_exception 当类型不是REG_MULTI_SZ时抛出
         */
        NEFORCE_NODISCARD vector<wstring> to_multi_string() const;
    };

private:
    ::HKEY hkey_ = nullptr;    ///< 注册表句柄
    bool owns_handle_ = false; ///< 是否拥有句柄所有权

    void close() noexcept;
    void throw_if_invalid() const;
    NEFORCE_NODISCARD value_info get_value_info(const wstring& name) const;

public:
    /**
     * @brief 默认构造函数
     *
     * 构造一个无效的注册表项对象。
     */
    registry_key() = default;

    /**
     * @brief 从原生句柄构造注册表项
     * @param key 原生注册表句柄
     *
     * 构造对象接管句柄的所有权，析构时自动关闭句柄。
     */
    explicit registry_key(::HKEY key);

    /**
     * @brief 析构函数
     *
     * 自动关闭注册表句柄。
     */
    ~registry_key();

    registry_key(const registry_key&) = delete;
    registry_key& operator=(const registry_key&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 源对象
     */
    registry_key(registry_key&& other) noexcept;

    /**
     * @brief 移动赋值运算符
     * @param other 源对象
     * @return 自身引用
     */
    registry_key& operator=(registry_key&& other) noexcept;

    /**
     * @brief 检查注册表项是否有效
     * @return 有效返回true，否则返回false
     */
    NEFORCE_NODISCARD bool is_valid() const noexcept { return hkey_ != nullptr; }

    /**
     * @brief 布尔转换运算符
     * @return 有效返回true，否则返回false
     */
    explicit operator bool() const noexcept { return is_valid(); }

    /**
     * @brief 创建子项
     * @param name 子项名称
     * @throws registry_key_exception 当创建失败时抛出
     *
     * 在当前项下创建指定的子项。如果子项已存在，则直接打开。
     */
    void create_sub_key(const wstring& name);

    /**
     * @brief 打开注册表项
     * @param root 根键
     * @param path 完整路径
     * @param sam_desired 访问权限，默认为读写
     * @throws registry_key_exception 当打开失败时抛出
     *
     * 打开指定的注册表路径，替换当前对象的内容。原有句柄将被关闭。
     */
    void open(::HKEY root, const wstring& path, ::REGSAM sam_desired = KEY_READ | KEY_WRITE);

    /**
     * @brief 打开子项
     * @param name 子项名称
     * @param sam_desired 访问权限，默认为只读
     * @return 子项对应的registry_key对象
     * @throws registry_key_exception 当子项不存在或打开失败时抛出
     */
    NEFORCE_NODISCARD registry_key open_sub_key(const wstring& name, REGSAM sam_desired = KEY_READ) const;

    /**
     * @brief 删除子项
     * @param name 子项名称
     * @throws registry_key_exception 当删除失败时抛出
     *
     * @note 子项必须为空，否则删除失败。如需删除包含子项的项，请使用delete_key_tree。
     */
    void delete_sub_key(const wstring& name);

    /**
     * @brief 删除值
     * @param name 值名称
     * @throws registry_key_exception 当值不存在或删除失败时抛出
     */
    void delete_value(const wstring& name);

    /**
     * @brief 递归删除注册表项树
     * @param root 根键
     * @param path 完整路径
     *
     * 递归删除指定路径下的所有子项和值。
     */
    static void delete_key_tree(::HKEY root, const wstring& path);

    /**
     * @brief 检查是否存在指定的子项
     * @param name 子项名称
     * @return 存在返回true，否则返回false
     */
    NEFORCE_NODISCARD bool has_sub_key(const wstring& name) const;

    /**
     * @brief 检查是否存在指定的值
     * @param name 值名称
     * @return 存在返回true，否则返回false
     */
    NEFORCE_NODISCARD bool has_value(const wstring& name) const;

    /**
     * @brief 枚举所有子项名称
     * @return 子项名称的字符串向量
     * @throws registry_key_exception 当枚举失败时抛出
     */
    NEFORCE_NODISCARD vector<wstring> enum_sub_key_names() const;

    /**
     * @brief 枚举所有值
     * @return 值信息结构的向量
     * @throws registry_key_exception 当枚举失败时抛出
     */
    NEFORCE_NODISCARD vector<value_info> enum_values() const;

    /**
     * @brief 设置字符串值
     * @param name 值名称
     * @param value 字符串值
     * @throws registry_key_exception 当写入失败时抛出
     */
    void set_string_value(const wstring& name, const wstring& value);

    /**
     * @brief 设置可扩展字符串值
     * @param name 值名称
     * @param value 可扩展字符串
     * @throws registry_key_exception 当写入失败时抛出
     */
    void set_expand_string_value(const wstring& name, const wstring& value);

    /**
     * @brief 设置32位整数值
     * @param name 值名称
     * @param value DWORD值
     * @throws registry_key_exception 当写入失败时抛出
     */
    void set_dword_value(const wstring& name, ::DWORD value);

    /**
     * @brief 设置64位整数值
     * @param name 值名称
     * @param value QWORD值
     * @throws registry_key_exception 当写入失败时抛出
     */
    void set_qword_value(const wstring& name, ::ULONGLONG value);

    /**
     * @brief 设置二进制值
     * @param name 值名称
     * @param data 二进制数据指针
     * @param size 数据大小（字节）
     * @throws registry_key_exception 当写入失败时抛出
     */
    void set_binary_value(const wstring& name, const ::BYTE* data, ::DWORD size);

    /**
     * @brief 设置多字符串值
     * @param name 值名称
     * @param values 字符串向量
     * @throws registry_key_exception 当写入失败时抛出
     *
     * 多字符串值是以空字符分隔、双空字符结尾的字符串列表。
     */
    void set_multi_string_value(const wstring& name, const vector<wstring>& values);

    /**
     * @brief 获取字符串值
     * @param name 值名称
     * @param default_val 默认值（当值不存在时返回）
     * @return 字符串值
     * @throws registry_key_exception 当值类型不是字符串类型时抛出
     */
    NEFORCE_NODISCARD wstring get_string_value(const wstring& name, const wstring& default_val = L"") const;

    /**
     * @brief 获取32位整数值
     * @param name 值名称
     * @param default_val 默认值（当值不存在时返回）
     * @return DWORD值
     */
    NEFORCE_NODISCARD ::DWORD get_dword_value(const wstring& name, DWORD default_val = 0) const noexcept;

    /**
     * @brief 获取64位整数值
     * @param name 值名称
     * @param default_val 默认值（当值不存在时返回）
     * @return QWORD值
     */
    NEFORCE_NODISCARD ::ULONGLONG get_qword_value(const wstring& name, ULONGLONG default_val = 0) const noexcept;

    /**
     * @brief 获取二进制值
     * @param name 值名称
     * @return 二进制数据向量
     * @throws registry_key_exception 当值不存在或类型不是REG_BINARY时抛出
     */
    NEFORCE_NODISCARD vector<byte_t> get_binary_value(const wstring& name) const;

    /**
     * @brief 获取多字符串值
     * @param name 值名称
     * @return 字符串向量
     * @throws registry_key_exception 当值不存在或类型不是REG_MULTI_SZ时抛出
     */
    NEFORCE_NODISCARD vector<wstring> get_multi_string_value(const wstring& name) const;

    /**
     * @brief 获取原生注册表句柄
     * @return HKEY句柄
     */
    NEFORCE_NODISCARD ::HKEY native_handle() const noexcept { return hkey_; }

    /**
     * @brief 打开注册表项
     * @param root 根键
     * @param path 完整路径
     * @param view WOW64 视图选择
     * @param sam_desired 访问权限，默认为读写
     * @throws registry_key_exception 当打开失败时抛出
     */
    void open(::HKEY root, const wstring& path, wow64_view view, ::REGSAM sam_desired = KEY_READ | KEY_WRITE);

    /**
     * @brief 监听注册表项变更（阻塞直到变更或超时）
     * @param watch_subtree 是否同时监听子项
     * @param timeout_ms 超时毫秒，-1 无限等待
     * @return 是否发生了变更
     * @throws registry_key_exception 监听失败时抛出
     */
    bool notify_change(bool watch_subtree = true, int timeout_ms = -1);
};

/** @} */ // Registry

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_CORE_SYSTEM_REGISTRY_HPP__
