#ifndef MSTL_CORE_CONFIG_EXCEPTION_HPP__
#define MSTL_CORE_CONFIG_EXCEPTION_HPP__
#include "../config/c++config.hpp"
#ifdef MSTL_SUPPORT_CUDA__
#include <cuda_runtime.h>
#endif
MSTL_BEGIN_NAMESPACE__

#define __MSTL_ERROR_CONSTRUCTOR(THIS, BASE, INFO) \
	constexpr explicit THIS(const char* const info = INFO, const char* const type = __type__) noexcept \
		: BASE(info, type) {}

#define __MSTL_ERROR_DERIVED_DESTRUCTOR(CLASS) \
	virtual ~CLASS() = default;

#define __MSTL_ERROR_FINAL_DESTRUCTOR(CLASS) \
	~CLASS() override = default;

#define __MSTL_ERROR_TYPE(CLASS) \
	static constexpr auto __type__ = #CLASS;

#define __MSTL_ERROR_WHAT() \
	const char* what() const { \
		return info; \
	}

#define MSTL_ERROR_BUILD_DERIVED_CLASS(THIS, BASE, INFO) \
	struct MSTL_API THIS : BASE { \
		__MSTL_ERROR_CONSTRUCTOR(THIS, BASE, INFO) \
		__MSTL_ERROR_DERIVED_DESTRUCTOR(THIS) \
		__MSTL_ERROR_TYPE(THIS) \
	};

#define MSTL_ERROR_BUILD_FINAL_CLASS(THIS, BASE, INFO) \
	struct MSTL_API THIS final : BASE { \
		__MSTL_ERROR_CONSTRUCTOR(THIS, BASE, INFO) \
		__MSTL_ERROR_FINAL_DESTRUCTOR(THIS) \
		__MSTL_ERROR_TYPE(THIS) \
	};


struct MSTL_API exception {
	const char* const info = nullptr;
	const char* const type = nullptr;

	constexpr explicit exception(const char* const info = __type__, const char* const type = __type__) noexcept
		: info(info), type(type) {}

	__MSTL_ERROR_DERIVED_DESTRUCTOR(exception)
	__MSTL_ERROR_TYPE(exception)
    __MSTL_ERROR_WHAT()
};

MSTL_ERROR_BUILD_FINAL_CLASS(assert_exception, exception, "Assertion Failed.")
MSTL_ERROR_BUILD_DERIVED_CLASS(memory_exception, exception, "Memory Operation Failed.")
MSTL_ERROR_BUILD_FINAL_CLASS(allocate_exception, memory_exception, "Memory Allocation Failed.")
MSTL_ERROR_BUILD_FINAL_CLASS(iterator_exception, memory_exception, "Iterator or Pointer Access Invalid.")
MSTL_ERROR_BUILD_DERIVED_CLASS(typecast_exception, memory_exception, "Type Cast Mismatch.")
MSTL_ERROR_BUILD_DERIVED_CLASS(value_exception, exception, "Function or Template Argument Invalid.")
MSTL_ERROR_BUILD_DERIVED_CLASS(link_exception, exception, "External Link Actions Failed.")
MSTL_ERROR_BUILD_DERIVED_CLASS(device_exception, exception, "Device Operation Failed.")
MSTL_ERROR_BUILD_FINAL_CLASS(file_exception, device_exception, "Device File Operation Failed.")
MSTL_ERROR_BUILD_FINAL_CLASS(math_exception, value_exception, "Math Function Argument Invalid.")
MSTL_ERROR_BUILD_DERIVED_CLASS(database_exception, exception, "Database Operation Failed.")


#ifdef MSTL_SUPPORT_CUDA__
// specialization of MSTL_ERROR_BUILD_FINAL_CLASS for CUDA
struct CUDAMemoryError final : MemoryError {
	__MSTL_ERROR_CONSTRUCTOR(CUDAMemoryError, MemoryError, "CUDA GPU Memory Operation Failed.")

	constexpr explicit CUDAMemoryError(const cudaError_t err) noexcept
		: MemoryError(cudaGetErrorString(err), __type__) {}

	__MSTL_ERROR_FINAL_DESTRUCTOR(CUDAMemoryError)
	__MSTL_ERROR_TYPE(CUDAMemoryError)
	__MSTL_ERROR_WHAT()
};
#endif

void MSTL_API throw_exception(const exception& err);

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_CONFIG_EXCEPTION_HPP__
