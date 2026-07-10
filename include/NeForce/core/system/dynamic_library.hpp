#ifndef NEFORCE_PLUGIN_DYNAMIC_LIBRARY_HPP__
#define NEFORCE_PLUGIN_DYNAMIC_LIBRARY_HPP__

/**
 * @file dynamic_library.hpp
 * @brief 动态库加载器
 *
 * 此文件提供了动态链接库加载和符号解析功能。
 */

#include "NeForce/core/exception/system_exception.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct dynamic_library_exception
 * @brief 动态库操作异常
 */
struct dynamic_library_exception final : system_exception {
    explicit dynamic_library_exception(const char* info = "Dynamic Library Operation Failed.",
                                       const error_code code = last_error()) noexcept :
    system_exception(info, code) {}

    explicit dynamic_library_exception(const exception& e) :
    system_exception(e) {}

    ~dynamic_library_exception() override = default;

    NEFORCE_NODISCARD const char* type() const noexcept override { return "dynamic_library_exception"; }
};

/** @} */ // Exceptions

/**
 * @defgroup DynamicLibrary 动态库
 * @brief 动态链接库加载和符号解析
 * @{
 */

/**
 * @class dynamic_library
 * @brief 动态链接库加载器
 *
 * 支持动态库的加载、卸载和符号解析操作。
 */
class NEFORCE_API dynamic_library {
public:
    /**
     * @enum load_mode
     * @brief 动态库加载模式
     */
    enum class load_mode : int {
        default_ = 0,      /**< 默认模式（惰性解析，本地符号可见性） */
        lazy = 1 << 0,     /**< 惰性符号解析 */
        now = 1 << 1,      /**< 立即符号解析 */
        global = 1 << 2,   /**< 全局符号可见性 */
        local = 1 << 3,    /**< 本地符号可见性 */
        deep_bind = 1 << 4 /**< 深层绑定 */
    };

private:
    void* handle_{nullptr};                    /**< 动态库句柄 */
    string path_;                              /**< 库文件路径 */
    load_mode load_mode_{load_mode::default_}; /**< 加载模式 */

private:
    void open();
    void close();

public:
    /**
     * @brief 默认构造函数
     *
     * 创建一个未初始化的动态库对象。
     */
    dynamic_library() noexcept = default;

    /**
     * @brief 构造函数，打开指定的动态库
     * @param pth 动态库路径
     * @throws dynamic_library_exception 加载失败时抛出
     */
    explicit dynamic_library(string pth);

    /**
     * @brief 以指定的模式打开动态库
     * @param pth 动态库路径
     * @param mode 加载模式
     * @throws dynamic_library_exception 加载失败时抛出
     */
    dynamic_library(string pth, load_mode mode);

    ~dynamic_library();

    dynamic_library(const dynamic_library&) = delete;
    dynamic_library& operator=(const dynamic_library&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 被移动的对象
     */
    dynamic_library(dynamic_library&& other) noexcept;

    /**
     * @brief 移动赋值运算符
     * @param other 被移动的对象
     * @return 自身引用
     */
    dynamic_library& operator=(dynamic_library&& other) noexcept;

    /**
     * @brief 获取符号
     * @tparam T 符号类型（函数指针类型）
     * @param name 符号名称
     * @return 符号地址，转换为指定类型
     * @throws dynamic_library_exception 符号不存在时抛出
     */
    template <typename T>
    T to_symbol(const string& name) const {
        return reinterpret_cast<T>(symbol(name));
    }

    /**
     * @brief 获取原始符号地址
     * @param name 符号名称
     * @return 符号地址
     * @throws dynamic_library_exception 符号不存在时抛出
     */
    NEFORCE_NODISCARD void* symbol(const string& name) const;

    /**
     * @brief 检查符号是否存在
     * @param name 符号名称
     * @return 符号是否存在
     */
    NEFORCE_NODISCARD bool has_symbol(const string& name) const noexcept;

    /**
     * @brief 检查动态库是否已加载
     * @return 是否已加载
     */
    NEFORCE_NODISCARD bool is_open() const noexcept { return handle_ != nullptr; }

    /**
     * @brief 卸载动态库
     */
    void unload() { close(); }

    /**
     * @brief 获取原生句柄
     * @return 平台相关的动态库句柄
     */
    NEFORCE_NODISCARD void* native_handle() const noexcept { return handle_; }

    /**
     * @brief 获取库文件路径
     * @return 路径字符串
     */
    NEFORCE_NODISCARD const string& path() const noexcept { return path_; }

    /**
     * @brief 获取当前加载模式
     * @return 加载模式枚举值
     */
    NEFORCE_NODISCARD load_mode get_load_mode() const noexcept { return load_mode_; }

    /**
     * @brief 加载自身（当前进程可执行文件）
     * @return 表示当前进程的动态库对象
     * @note 返回的对象析构时不会卸载自身
     */
    static dynamic_library load_self();

    /**
     * @brief 按名称加载动态库
     * @param name 库名称（不含前缀和后缀）
     * @param mode 加载模式
     * @return 动态库对象
     * @throws dynamic_library_exception 加载失败时抛出
     */
    static dynamic_library load_by_name(const string& name, load_mode mode = load_mode::default_);

    /**
     * @brief 列出动态库中的符号
     * @param name_filter 名称过滤（留空返回全部）
     * @return 符号名称列表
     * @note Windows 不支持，返回空列表
     */
    NEFORCE_NODISCARD vector<string> list_symbols(const string& name_filter = "") const;

    /**
     * @brief 获取当前程序的可执行文件路径
     * @return 可执行文件完整路径
     */
    NEFORCE_NODISCARD static string program_location();

    /**
     * @brief 获取指定符号所在的动态库路径
     * @param symbol_ptr 符号地址
     * @return 动态库文件路径，失败返回空字符串
     */
    NEFORCE_NODISCARD static string symbol_location(void* symbol_ptr);
};

/** @} */ // DynamicLibrary

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_PLUGIN_DYNAMIC_LIBRARY_HPP__
