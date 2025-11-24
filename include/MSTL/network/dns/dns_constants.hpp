#ifndef MSTL_DNS_CONSTANTS_HPP__
#define MSTL_DNS_CONSTANTS_HPP__
#include "MSTL/core/typeinfo/types.hpp"
MSTL_BEGIN_NAMESPACE__

enum class DNS_RECORD : uint16_t {
    A = 1,       // IPv4地址
    NS = 2,      // 名称服务器
    CNAME = 5,   // 规范名称
    SOA = 6,     // 授权开始
    PTR = 12,    // 指针记录
    MX = 15,     // 邮件交换
    TXT = 16,    // 文本记录
    AAAA = 28,   // IPv6地址
    SRV = 33     // 服务记录
};

enum class DNS_QUERY : uint16_t {
    INTERNET = 1,
    CHAOS = 3,
    HESIOD = 4,
    ANY = 255
};

enum class DNS_RESPONSE : uint8_t {
    NON_ERROR = 0,
    FORMAT_ERROR = 1,
    SERVER_FAILURE = 2,
    NAME_ERROR = 3,
    NOT_IMPLEMENTED = 4,
    REFUSED = 5
};

MSTL_END_NAMESPACE__
#endif // MSTL_DNS_CONSTANTS_HPP__
