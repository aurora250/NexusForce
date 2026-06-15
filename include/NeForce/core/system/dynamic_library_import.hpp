#ifndef NEFORCE_CORE_SYSTEM_DYNAMIC_LIBRARY_IMPORT_HPP__
#define NEFORCE_CORE_SYSTEM_DYNAMIC_LIBRARY_IMPORT_HPP__

/**
 * @file dynamic_library_import.hpp
 * @brief 动态库符号导入宏工具
 *
 * 提供便捷宏用于从动态库中导入函数符号。
 */

#include "NeForce/core/system/dynamic_library.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @def NEFORCE_DLL_IMPORT
 * @brief 从动态库导入函数符号
 * @param ret 返回类型
 * @param name 变量名（作为符号在代码中的名称）
 * @param lib 动态库引用
 * @param sym 符号名称（动态库中的实际符号名）
 *
 * @code
 * dynamic_library dll("myplugin.so");
 * NEFORCE_DLL_IMPORT(int, my_func, dll, "my_function");
 * int result = my_func(42);
 * @endcode
 */
#define NEFORCE_DLL_IMPORT(ret, name, lib, sym) const auto name = (lib).template to_symbol<ret>(sym)

/**
 * @def NEFORCE_DLL_IMPORT_OR
 * @brief 从动态库导入函数符号，失败时返回默认值
 * @param ret 返回类型
 * @param name 变量名
 * @param lib 动态库引用
 * @param sym 符号名称
 * @param default_val 符号不存在时的默认值
 */
#define NEFORCE_DLL_IMPORT_OR(ret, name, lib, sym, default_val) \
    const auto name = (lib).has_symbol(sym) ? (lib).template to_symbol<ret>(sym) : (default_val)

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_DYNAMIC_LIBRARY_IMPORT_HPP__
