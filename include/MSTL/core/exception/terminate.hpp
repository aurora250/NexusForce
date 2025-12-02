#ifndef MSTL_CORE_CONFIG_TERMINATE_HPP__
#define MSTL_CORE_CONFIG_TERMINATE_HPP__
#include "../config/c++config.hpp"
MSTL_BEGIN_NAMESPACE__

using terminate_handler = void(*)();

void MSTL_API set_terminate(terminate_handler handler) noexcept;
MSTL_NORETURN void MSTL_API terminate() noexcept;

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_CONFIG_TERMINATE_HPP__
