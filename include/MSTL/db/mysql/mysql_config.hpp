#ifndef MSTL_MYSQL_CONFIG_HPP__
#define MSTL_MYSQL_CONFIG_HPP__
#ifdef MSTL_SUPPORT_MYSQL__
#ifdef CR_OUT_OF_MEMORY
#undef CR_OUT_OF_MEMORY
#endif
#include <mysql.h>
#include "MSTL/core/c++config.hpp"
MSTL_BEGIN_NAMESPACE__
MSTL_BEGIN_MYSQL__
using ::MYSQL;
using ::MYSQL_RES;
using ::MYSQL_ROW;
using ::MYSQL_ROWS;
using ::MYSQL_FIELD;
using ::MYSQL_STMT;
using ::MYSQL_BIND;
using ::enum_field_types;
using ::mysql_option;
using ::mysql_init;
using ::mysql_real_connect;
using ::mysql_error;
using ::mysql_errno;
using ::mysql_options;
using ::mysql_set_character_set;
using ::mysql_character_set_name;
using ::mysql_num_rows;
using ::mysql_num_fields;
using ::mysql_fetch_row;
using ::mysql_fetch_field;
using ::mysql_free_result;
using ::mysql_close;
using ::mysql_query;
using ::mysql_store_result;
using ::mysql_stmt_init;
using ::mysql_stmt_prepare;
using ::mysql_stmt_error;
using ::mysql_stmt_close;
using ::mysql_stmt_param_count;
using ::mysql_stmt_errno;
using ::mysql_stmt_execute;
using ::mysql_stmt_bind_param;
using ::MYSQL_TYPE_BOOL;
using ::MYSQL_TYPE_SHORT;
using ::MYSQL_TYPE_TINY;
using ::MYSQL_TYPE_LONG;
using ::MYSQL_TYPE_INT24;
using ::MYSQL_TYPE_LONGLONG;
using ::MYSQL_TYPE_FLOAT;
using ::MYSQL_TYPE_DOUBLE;
using ::MYSQL_TYPE_DECIMAL;
using ::MYSQL_TYPE_NEWDECIMAL;
using ::MYSQL_TYPE_BLOB;
using ::MYSQL_TYPE_TINY_BLOB;
using ::MYSQL_TYPE_MEDIUM_BLOB;
using ::MYSQL_TYPE_LONG_BLOB;
using ::MYSQL_TYPE_SET;
using ::MYSQL_TYPE_STRING;
using ::MYSQL_TYPE_BIT;
using ::MYSQL_TYPE_DATE;
using ::MYSQL_TYPE_DATETIME;
using ::MYSQL_TYPE_TIMESTAMP;
MSTL_END_MYSQL__
MSTL_END_NAMESPACE__
#endif // MSTL_SUPPORT_MYSQL__
#endif // MSTL_MYSQL_CONFIG_HPP__
