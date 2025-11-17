#ifndef MSTL_SQLITE_CONFIG_HPP__
#define MSTL_SQLITE_CONFIG_HPP__
#ifdef MSTL_SUPPORT_SQLITE3__
#include <sqlite3.h>
#include "MSTL/core/config/c++config.hpp"
MSTL_BEGIN_NAMESPACE__
MSTL_BEGIN_SQLITE__
using ::sqlite3;
using ::sqlite3_stmt;
using ::sqlite3_column_count;
using ::sqlite3_column_name;
using ::sqlite3_column_type;
using ::sqlite3_step;
using ::sqlite3_finalize;
using ::sqlite3_column_text;
using ::sqlite3_column_int;
using ::sqlite3_column_int64;
using ::sqlite3_column_double;
using ::sqlite3_clear_bindings;
using ::sqlite3_open;
using ::sqlite3_reset;
using ::sqlite3_bind_parameter_count;
using ::sqlite3_bind_text;
using ::sqlite3_bind_int;
using ::sqlite3_bind_int64;
using ::sqlite3_bind_double;
using ::sqlite3_bind_blob;
using ::sqlite3_prepare_v2;
using ::sqlite3_errmsg;
using ::sqlite3_errcode;
using ::sqlite3_exec;
using ::sqlite3_free;
using ::sqlite3_close;
MSTL_END_SQLITE__
MSTL_END_NAMESPACE__
#endif
#endif // MSTL_SQLITE_CONFIG_HPP__
