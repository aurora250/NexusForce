#ifndef MSTL_PLUGIN_IPLUGIN_HPP__
#define MSTL_PLUGIN_IPLUGIN_HPP__

/**
 * @file iplugin.hpp
 * @brief 插件接口定义
 *
 * 此文件定义了插件系统的核心接口，包括插件信息结构、
 * 插件生命周期接口和插件指针管理类型。
 */

#include "MSTL/core/memory/unique_ptr.hpp"
#include "MSTL/core/string/string.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup Plugin 插件
 * @brief 动态加载插件管理
 * @{
 */

/**
 * @struct plugin_info
 * @brief 插件信息结构
 *
 * 包含插件的基本元数据，用于插件识别和管理。
 */
struct plugin_info {
    string name;           ///< 插件名称
    string version;        ///< 插件版本
    string author;         ///< 插件作者
    string description;    ///< 插件描述
    string library_path;   ///< 插件库路径
};


/**
 * @struct iplugin
 * @brief 插件接口基类
 *
 * 所有插件必须实现的接口，定义了插件的生命周期管理方法。
 * 插件需要实现初始化、执行和关闭等操作。
 */
struct iplugin {
    virtual ~iplugin() = default;

    /**
     * @brief 获取插件信息
     * @return 插件信息结构引用
     */
    virtual const plugin_info& get_info() const = 0;

    /**
     * @brief 初始化插件
     *
     * 在插件加载后调用，用于执行初始化操作。
     */
    virtual void initialize() = 0;

    /**
     * @brief 执行插件的主要功能
     */
    virtual void execute() = 0;

    /**
     * @brief 关闭插件
     *
     * 在插件卸载前调用，用于清理资源。
     */
    virtual void shutdown() = 0;
};

/**
 * @struct plugin_deleter
 * @brief 插件自定义删除器
 *
 * 用于 unique_ptr 管理插件对象的生命周期。
 * 通过函数指针调用插件库提供的删除函数。
 */
struct plugin_deleter {
public:
    using deleter_type = void(*)(iplugin*);  ///< 删除函数类型

private:
    deleter_type func_ = nullptr;  ///< 删除函数指针

public:
    /**
     * @brief 默认构造函数
     */
    plugin_deleter() noexcept = default;

    /**
     * @brief 析构函数
     */
    ~plugin_deleter() = default;

    /**
     * @brief 构造函数
     * @param func 删除函数指针
     */
    explicit plugin_deleter(deleter_type func) noexcept
    : func_(func) {}

    plugin_deleter(const plugin_deleter&) = delete;
    plugin_deleter& operator =(const plugin_deleter&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 被移动的删除器
     */
    plugin_deleter(plugin_deleter&& other) noexcept
    : func_(other.func_) {
        other.func_ = nullptr;
    }

    /**
     * @brief 移动赋值运算符
     * @param other 被移动的删除器
     * @return 自身引用
     */
    plugin_deleter& operator =(plugin_deleter&& other) noexcept {
        func_ = other.func_;
        other.func_ = nullptr;
        return *this;
    }

    /**
     * @brief 函数调用运算符
     * @param plugin 要删除的插件指针
     *
     * 调用实际的删除函数销毁插件对象。
     */
    void operator ()(iplugin* plugin) const
    noexcept(is_nothrow_invocable_v<deleter_type, iplugin*>) {
        if (plugin) func_(plugin);
    }

    /**
     * @brief 重新绑定删除器
     * @return 新的删除器
     *
     * 用于在移动语义中获取新的删除器实例。
     */
    plugin_deleter rebind() && noexcept {
        return plugin_deleter(_MSTL move(*this));
    }
};

/**
 * @brief 插件唯一指针类型
 *
 * 使用自定义删除器管理插件对象生命周期，
 * 确保插件从正确的动态库中卸载。
 */
using plugin_ptr = unique_ptr<iplugin, plugin_deleter>;

/** @} */ // Plugin

MSTL_END_NAMESPACE__
#endif // MSTL_PLUGIN_IPLUGIN_HPP__
