#ifndef MSTL_POSTGRESQL_CONFIG_HPP__
#define MSTL_POSTGRESQL_CONFIG_HPP__
#ifdef MSTL_SUPPORT_POSTGRESQL__
#include <libpq-fe.h>
#include "MSTL/core/c++config.hpp"
#include "MSTL/core/undef_cmacro.hpp"
MSTL_BEGIN_NAMESPACE__
MSTL_BEGIN_POSTGRESQL__
using ::PGconn;
using ::PGresult;
using ::ExecStatusType;
using ::PQntuples;
using ::PQnfields;
using ::PQclear;
using ::PQgetisnull;
using ::PQgetvalue;
using ::PQgetlength;
using ::PQunescapeBytea;
using ::PQfreemem;
using ::PQexec;
using ::PQprepare;
using ::PQresultStatus;
using ::PQerrorMessage;
using ::PQexecPrepared;
using ::PQconnectdb;
using ::PQstatus;
using ::PQfinish;
using ::CONNECTION_OK;
using ::PGRES_TUPLES_OK;
using ::PGRES_COMMAND_OK;
MSTL_END_POSTGRESQL__
MSTL_END_NAMESPACE__
#endif
#endif // MSTL_POSTGRESQL_CONFIG_HPP__
