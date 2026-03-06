#ifndef NEFORCE_CORE_CONFIG_WINDEF_HPP__
#define NEFORCE_CORE_CONFIG_WINDEF_HPP__

/**
 * @file windef.hpp
 * @brief NeForce WindowsAPI 平台宏
 *
 * 此头文件定义了WindowsAPI所需的平台宏，它从<Windows.h>的部分中复制出来，
 * 用以在不引入<Windows.h>的情况下使用子头文件
 */

#include "NeForce/core/config/c++config.hpp"
#ifdef NEFORCE_PLATFORM_WINDOWS

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && !defined(_ARM_) && !defined(_ARM64_) && !defined(_ARM64EC_) && defined(_M_IX86)
#define _X86_
#if !defined(_CHPE_X86_ARM64_) && defined(_M_HYBRID)
#define _CHPE_X86_ARM64_
#endif
#endif

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && !defined(_ARM_) && !defined(_ARM64_) && (defined(_M_AMD64) || defined(_M_ARM64EC))
#define _AMD64_
#endif

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && !defined(_ARM_) && !defined(_ARM64_) && !defined(_ARM64EC_) && defined(_M_ARM)
#define _ARM_
#endif

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && !defined(_ARM_) && !defined(_ARM64_) && !defined(_ARM64EC_) && defined(_M_ARM64)
#define _ARM64_
#endif

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_ARM_) && !defined(_ARM64_) && !defined(_ARM64EC_) && defined(_M_ARM64EC)
#define _ARM64EC_
#endif

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && !defined(_ARM_) && !defined(_ARM64_) && !defined(_ARM64EC_) && defined(_M_M68K)
#define _68K_
#endif

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && !defined(_ARM_) && !defined(_ARM64_) && !defined(_ARM64EC_) && defined(_M_MPPC)
#define _MPPC_
#endif

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_M_IX86) && !defined(_AMD64_) && !defined(_ARM_) && !defined(_ARM64_) && !defined(_ARM64EC_) && defined(_M_IA64)
#if !defined(_IA64_)
#define _IA64_
#endif
#endif

#ifndef _MAC
#if defined(_68K_) || defined(_MPPC_)
#define _MAC
#endif
#endif

#endif
#endif // NEFORCE_CORE_CONFIG_WINDEF_HPP__
