#ifndef MSTL_DATABASE_REDIS_CONFIG_HPP__
#define MSTL_DATABASE_REDIS_CONFIG_HPP__
#ifdef MSTL_SUPPORT_REDIS__
#include <hiredis.h>
#include "MSTL/core/config/c++config.hpp"
MSTL_BEGIN_NAMESPACE__
MSTL_BEGIN_REDIS__
using ::redisReply;
using ::redisContext;
using ::freeReplyObject;
using ::redisCommand;
using ::redisCommandArgv;
using ::redisConnect;
using ::redisFree;
MSTL_END_REDIS__
MSTL_END_NAMESPACE__
#endif
#endif // MSTL_DATABASE_REDIS_CONFIG_HPP__
