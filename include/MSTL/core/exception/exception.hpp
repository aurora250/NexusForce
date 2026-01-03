#ifndef MSTL_CORE_CONFIG_EXCEPTION_HPP__
#define MSTL_CORE_CONFIG_EXCEPTION_HPP__
#include "../memory/unique_ptr.hpp"
#ifdef MSTL_SUPPORT_CUDA__
#include <cuda_runtime.h>
#endif
MSTL_BEGIN_NAMESPACE__

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
private:
    struct impl;
    unique_ptr<impl> ptr_;

public:
    explicit exception(const char* info = static_type, const char* type = static_type);

    exception(const exception&);
    exception& operator =(const exception&);
    exception(exception&&) noexcept = default;
    exception& operator =(exception&&) noexcept = default;

    template <typename Error>
    explicit exception(const Error& error)
    : ptr_(_MSTL make_unique<impl>(*error.ptr_)) {}

	virtual ~exception();

    MSTL_NODISCARD virtual const char* what() const noexcept;
    MSTL_NODISCARD virtual const char* type() const noexcept;

	__MSTL_ERROR_TYPE(exception)
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


void MSTL_API throw_with_stack(const exception& err);

#if defined(MSTL_STATE_DEBUG__) || !defined(NDEBUG)
#define throw_exception(err) throw_with_stack(err)
#else
#define throw_exception(err) { throw err; }
#endif

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_CONFIG_EXCEPTION_HPP__
