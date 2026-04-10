#ifndef NEFORCE_CORE_EXCEPTION_ERRC_HPP__
#define NEFORCE_CORE_EXCEPTION_ERRC_HPP__

/**
 * @file errc.hpp
 * @brief 系统错误码枚举
 *
 * 此文件提供了跨平台的系统错误码枚举，封装了POSIX errno值。
 * 通过统一的枚举类型，可以在不同平台上使用相同的错误码名称。
 */

#include "NeForce/core/typeinfo/types.hpp"
#include <cerrno>
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup ErrorCode 错误码
 * @brief 错误码系统实现
 * @{
 */

/**
 * @enum errc
 * @brief 系统错误码枚举
 *
 * 定义了常见的系统错误码，与POSIX errno值对应。
 * 提供类型安全的错误码表示，避免使用原始整数。
 */
enum class errc : int32_t {
    success = 0, ///< 操作成功

    address_family_not_supported = EAFNOSUPPORT, ///< 地址族不支持
    address_in_use = EADDRINUSE,                 ///< 地址已在使用中
    address_not_available = EADDRNOTAVAIL,       ///< 地址不可用
    already_connected = EISCONN,                 ///< 已连接

    argument_list_too_long = E2BIG, ///< 参数列表过长
    argument_out_of_domain = EDOM,  ///< 参数超出域
    bad_address = EFAULT,           ///< 错误地址
    bad_file_descriptor = EBADF,    ///< 错误的文件描述符
    invalid_argument = EINVAL,      ///< 无效参数
    value_too_large = EOVERFLOW,    ///< 值过大
    result_out_of_range = ERANGE,   ///< 结果超出范围

    broken_pipe = EPIPE,                         ///< 管道破裂
    connection_aborted = ECONNABORTED,           ///< 连接中止
    connection_already_in_progress = EALREADY,   ///< 连接已在进行中
    connection_refused = ECONNREFUSED,           ///< 连接被拒绝
    connection_reset = ECONNRESET,               ///< 连接被重置
    destination_address_required = EDESTADDRREQ, ///< 需要目标地址
    host_unreachable = EHOSTUNREACH,             ///< 主机不可达
    message_size = EMSGSIZE,                     ///< 消息大小错误
    network_down = ENETDOWN,                     ///< 网络故障
    network_reset = ENETRESET,                   ///< 网络重置
    network_unreachable = ENETUNREACH,           ///< 网络不可达
    no_buffer_space = ENOBUFS,                   ///< 无缓冲区空间
    no_protocol_option = ENOPROTOOPT,            ///< 无协议选项
    not_connected = ENOTCONN,                    ///< 未连接
    operation_in_progress = EINPROGRESS,         ///< 操作进行中
    operation_would_block = EWOULDBLOCK,         ///< 操作会阻塞
    protocol_error = EPROTO,                     ///< 协议错误
    protocol_not_supported = EPROTONOSUPPORT,    ///< 协议不支持
    stream_timeout = ETIME,                      ///< 流超时
    timed_out = ETIMEDOUT,                       ///< 超时
    wrong_protocol_type = EPROTOTYPE,            ///< 错误的协议类型

    cross_device_link = EXDEV,                   ///< 跨设备链接
    device_or_resource_busy = EBUSY,             ///< 设备或资源忙
    directory_not_empty = ENOTEMPTY,             ///< 目录非空
    file_exists = EEXIST,                        ///< 文件已存在
    file_too_large = EFBIG,                      ///< 文件过大
    filename_too_long = ENAMETOOLONG,            ///< 文件名过长
    inappropriate_io_control_operation = ENOTTY, ///< 不适当的I/O控制操作
    invalid_seek = ESPIPE,                       ///< 无效的偏移定位
    io_error = EIO,                              ///< I/O错误
    is_a_directory = EISDIR,                     ///< 是目录
    no_space_on_device = ENOSPC,                 ///< 设备无空间
    no_such_device_or_address = ENXIO,           ///< 无此设备或地址
    no_such_device = ENODEV,                     ///< 无此设备
    no_such_file_or_directory = ENOENT,          ///< 无此文件或目录
    not_a_directory = ENOTDIR,                   ///< 不是目录
    read_only_file_system = EROFS,               ///< 只读文件系统
    text_file_busy = ETXTBSY,                    ///< 文本文件忙
    too_many_files_open_in_system = ENFILE,      ///< 系统中打开文件过多
    too_many_files_open = EMFILE,                ///< 打开文件过多
    too_many_links = EMLINK,                     ///< 链接过多
    too_many_symbolic_link_levels = ELOOP,       ///< 符号链接层次过多

    no_child_process = ECHILD,       ///< 无子进程
    no_such_process = ESRCH,         ///< 无此进程
    operation_not_permitted = EPERM, ///< 操作不允许
    permission_denied = EACCES,      ///< 权限被拒绝

    not_enough_memory = ENOMEM, ///< 内存不足
    no_lock_available = ENOLCK, ///< 无可用锁

    interrupted = EINTR,                     ///< 被中断
    owner_dead = EOWNERDEAD,                 ///< 所有者已死
    state_not_recoverable = ENOTRECOVERABLE, ///< 状态不可恢复

    executable_format_error = ENOEXEC,    ///< 可执行文件格式错误
    function_not_supported = ENOSYS,      ///< 功能不支持
    illegal_byte_sequence = EILSEQ,       ///< 非法字节序列
    not_supported = ENOTSUP,              ///< 不支持
    operation_not_supported = EOPNOTSUPP, ///< 操作不支持

    no_message_available = ENODATA,          ///< 无可用消息
    no_message = ENOMSG,                     ///< 无消息
    operation_canceled = ECANCELED,          ///< 操作已取消
    resource_deadlock_would_occur = EDEADLK, ///< 可能发生死锁
    resource_unavailable_try_again = EAGAIN, ///< 资源不可用，请重试

    identifier_removed = EIDRM, ///< 标识符已移除
    no_link = ENOLINK,          ///< 无链接
    not_a_socket = ENOTSOCK,    ///< 不是套接字
};

/** @} */ // ErrorCode

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_EXCEPTION_ERRC_HPP__
