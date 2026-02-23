#ifndef MSTL_PLUGIN_DYNAMIC_LIBRARY_HPP__
#define MSTL_PLUGIN_DYNAMIC_LIBRARY_HPP__

/**
 * @file dynamic_library.hpp
 * @brief 动态库加载器
 *
 * 此文件提供了动态链接库加载和符号解析功能。
 */

#include "MSTL/core/string/string.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief MSTL异常类集
 * @{
 */

/**
 * @struct dynamic_library_exception
 * @extends system_exception
 * @brief 动态库操作异常
 */
MSTL_ERROR_BUILD_FINAL_CLASS(dynamic_library_exception, system_exception, "Dynamic Library Operation Failed")

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
class MSTL_API dynamic_library {
private:
    void* handle_;  ///< 动态库句柄
    string path_;   ///< 库文件路径

private:
    /**
     * @brief 打开动态库
     * @throws dl_exception 加载失败时抛出
     */
    void open();

    /**
     * @brief 关闭动态库
     */
    void close();

public:
    /**
     * @brief 构造函数，打开指定的动态库
     * @param pth 动态库路径
     * @throws dynamic_library_exception 加载失败时抛出
     */
    explicit dynamic_library(const string& pth);

    ~dynamic_library();

    dynamic_library(const dynamic_library&) = delete;
    dynamic_library& operator =(const dynamic_library&) = delete;

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
    dynamic_library& operator =(dynamic_library&& other) noexcept;

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
    MSTL_NODISCARD void* symbol(const string& name) const;

    /**
     * @brief 检查符号是否存在
     * @param name 符号名称
     * @return 符号是否存在
     */
    MSTL_NODISCARD bool has_symbol(const string& name) const noexcept;

    /**
     * @brief 检查动态库是否已加载
     * @return 是否已加载
     */
    MSTL_NODISCARD bool is_open() const noexcept {
        return handle_ != nullptr;
    }

    /**
     * @brief 卸载动态库
     */
    void unload() {
        close();
    }

    /**
     * @brief 获取原生句柄
     * @return 平台相关的动态库句柄
     */
    MSTL_NODISCARD void* native_handle() const noexcept {
        return handle_;
    }

    /**
     * @brief 获取库文件路径
     * @return 路径字符串
     */
    MSTL_NODISCARD const string& path() const noexcept {
        return path_;
    }
};

/** @} */ // DynamicLibrary

MSTL_END_NAMESPACE__
#endif // MSTL_PLUGIN_DYNAMIC_LIBRARY_HPP__
