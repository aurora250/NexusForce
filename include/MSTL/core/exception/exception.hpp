#ifndef MSTL_CORE_CONFIG_EXCEPTION_HPP__
#define MSTL_CORE_CONFIG_EXCEPTION_HPP__

/**
 * @file exception.hpp
 * @brief MSTL异常处理框架
 *
 * 此文件提供了异常处理框架，
 * 包括异常基类、各种特定异常类型以及相关的辅助宏和工具函数。
 */

#include "../string/cstring.hpp"
#include "../memory/memory.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup ExceptionHandling 异常处理
 * @brief MSTL异常处理框架的实现
 * @{
 */

#define __MSTL_ERROR_CONSTRUCTOR(THIS, BASE, INFO) \
	explicit THIS(const char* info = INFO, const char* type = static_type) noexcept \
		: BASE(info, type) {} \
	explicit THIS(const exception& e) : BASE(e) {}

#define __MSTL_ERROR_DERIVED_DESTRUCTOR(CLASS) \
	virtual ~CLASS() = default;

#define __MSTL_ERROR_FINAL_DESTRUCTOR(CLASS) \
	~CLASS() override = default;

#define __MSTL_ERROR_TYPE(CLASS) \
	static constexpr auto static_type = #CLASS;

/**
 * @def MSTL_ERROR_BUILD_DERIVED_CLASS
 * @brief 构建可派生的异常类宏
 * @param THIS 当前类名
 * @param BASE 基类名
 * @param INFO 默认错误信息
 *
 * 快速定义可进一步派生的异常类。
 */
#define MSTL_ERROR_BUILD_DERIVED_CLASS(THIS, BASE, INFO) \
	struct MSTL_API THIS : BASE { \
		__MSTL_ERROR_CONSTRUCTOR(THIS, BASE, INFO) \
		__MSTL_ERROR_DERIVED_DESTRUCTOR(THIS) \
		__MSTL_ERROR_TYPE(THIS) \
	};

/**
 * @def MSTL_ERROR_BUILD_FINAL_CLASS
 * @brief 构建最终异常类宏
 * @param THIS 当前类名
 * @param BASE 基类名
 * @param INFO 默认错误信息
 *
 * 快速定义不可派生的异常类。
 */
#define MSTL_ERROR_BUILD_FINAL_CLASS(THIS, BASE, INFO) \
	struct MSTL_API THIS final : BASE { \
		__MSTL_ERROR_CONSTRUCTOR(THIS, BASE, INFO) \
		__MSTL_ERROR_FINAL_DESTRUCTOR(THIS) \
		__MSTL_ERROR_TYPE(THIS) \
	};


/**
 * @struct exception
 * @brief 异常基类
 */
struct MSTL_API exception {
private:
	static constexpr size_t INFO_SIZE = 256;  // 错误信息长度
	static constexpr size_t TYPE_SIZE = 48;   // 类型名称长度

	char info_[INFO_SIZE];  // 错误信息
	char type_[TYPE_SIZE];  // 类型名称

public:
	/**
	 * @brief 构造函数
	 * @param info 错误信息
	 * @param type 异常类型
	 */
    explicit exception(const char* info = static_type, const char* type = static_type) {
    	string_copy_n(info_, info, INFO_SIZE - 1);
    	string_copy_n(type_, type, TYPE_SIZE - 1);
    	info_[INFO_SIZE - 1] = '\0';
    	type_[TYPE_SIZE - 1] = '\0';
    }

	/**
	 * @brief 复制构造函数
	 */
	exception(const exception& other) noexcept {
    	memory_copy(info_, other.info_, INFO_SIZE);
    	memory_copy(type_, other.type_, TYPE_SIZE);
    }

	/**
	 * @brief 复制赋值运算符
	 */
	exception& operator =(const exception& other) noexcept {
    	if (this != &other) {
    		memory_copy(info_, other.info_, INFO_SIZE);
    		memory_copy(type_, other.type_, TYPE_SIZE);
    	}
    	return *this;
    }

	/**
	 * @brief 移动构造函数
	 */
	exception(exception&& other) noexcept {
		memory_copy(this, &other, sizeof(*this));
		other.info_[0] = '\0';
		other.type_[0] = '\0';
	}

	/**
	 * @brief 移动赋值运算符
	 */
	exception& operator =(exception&& other) noexcept {
    	if (this != &other) {
    		memory_copy(info_, other.info_, INFO_SIZE);
    		memory_copy(type_, other.type_, TYPE_SIZE);
    		other.info_[0] = '\0';
    		other.type_[0] = '\0';
    	}
    	return *this;
    }

	/**
	 * @brief 模板构造函数
	 * @tparam Error 错误类型
	 * @param error 错误对象
	 *
	 * 从其他异常类型构造。
	 */
    template <typename Error>
    explicit exception(const Error& error)
    : exception(error.what(), error.type()) {}

	/**
	 * @brief 虚析构函数
	 */
	virtual ~exception() = default;

	/**
	 * @brief 获取错误信息
	 * @return 错误信息字符串
	 */
    MSTL_NODISCARD const char* what() const noexcept { return info_; }

	/**
	 * @brief 获取异常类型
	 * @return 异常类型字符串
	 */
    MSTL_NODISCARD const char* type() const noexcept { return type_; }

	__MSTL_ERROR_TYPE(exception)  ///< 静态类型字符串
};

MSTL_ERROR_BUILD_DERIVED_CLASS(memory_exception, exception, "Memory Operation Failed.")
MSTL_ERROR_BUILD_DERIVED_CLASS(system_exception, exception, "System Operation Failed.")
MSTL_ERROR_BUILD_FINAL_CLASS(allocate_exception, memory_exception, "Memory Allocation Failed.")
MSTL_ERROR_BUILD_FINAL_CLASS(iterator_exception, memory_exception, "Iterator or Pointer Access Invalid.")
MSTL_ERROR_BUILD_DERIVED_CLASS(typecast_exception, memory_exception, "Type Cast Mismatch.")
MSTL_ERROR_BUILD_DERIVED_CLASS(value_exception, exception, "Variable Operation Invalid.")
MSTL_ERROR_BUILD_DERIVED_CLASS(device_exception, system_exception, "Device Operation Failed.")
MSTL_ERROR_BUILD_FINAL_CLASS(file_exception, system_exception, "File Operation Failed.")
MSTL_ERROR_BUILD_FINAL_CLASS(math_exception, value_exception, "Math Calculation Invalid.")
MSTL_ERROR_BUILD_DERIVED_CLASS(database_exception, system_exception, "Database Operation Failed.")


/**
 * @brief 抛出异常并打印堆栈信息
 * @param err 要抛出的异常对象
 */
void MSTL_API throw_with_stack(const exception& err);

#if defined(MSTL_STATE_DEBUG__) || !defined(NDEBUG)
#define throw_exception(err) throw_with_stack(err)
#else
#define throw_exception(err) throw err
#endif

/** @} */ // ExceptionHandling

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_CONFIG_EXCEPTION_HPP__
