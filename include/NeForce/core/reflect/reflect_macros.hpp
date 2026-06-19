#ifndef NEFORCE_CORE_REFLECT_REFLECT_MACROS_HPP__
#define NEFORCE_CORE_REFLECT_REFLECT_MACROS_HPP__

/**
 * @file reflect_macros.hpp
 * @brief 反射扫描器标记宏
 *
 * 此文件定义了用于 reflect_scanner 扫描的标记宏。
 * 这些宏放在类体内作为占位标记，由扫描器识别并生成对应的注册代码。
 */

/**
 * @defgroup Reflection 反射系统
 * @brief 运行时类型反射系统
 * @{
 */

/**
 * @def NEFORCE_REFLECT_OBJ
 * @brief 标记类需要反射注册
 * @param Class 类名
 *
 * 在类体内放置此宏，扫描器将为该类生成完整的 meta_type 注册代码。
 */
#define NEFORCE_REFLECT_OBJ(Class)

/**
 * @def NEFORCE_REFLECT_PROP
 * @brief 标记成员属性
 * @param type 属性类型
 * @param name 属性名称
 *
 * 在类体内放置此宏标记需要反射的成员变量。
 * 扫描器会检查类定义中对应的成员声明。
 */
#define NEFORCE_REFLECT_PROP(type, name)

/**
 * @def NEFORCE_REFLECT_PROP_ATTR
 * @brief 标记带注解的成员属性
 * @param type 属性类型
 * @param name 属性名称
 * @param attrs 属性注解（PROP_TRANSIENT | PROP_OPTIONAL 等）
 */
#define NEFORCE_REFLECT_PROP_ATTR(type, name, attrs)

/**
 * @def NEFORCE_REFLECT_FUNC
 * @brief 标记成员函数
 * @param ret 返回值类型
 * @param name 函数名称
 * @param ... 参数类型列表
 */
#define NEFORCE_REFLECT_FUNC(ret, name, ...)

/**
 * @def NEFORCE_REFLECT_SIGNAL
 * @brief 标记信号成员
 * @param sig_type 信号类型（如 neforce::signal<int>）
 * @param name 信号成员名称
 * @param ... 信号参数类型列表（可选）
 *
 * 在类体内放置此宏标记信号成员变量。
 * 扫描器会识别并生成信号注册代码。
 */
#define NEFORCE_REFLECT_SIGNAL(sig_type, name, ...)

/**
 * @def NEFORCE_REFLECT_ENUM
 * @brief 标记枚举类型需要反射注册
 * @param Enum 枚举类型名
 * @param Underlying 底层整数类型
 *
 * 此标记宏用于让扫描器识别枚举类型并生成反射注册代码。
 * 功能版本在 reflect.hpp 中定义。
 */
#define NEFORCE_REFLECT_ENUM(Enum, Underlying)

/**
 * @def NEFORCE_REFLECT_ENUM_VAL
 * @brief 标记枚举值条目
 * @param Enum 枚举类型名
 * @param Name 枚举值名称
 */
#define NEFORCE_REFLECT_ENUM_VAL(Enum, Name)

/**
 * @def NEFORCE_DB_TABLE
 * @brief 指定数据库表名
 * @param TableName 数据库表名字符串字面量
 *
 * 当目标表名与类名不一致时，在类体内放置此宏指定表名映射。
 * 扫描器会生成 table_name() 调用将表名写入 meta_type。
 */
#define NEFORCE_DB_TABLE(TableName)

/** @} */ // Reflection

#endif // NEFORCE_CORE_REFLECT_REFLECT_MACROS_HPP__
