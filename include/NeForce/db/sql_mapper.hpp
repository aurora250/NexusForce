#ifndef NEFORCE_DATABASE_SQL_MAPPER_HPP__
#define NEFORCE_DATABASE_SQL_MAPPER_HPP__

/**
 * @file sql_mapper.hpp
 * @brief ORM 对象-关系映射器
 *
 * 纯运行时消费反射元数据，自动生成 SQL 语句并完成实体与结果行之间的映射。
 * 依赖 reflect_scanner 或手动注册的反射元数据。
 *
 * 使用示例：
 * @code
 * struct User {
 *     NEFORCE_REFLECT_OBJ(User)
 *     NEFORCE_REFLECT_PROP_ATTR(int, id, PROP_PRIMARY_KEY | PROP_AUTO_INC)
 *     NEFORCE_REFLECT_PROP(string, name)
 *     int id = 0;
 *     string name;
 * };
 * // 运行时：
 * auto sql = sql_mapper<User>::create_table_sql();
 * auto user = sql_mapper<User>::from_row(result_row);
 * @endcode
 */

#include "NeForce/core/reflect/reflect.hpp"
#include "NeForce/core/string/string.hpp"
#include "NeForce/db/db_interface.hpp"
#include "NeForce/db/sql_builder.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup SQLMapper ORM对象-关系映射
 * @brief 从反射元数据自动生成SQL并完成实体映射
 * @{
 */

/**
 * @brief 将 C++ 类型 ID 映射为 SQL 类型名称
 * @param tid 反射类型ID
 * @return SQL 类型名称字符串
 */
inline string cpp_type_to_sql_type(const reflect::type_id tid) {
    using reflect::type_id_for;
    if (tid == type_id_for<short>()) {
        return "SMALLINT";
    }
    if (tid == type_id_for<unsigned short>()) {
        return "SMALLINT";
    }
    if (tid == type_id_for<int>()) {
        return "INTEGER";
    }
    if (tid == type_id_for<unsigned int>()) {
        return "INTEGER";
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (tid == type_id_for<long>()) {
        return "INTEGER";
    }
    if (tid == type_id_for<unsigned long>()) {
        return "INTEGER";
    }
#else
    if (tid == type_id_for<long>()) {
        return "BIGINT";
    }
    if (tid == type_id_for<unsigned long>()) {
        return "BIGINT";
    }
#endif
    if (tid == type_id_for<long long>()) {
        return "BIGINT";
    }
    if (tid == type_id_for<unsigned long long>()) {
        return "BIGINT";
    }
    if (tid == type_id_for<double>()) {
        return "DOUBLE PRECISION";
    }
    if (tid == type_id_for<float>()) {
        return "REAL";
    }
    if (tid == type_id_for<bool>()) {
        return "BOOLEAN";
    }
    if (tid == type_id_for<string>()) {
        return "TEXT";
    }
    return "TEXT";
}

/**
 * @brief 从结果行中按类型ID读取值到 meta_any
 * @param row   结果行
 * @param col   列索引
 * @param tid   目标类型ID
 * @return 读取到的值
 */
inline reflect::meta_any read_column_by_type(const idb_tb_result& row, const size_t col, const reflect::type_id tid) {
    using reflect::type_id_for;
    if (tid == type_id_for<int>()) {
        return reflect::meta_any(static_cast<int>(row.get_int32(col)));
    } else if (tid == type_id_for<int64_t>()) {
        return reflect::meta_any(row.get_int64(col));
    } else if (tid == type_id_for<short>()) {
        return reflect::meta_any(static_cast<short>(row.get_int32(col)));
    } else if (tid == type_id_for<double>()) {
        return reflect::meta_any(row.get_float64(col));
    } else if (tid == type_id_for<float>()) {
        return reflect::meta_any(row.get_float32(col));
    } else if (tid == type_id_for<bool>()) {
        return reflect::meta_any(row.get_bool(col));
    } else if (tid == type_id_for<string>()) {
        return reflect::meta_any(string(row.get(col)));
    } else {
        return reflect::meta_any(string(row.get(col)));
    }
}

/**
 * @brief 将 meta_any 值转换为 SQL 字面量字符串
 * @param val 元数据值
 * @return SQL 字面量表示
 */
inline string meta_any_to_sql_literal(const reflect::meta_any& val) {
    if (!val.has_value()) {
        return "NULL";
    }
    const auto tid = val.type_id();
    using reflect::type_id_for;

    if (tid == type_id_for<int>()) {
        return integer32(val.get<int>()).to_string();
    } else if (tid == type_id_for<int64_t>()) {
        return integer64(val.get<int64_t>()).to_string();
    } else if (tid == type_id_for<short>()) {
        return integer32(val.get<short>()).to_string();
    } else if (tid == type_id_for<double>()) {
        return float64(val.get<double>()).to_string();
    } else if (tid == type_id_for<float>()) {
        return float64(val.get<float>()).to_string();
    } else if (tid == type_id_for<bool>()) {
        return val.get<bool>() ? "1" : "0";
    } else if (tid == type_id_for<string>()) {
        // simple quoting — caller should use parameterized queries when possible
        const auto& s = val.get<string>();
        string escaped;
        escaped.reserve(s.size() + 2);
        escaped += '\'';
        for (const auto ch: s) {
            if (ch == '\'') {
                escaped += "''";
            } else {
                escaped += ch;
            }
        }
        escaped += '\'';
        return escaped;
    }
    return "NULL";
}

/**
 * @class sql_mapper
 * @brief ORM 对象-关系映射器
 * @tparam T 实体类型
 *
 * 通过反射元数据自动完成实体类型与数据库表之间的映射。
 * 所有方法均为静态方法，无需实例化。
 */
template <typename T>
class sql_mapper {
private:
    static const reflect::meta_type* get_meta() {
        auto* meta = reflect::registry::instance().find(reflect::type_id_for<T>());
        if (meta == nullptr) {
            NEFORCE_THROW_EXCEPTION(value_exception(
                    _NEFORCE format("sql_mapper: type '{}' not registered in reflect registry", reflect::type_name_v<T>)
                            .data()));
        }
        return meta;
    }

    static string resolve_table_name() {
        const auto* meta = get_meta();
        const auto tn = meta->table_name();
        if (!tn.empty()) {
            return tn;
        }
        return meta->name();
    }

    /**
     * @brief 收集非瞬态、非自增的属性列表
     */
    static vector<const reflect::meta_property*> collect_persistent_props(bool include_pk = true) {
        const auto* meta = get_meta();
        vector<const reflect::meta_property*> result;
        for (const auto& entry: meta->all_properties()) {
            const auto* prop = entry.second;
            if (prop->is_transient()) {
                continue;
            }
            if (!include_pk && prop->is_auto_increment()) {
                continue;
            }
            result.push_back(prop);
        }
        return result;
    }

    /**
     * @brief 查找主键属性
     */
    static const reflect::meta_property* find_pk_prop() {
        const auto* meta = get_meta();
        for (const auto& entry: meta->all_properties()) {
            if (entry.second->is_primary_key()) {
                return entry.second;
            }
        }
        return nullptr;
    }

public:
    /**
     * @brief 获取映射的表名
     * @return 表名字符串
     */
    static string table_name() { return resolve_table_name(); }

    /**
     * @brief 生成 CREATE TABLE SQL
     * @param table_override 可选的表名覆盖（为空则使用反射元数据的表名）
     * @return CREATE TABLE SQL 字符串
     */
    static string create_table_sql(string_view table_override = "") {
        const auto* meta = get_meta();
        const string tbl = table_override.empty() ? resolve_table_name() : string(table_override);

        sql_builder b;
        b.create_table_if_not_exists(tbl);

        for (const auto& entry: meta->all_properties()) {
            const auto* prop = entry.second;
            if (prop->is_transient()) {
                continue;
            }

            const string sql_type = cpp_type_to_sql_type(prop->type_id());

            if (prop->is_primary_key() && prop->is_auto_increment()) {
                b.column_auto_increment(prop->name(), sql_type);
            } else if (prop->is_primary_key()) {
                b.column_primary_key(prop->name(), sql_type);
            } else if (prop->is_unique()) {
                b.column_unique(prop->name(), sql_type);
            } else if (prop->is_required()) {
                b.column_not_null(prop->name(), sql_type);
            } else {
                b.column(prop->name(), sql_type);
            }
        }

        return b.build();
    }

    /**
     * @brief 生成 DROP TABLE SQL
     * @param table_override 可选的表名覆盖
     * @return DROP TABLE SQL 字符串
     */
    static string drop_table_sql(string_view table_override = "") {
        const string tbl = table_override.empty() ? resolve_table_name() : string(table_override);
        sql_builder b;
        b.drop_table_if_exists(tbl);
        return b.build();
    }

    /**
     * @brief 生成 SELECT 查询 SQL
     * @param table_override 可选的表名覆盖
     * @return SELECT SQL 字符串
     */
    static string select_sql(string_view table_override = "") {
        const string tbl = table_override.empty() ? resolve_table_name() : string(table_override);

        sql_builder b;
        const auto* meta = get_meta();

        for (const auto& entry: meta->all_properties()) {
            if (entry.second->is_transient()) {
                continue;
            }
            b.select(entry.second->name());
        }
        b.from(tbl);
        return b.build();
    }

    /**
     * @brief 生成 INSERT SQL（占位符由方言自动决定）
     * @param table_override 可选的表名覆盖
     * @param dialect 数据库方言（默认 GENERIC 即 `?`）
     * @return INSERT SQL 字符串
     */
    static string insert_sql(string_view table_override = "", const sql_dialect dialect = sql_dialect::GENERIC) {
        const string tbl = table_override.empty() ? resolve_table_name() : string(table_override);

        const auto props = collect_persistent_props(false);
        vector<string> columns;
        columns.reserve(props.size());
        for (const auto* p: props) {
            columns.push_back(p->name());
        }

        sql_builder b;
        b.set_dialect(dialect);
        b.insert_into(tbl, columns);
        return b.build();
    }

    /**
     * @brief 生成 DELETE SQL（按主键删除）
     * @param table_override 可选的表名覆盖
     * @param dialect 数据库方言
     * @return DELETE SQL 字符串
     * @throws value_exception 无主键属性时抛出
     */
    static string delete_sql(string_view table_override = "", const sql_dialect dialect = sql_dialect::GENERIC) {
        const string tbl = table_override.empty() ? resolve_table_name() : string(table_override);
        const auto* pk = find_pk_prop();
        if (pk == nullptr) {
            NEFORCE_THROW_EXCEPTION(value_exception("sql_mapper: no primary key defined for DELETE"));
        }

        sql_builder b;
        b.set_dialect(dialect);
        b.delete_from(tbl);
        b.where_eq(pk->name(), b.placeholder(1));
        return b.build();
    }

    /**
     * @brief 生成 UPDATE SQL（按主键更新所有非主键字段）
     * @param table_override 可选的表名覆盖
     * @param dialect 数据库方言
     * @return UPDATE SQL 字符串
     * @throws value_exception 无主键属性时抛出
     */
    static string update_sql(string_view table_override = "", const sql_dialect dialect = sql_dialect::GENERIC) {
        const string tbl = table_override.empty() ? resolve_table_name() : string(table_override);
        const auto* pk = find_pk_prop();
        if (pk == nullptr) {
            NEFORCE_THROW_EXCEPTION(value_exception("sql_mapper: no primary key defined for UPDATE"));
        }

        const auto props = collect_persistent_props(true);

        sql_builder b;
        b.set_dialect(dialect);
        b.update(tbl);

        for (const auto* p: props) {
            if (p->is_primary_key() || p->is_auto_increment()) {
                continue;
            }
            b.set_param(p->name());
        }

        b.where_eq(pk->name(), b.next_placeholder());
        return b.build();
    }

    /**
     * @brief 从实体提取所有持久化字段的值（以 SQL 字面量表示）
     * @param entity 实体对象引用
     * @param include_pk 是否包含主键值
     * @return 值的字符串列表
     */
    static vector<string> get_values(const T& entity, bool include_pk = true) {
        const auto props = collect_persistent_props(include_pk);
        vector<string> values;
        values.reserve(props.size());
        for (const auto* prop: props) {
            reflect::meta_any val = prop->get(&entity);
            values.push_back(meta_any_to_sql_literal(val));
        }
        return values;
    }

    /**
     * @brief 获取主键值
     * @param entity 实体对象引用
     * @return 主键值
     */
    static reflect::meta_any get_pk_value(const T& entity) {
        const auto* pk = find_pk_prop();
        if (pk == nullptr) {
            return reflect::meta_any{};
        }
        return pk->get(&entity);
    }

    /**
     * @brief 从结果行构建实体对象
     * @param row 结果行（当前已定位到目标行）
     * @param col_map 列名到列索引的映射（为空时自动按属性名匹配）
     * @return 构建的实体对象
     */
    static T from_row(const idb_tb_result& row, const unordered_map<string, size_t>& col_map = {}) {
        const auto* meta = get_meta();
        auto obj_any = meta->create();
        if (!obj_any.has_value()) {
            NEFORCE_THROW_EXCEPTION(value_exception("sql_mapper: failed to create entity instance"));
        }
        void* raw = obj_any.raw();

        unordered_map<string, size_t> effective_map;
        if (col_map.empty()) {
            const auto& names = row.column_names();
            for (size_t i = 0; i < names.size(); ++i) {
                effective_map[names[i]] = i;
            }
        } else {
            effective_map = col_map;
        }

        for (const auto& entry: meta->all_properties()) {
            const auto* prop = entry.second;
            if (prop->is_transient() || prop->is_readonly()) {
                continue;
            }

            auto it = effective_map.find(prop->name());
            if (it == effective_map.end()) {
                continue;
            }

            reflect::meta_any val = read_column_by_type(row, it->second, prop->type_id());
            prop->set(raw, val);
        }

        return *static_cast<T*>(raw);
    }

    /**
     * @brief 从查询结果集构建实体对象列表
     * @param result 查询结果集
     * @return 实体对象列表
     */
    static vector<T> from_result(unique_ptr<idb_tb_result> result) {
        vector<T> entities;
        if (result == nullptr || result->empty()) {
            return entities;
        }

        unordered_map<string, size_t> col_map;
        const auto& names = result->column_names();
        for (size_t i = 0; i < names.size(); ++i) {
            col_map[names[i]] = i;
        }

        while (result->next()) {
            entities.push_back(from_row(*result, col_map));
        }
        return entities;
    }

    /**
     * @brief 生成按主键查询的 SELECT SQL
     * @param table_override 可选的表名覆盖
     * @param dialect 数据库方言
     * @return SELECT ... WHERE pk = placeholder SQL 字符串
     */
    static string select_by_pk_sql(string_view table_override = "", const sql_dialect dialect = sql_dialect::GENERIC) {
        const string tbl = table_override.empty() ? resolve_table_name() : string(table_override);
        const auto* pk = find_pk_prop();
        if (pk == nullptr) {
            NEFORCE_THROW_EXCEPTION(value_exception("sql_mapper: no primary key defined for SELECT BY PK"));
        }

        const auto* meta = get_meta();
        sql_builder b;
        b.set_dialect(dialect);
        for (const auto& entry: meta->all_properties()) {
            if (entry.second->is_transient()) {
                continue;
            }
            b.select(entry.second->name());
        }
        b.from(tbl);
        b.where_eq(pk->name(), b.placeholder(1));
        return b.build();
    }
};

/** @} */ // SQLMapper

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_DATABASE_SQL_MAPPER_HPP__
